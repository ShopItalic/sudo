"""Execute the actual P09 SFUD/LittleFS and SPI adapters with failing devices."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/firmware'))
import factory_short_v9 as patch
from factory_local_recording_v8 import function_span
def vendor(path):
    return subprocess.check_output(['git','show','102bfd2:firmware/'+path],cwd=ROOT).decode('latin1').replace('\r\n','\n')
def extract(source,signature):
    a,b=function_span(source,signature); return source[a:b]
def run(code):
    with tempfile.TemporaryDirectory(prefix='p09-storage-') as folder:
        out=Path(folder); (out/'test.c').write_text(code,encoding='latin1')
        subprocess.run([os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-Werror','-O1','-g',
                        '-fsanitize=address,undefined',str(out/'test.c'),'-o',str(out/'test')],check=True)
        subprocess.run([str(out/'test')],check=True)
class Storage(unittest.TestCase):
    def test_littlefs_errors_ranges_and_no_format(self):
        source=patch.patch_lfs(vendor('bc_ros/bc_module/file/LittleFS/lfs_port.c'))
        code=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#define LFS_ERR_OK 0
#define LFS_ERR_IO -5
#define SFUD_SUCCESS 0
typedef unsigned lfs_block_t;
typedef unsigned lfs_off_t;
typedef unsigned lfs_size_t;
typedef struct { int unused; } lfs_t;
struct lfs_config { unsigned block_size,block_count; };
static const struct lfs_config cfg={4096,256};
static struct { bool init_ok; struct { unsigned capacity; } chip; } device={true,{4096*256}},*flash=&device;
static int io_error, init_error, mount_error;
static unsigned calls,opens,closes,mounts;
static int sfud_read(void *f,unsigned address,unsigned n,void *data)
{ assert(f==flash && address+n<=device.chip.capacity && data); ++calls; return io_error; }
static int sfud_write(void *f,unsigned address,unsigned n,const void *data)
{ return sfud_read(f,address,n,(void *)data); }
static int sfud_erase(void *f,unsigned address,unsigned n)
{ assert(f==flash && address+n<=device.chip.capacity); ++calls; return io_error; }
static int sfud_init(void) { return init_error; }
static void *sfud_get_device_table(void) { return &device; }
static void bc_spi_flash_device_open(void) { ++opens; }
static void bc_spi_flash_device_close(void) { ++closes; }
static int lfs_mount(lfs_t *fs,const struct lfs_config *c)
{ assert(fs && c==&cfg); ++mounts; return mount_error; }
'''
        for name in ('read','prog','erase'):
            start=source.index('static int lfs_deskio_'+name+'(')
            code+='\n'+source[start:source.index('\n}',start)+2]
        code+='\n'+extract(source,'int lfs_sfud_init(lfs_t *lfs)')
        # No lfs_format symbol is provided: an accidental format call fails to link.
        code+=r'''
int main(void) {
    uint8_t data[256]; lfs_t fs={0}; unsigned before;
    assert(lfs_deskio_read(&cfg,0,0,data,sizeof data)==0);
    assert(lfs_deskio_prog(&cfg,255,3840,data,sizeof data)==0);
    assert(lfs_deskio_erase(&cfg,255)==0);
    io_error=7;
    assert(lfs_deskio_read(&cfg,0,0,data,1)==LFS_ERR_IO);
    assert(lfs_deskio_prog(&cfg,0,0,data,1)==LFS_ERR_IO);
    assert(lfs_deskio_erase(&cfg,0)==LFS_ERR_IO);
    before=calls;
    assert(lfs_deskio_read(&cfg,256,0,data,1)==LFS_ERR_IO);
    assert(lfs_deskio_read(&cfg,0,4096,data,1)==LFS_ERR_IO);
    assert(lfs_deskio_read(&cfg,0,0,NULL,1)==LFS_ERR_IO);
    assert(lfs_deskio_prog(NULL,0,0,data,1)==LFS_ERR_IO);
    assert(lfs_deskio_erase(&cfg,256)==LFS_ERR_IO);
    device.init_ok=false; assert(lfs_deskio_read(&cfg,0,0,data,1)==LFS_ERR_IO); device.init_ok=true;
    device.chip.capacity=4096; assert(lfs_deskio_erase(&cfg,1)==LFS_ERR_IO);
    assert(calls==before);
    init_error=1; assert(lfs_sfud_init(&fs)==LFS_ERR_IO && mounts==0 && opens==closes);
    init_error=0; mount_error=-84; assert(lfs_sfud_init(&fs)==-84 && mounts==1 && opens==closes);
    mount_error=0; assert(lfs_sfud_init(&fs)==0 && mounts==2 && opens==closes);
    puts("PASS P09 LittleFS port: propagated I/O errors, ranges, init/mount failures, no formatting");
    return 0;
}
'''
        run(code)
    def test_spi_chunking_and_failure_latch(self):
        source=patch.patch_spi(vendor('bc_ros/bc_module/spi_flash/bc_spi_flash_port.c'))
        code=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define RESULT_OK 0
#define GPIO_OUTPUT_HIGH 1
#define GPIO_OUTPUT_LOW 0
typedef int q_device_t;
static q_device_t dev,*spi_flash_dev=&dev;
static bool factory_flash_opened,factory_flash_failed;
static struct spi_package { uint8_t *write_buff,*read_buff; uint8_t write_length,read_length; } spi_pack;
static unsigned opens,closes,power,tx,rx,calls,cs;
static int open_error,fail_at;
static uint8_t written[1024];
static void bc_ldo_flash_power_on(void) { ++power; }
static void bc_ldo_flash_power_off(void) { --power; }
static void bc_delay_ms(unsigned n) { (void)n; }
static int q_device_open(q_device_t *d) { assert(d==&dev); ++opens; return open_error; }
static void q_device_close(q_device_t *d) { assert(d==&dev); ++closes; }
static void spi_flash_device_wakeup(void) {}
static void spi_flash_device_lowpower(void) {}
static void q_device_ctrl(q_device_t *d,unsigned n,unsigned v) { assert(d==&dev); (void)n;(void)v;++cs; }
static int q_device_write(q_device_t *d,unsigned p,struct spi_package *s,unsigned n) {
    (void)p;(void)n;assert(d==&dev && s==&spi_pack && (s->write_length || s->read_length));
    if((int)++calls==fail_at) return -1;
    if(s->write_length) { assert(tx+s->write_length<=sizeof written); memcpy(written+tx,s->write_buff,s->write_length);tx+=s->write_length; }
    if(s->read_length) { memset(s->read_buff,0x53,s->read_length); rx+=s->read_length; }
    return 0;
}
'''
        for signature in ('void bc_spi_flash_device_open(void)','void bc_spi_flash_device_close(void)',
                          'void bc_spi_flash_cs_high(void)','void bc_spi_flash_cs_low(void)',
                          'bool bc_spi_flash_write_and_read(uint8_t *write_buff,uint32_t write_length,uint8_t *read_buff,uint32_t read_length)'):
            code+='\n'+extract(source,signature)
        code+=r'''
int main(void) {
    uint8_t send[700],receive[600]; unsigned i,before;
    for(i=0;i<sizeof send;++i) send[i]=(uint8_t)i;
    bc_spi_flash_cs_low(); assert(cs==0);
    assert(!bc_spi_flash_write_and_read(send,1,receive,1));
    open_error=-1; bc_spi_flash_device_open(); assert(power==0 && !factory_flash_opened);
    open_error=0; bc_spi_flash_device_open(); bc_spi_flash_device_open(); assert(opens==2 && power==1);
    bc_spi_flash_cs_low();
    assert(bc_spi_flash_write_and_read(send,sizeof send,receive,sizeof receive));
    bc_spi_flash_cs_high();
    assert(tx==700 && rx==600 && calls==6 && cs==2 && !memcmp(send,written,700));
    for(i=0;i<sizeof receive;++i) assert(receive[i]==0x53);
    assert(!bc_spi_flash_write_and_read(NULL,1,NULL,0));
    fail_at=(int)calls+1; assert(!bc_spi_flash_write_and_read(send,1,NULL,0)); before=calls;
    assert(!bc_spi_flash_write_and_read(NULL,0,receive,1) && calls==before);
    bc_spi_flash_device_close(); bc_spi_flash_device_close(); assert(power==0 && closes==1);
    bc_spi_flash_device_open(); assert(bc_spi_flash_write_and_read(send,1,NULL,0));
    bc_spi_flash_device_close(); assert(power==0 && opens==3 && closes==2);
    puts("PASS P09 SPI port: long transfers, open/write failures, latched transaction errors, balanced power");
    return 0;
}
'''
        run(code)
if __name__=='__main__': unittest.main()
