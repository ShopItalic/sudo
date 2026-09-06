#include "bc_pdm.h"

#include "q_device.h"



static q_device_t *pdm_dev;
static struct pdm_data_config pdm_config = {0};


//void bc_pdm_config(int16_t *pdm_data_buff,int16_t pdm_data_buff_length,void *pdm_data_callback)
//{
//	pdm_config.pdm_data_buff = pdm_data_buff;
//	pdm_config.pdm_data_buff_legth = pdm_data_buff_length;
//	pdm_config.pdm_data_callback = pdm_data_callback;
//	q_device_cfg(pdm_dev, &pdm_config, NULL);
//}

//void bc_pdm_stop(void)
//{
//	q_device_close(pdm_dev);
//}

//void bc_pdm_start(void)
//{
//	q_device_open(pdm_dev);
//}

void bc_pdm_device_find(void)
{
	pdm_dev = q_device_find("device_pdm");
	q_device_assert(pdm_dev);
}

