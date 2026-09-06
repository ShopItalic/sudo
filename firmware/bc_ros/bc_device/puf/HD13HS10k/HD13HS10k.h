#ifndef __HD13HS10K_H__
#define __HD13HS10K_H__


#include <stdbool.h>
#include <stdint.h>


#define PUF_BUS_NUM2	0x01
#define PUF_BUS_NUM9	0x08

#define PUF_DEV_ADDR	0x64	/* PUF的设备地址 */
#define PUF_DATA_ADDR	0x00	/* PUF的寄存器地址 */

#define PUF_WAIT_NANO	10000000  /* 10毫秒 */

enum puf_command {
	READ_PUF_EEROM		= 0x08,
	WRITE_PUF_EEROM		= 0x09,
	DISABLE_PUF		= 0x10,
	ERASE_PUF_EEROM		= 0x0A,
	ENABLE_PUF 		= 0x20,
	WORK_PUF		= 0x50,
};

struct puf_eeprom_prefix {
	char high_addr:3;   /* eeprom第8到10的高3位地址 */
	char nvr:1;	  /* 8K或者2K地址片选 */
	char command:4;
	char low_addr:8;  /* eeprom低8位地址 */
	char len:3;	  /* 以字为单位的长度 */
	char unused:5;
}__attribute__((packed));

#define PUF_EEROM_MAX_ADDR	(10*1024)	/* 最大10K大小 */


/* 使能PUF设备 */
bool hd13hs10k_puf_set_enable(void);

/* 设置PUF为disable状态 */
bool hd13hs10k_puf_set_disable(void);

/* 获取PUF的ID */
bool hd13hs10k_get_puf_id(char *id, unsigned int size);

/* 通过挑战值获取PUF设备响应值 */
bool hd13hs10k_get_puf_resp(char *challenge, unsigned int challenge_len,  char *resp, unsigned int resp_len);



#endif





