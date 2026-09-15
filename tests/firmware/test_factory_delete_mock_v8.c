#include "app_factory_delete.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(c) do { checks++; if (!(c)) { fprintf(stderr,"FAIL %u: %s\n",__LINE__,#c); exit(1); } } while(0)
static unsigned checks, critical, opens, closes, stats, removes;
enum { PPG_FLS_IDIE, PPG_FLS_UPLOAD, PPG_FLS_WRITE, PPG_FLS_BUSY, PPG_FLS_SIZE_ERROR };
static struct { lfs_t lfs_fls_ppg_handle; unsigned fls_status; bool ppg_file_status; } app_ppg_file_hardle;
#define taskENTER_CRITICAL() (++critical)
#define taskEXIT_CRITICAL() do { CHECK(critical); --critical; } while(0)
static void bc_spi_flash_device_open(void) { CHECK(!critical); CHECK(app_ppg_file_hardle.fls_status==PPG_FLS_BUSY); ++opens; }
static void bc_spi_flash_device_close(void) { CHECK(!critical); ++closes; }
#include "factory_delete_adapter.inc"

static int pre_error, remove_error, post_error;
static unsigned entry_type;
static bool reenter;
int lfs_stat(lfs_t *fs, const char *path, struct lfs_info *info)
{
    CHECK(fs == &app_ppg_file_hardle.lfs_fls_ppg_handle);
    CHECK(!critical && app_ppg_file_hardle.fls_status == PPG_FLS_BUSY);
    CHECK(!strcmp(path, "/test.bin"));
    ++stats;
    if (reenter) {
        reenter=false;
        CHECK(!app_ppg_file_delete_request((const uint8_t *)"other.bin",9));
        CHECK(!strcmp(path,"/test.bin"));
    }
    info->type=entry_type;
    return stats == 1 ? pre_error : post_error;
}
int lfs_remove(lfs_t *fs,const char *path)
{
    CHECK(fs == &app_ppg_file_hardle.lfs_fls_ppg_handle);
    CHECK(!critical && app_ppg_file_hardle.fls_status == PPG_FLS_BUSY);
    CHECK(!strcmp(path,"/test.bin")); ++removes; return remove_error;
}
static void reset(void)
{
    memset(&app_ppg_file_hardle,0,sizeof app_ppg_file_hardle);
    opens=closes=stats=removes=critical=0;
    pre_error=remove_error=0; post_error=LFS_ERR_NOENT;
    entry_type=LFS_TYPE_REG; reenter=false;
}
struct app_cmd_package { uint8_t frame_type,frame_id,cmd,subcmd,data[250],length; };
static unsigned response_length;
static void app_package_send_enqueue(struct app_cmd_package *p,unsigned n) { CHECK(p->cmd==0x36); response_length=n; }
#include "factory_delete_command.inc"

int main(void)
{
    factory_delete_workspace workspace;
    const uint8_t name[]="test.bin";
    const char *bad[]={"", ".", "..", ".receipt", "/test.bin", "../test.bin", "a/b", "a\\b", "a b", "a\nb"};
    unsigned i,n;
    for(i=0;i<sizeof bad/sizeof *bad;i++) {
        reset(); CHECK(!app_ppg_file_delete_request((const uint8_t *)bad[i],(unsigned)strlen(bad[i])));
        CHECK(opens==0 && removes==0 && stats==0 && !critical);
        CHECK(app_ppg_file_hardle.fls_status==PPG_FLS_IDIE);
    }
    CHECK(!factory_delete_parse_name(NULL,1,&workspace));
    CHECK(!factory_delete_parse_name(name,8,NULL));
    CHECK(!factory_delete_parse_name(name,251,&workspace));
    for(n=1;n<=59;n++) {
        uint8_t *exact=malloc(n); CHECK(exact!=NULL); memset(exact,'a',n);
        CHECK(factory_delete_parse_name(exact,n,&workspace)==(n<=58)); free(exact);
    }
    { uint8_t padded[250]={0}; memcpy(padded,name,8);
      CHECK(factory_delete_parse_name(padded,sizeof padded,&workspace));
      padded[249]='x'; CHECK(!factory_delete_parse_name(padded,sizeof padded,&workspace)); }
    for(i=1;i<=PPG_FLS_SIZE_ERROR;i++) {
        reset(); app_ppg_file_hardle.fls_status=i;
        CHECK(!app_ppg_file_delete_request(name,8));
        CHECK(opens==0 && stats==0 && removes==0 && !critical);
        CHECK(app_ppg_file_hardle.fls_status==i);
    }
    reset(); app_ppg_file_hardle.ppg_file_status=true;
    CHECK(!app_ppg_file_delete_request(name,8)); CHECK(opens==0);
    reset(); reenter=true;
    CHECK(app_ppg_file_delete_request(name,8)); CHECK(stats==2 && removes==1 && opens==closes);
    CHECK(factory_delete_last_result==FACTORY_DELETE_REMOVED);
    reset(); pre_error=LFS_ERR_NOENT;
    CHECK(app_ppg_file_delete_request(name,8)); CHECK(stats==1 && removes==0 && opens==closes);
    CHECK(factory_delete_last_result==FACTORY_DELETE_ALREADY_ABSENT);
    reset(); entry_type=LFS_TYPE_DIR;
    CHECK(!app_ppg_file_delete_request(name,8)); CHECK(removes==0 && opens==closes);
    CHECK(factory_delete_last_result==FACTORY_DELETE_NOT_REGULAR);
    reset(); pre_error=LFS_ERR_IO;
    CHECK(!app_ppg_file_delete_request(name,8)); CHECK(removes==0 && opens==closes);
    CHECK(factory_delete_last_storage_error==LFS_ERR_IO);
    reset(); remove_error=LFS_ERR_IO;
    CHECK(!app_ppg_file_delete_request(name,8)); CHECK(stats==1 && opens==closes);
    reset(); post_error=0;
    CHECK(!app_ppg_file_delete_request(name,8)); CHECK(factory_delete_last_result==FACTORY_DELETE_NOT_CONFIRMED);
    reset(); post_error=LFS_ERR_IO;
    CHECK(!app_ppg_file_delete_request(name,8)); CHECK(factory_delete_last_result==FACTORY_DELETE_STORAGE_ERROR);
    CHECK(app_ppg_file_hardle.fls_status==PPG_FLS_IDIE && opens==closes);
    for(n=0;n<=4;n++) {
        struct app_cmd_package cmd; memset(&cmd,0,sizeof cmd); cmd.cmd=0x36; cmd.subcmd=0x12; cmd.length=n;
        memcpy(cmd.data,name,8); reset(); command_delete(&cmd);
        CHECK(cmd.data[0]==0 && removes==0 && opens==0 && response_length==5);
    }
    { struct app_cmd_package cmd; memset(&cmd,0x7f,sizeof cmd); cmd.cmd=0x36; cmd.subcmd=0x12; cmd.length=12;
      memcpy(cmd.data,name,8); reset(); command_delete(&cmd);
      CHECK(cmd.data[0]==1 && removes==1 && response_length==5); }
    parser_dispatches=0;
    parse_fixture(NULL,12);
    for(n=0;n<4;n++) { uint8_t *short_frame=malloc(n+1); CHECK(short_frame!=NULL);
        parse_fixture(short_frame,n); free(short_frame); }
    { uint8_t short_frame[4]={0}; parse_fixture(short_frame,255); parse_fixture(short_frame,65535); }
    CHECK(parser_dispatches==0);
    { struct app_cmd_package cmd; memset(&cmd,0,sizeof cmd); cmd.cmd=0x36; cmd.subcmd=0x12;
      memcpy(cmd.data,name,8); reset(); parse_fixture((uint8_t *)&cmd,12);
      CHECK(parser_dispatches==1 && cmd.data[0]==1 && cmd.length==12 && removes==1); }
    printf("PASS P08 delete boundary: %u checks (wire lengths, no NUL, busy, reentry, faults, readback)\n",checks);
    return 0;
}
