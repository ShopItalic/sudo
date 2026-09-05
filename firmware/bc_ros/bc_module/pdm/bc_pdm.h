#ifndef __BC_PDM_H__
#define __BC_PDM_H__

#include <stdint.h>


#if defined(USE_OPUS)
#define PDM_DATA_BUFF_SIZE        4800//20ms-8000hz 
#else
#define PDM_DATA_SEND_SIZE          220
#define PDM_DATA_SAMPLE_SIZE        (PDM_DATA_SEND_SIZE*2)
#define PDM_DATA_BUFF_SIZE          (PDM_DATA_SAMPLE_SIZE*10)
#endif
//#define PDM_DATA_BUFF_SIZE        440



struct bc_pdm_package
{

#if  defined(HANDWARE_1_23_1) 
  int16_t pdm_data_buff[PDM_DATA_BUFF_SIZE];
#else
  int16_t pdm_data_buff[PDM_DATA_BUFF_SIZE*2];
#endif  
  
};

#pragma pack (1)
struct bc_adpcm_package
{
#if ( defined(HANDWARE_1_19_1x) )      
uint8_t pdm_data_buff[ PDM_DATA_BUFF_SIZE*2];  

#elif ( defined(HANDWARE_1_17_1x) ) 
uint8_t pdm_data_buff[ PDM_DATA_BUFF_SIZE];  
#else
uint8_t pdm_data_buff[PDM_DATA_SEND_SIZE+6];
#endif  
  
};
#pragma pack ()
void bc_pdm_config(int16_t *pdm_data_buff,int16_t pdm_data_buff_length,void *pdm_data_callback);

void bc_pdm_stop(void);

void bc_pdm_start(void);

void bc_pdm_device_find(void);


#endif

