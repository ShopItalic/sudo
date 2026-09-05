/**
 * @copyright (c) 2003 - 2022, Goodix Co., Ltd. All rights reserved.
 * 
 * @file    gh3x2x_demo_hook.c
 * 
 * @brief   gh3x2x driver lib demo code for hook
 * 
 * @author  Gooidx Iot Tebc
 * 
 */

#include "gh_demo.h"
#include "gh_demo_config.h"
#include "gh_demo_inner.h"

#include <stdbool.h>

#include "bc_logger.h"

#if (__DRIVER_LIB_MODE__ == __DRV_LIB_WITH_ALGO__)
#include "gh3x2x_demo_algo_call.h"
#endif

typedef void (*ppg_green_data_callback)(uint32_t *data,int16_t *acc_data,uint8_t length);
typedef void (*ppg_ecg_data_callback)(uint32_t *data,uint8_t length);
typedef void (*ppg_red_and_ir_data_callback)(void *red_data,uint8_t red_length,void *ir_data,uint8_t ir_length,int16_t *acc_data);
typedef void (*ppg_red_and_ir_and_gre_data_callback)(void *red_data,uint8_t red_length,void *ir_data,uint8_t ir_length,void *gre_data,uint8_t gre_length,int16_t *acc_data); 
typedef void (*ppg_pwtt_data_callback)(uint32_t *data,uint8_t length);
typedef void (*ppg_hr_result_callback)(uint8_t heart_rate,uint8_t hrv1);
typedef void (*ppg_hrv_result_callback)(int32_t *hrv_data,uint8_t length);
typedef void (*ppg_spo2_result_callback)(uint8_t spo2,uint8_t heart_rate);
typedef void (*ppg_garyCard_result_callback)(void *green_data,void* red_data,void* ir_data);
typedef void (*ppg_signal_check_callback)(uint16_t signal_strength);


static ppg_green_data_callback          green_data_callback = NULL;
static ppg_red_and_ir_data_callback     red_ir_data_callback = NULL;
static ppg_red_and_ir_and_gre_data_callback  red_and_ir_and_gre_data_callback = NULL;
static ppg_ecg_data_callback            ecg_data_callback = NULL;
static ppg_pwtt_data_callback           pwtt_data_callback = NULL;
static ppg_hr_result_callback           hr_result_callback = NULL;
static ppg_spo2_result_callback		 	spo2_result_callback = NULL;
static ppg_garyCard_result_callback     garyCard_result_callback = NULL;
static ppg_signal_check_callback     	hr_signal_check_callback = NULL;
static ppg_signal_check_callback     	spo2_signal_check_callback = NULL;
static ppg_hrv_result_callback          hrv_result_callback = NULL;

static uint8_t wear_flag = 0;
static uint8_t dataNum = 0;
static float ecg_v = 0;
static GU32 green_data1[10] = {0};
static GU32 red_data1[10] = {0};
static GU32 ir_data1[20] = {0};
static GU32 ecg_data1[80] = {0};
static GU32 gary_data1[3] = {0};

struct gh3026_g_sensor_data
{
	int16_t acc_data[3];
};

static struct gh3026_g_sensor_data g_sensor_data[10];



/* hook functions */

/**
 * @fn     void gh3x2x_init_hook_func(void)
 * 
 * @brief  gh3x2x init hook
 *
 * @attention   None
 *
 * @parbc[in]   None
 * @parbc[out]  None
 *
 * @return  None
 */
void gh3x2x_init_hook_func(void)
{
#if (__ADT_ONLY_PARTICULAR_WM_CONFIG__)
    g_usCurrentConfigListFifoWmBak = 0;
#endif

#if (__DRIVER_LIB_MODE__ == __DRV_LIB_WITH_ALGO__)
    if(0 == GH3x2x_GetChipResetRecoveringFlag())
    {
        GH3X2X_AlgoCallConfigInit(g_pstGh3x2xFrameInfo, g_uchGh3x2xRegCfgArrIndex);
    }
#endif

#if __GH_MSG_WITH_ALGO_LAYER_EN__
    if(0 == GH3x2x_GetChipResetRecoveringFlag())
    {
        GH3X2X_SEND_MSG_ALGO_CFG_INIT(g_pstGh3x2xFrameInfo, g_uchGh3x2xRegCfgArrIndex);
    }
#endif

}

/**
 * @fn     void gh3x2x_sampling_start_hook_func(void)
 * 
 * @brief  gh3x2x start hook
 *
 * @attention   None
 *
 * @parbc[in]   None
 * @parbc[out]  None
 *
 * @return  None
 */
static uint8_t count_temp = 0;
void gh3x2x_sampling_start_hook_func(void)
{
//    GOODIX_PLATFORM_SbcPLING_START_HOOK_ENTITY();
   count_temp = 0;
	wear_flag = 0;
    
#if (CUS_VERSION == 11)
    if(app_get_ppgStatus() == PPG_MEASURE_WAVE_GREEN_SHOUSHI)
    {
        uint8_t temp[4] = {0};
        *(uint32_t*)temp = am_bsp_rtc_getUnixTime();
        app_ppgFile_earse_test();
        app_ppgFile_write_test(temp,sizeof(temp));
    }
#endif
}

/**
 * @fn     void gh3x2x_sampling_stop_hook_func(void)
 * 
 * @brief  gh3x2x stop hook
 *
 * @attention   None
 *
 * @parbc[in]   None
 * @parbc[out]  None
 *
 * @return  None
 */
void gh3x2x_sampling_stop_hook_func(void)
{
//    GOODIX_PLATFORM_SbcPLING_STOP_HOOK_ENTITY();
#if (CUS_VERSION == 11)
    uint8_t temp[5] = {0,0xFF,0xFF,0xFF,0xFF};
    temp[0] = 1;
    app_ppgFile_write_test(temp,sizeof(temp));
#endif
}

/**
 * @fn     void gh3x2x_get_rawdata_hook_func(GU8 *read_buffer_ptr, GU16 length)
 * 
 * @brief  gh3x2x get rawdata hook
 *
 * @attention   None
 *
 * @parbc[in]   read_buffer_ptr     pointer to read rawdata buffer
 * @parbc[in]   length              length
 * @parbc[out]  None
 *
 * @return  None
 */
 typedef struct
{
    GU32 uiAdcCode;                      //sbcpling rawdata of ADC
    GU8  ubSlotNo;                       //slot number
    GU8  ubAdcNo;                        //adc number
    GU8  ubFlagLedAdjIsAgc_EcgRecover;   //adj flag of ppg data or fast recover flag of ecg data
    GU8  ubFlagLedAdjAgcUp;            //adj down flag of ppg data   0: down  1:up
}StFifoDataInformation;
void gh3x2x_get_rawdata_hook_func(GU8 *read_buffer_ptr, GU16 length)
{
#if (GH3X2X_FIFO_MONITOR_EN&(__GH3X2X_INTERFACE__ == __GH3X2X_INTERFACE_I2C__))
     Gh3x2xFifoMonitorPro(&g_stFifoMonitorInfo, read_buffer_ptr, length);
#endif

    
    /* code implement by user */
    /****************** FOLLOWING CODE IS EXbcPLE **********************************/
#if 0
    StFifoDataInformation stTempFifoInfo = {0};
    if (length/4 > 0)
    {
            for (int i = 0; i < length; i += 4)
            {
                GU32 temp = 0;
                // big endian to little endian
                temp =  ((GU32)read_buffer_ptr[i+ 0]) << 24;
                temp += ((GU32)read_buffer_ptr[i+ 1])<< 16;
                temp += ((GU32)read_buffer_ptr[i+ 2]) << 8;
                temp += ((GU32)read_buffer_ptr[i+ 3]);    
                // pick rawdata and flag
                stTempFifoInfo.uiAdcCode = ((temp >> 0) & 0x00FFFFFF);
                stTempFifoInfo.ubSlotNo =  ((temp >> 29) & 0x00000007);
                stTempFifoInfo.ubAdcNo = ((temp >> 27) & 0x00000003);
                stTempFifoInfo.ubFlagLedAdjIsAgc_EcgRecover = ((temp >> 26) & 0x00000001);
                stTempFifoInfo.ubFlagLedAdjAgcUp = ((temp >> 25) & 0x00000001);
                GH3X2X_INFO_LOG("Received rawdata:slot%d,adc%d,AdcCode = %d\r\n", stTempFifoInfo.ubSlotNo, stTempFifoInfo.ubAdcNo, \
                                                                    stTempFifoInfo.uiAdcCode);
                
                {
                    green_data1[data_num++] = stTempFifoInfo.uiAdcCode;
                    if(data_num >= 10)
                    {
                        ring_send_hrWave(data_num,green_data1,acc_data);
                        data_num = 0;
                    }
                }
            }
    }
#endif
}

/**
 * @fn      void gh3x2x_algorithm_get_io_data_hook_func(const STGh3x2xFrameInfo * const pstFrameInfo)
 * 
 * @brief  get algorithm input and output data
 *
 * @attention   None        
 *
 * @parbc[in]   pstFrameInfo
 * @parbc[out]  None
 *
 * @return  None
 */


 
void gh3x2x_algorithm_get_io_data_hook_func(const STGh3x2xFrameInfo * const pstFrameInfo)
{
    /* algo calculate */
#if (__DRIVER_LIB_MODE__ == __DRV_LIB_WITH_ALGO__)
    //GOODIX_PLATFORM_SAMPLING_START_HOOK_ENTITY();
    GH3X2X_AlgoCalculate(pstFrameInfo->unFunctionID);
#endif

#if __GH_MSG_WITH_ALGO_LAYER_EN__
    GH3X2X_SEND_MSG_ALGO_CAL(pstFrameInfo->unFunctionID);
#endif

#if (__SUPPORT_ALGO_INPUT_OUTPUT_DATA_HOOK_CONFIG__)
    /****************** FOLLOWING CODE IS EXAMPLE **********************************/
#if 1
    //function id and channel num
//    GH3X2X_INFO_LOG("[IO_DATA]Function ID: 0x%X, channel num = %d, frame cnt = %d\r\n",(int)(pstFrameInfo->unFunctionID),(int)(pstFrameInfo->pstFunctionInfo->uchChnlNum),(int)(pstFrameInfo->punFrameCnt[0]));
    //gsensor data
//    GH3X2X_INFO_LOG("[IO_DATA]Gsensor: x = %d, y = %d, z = %d\r\n",\
//                        (int)(pstFrameInfo->pusFrameGsensordata[0]),\
//                        (int)(pstFrameInfo->pusFrameGsensordata[1]),\
//                        (int)(pstFrameInfo->pusFrameGsensordata[2])\
//                );
    //rawdata
//    printf("lllllllrrrrr rrrrrrrrrr \r\n");
//    for(GU8 uchChnlCnt = 0; uchChnlCnt < pstFrameInfo->pstFunctionInfo->uchChnlNum; uchChnlCnt ++)
//    {
//        GH3X2X_INFO_LOG("[IO_DATA]Ch%d rawdata= %d\r\n",(int)(uchChnlCnt),(int)(pstFrameInfo->punFrameRawdata[uchChnlCnt]));
//    }

     switch(pstFrameInfo->unFunctionID)
	 {
		 case GH3X2X_FUNCTION_HR:
		 {
//       printf("GH3X2X_FUNCTION_HR \r\n");
			 g_sensor_data[dataNum].acc_data[0] = pstFrameInfo->pusFrameGsensordata[0];
			 g_sensor_data[dataNum].acc_data[1] = pstFrameInfo->pusFrameGsensordata[1];
			 g_sensor_data[dataNum].acc_data[2] = pstFrameInfo->pusFrameGsensordata[2];
			 green_data1[dataNum++] = pstFrameInfo->punFrameRawdata[0];
			
			 if(green_data_callback != NULL && dataNum >= 10)
			 {
				// printf("lllllllllllllllll   %d  %d\r\n",dataNum,count_temp++);
				 green_data_callback(green_data1,(int16_t*)&g_sensor_data,dataNum);
				 dataNum = 0;
				 if(hr_signal_check_callback != NULL)
				 {
					 hr_signal_check_callback(wear_flag);
				 }
			 }
			 
			 if(pstFrameInfo->pstAlgoResult->uchUpdateFlag == 1 && hr_result_callback != NULL)
			 {
//         printf("lllllfffffff   %d  %d\r\n",pstFrameInfo->pstAlgoResult->uchUpdateFlag,pstFrameInfo->pstAlgoResult->snResult[0]);
				 hr_result_callback(pstFrameInfo->pstAlgoResult->snResult[0],0);
			 }
			 break;
		 }
		 case GH3X2X_FUNCTION_HRV:
		 {
			 if(pstFrameInfo->pstAlgoResult->uchUpdateFlag == 1 && hrv_result_callback != NULL)
			{
        printf("GH3X2X_FUNCTION_HRV  %d  %d  %d %d %d %d\r\n",pstFrameInfo->pstAlgoResult->snResult[0],pstFrameInfo->pstAlgoResult->snResult[1],
		                                                   pstFrameInfo->pstAlgoResult->snResult[2],pstFrameInfo->pstAlgoResult->snResult[3],
		                                                   pstFrameInfo->pstAlgoResult->snResult[4],pstFrameInfo->pstAlgoResult->snResult[5]);
        if(pstFrameInfo->pstAlgoResult->snResult[4] >= 70)
        {
          hrv_result_callback(&pstFrameInfo->pstAlgoResult->snResult[0],pstFrameInfo->pstAlgoResult->snResult[5]);
        }
			}
			 break;
		 }
		 case GH3X2X_FUNCTION_SPO2:
		 {
			 g_sensor_data[dataNum].acc_data[0] = pstFrameInfo->pusFrameGsensordata[0];
			g_sensor_data[dataNum].acc_data[1] = pstFrameInfo->pusFrameGsensordata[1];
			g_sensor_data[dataNum].acc_data[2] = pstFrameInfo->pusFrameGsensordata[2];
			red_data1[dataNum] = pstFrameInfo->punFrameRawdata[0];
			ir_data1[dataNum++] = pstFrameInfo->punFrameRawdata[1];
			//printf("lllllllllllllllll   %d \r\n",dataNum);
			if(red_ir_data_callback != NULL && dataNum >= 10)
			{
				red_ir_data_callback(red_data1,dataNum,ir_data1,dataNum,(int16_t*)&g_sensor_data);
				dataNum = 0;
				if(spo2_signal_check_callback != NULL)
				{
					spo2_signal_check_callback(wear_flag);
				}
			}
			
			if(pstFrameInfo->pstAlgoResult->uchUpdateFlag == 1 && spo2_result_callback != NULL)
			{
        printf("lllllgggggggggg   %d  %d\r\n",pstFrameInfo->pstAlgoResult->uchUpdateFlag,pstFrameInfo->pstAlgoResult->snResult[0]);
				spo2_result_callback(pstFrameInfo->pstAlgoResult->snResult[0],pstFrameInfo->pstAlgoResult->snResult[4]);
			}
			 break;
		 }
		 case GH3X2X_FUNCTION_ECG:
		 {
			 //        ecg_v = ((float)pstFrameInfo->punFrameRawdata[0] - 8388608.0f) * 1.8f / 8388608.0f / 20.0f;
	//        float aaa = (float)pstFrameInfo->punFrameRawdata[0] - 8388608.0f;
	//        float bbb = aaa * 1.8f / 8388608.0f / 20.0f;
	//        ecg_data1[dataNum++] = bbb * 1000000.0f;
			ecg_data1[dataNum++] = pstFrameInfo->punFrameRawdata[0];
	//        GH3X2X_INFO_LOG("dataNum : %d %d %f %f %f %d \r\n",dataNum,pstFrameInfo->punFrameRawdata[0],aaa,bbb,bbb * 1000000.0f,ecg_data1[dataNum-1]);
			if(ecg_data_callback != NULL && dataNum >= 40)
			{
				ecg_data_callback(ecg_data1,dataNum);
				dataNum = 0;
			}
			 break;
		 }
		 case GH3X2X_FUNCTION_TEST1:
		 {
			gary_data1[0] = pstFrameInfo->punFrameRawdata[0] < pstFrameInfo->punFrameRawdata[1] ? pstFrameInfo->punFrameRawdata[0] : pstFrameInfo->punFrameRawdata[1];
			gary_data1[1] = pstFrameInfo->punFrameRawdata[4] < pstFrameInfo->punFrameRawdata[5] ? pstFrameInfo->punFrameRawdata[4] : pstFrameInfo->punFrameRawdata[5];
			gary_data1[2] = pstFrameInfo->punFrameRawdata[2] < pstFrameInfo->punFrameRawdata[3] ? pstFrameInfo->punFrameRawdata[2] : pstFrameInfo->punFrameRawdata[3];
			
			if(garyCard_result_callback != NULL)
			{
				garyCard_result_callback(&gary_data1[0],&gary_data1[2],&gary_data1[1]);
			}
			GH3X2X_INFO_LOG("garyCard : %d %d %d %d %d %d \r\n",pstFrameInfo->punFrameRawdata[0],pstFrameInfo->punFrameRawdata[1],pstFrameInfo->punFrameRawdata[2],pstFrameInfo->punFrameRawdata[3],pstFrameInfo->punFrameRawdata[4],pstFrameInfo->punFrameRawdata[5]);
				 
			 break;
		 }
		 case GH3X2X_FUNCTION_TEST2:
		 {
			 g_sensor_data[dataNum].acc_data[0] = pstFrameInfo->pusFrameGsensordata[0];
			 g_sensor_data[dataNum].acc_data[1] = pstFrameInfo->pusFrameGsensordata[1];
			 g_sensor_data[dataNum].acc_data[2] = pstFrameInfo->pusFrameGsensordata[2];
			 green_data1[dataNum] = pstFrameInfo->punFrameRawdata[0];
			 ir_data1[dataNum] = pstFrameInfo->punFrameRawdata[1];
			 red_data1[dataNum++] = pstFrameInfo->punFrameRawdata[2];
       
//        printf("pstFrameInfo;%d   %d  %d \r\n \r\n",pstFrameInfo->punFrameRawdata[0],pstFrameInfo->punFrameRawdata[1],pstFrameInfo->punFrameRawdata[2]);
			 
#if(HARDWARE_1141_ENABLED == 1 || HARDWARE_1142_ENABLED == 1)
			 if(red_and_ir_and_gre_data_callback != NULL && dataNum >= 5)
			 {
//				 printf("GH3X2X_FUNCTION_TEST2 \r\n");
				 red_and_ir_and_gre_data_callback(red_data1,dataNum,ir_data1,dataNum,green_data1,dataNum,(int16_t*)&g_sensor_data);
				 dataNum = 0;
			 }
#else
			if(red_and_ir_and_gre_data_callback != NULL && dataNum >= 10)
			 {
				 printf("GH3X2X_FUNCTION_TEST2 \r\n");
				 red_and_ir_and_gre_data_callback(red_data1,dataNum,ir_data1,dataNum,green_data1,dataNum,(int16_t*)&g_sensor_data);
				 dataNum = 0;
			 }

#endif			 
			 
	 
			 break;
		 }
		 case GH3X2X_FUNCTION_SOFT_ADT_GREEN:
		 {
			  static uint8_t wear_num = 0;
				dataNum++;
				if(dataNum >= 10)
				{
					dataNum = 0;
					if(pstFrameInfo->pstAlgoResult->uchUpdateFlag == 1)
					{

						if(pstFrameInfo->pstAlgoResult->snResult[1] >= 80)
						{
							wear_num++;
						}

						if(pstFrameInfo->pstAlgoResult->snResult[0] == 1 || wear_num >= 2)     //Åå´÷
						{
							wear_num = 0;
							wear_flag = 1;
							Gh3x2xDemoStopSampling(GH3X2X_FUNCTION_SOFT_ADT_GREEN);
							GH3X2X_FifoWatermarkThrConfig(20);
						}
		//                GH3X2X_INFO_LOG("nadt_cnt = %d \r\n",nadt_cnt);
						BC_LOG_INFO("======== nadt = %d %d %d 0x%x \r\n",pstFrameInfo->pstAlgoResult->snResult[0], pstFrameInfo->pstAlgoResult->snResult[1], pstFrameInfo->pstAlgoResult->uchResultNum, pstFrameInfo->pstAlgoResult->usResultBit);
					}
				}
			 break;
		 }
		 case GH3X2X_FUNCTION_BT:
		 {
			 break;
		 }
	 }
#endif
#endif
}

void gh3x2x_frame_data_hook_func(const STGh3x2xFrameInfo * const pstFrameInfo)
{
#if (__DRIVER_LIB_MODE__ == __DRV_LIB_WITH_ALGO__)
    if (GH3X2X_TimestampSyncGetFrameDataFlag() == 0 && GH3X2X_GetGsensorEnableFlag())
    {
        /* user set time stbcp code here */
        GU32 unTimeStbcp = 0;
        //GU8 uchFuncOffsetID = 0;
        //for(GU8 uchFunCnt = 0; uchFunCnt < __SYNC_FUNC_MAX__; uchFunCnt ++)
        //{
        //    if (((1<<uchFunCnt) & pstFrameInfo->unFunctionID) == (1<<uchFunCnt))
        //    {
        //        uchFuncOffsetID = uchFunCnt;
        //        break;
        //    }
        //}
        //unTimeStbcp = func_ppg_timestbcp[uchFuncOffsetID];
        GH3X2X_TimestampSyncFillPpgSyncBuffer(unTimeStbcp, pstFrameInfo);
        //func_ppg_timestbcp[uchFuncOffsetID] += 40;
    }
    else
    {
#endif
        gh3x2x_algorithm_get_io_data_hook_func(pstFrameInfo);
#if (__DRIVER_LIB_MODE__ == __DRV_LIB_WITH_ALGO__)
    }
#endif
}

void gh3x2x_reset_by_protocol_hook(void)
{
    //Gh3x2x_BspDelayMs(20);
    //Gh3x2xDemoInterruptProcess();
    GH3X2X_INFO_LOG("[%s]:handle protocol reset\r\n", __FUNCTION__);
}

void gh3x2x_config_set_start_hook(void)
{
    /* code implement by user */
}

void gh3x2x_config_set_stop_hook(void)
{
    Gh3x2xIntPinTriggerModePro();
    GH3x2xSlotTimeInfo();
#if (__DRIVER_LIB_MODE__ == __DRV_LIB_WITH_ALGO__)
    GH3X2X_AlgoSensorEnable(GH3X2X_GetGsensorEnableFlag(),GH3X2X_GetCapEnableFlag(),GH3X2X_GetTempEnableFlag());
#endif
}

void gh3x2x_write_algo_config_hook(GU16 usVirtualRegAddr, GU16 usVirtualRegValue)
{
#if (__DRIVER_LIB_MODE__ == __DRV_LIB_WITH_ALGO__)
    GH3X2X_WriteAlgConfigWithVirtualReg(usVirtualRegAddr, usVirtualRegValue);
#endif
}


#if (__FUNC_TYPE_ECG_ENABLE__)
/**
 * @fn     void Gh3x2x_LeadOnEventHook(void)
 * 
 * @brief  Lead on event hook
 *
 * @attention   None        
 *
 * @parbc[in]   None
 * @parbc[out]  None
 *
 * @return  None
 */
void Gh3x2x_LeadOnEventHook(void)
{
    GOODIX_PLATFORM_LEAD_ON_EVENT();
}

/**
 * @fn     void Gh3x2x_LeadOffEventHook(void)
 * 
 * @brief  Lead off event hook
 *
 * @attention   None        
 *
 * @parbc[in]   None
 * @parbc[out]  None
 *
 * @return  None
 */
void Gh3x2x_LeadOffEventHook(void)
{
    GOODIX_PLATFORM_LEAD_OFF_EVENT();
}
#endif



#if __GH_MULTI_SENSOR_EVENT_PRO_CONIG__

extern const STGhMultiSensorTimerOps g_stGhMultiSensorTimerOps;

void GhMultiSensorWearDetStop(void)
{
    GhMultiSensorTimerStop();
    GhMultiSensorTimerOpsUnregister();

    GhMultSensorWearEventManagerDisable();
    GhGsMoveDetecterDisable();
}


/** @brief hook when new multi sensor wear event is generated,
 you can do operation as below in this hook. 
firstly, you can check event record in Event list via API Gh3x2xMultiSensorConditionCheckInWindow or Gh3x2xMultiSensorConditionCheckIsNew;
soconly, you can disable multi sensor event manager via API GhMultSensorWearEventManagerDisable
thirdly, you can open/close function via API Gh3x2xDemoStartSampling/Gh3x2xDemoStopSampling according event record 
**/
/// @parbc uchNewEvent such as: GH3X2X_MULTI_SENSOR_EVENT_GH_WEAR_ON
void GhMultiSensorEventHook(GU32 uchNewEvent)
{
    #if 1
    if(GH3X2X_MULTI_SENSOR_EVENT_GS_MOVE_TIME_OUT == uchNewEvent)
    {
        GH3X2X_DEBUG_LOG("[GhMultiSensorEventHook] gs move time out, stop adt\r\n");
        Gh3x2xDemoStopSampling(GH3X2X_FUNCTION_ADT);
    }
    if(GH3X2X_MULTI_SENSOR_EVENT_GS_MOVE == uchNewEvent)
    {
        GH3X2X_DEBUG_LOG("[GhMultiSensorEventHook] sensor gs move, start adt\r\n");
        Gh3x2xDemoStartSampling(GH3X2X_FUNCTION_ADT);
    }
    if(GhMultiSensorConditionCheckIsNew(GH3X2X_MULTI_SENSOR_EVENT_GH_WEAR_ON, GH3X2X_MULTI_SENSOR_EVENT_GH_WEAR_OFF))
    {
        if(GhMultiSensorConditionCheckInWindow(GH3X2X_MULTI_SENSOR_EVENT_GS_MOVE, 3000, 1))
        {
            //final wear on
            GH3X2X_INFO_LOG("[GhMultiSensorEventHook] final wear on !!!! \r\n");
            GhMultiSensorWearDetStop();
            #if (__SUPPORT_PROTOCOL_ANALYZE__)
            Gh3x2xDemoReportEvent(GH3X2X_IRQ_MSK_WEAR_ON_BIT, GH3X2X_EVENT_EX_BIT_MULTI_SENSOR);
            #endif
            /****************START:   add your handle you want after final wear on ***************/
            #if (__GH_MULTI_SENSOR_WEAR_EXbcPLE_EN__&&__GH_MULTI_SENSOR_EVENT_PRO_CONIG__)
            Gh3x2xDemoStartSampling(GH3X2X_FUNCTION_HR|GH3X2X_FUNCTION_SOFT_ADT_GREEN);
            #endif

            /****************END:   add your handle you want after final wear on ***************/

        }
    }
    #endif
}

void GhMultiSensorWearDetStart(void)
{
    GH3X2X_DEBUG_LOG("[GhMultiSensorWearDetStart] Multi Sensor Wear Detect Start\r\n");
    Gh3x2xDemoStopSampling(GH3X2X_FUNCTION_ADT);

    GhMultiSensorTimerInit();
    GhMultiSensorTimerOpsRegister(&g_stGhMultiSensorTimerOps);
    GhMultiSensorTimerStart();

    GhMultSensorWearEventManagerInit();
    GhMultSensorWearEventManagerHookRegister(GhMultiSensorEventHook);
    GhMultSensorWearEventManagerEnable();

    GhGsMoveDetecterInit();
    GhGsMoveDetecterEnable();
}
#endif


#if (__SUPPORT_HARD_ADT_CONFIG__)
/**
 * @fn     extern void Gh3x2x_WearEventHook(GU16 usGotEvent, GU8 uchWearOffType);
 * 
 * @brief  Wear event hook
 *
 * @attention   None        
 *
 * @parbc[in]   wear event
 * @parbc[in]   wear off type  0: no object  1: nonliving object    wear on type   0: object     1: living object
 * @parbc[out]  None
 *
 * @return  None
 */



extern GU8 g_uchNeedStartMultiSensorWearOn;
void Gh3x2x_WearEventHook(GU16 usGotEvent, GU8 uchExentEx)
{
    if (usGotEvent & GH3X2X_IRQ_MSK_WEAR_OFF_BIT)
    {
        
        GOODIX_PLATFORM_WEAR_OFF_EVENT();
        if(uchExentEx&GH3X2X_EVENT_EX_BIT_WEAR_LIVING_TYPE)
        {
            GOODIX_PLATFORM_NONLIVING_WEAR_OFF_EVENT();
            GH3X2X_INFO_LOG("Wear off, no living-object!!!\r\n");
            #if (__GH_MULTI_SENSOR_WEAR_EXbcPLE_EN__&&__GH_MULTI_SENSOR_EVENT_PRO_CONIG__)
            Gh3x2xDemoStopSampling(GH3X2X_FUNCTION_HR|GH3X2X_FUNCTION_SOFT_ADT_GREEN);
            GhMultiSensorWearDetStart();
            #endif

        }
        else
        {
            GH3X2X_INFO_LOG("Wear off, no object!!!\r\n");
            #if (__GH_MULTI_SENSOR_WEAR_EXbcPLE_EN__&&__GH_MULTI_SENSOR_EVENT_PRO_CONIG__)
            if(0 == GhMultSensorWearEventManagerIsEnable())   // multi-sensor wear detection is not opened
            {
                                GH3X2X_DEBUG_LOG("[Gh3x2x_WearEventHook] no object!!! stop hr and soft_adt\r\n");
                Gh3x2xDemoStopSampling(GH3X2X_FUNCTION_HR|GH3X2X_FUNCTION_SOFT_ADT_GREEN);
            }
            #endif
            #if __GH_MULTI_SENSOR_EVENT_PRO_CONIG__
            GhMultSensorWearEventSend(GH3X2X_MULTI_SENSOR_EVENT_GH_WEAR_OFF);
            #endif
        }
        
        wear_flag = 0;

    }
    else if (usGotEvent & GH3X2X_IRQ_MSK_WEAR_ON_BIT)
    {

        GOODIX_PLATFORM_WEAR_ON_EVENT();

        if(uchExentEx&GH3X2X_EVENT_EX_BIT_WEAR_LIVING_TYPE)
        {
            GH3X2X_INFO_LOG("Wear on, living-object!!!\r\n");
        }
        else
        {
            GH3X2X_INFO_LOG("Wear on, object !!!\r\n");

            #if (__GH_MULTI_SENSOR_WEAR_EXbcPLE_EN__&&__GH_MULTI_SENSOR_EVENT_PRO_CONIG__)
            if(0 == GhMultSensorWearEventManagerIsEnable())   // multi-sensor wear detection is not opened
            {
                Gh3x2xDemoStartSampling(GH3X2X_FUNCTION_HR|GH3X2X_FUNCTION_SOFT_ADT_GREEN);  // open main function
            }
            #endif
            #if __GH_MULTI_SENSOR_EVENT_PRO_CONIG__
            GhMultSensorWearEventSend(GH3X2X_MULTI_SENSOR_EVENT_GH_WEAR_ON);
            #endif
        }
        
        wear_flag = 1;
    }
}
#endif

#if (__GH3X2X_CASCADE_EN__)
void Gh3x2x_WearEventCascadeEcgHandle(GU16 usGotEvent)
{
    if (usGotEvent & GH3X2X_IRQ_MSK_WEAR_OFF_BIT)
    {
        if (GH3X2X_CascadeGetEcgEnFlag())
        {
            GH3X2X_CascadeEcgSlaverLeadDectDis();
        }
    }
    else if (usGotEvent & GH3X2X_IRQ_MSK_WEAR_ON_BIT)
    {
        if (GH3X2X_CascadeGetEcgEnFlag())
        {
            GH3X2X_CascadeEcgSlaverLeadDectEn();
            
        }
    }
}
#endif

void Gh3x2x_BeforeWakeUpHook(void)
{
    GOODIX_PLATFORM_BEFORE_WAKE_UP_HOOK_ENTITY();
}
void Gh3x2x_WakeUpHook(void)
{
    GOODIX_PLATFORM_WAKE_UP_HOOK_ENTITY();
}

uint8_t Gh3x2x_get_wearFlag(void)
{
    return wear_flag;
}

bool gh3228t_hr_data_callback_register(void *function_callback)
{
    if(function_callback == NULL)
    {
        return false;
    }
    green_data_callback = (ppg_green_data_callback)function_callback;
    return true;
}

bool gh3228t_spo2_data_callback_register(void *function_callback)
{
    if(function_callback == NULL)
    {
        return false;
    }
    red_ir_data_callback = (ppg_red_and_ir_data_callback)function_callback;
    return true;
}

bool gh3228t_spo2_hr_data_callback_register(void *function_callback)
{
	if(function_callback == NULL)
	{
		return false;
	}
	red_and_ir_and_gre_data_callback = (ppg_red_and_ir_and_gre_data_callback)function_callback;
	return true;
}


bool gh3228t_ecg_data_callback_register(void *function_callback)
{
    if(function_callback == NULL)
    {
        return false;
    }
    ecg_data_callback = (ppg_ecg_data_callback)function_callback;
    return true;
}

bool gh3228t_pwtt_data_callback_register(void *function_callback)
{
    if(function_callback == NULL)
    {
        return false;
    }
    pwtt_data_callback = (ppg_pwtt_data_callback)function_callback;
    return true;
}

bool gh3228t_hr_result_callback_register(void *function_callback)
{
    if(function_callback == NULL)
    {
        return false;
    }
    hr_result_callback = (ppg_hr_result_callback)function_callback;
    return true;
}


bool gh3228t_hrv_result_callback_register(void *function_callback)
{
    if(function_callback == NULL)
    {
        return false;
    }
    hrv_result_callback = (ppg_hrv_result_callback )function_callback;
    return true;
}

bool gh3228t_spo2_result_callback_register(void *function_callback)
{
    if(function_callback == NULL)
    {
        return false;
    }
    spo2_result_callback = (ppg_spo2_result_callback)function_callback;
    return true;
}

bool gh3228t_spo2_signal_check_callback_register(void *function_callback)
{
    if(function_callback == NULL)
    {
        return false;
    }
    spo2_signal_check_callback = (ppg_signal_check_callback)function_callback;
    return true;
}

bool gh3228t_hr_signal_check_callback_register(void *function_callback)
{
    if(function_callback == NULL)
    {
        return false;
    }
    hr_signal_check_callback = (ppg_signal_check_callback)function_callback;
    return true;
}

bool gh3228t_garyCard_result_callback_register(void *function_callback)
{
    if(function_callback == NULL)
    {
        return false;
    }
    garyCard_result_callback = (ppg_garyCard_result_callback)function_callback;
    return true;
}

void gh3228t_clear_dataNum(void)
{
    dataNum = 0;
}


