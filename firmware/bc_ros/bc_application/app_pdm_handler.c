#include "app_pdm_handler.h"
#include "bc_ble.h"
#include "bc_watchdog.h"
#include "bc_pdm.h"

#if defined(HANDWARE_1_23_2_ONE_SEC)
#include "bc_pmic.h"
#include "app_linear_motor_handler.h"
#include "app_pmic_handler.h"
#endif

#if defined(HANDWARE_1_23_2)
#include "app_linear_motor_handler.h"
#endif


#include <nrfx_pdm.h>
#include "nrf_gpio.h"

#include "bc_queue.h"
#include "bc_logger.h"
#include "bc_sem.h"
#include "bc_ble_modu_interface.h"
//#include "bc_led.h"
#include "bc_delay.h"
#include "bc_ldo_switch.h"
//#include "bc_ic_led.h"
#include "bc_alg_adpcm.h"
#include "bc_device_info.h"

#include "app_package.h"
#include "app_ble_handler.h"
#include "app_ppg_data_handler.h"
#include "app_touch_button_handler.h"

#include "app_opus.h"

#if (defined(HANDWARE_1_17_1)   || defined(HANDWARE_1_19_1) || defined(HANDWARE_1_23_1) || defined(HANDWARE_1_23_4))
 
#include "app_ppg_file_data_handler.h"
 
#endif 

#if ( HARDWARE_1191_ENABLED == 1)	

#include "bc_led_pwm.h"

#endif	



#if ( HARDWARE_1231_ENABLED == 1)	

#include "bc_ic_led.h"
#if (defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4 ))
#include "bc_linear_motor_ic.h"
#else
#include "bc_linear_motor.h"
#endif

#endif	

#include "yamaha_adpcm_a.h"
#include "yamaha_adpcm_xs.h"

#include "nrf_gpio.h"

#include "string.h"
#include "adpcm_a.h"

#include "bc_waveform_generator.h"
#if defined(TEST_ADPCM_GEN)
static MonoAdpcmProcessor mono_processor;
#endif

#if( defined(HANDWARE_1_23_1) )
static MonoAdpcmProcessor mono_processor;
#else
static  StereoAdpcmProcessor processor;
#endif



//YamahaADPCM_normal codec;

static struct bc_pdm_package  pdm_package_1 = {0};
static struct bc_pdm_package  pdm_package_2 = {0};

static struct bc_adpcm_package  adpcm_package = {0};
static struct bc_adpcm_package  dequeue_adpcm_package = {0};

static bool buffer_requested_flag = false;
static struct bc_ble_calss ble_calss = {0};
static bool pdm_poll_flag = false;
static bool pdm_stop_flag = true;

#if defined(HANDWARE_1_23_3)
static bool offline_pdm_on = false;

static void app_pdm_offline_notify(uint8_t on_off)
{
    struct app_cmd_package package = {0};
    package.cmd = 0x71;
    package.frame_id = 0x9;
    package.subcmd = 0x0c;
    package.data[0] = on_off;
    app_package_send_enqueue(&package, 4 + 1);
}
#endif

#pragma pack (1)
struct bc_pdm_ble_package
{
	struct app_package_basic package_basic;
//	struct bc_pdm_package  pdm_package;
  struct bc_adpcm_package  pdm_package;
};
#pragma pack ()
struct bc_pdm_ble_package  pdm_ble_package = {0};

enum app_pdm_event
{
	PDM_SEND = 0,
#if (defined(HANDWARE_1_17_1)   || defined(HANDWARE_1_19_1)  || defined(HANDWARE_1_23_1))
	PDM_IRQ_HANDLER,
#endif   

	PDM_TASK_TYPE_NUM
};
enum bc_pdm_status
{
	PDM_IDIE = 0,
	PDM_WORK,
};



static enum bc_pdm_status pdm_status = PDM_IDIE;
uint16_t pdm_seq = 0;

#if defined(HANDWARE_1_23_4)
/* 录音暂停标志 */
static bool pdm_paused = false;
#endif

#if defined(HANDWARE_1_23_2_ONE_SEC)
/* 录音优先级：数字越大优先级越高 */
enum pdm_record_priority
{
    PDM_PRIO_NONE = 0,
    PDM_PRIO_TIMER,     // 定时自动录音（最低）
    PDM_PRIO_TOUCH,     // 双击录音（中）
    PDM_PRIO_APP,       // APP开启录音（最高）
};
static enum pdm_record_priority current_record_prio = PDM_PRIO_NONE;
static enum pdm_record_priority next_record_prio = PDM_PRIO_TOUCH; // 默认双击优先级
static bool timer_record_preempted = false;                         // 定时录音是否被高优先级抢占
#endif

#if defined(HANDWARE_1_23_2)
/*******************************************************************************
 * Function Name     : app_pdm_motor_start_by_config
 * Description       : 根据配置触发开启录音马达
 * Input             :
 * Output            :
 * Return            :
 *******************************************************************************/
static void app_pdm_motor_start_by_config(void)
{
    bc_device_led_motor_mode_info info = {0};
    bc_device_info_led_motor_mode_info_get(&info);
    if(info.motor_start_mode == 0x02)
    {
        app_vibrate_start(VIBRATE_MODE_LONG, 1);
    }
    else
    {
        app_vibrate_start(VIBRATE_MODE_SHORT, 1);
    }
}

/*******************************************************************************
 * Function Name     : app_pdm_motor_stop_by_config
 * Description       : 根据配置触发关闭录音马达
 * Input             :
 * Output            :
 * Return            :
 *******************************************************************************/
static void app_pdm_motor_stop_by_config(void)
{
    bc_device_led_motor_mode_info info = {0};
    bc_device_info_led_motor_mode_info_get(&info);
    if(info.motor_stop_mode == 0x02)
    {
        app_vibrate_start(VIBRATE_MODE_LONG, 2);
    }
    else
    {
        app_vibrate_start(VIBRATE_MODE_SHORT, 2);
    }
}
#endif


static void app_pdm_handler_thread(void *thread_handler);
static void app_pdm_irq_handler_thread(void *thread_handler);

static bc_rtos_thread_struct thread_struct[PDM_TASK_TYPE_NUM] = {
                                                                    {
                                                                      .thread_name          = "pdm send handler",
                                                                      .thread_stack_depth   = APP_TASK_MIC_SED_STACK_SIZE ,
                                                                      .thread_priority      = APP_TASK_MIC_SED_PRIO,
                                                                      .thread_parameters    = NULL,
                                                                      .thread_task_code     = app_pdm_handler_thread,
                                                                    },	
#if (defined(HANDWARE_1_17_1)   || defined(HANDWARE_1_19_1) || defined(HANDWARE_1_23_1))
                                                                    {
                                                                      .thread_name          = "pdm irq handler",
                                                                      .thread_stack_depth   = APP_TASK_MIC_IRQ_STACK_SIZE ,
                                                                      .thread_priority      = APP_TASK_MIC_IRQ_PRIO,
                                                                      .thread_parameters    = NULL,
                                                                      .thread_task_code     = app_pdm_irq_handler_thread,
                                                                    }, 
#endif                                                                       
                                                                                                                                       
                                                                 };


static uint8_t pdm_data_count_temp = 0;
static uint8_t adpdm_data_count_temp = 0;

#if defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_2_ONE_SEC)
static bool pdm_online_save_to_flash = false;  // 在线录音同时保存到本地Flash的标记
static uint16_t pdm_ble_tx_delay_count = 0;    // 蓝牙重连后BLE发送延迟计数
#endif
                                                                  
static   enum app_pdm_mode   pdm_mode =   PDM_MODE_IDIE;

static void app_pdm_mode_set(enum app_pdm_mode mode) 
{
    bc_rtos_taskENTER_CRITICAL();
  pdm_mode = mode;
    bc_rtos_taskEXIT_CRITICAL();
}  


enum app_pdm_mode app_pdm_mode_get(void) 
{
  return pdm_mode;
}  


void buffer_event_handler(nrfx_pdm_evt_t *p_evt)
{
  nrfx_err_t err_code;
  //BC_LOG_INFO("p_evt->buffer_requested:%d\r\n",p_evt->buffer_requested);
  if(p_evt->buffer_requested==true)
  {	  
#if ( defined(HANDWARE_1_17_1) || defined(HANDWARE_1_19_1) ) 
      if(!buffer_requested_flag)
      {
        err_code=nrfx_pdm_buffer_set(pdm_package_2.pdm_data_buff,400*2);
        APP_ERROR_CHECK(err_code);
        buffer_requested_flag = true;

          //bc_rtos_thread_notify_give_from_isr(thread_struct[PDM_IRQ_HANDLER].thread_handler,bc_pdFALSE);
          bc_rtos_thread_notify_give(thread_struct[PDM_IRQ_HANDLER].thread_handler);
        
      }
      else
      {
        err_code=nrfx_pdm_buffer_set(pdm_package_1.pdm_data_buff,400*2);
        APP_ERROR_CHECK(err_code);
        buffer_requested_flag = false;

          //bc_rtos_thread_notify_give_from_isr(thread_struct[PDM_IRQ_HANDLER].thread_handler,bc_pdFALSE);
        bc_rtos_thread_notify_give(thread_struct[PDM_IRQ_HANDLER].thread_handler);
      }
#elif ( defined(HANDWARE_1_23_1) )     
       if(!buffer_requested_flag)
      {
        err_code=nrfx_pdm_buffer_set(pdm_package_2.pdm_data_buff,PDM_DATA_BUFF_SIZE);
        APP_ERROR_CHECK(err_code);
        buffer_requested_flag = true;

          //bc_rtos_thread_notify_give_from_isr(thread_struct[PDM_IRQ_HANDLER].thread_handler,bc_pdFALSE);
          bc_rtos_thread_notify_give(thread_struct[PDM_IRQ_HANDLER].thread_handler);
        
      }
      else
      {
        err_code=nrfx_pdm_buffer_set(pdm_package_1.pdm_data_buff,PDM_DATA_BUFF_SIZE);
        APP_ERROR_CHECK(err_code);
        buffer_requested_flag = false;

          //bc_rtos_thread_notify_give_from_isr(thread_struct[PDM_IRQ_HANDLER].thread_handler,bc_pdFALSE);
        bc_rtos_thread_notify_give(thread_struct[PDM_IRQ_HANDLER].thread_handler);
      }     
#else
      pdm_seq+=1;
        *(uint32_t*)&adpcm_package.pdm_data_buff[2] = pdm_seq;
        if(pdm_seq >= 1)
        {
          for(uint8_t i = 0 ; i <200;i++)
          {
            
            pdm_package_1.pdm_data_buff[i] = pdm_package_1.pdm_data_buff[i*2];
          }			
        }
        yma_encode(pdm_package_1.pdm_data_buff, &adpcm_package.pdm_data_buff[6], 200);
        bc_queue_isr_enqueue(BC_QUEUE_TYPE_PDM_COLLECTION_DATA,&adpcm_package);
        err_code=nrfx_pdm_buffer_set(pdm_package_1.pdm_data_buff,400);
        APP_ERROR_CHECK(err_code);
#endif       



//	 BC_LOG_INFO("pdm time\r\n");
	 pdm_poll_flag = true;
  }
  
}

nrfx_pdm_config_t pdm_config =
{
 #if ( defined(HANDWARE_1_17_1)  || defined(HANDWARE_1_19_1)) 
      .mode=                (nrf_pdm_mode_t)NRF_PDM_MODE_STEREO,
      .edge=                (nrf_pdm_edge_t)NRF_PDM_EDGE_LEFTRISING,
#elif (defined(HANDWARE_1_18_1) || defined(HANDWARE_1_23_1))
      .mode=                (nrf_pdm_mode_t)NRF_PDM_MODE_MONO,
      .edge=                (nrf_pdm_edge_t)NRF_PDM_EDGE_LEFTRISING,
#endif  
#if defined(HANDWARE_1_5_3)

	  .pin_clk=              NRF_GPIO_PIN_MAP(0,30),
      .pin_din=              NRF_GPIO_PIN_MAP(0,3),
#elif defined(HANDWARE_1_12_1)

	  .pin_clk=              NRF_GPIO_PIN_MAP(1,9),
      .pin_din=              NRF_GPIO_PIN_MAP(0,12),	
#elif defined(HANDWARE_1_18_1)

	  .pin_clk=              NRF_GPIO_PIN_MAP(0,04),
      .pin_din=              NRF_GPIO_PIN_MAP(0,21),	
  
#elif defined(HANDWARE_1_5_8)

	  .pin_clk=              NRF_GPIO_PIN_MAP(0,31),
      .pin_din=              NRF_GPIO_PIN_MAP(0,4),	
#elif defined(HANDWARE_1_17_1)

	  .pin_clk=              NRF_GPIO_PIN_MAP(0,15),
      .pin_din=              NRF_GPIO_PIN_MAP(0,14),	
      
#elif defined(HANDWARE_1_19_1)

	  .pin_clk=              NRF_GPIO_PIN_MAP(1,14),
    .pin_din=              NRF_GPIO_PIN_MAP(1,10),   
#elif defined(HANDWARE_1_23_1)
#if defined(HANDWARE_1_23_3)
	  .pin_clk=              NRF_GPIO_PIN_MAP(0,15),
    .pin_din=              NRF_GPIO_PIN_MAP(0,11), 
#elif defined(HANDWARE_1_23_4 )
     .pin_clk=              NRF_GPIO_PIN_MAP(1,9),
    .pin_din=              NRF_GPIO_PIN_MAP(0,4), 
#else
	  .pin_clk=              NRF_GPIO_PIN_MAP(0,04),
    .pin_din=              NRF_GPIO_PIN_MAP(0,21), 
#endif
#elif (defined(HANDWARE_BCL601_151))
	  .pin_clk=              NRF_GPIO_PIN_MAP(0,30),
      .pin_din=              NRF_GPIO_PIN_MAP(0,28),	  
#endif		

      .clock_freq=          (nrf_pdm_freq_t)NRFX_PDM_CONFIG_CLOCK_FREQ,
      .gain_l=              NRF_PDM_GAIN_MAXIMUM,                       
      .gain_r=              NRF_PDM_GAIN_MAXIMUM,  
      
      .interrupt_priority = NRFX_PDM_CONFIG_IRQ_PRIORITY
};


static void app_pdm_open(void)
{

   bc_queue_clear(BC_QUEUE_TYPE_PDM_COLLECTION_DATA);
#if (defined(HANDWARE_1_23_1))
  mono_adpcm_init(&mono_processor);
#else
 stereo_adpcm_init(&processor);
#endif  
 
 
//  yamaha_adpcm_init_normal(&normal);;
  
#if defined(TEST_ADPCM_GEN)  
  mono_adpcm_init(&mono_processor);
#endif  

 #if (defined(HANDWARE_1_18_1) || defined(HANDWARE_1_17_1) || defined(HANDWARE_1_19_1) || defined(HANDWARE_1_23_1))
  nrfx_err_t err_code;
  buffer_requested_flag = false;
	err_code = nrfx_pdm_init(&pdm_config,(nrfx_pdm_event_handler_t)buffer_event_handler);
  BC_LOG_INFO("err_code:%d\r\n",err_code);
    err_code = nrfx_pdm_start();
    bc_rtos_thread_resume(thread_struct[PDM_SEND].thread_handler);
  BC_LOG_INFO("err_code:%d\r\n",err_code);
#else
  pdm_dual_mic_start();
#endif   
}

static void app_pdm_close(void)
{
   bc_queue_clear(BC_QUEUE_TYPE_PDM_COLLECTION_DATA);
#if (defined(HANDWARE_1_18_1) || defined(HANDWARE_1_17_1) || defined(HANDWARE_1_19_1)|| defined(HANDWARE_1_23_1))
	nrfx_pdm_stop();
	nrfx_pdm_uninit();
#else
   pdm_dual_mic_stop();
#endif   
}


static SineWaveGenerator gen;

static void app_pdm_irq_handler_thread(void *thread_handler)
{

  uint32_t last_count = 0;
  while(true)
  {

      bc_rtos_thread_notify_take(bc_pdTRUE, bc_rtos_max_delay);

#if defined(TEST_ADPCM_GEN)
        
        for(uint8_t i = 0; i < 4;i++)
       {
          pdm_seq+=1;
          *(uint32_t*)&adpcm_package.pdm_data_buff[2] = pdm_seq;
          adpcm_encoder(saw_wave_8k_buffer, (char*) &adpcm_package.pdm_data_buff[6], 200, &mono_processor.mono_state);
        // yamaha_encode(saw_wave_8k_buffer,  &adpcm_package.pdm_data_buff[6], 200);
//          adpcm_encoder(sine_wave_8k_buffer, (char*) &adpcm_package.pdm_data_buff[6], 200, &mono_processor.mono_state);
//          adpcm_encoder(sqr_wave_8k_buffer, (char*) &adpcm_package.pdm_data_buff[6], 200, &mono_processor.mono_state);
//          adpcm_encoder(tri_wave_8k_buffer, (char*) &adpcm_package.pdm_data_buff[6], 200, &mono_processor.mono_state);
          bc_queue_isr_enqueue(BC_QUEUE_TYPE_PDM_COLLECTION_DATA,&adpcm_package);
       }
       
//        for(uint8_t i = 0; i < 2;i++)
//       {
//          pdm_seq+=1;
//          *(uint32_t*)&adpcm_package.pdm_data_buff[2] = pdm_seq;

//          adpcm_encoder(saw_wave_16k_buffer, (char*) &adpcm_package.pdm_data_buff[6], 200, &mono_processor.mono_state);
////          adpcm_encoder(sine_wave_16k_buffer, (char*) &adpcm_package.pdm_data_buff[6], 200, &mono_processor.mono_state);
////          adpcm_encoder(sqr_wave_16k_buffer, (char*) &adpcm_package.pdm_data_buff[6], 200, &mono_processor.mono_state);
////          adpcm_encoder(tri_wave_16k_buffer, (char*) &adpcm_package.pdm_data_buff[6], 200, &mono_processor.mono_state);
//          bc_queue_isr_enqueue(BC_QUEUE_TYPE_PDM_COLLECTION_DATA,&adpcm_package);
//         
//         pdm_seq+=1;
//          *(uint32_t*)&adpcm_package.pdm_data_buff[2] = pdm_seq;

//          adpcm_encoder(&saw_wave_16k_buffer[200], (char*) &adpcm_package.pdm_data_buff[6], 200, &mono_processor.mono_state);
////          adpcm_encoder(&sine_wave_16k_buffer[200], (char*) &adpcm_package.pdm_data_buff[6], 200, &mono_processor.mono_state);
////          adpcm_encoder(&sqr_wave_16k_buffer[200], (char*) &adpcm_package.pdm_data_buff[6], 200, &mono_processor.mono_state);
////          adpcm_encoder(&tri_wave_16k_buffer[200], (char*) &adpcm_package.pdm_data_buff[6], 200, &mono_processor.mono_state);
//          bc_queue_isr_enqueue(BC_QUEUE_TYPE_PDM_COLLECTION_DATA,&adpcm_package);
//       }
#else



#if(0)

      if(!buffer_requested_flag)
      {
//        //memset((uint8_t*)&pdm_package_1.pdm_data_buff[0],0,800*2);
        for(uint8_t i = 0; i < 16;i++)
        {
          pdm_seq+=1;
          *(uint32_t*)&adpcm_package.pdm_data_buff[2] = pdm_seq;
          memcpy(&adpcm_package.pdm_data_buff[6],(uint8_t*)&pdm_package_1.pdm_data_buff[i*50],100);
          bc_queue_enqueue(BC_QUEUE_TYPE_PDM_COLLECTION_DATA,&adpcm_package);
        }
        
       
      
        
      }
      else
      {
        for(uint8_t i = 0; i < 16;i++)
        {
          pdm_seq+=1;
          *(uint32_t*)&adpcm_package.pdm_data_buff[2] = pdm_seq;
          memcpy(&adpcm_package.pdm_data_buff[6],(uint8_t*)&pdm_package_2.pdm_data_buff[i*50],100);
          bc_queue_enqueue(BC_QUEUE_TYPE_PDM_COLLECTION_DATA,&adpcm_package);
        }
      } 
#elif( defined(HANDWARE_1_23_1) )
      /* HANDWARE_1_23_4: 暂停状态下不处理数据 */
#if defined(HANDWARE_1_23_4)
      if(pdm_paused)
      {
          /* 暂停时直接返回，不做压缩和入队处理 */
          continue;
      }
#endif
      if(!buffer_requested_flag)
      {
          uint16_t ilen = PDM_DATA_BUFF_SIZE/2;
        for(uint16_t i = 0 ; i <ilen;i++)
				{
					
					pdm_package_1.pdm_data_buff[i] = pdm_package_1.pdm_data_buff[i*2];
				}
//          for(uint8_t i = 0; i < 10;i++)
//        {
//          pdm_seq+=1;
//           *(uint32_t*)&adpcm_package.pdm_data_buff[2] = pdm_seq;
//          
//         
//          adpcm_encoder( &pdm_package_1.pdm_data_buff[i*200], (char*) &adpcm_package.pdm_data_buff[6], 200, &mono_processor.mono_state);
////          yamaha_adpcm_encode_stereo_normal(&normal,&pdm_package_1.pdm_data_buff[i*200], 200, &adpcm_package.pdm_data_buff[6]);
//          bc_queue_isr_enqueue(BC_QUEUE_TYPE_PDM_COLLECTION_DATA,&adpcm_package);
//        }
                
        #ifndef USE_OPUS
            for(uint8_t i = 0; i < 5;i++)
            {
                pdm_seq+=1;
                *(uint32_t*)&adpcm_package.pdm_data_buff[2] = pdm_seq;
                adpcm_encoder( &pdm_package_1.pdm_data_buff[i*PDM_DATA_SAMPLE_SIZE], (char*) &adpcm_package.pdm_data_buff[6], PDM_DATA_SAMPLE_SIZE, &mono_processor.mono_state);
                    bc_queue_enqueue(BC_QUEUE_TYPE_PDM_COLLECTION_DATA,&adpcm_package);
            }
        #else
            for(uint8_t i = 0; i < 15;i++)
            {
                adpcm_package.pdm_data_buff[0] = 1; // 
                adpcm_package.pdm_data_buff[1] = 1; // 1-8K  2-16k
                adpcm_package.pdm_data_buff[2] = 1;
                adpcm_package.pdm_data_buff[3] = 3;//4; 
                pdm_seq+=1;
                *(uint16_t*)&adpcm_package.pdm_data_buff[4] = pdm_seq;
                int len = start_opus_encode((int16_t *)&pdm_package_1.pdm_data_buff[i*FRAME_SIZE], FRAME_SIZE, &adpcm_package.pdm_data_buff[6], 100);            
                if(len > 0)
                    bc_queue_enqueue(BC_QUEUE_TYPE_PDM_COLLECTION_DATA,&adpcm_package);
                else
                    BC_LOG_INFO("start_opus_encode return len:%d\r\n",len);
            }
        #endif
      }
      else
      {
          uint16_t ilen = PDM_DATA_BUFF_SIZE/2;
                  for(uint16_t i = 0 ; i <ilen;i++)
				{
					
					pdm_package_2.pdm_data_buff[i] = pdm_package_2.pdm_data_buff[i*2];
				}
                
          #ifndef USE_OPUS
                for(uint8_t i = 0; i < 5;i++)
            { 
                pdm_seq+=1;
                
                *(uint32_t*)&adpcm_package.pdm_data_buff[2] = pdm_seq;
                adpcm_encoder( &pdm_package_2.pdm_data_buff[i*PDM_DATA_SAMPLE_SIZE], (char*) &adpcm_package.pdm_data_buff[6], PDM_DATA_SAMPLE_SIZE, &mono_processor.mono_state);
                bc_queue_enqueue(BC_QUEUE_TYPE_PDM_COLLECTION_DATA,&adpcm_package);
            }
            #else
             for(uint8_t i = 0; i < 15;i++)
            {
                adpcm_package.pdm_data_buff[0] = 1; 
                adpcm_package.pdm_data_buff[1] = 1; 
                adpcm_package.pdm_data_buff[2] = 1;
                adpcm_package.pdm_data_buff[3] = 3;//4; 
                pdm_seq+=1;
                *(uint16_t*)&adpcm_package.pdm_data_buff[4] = pdm_seq;

                int len = start_opus_encode((int16_t *)&pdm_package_2.pdm_data_buff[i*FRAME_SIZE], FRAME_SIZE, &adpcm_package.pdm_data_buff[6], 100);            
                if(len > 0)
                    bc_queue_enqueue(BC_QUEUE_TYPE_PDM_COLLECTION_DATA,&adpcm_package);
                else
                    BC_LOG_INFO("start_opus_encode2 return len:%d\r\n",len);
            }
            #endif

//       // memset((uint8_t*)&pdm_package_2.pdm_data_buff[0],0,sizeof(pdm_package_2.pdm_data_buff) * 2);
//        for(uint8_t i = 0; i < 10;i++)
//        {
//          pdm_seq+=1;
//           *(uint32_t*)&adpcm_package.pdm_data_buff[2] = pdm_seq;
//         adpcm_encoder( &pdm_package_2.pdm_data_buff[i*200], (char*) &adpcm_package.pdm_data_buff[6], 200, &mono_processor.mono_state);
////          yamaha_adpcm_encode_stereo_normal(&normal,&pdm_package_2.pdm_data_buff[i*200], 200, &adpcm_package.pdm_data_buff[6]);
//          bc_queue_isr_enqueue(BC_QUEUE_TYPE_PDM_COLLECTION_DATA,&adpcm_package);
//        }
      } 
#else

      if(!buffer_requested_flag)
      {
        //memset((uint8_t*)&pdm_package_1.pdm_data_buff[0],0,sizeof(pdm_package_1.pdm_data_buff) * 2);
          for(uint8_t i = 0; i < 4;i++)
        {
          pdm_seq+=1;
           *(uint32_t*)&adpcm_package.pdm_data_buff[2] = pdm_seq;
          stereo_adpcm_encode(&processor, &pdm_package_1.pdm_data_buff[i*200],(char*) &adpcm_package.pdm_data_buff[6], 100);
//          yamaha_adpcm_encode_stereo_normal(&normal,&pdm_package_1.pdm_data_buff[i*200], 200, &adpcm_package.pdm_data_buff[6]);
          bc_queue_isr_enqueue(BC_QUEUE_TYPE_PDM_COLLECTION_DATA,&adpcm_package);
        }
      
        
      }
      else
      {
       // memset((uint8_t*)&pdm_package_2.pdm_data_buff[0],0,sizeof(pdm_package_2.pdm_data_buff) * 2);
        for(uint8_t i = 0; i < 4;i++)
        {
          pdm_seq+=1;
           *(uint32_t*)&adpcm_package.pdm_data_buff[2] = pdm_seq;
          stereo_adpcm_encode(&processor, &pdm_package_2.pdm_data_buff[i*200],(char*) &adpcm_package.pdm_data_buff[6], 100);
//          yamaha_adpcm_encode_stereo_normal(&normal,&pdm_package_2.pdm_data_buff[i*200], 200, &adpcm_package.pdm_data_buff[6]);
          bc_queue_isr_enqueue(BC_QUEUE_TYPE_PDM_COLLECTION_DATA,&adpcm_package);
        }
      } 
#endif    
        
 #endif       

  }
}

static void app_pdm_handler_thread(void *thread_handler)
{
 
  while(true)
  {

    if(bc_queue_dequeue(BC_QUEUE_TYPE_PDM_COLLECTION_DATA,(void*)&pdm_ble_package.pdm_package))  
    {
      
      switch(app_pdm_mode_get())
      {
        case PDM_MODE_OFFLINE:
        case PDM_MODE_KEY_OFFLINE:
        {
          if(app_pdm_mode_get() != PDM_MODE_IDIE)
          {
            #ifndef USE_OPUS
            app_ppg_file_write(&pdm_ble_package.pdm_package.pdm_data_buff[6],PDM_DATA_SEND_SIZE);
            #else
              app_ppg_file_write(&pdm_ble_package.pdm_package.pdm_data_buff[6],48);
            #endif
          }        
          break;
        }
        case PDM_MODE_ONLINE:
        {
          if(ble_calss.ble_connect_status)
          {
#if defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_2_ONE_SEC)
              // 蓝牙重连后延迟发送，等GATT协商完成，避免SoftDevice断言
              if(pdm_ble_tx_delay_count > 0)
              {
                  pdm_ble_tx_delay_count--;
              }
              else
#endif
              {
              #ifndef USE_OPUS
                  *(uint16_t*)&pdm_ble_package.pdm_package.pdm_data_buff[0] = PDM_DATA_SEND_SIZE;
                  ble_calss.ble_send((uint8_t*)&pdm_ble_package,PDM_DATA_SEND_SIZE+10);
              #else
                  pdm_ble_package.package_basic.subcmd = 0x0a;
                  ble_calss.ble_send((uint8_t*)&pdm_ble_package,(48)+4+2+4);
              #endif
              }
          }
#if defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_2_ONE_SEC)
          // 在线录音同时保存到本地Flash
          if(pdm_online_save_to_flash)
          {
            #ifndef USE_OPUS
            app_ppg_file_write(&pdm_ble_package.pdm_package.pdm_data_buff[6],PDM_DATA_SEND_SIZE);
            #else
            app_ppg_file_write(&pdm_ble_package.pdm_package.pdm_data_buff[6],48);
            #endif
          }
#endif
          break;
        }
        default:
        {
          break;
        }
      }    
    }

  }
}


void bc_pdm_start(void)
{


 #if (defined(HANDWARE_1_18_1) || defined(HANDWARE_1_17_1) || defined(HANDWARE_1_19_1) || defined(HANDWARE_1_23_1))
	nrfx_pdm_init(&pdm_config,(nrfx_pdm_event_handler_t)buffer_event_handler);
	nrfx_pdm_start();
#else
  pdm_dual_mic_start();
#endif   
  
	pdm_seq = 0;
}


void app_pdm_start(struct app_cmd_package * pack)
{
#if defined(HANDWARE_1_23_2_ONE_SEC)
	/* 充电时禁止录音 */
	if(bc_pmic_get_charge_status() == PMIC_CHARGED_ING)
	{
		BC_LOG_INFO("pdm start blocked: charging\r\n");
		return ;
	}
	
	/* APP录音优先级最高，如果有低优先级录音，先停止 */
	if(pdm_status != PDM_IDIE)
	{
		if(current_record_prio < PDM_PRIO_APP)
		{
			BC_LOG_INFO("pdm start: stop low priority recording (prio=%d)\r\n", current_record_prio);
			/* 停止定时录音循环（定时录音时内部会调用app_pdm_recording_stop） */
			if(current_record_prio == PDM_PRIO_TIMER)
			{
				app_timer_record_stop();
				/* 标记定时录音被抢占（必须在stop之后设置，避免stop内部清除标志） */
				timer_record_preempted = true;
			}
			else if(app_pdm_mode_get() == PDM_MODE_OFFLINE)
			{
				/* 双击离线录音：直接停止录音 */
				app_pdm_recording_stop();
			}
			else
			{
				/* 在线录音（触摸启动等低优先级）：停止在线录音 */
				BC_LOG_INFO("pdm start: preempt online recording (prio=%d)\r\n", current_record_prio);
				app_pdm_stop(NULL);
			}
		}
		else
		{
			/* 同级或更高优先级，不重复启动 */
			return ;
		}
	}
#else
	if(pdm_status != PDM_IDIE)
	{
		return ;
	}
#endif

	memcpy((uint8_t*)&pdm_ble_package,(uint8_t*)pack,4);
	pdm_status = PDM_WORK;

	pdm_stop_flag = false;
	BC_LOG_INFO("pdm_seq:%d \r\n",pdm_seq);
#if(defined(HANDWARE_1_23_1))  
  bc_ic_led_mic_online_recording_on();
#if defined(HANDWARE_1_23_3)
    app_linear_motor_ic_start(1);
#elif defined(HANDWARE_1_23_2)
    app_pdm_motor_start_by_config();
#else
#ifndef HANDWARE_1_23_4
  bc_linear_motor_start(LINEAR_MOTOR_MIC_START);
#endif
#endif
#endif 

#if defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_2_ONE_SEC)
#if defined(HANDWARE_1_23_2_ONE_SEC)
  // 在线录音同时保存到本地Flash，打开录音文件
  uint8_t sts = lk_app_ppg_file_open(PPG_FILE_TYPE_16K_2_MIC_ADPCM);
  if(!sts)
#else
  if(true == lk_app_ppg_file_open(PPG_FILE_TYPE_16K_2_MIC_ADPCM))
#endif
  {
      BC_LOG_INFO("app_pdm_start file_open ok\r\n");
      pdm_online_save_to_flash = true;
  }
  else
  {
      BC_LOG_INFO("app_pdm_start file_open fail\r\n");
      pdm_online_save_to_flash = false;
  }
#endif

  app_pdm_open();  
  
  app_pdm_mode_set(PDM_MODE_ONLINE);
	pdm_seq = 0;
	app_ble_conn_time_audio_set();
	
#if defined(HANDWARE_1_23_2_ONE_SEC)
	current_record_prio = PDM_PRIO_APP;
#endif
}
void app_pdm_stop(struct app_cmd_package * pack)
{

	pdm_poll_flag = false;	
	pdm_stop_flag = true; 

	bc_ldo_mic_power_off();
	bc_delay_ms(20);
  app_pdm_close(); 
	BC_LOG_INFO("pdm_seq end cnt:%d \r\n",pdm_seq);
//	bc_ic_led_pdm_off();
	if(app_ble_connect_status())
	{
//		bc_pdm_led_on();
	}
	
	pdm_seq = 0;
#if(defined(HANDWARE_1_23_1))  
  bc_ic_led_mic_online_recording_off();
#if defined(HANDWARE_1_23_3)
    app_linear_motor_ic_start(2);
#elif defined(HANDWARE_1_23_2)
    app_pdm_motor_stop_by_config();
#else
#ifndef HANDWARE_1_23_4
  bc_linear_motor_start(LINEAR_MOTOR_MIC_STOP);
#endif
#endif
#endif 

#if defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_2_ONE_SEC)
  // 关闭在线录音同时保存的本地文件
  if(pdm_online_save_to_flash)
  {
      if(lk_app_ppg_file_close())
      {
          BC_LOG_INFO("app_pdm_stop file_close ok\r\n");
      }
      else
      {
          BC_LOG_INFO("app_pdm_stop file_close fail\r\n");
      }
      pdm_online_save_to_flash = false;
  }
#endif

	bc_queue_clear(BC_QUEUE_TYPE_PDM_COLLECTION_DATA);
  app_pdm_mode_set(PDM_MODE_IDIE);
	app_ble_conn_time_audio_reset();
	pdm_status = PDM_IDIE;
    app_touch_pdm_key_flag_set(true);


}

bool app_pdm_recording_start(void)
{
#if (defined(HANDWARE_1_17_1)   || defined(HANDWARE_1_19_1)  || defined(HANDWARE_1_18_1) || defined(HANDWARE_1_23_1))  

#if defined(HANDWARE_1_23_2_ONE_SEC)
    /* 优先级判断：如果已有同级或更高优先级录音在运行，不启动 */
    if(pdm_status != PDM_IDIE)
    {
        if(next_record_prio <= current_record_prio)
        {
            BC_LOG_INFO("pdm recording start: blocked by higher priority (cur=%d, new=%d)\r\n", 
                        current_record_prio, next_record_prio);
            /* 恢复默认优先级 */
            next_record_prio = PDM_PRIO_TOUCH;
            return false;
        }
        else
        {
            /* 高优先级抢占：先停止当前低优先级录音 */
            BC_LOG_INFO("pdm recording start: preempt low priority (cur=%d, new=%d)\r\n", 
                        current_record_prio, next_record_prio);
            if(current_record_prio == PDM_PRIO_TIMER)
            {
                /* 定时录音：先停止定时循环和录音，再标记被抢占 */
                app_timer_record_stop();
                timer_record_preempted = true;
            }
            else
            {
                /* 双击离线录音：直接停止录音 */
                app_pdm_recording_stop();
            }
        }
    }
#else
	if(pdm_status != PDM_IDIE)
	{
		return false;
	}	
#endif
    
    if(0 != app_ppg_file_status_get())
        return false;

#if defined(HANDWARE_1_23_2_ONE_SEC)
	/* 充电时禁止录音 */
	if(bc_pmic_get_charge_status() == PMIC_CHARGED_ING)
	{
		BC_LOG_INFO("pdm recording start blocked: charging\r\n");
		return false;
	}
	/* 电量小于15%时禁止录音，长振动2次 */
    if(precent < 15)
	{
		BC_LOG_INFO("pdm recording start blocked: battery low (%d%%)\r\n", precent);
		app_vibrate_start(VIBRATE_MODE_LONG, 2);
		return false;
	}
#endif

	pdm_status = PDM_WORK;

#if defined(HANDWARE_1_23_2_ONE_SEC)
    /* 设置当前录音优先级 */
    current_record_prio = next_record_prio;
    next_record_prio = PDM_PRIO_TOUCH; /* 恢复默认 */
#endif

	

	pdm_stop_flag = false;
	BC_LOG_INFO("pdm_seq:%d \r\n",pdm_seq);
    
      BC_LOG_INFO("start bc_ic_led_mic_offline_recording_on\r\n");
#if(defined(HANDWARE_1_23_1))  
  bc_ic_led_mic_offline_recording_on();
#if (defined(HANDWARE_1_23_3))   
    app_linear_motor_ic_start(1);
#elif defined(HANDWARE_1_23_2)
    app_pdm_motor_start_by_config();
#else
#ifndef HANDWARE_1_23_4
  bc_linear_motor_start(LINEAR_MOTOR_MIC_START);
#endif
#endif
#endif 
    
#if(defined(HANDWARE_1_19_1) || defined(HANDWARE_1_23_2) || defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))  
  /* 关闭之前未清理的文件句柄（如蓝牙断开时未正常关闭的文件） */
  lk_app_ppg_file_close();
#if defined(HANDWARE_1_23_2_ONE_SEC)
    uint8_t filests = lk_app_ppg_file_open(PPG_FILE_TYPE_16K_2_MIC_ADPCM);
  if(0 == filests)
#else
  if(true == lk_app_ppg_file_open(PPG_FILE_TYPE_16K_2_MIC_ADPCM))
#endif
#else
  if(app_ppg_file_open(PPG_FILE_TYPE_8K_1_MIC_ADPCM))
#endif 
  
  {
     BC_LOG_INFO("file_open  ok\r\n");
  }
  else
  {
    BC_LOG_INFO("file_open fail\r\n");
      pdm_status = PDM_IDIE;
      #if(defined(HANDWARE_1_23_1))  
      app_touch_pdm_key_flag_set(true);
      #if defined(HANDWARE_1_23_4)
        /* HANDWARE_1_23_4: 文件打开失败，红灯快闪2次熄灭 */
        bc_ic_led_breathing_start(LED_BREATHING_FAST, 2, 0, 20, 0);
      #else
      bc_ic_led_mic_offline_recording_off();
      #endif
    #if defined(HANDWARE_1_23_3)
        app_linear_motor_ic_start(2);
    #else
    #ifndef HANDWARE_1_23_4
    #if defined(HANDWARE_1_23_2_ONE_SEC)
      if(1 == filests)
        app_vibrate_start(VIBRATE_MODE_LONG, 1);
      else if (2 == filests)
      {
        app_vibrate_start(VIBRATE_MODE_LONG, 2);
        /* HANDWARE_1_23_2_ONE_SEC: 文件被占用，红蓝交替快闪3次 */
        for(uint8_t i = 0; i < 3; i++)
        {
            bc_ic_led_test_cmd(0, 20, 0);  /* 红 */
            bc_delay_ms(120);
            bc_ic_led_test_cmd(0, 0, 20);  /* 蓝 */
            bc_delay_ms(120);
        }
        bc_ic_led_test_cmd(0, 0, 0);  /* 熄灭 */
      }
    #else
      bc_linear_motor_start(LINEAR_MOTOR_MIC_STOP);
    #endif
    #endif
    #endif
    #endif

    return false;
  }

  app_pdm_open(); 
	pdm_seq = 0;
#endif  
  app_pdm_mode_set(PDM_MODE_OFFLINE);
#if(defined(HANDWARE_1_19_1))  
  bc_led_white_breathe_start(LED_WHITE_LONG_LIGHT);
#endif  
#if defined(HANDWARE_1_23_3)
  offline_pdm_on = true;
  app_pdm_offline_notify(0x01);
#endif
  return true;
	
}

bool app_pdm_recording_stop(void)
{
  if(pdm_status != PDM_WORK)
  {
     return true;
  }
  if(app_pdm_mode_get() != PDM_MODE_OFFLINE)
  {
    return true;
  }
#if (defined(HANDWARE_1_17_1)   || defined(HANDWARE_1_19_1)  || defined(HANDWARE_1_18_1) || defined(HANDWARE_1_23_1))  
  
  app_pdm_close();    
	bc_delay_ms(20);
	pdm_seq = 0;
	pdm_status = PDM_IDIE;
  app_pdm_mode_set(PDM_MODE_IDIE);
  app_touch_pdm_key_flag_set(true);
#if defined(HANDWARE_1_23_2_ONE_SEC)
  current_record_prio = PDM_PRIO_NONE;
  /* 如果定时录音被抢占，恢复定时录音 */
  if(timer_record_preempted)
  {
    BC_LOG_INFO("pdm recording stop: restore timer record after preempted\r\n");
    timer_record_preempted = false;
    app_timer_record_start();
  }
#endif
#if(defined(HANDWARE_1_19_1))    
  bc_led_stop();
#endif    

#if(defined(HANDWARE_1_23_1))  
  bc_ic_led_mic_offline_recording_off();
#if defined(HANDWARE_1_23_4)
  /* 清除暂停状态 */
  if(pdm_paused)
  {
    pdm_paused = false;
    bc_ic_led_recording_pause_off();
  }
#endif
#if defined(HANDWARE_1_23_3)
    app_linear_motor_ic_start(2);
#elif defined(HANDWARE_1_23_2)
    app_pdm_motor_stop_by_config();
#else
#ifndef HANDWARE_1_23_4
  bc_linear_motor_start(LINEAR_MOTOR_MIC_STOP);
#endif
#endif
#endif 

  #if 1
  if(lk_app_ppg_file_close())
  #else
  if(app_ppg_file_close())
  #endif
  {
    BC_LOG_INFO("file_close ok\r\n");
  }
  else
  {
    BC_LOG_INFO("file_close fail\r\n");
    
    return false;
  }
#endif  
#if defined(HANDWARE_1_23_3)
  offline_pdm_on = false;
  app_pdm_offline_notify(0x00);
#endif
  return true;
}

#if defined(HANDWARE_1_23_4)
/*******************************************************************************
 * Function Name     : app_pdm_recording_pause
 * Description       : 暂停录音（不关闭MIC，不处理数据）
 * Input             : 无
 * Output            : 无
 * Return            : true-成功 false-失败
 * Author            : liukun
 *******************************************************************************/
bool app_pdm_recording_pause(void)
{
    if(pdm_status != PDM_WORK)
    {
        return false;
    }
    if(pdm_paused)
    {
        return false;
    }
    
    pdm_paused = true;
    BC_LOG_INFO("recording paused\r\n");
    return true;
}

/*******************************************************************************
 * Function Name     : app_pdm_recording_resume
 * Description       : 恢复录音
 * Input             : 无
 * Output            : 无
 * Return            : true-成功 false-失败
 * Author            : liukun
 *******************************************************************************/
bool app_pdm_recording_resume(void)
{
    if(pdm_status != PDM_WORK)
    {
        return false;
    }
    if(!pdm_paused)
    {
        return false;
    }
    
    pdm_paused = false;
    BC_LOG_INFO("recording resumed\r\n");
    return true;
}

/*******************************************************************************
 * Function Name     : app_pdm_recording_is_paused
 * Description       : 查询录音是否处于暂停状态
 * Input             : 无
 * Output            : 无
 * Return            : true-暂停 false-未暂停
 * Author            : liukun
 *******************************************************************************/
bool app_pdm_recording_is_paused(void)
{
    return pdm_paused;
}
#endif

bool app_pdm_capture_recording_start(void)
{
#if (defined(HANDWARE_1_17_1)   || defined(HANDWARE_1_19_1)  || defined(HANDWARE_1_18_1) || defined(HANDWARE_1_23_1))  
	if(pdm_status != PDM_IDIE)
	{
		return false;
	}	
	pdm_status = PDM_WORK;	
	pdm_stop_flag = false;
	BC_LOG_INFO("pdm_seq:%d \r\n",pdm_seq);

    #if 0 // by liukun 
  if(app_ppg_file_open(PPG_FILE_TYPE_16K_2_MIC_ADPCM_CAPTURE))
    #else
    if(app_ppg_file_open(PPG_FILE_TYPE_16K_2_MIC_ADPCM))
    #endif
 
  {
     BC_LOG_INFO("file_open  ok\r\n");
  }
  else
  {
    BC_LOG_INFO("file_open fail\r\n");
      pdm_status = PDM_IDIE;
    return false;
  }

  app_pdm_open(); 
	pdm_seq = 0;
#endif  
  app_pdm_mode_set(PDM_MODE_KEY_OFFLINE);
#if(defined(HANDWARE_1_19_1))   
  bc_led_white_breathe_start(LED_WHITE_FlLASH_CYCLE_300ms_500ms);
#endif  
  return true;
	
}

bool app_pdm_capture_recording_stop(void)
{
  if(pdm_status != PDM_WORK)
  {
     return true;
  }
  if(app_pdm_mode_get() != PDM_MODE_KEY_OFFLINE)
  {
    return true;
  }
#if (defined(HANDWARE_1_17_1)   || defined(HANDWARE_1_19_1)  || defined(HANDWARE_1_18_1) || defined(HANDWARE_1_23_1))  
  
  app_pdm_close();    
	bc_delay_ms(20);
  bc_queue_clear(BC_QUEUE_TYPE_PDM_COLLECTION_DATA);
	pdm_seq = 0;
	pdm_status = PDM_IDIE;
  app_pdm_mode_set(PDM_MODE_IDIE);
#if(defined(HANDWARE_1_19_1))     
  bc_led_stop();
#endif  
  if(app_ppg_file_close())
  {
    BC_LOG_INFO("file_close ok\r\n");
  }
  else
  {
    BC_LOG_INFO("file_close fail\r\n");
    return false;
  }
#endif  
  return true;
}





void app_pdm_ble_stop(void)
{
	
	
#if (defined(HANDWARE_1_18_1) || defined(HANDWARE_1_17_1) || defined(HANDWARE_1_19_1) || defined(HANDWARE_1_23_1))
	nrfx_pdm_stop();
	nrfx_pdm_uninit();
#else
   pdm_dual_mic_stop();
#endif  
	BC_LOG_INFO("pdm_seq:%d \r\n",pdm_seq);
	pdm_seq = 0;
	bc_queue_clear(BC_QUEUE_TYPE_PDM_COLLECTION_DATA);
	pdm_status = PDM_IDIE;
	/* 重置录音模式，避免BLE断开后状态不一致 */
	app_pdm_mode_set(PDM_MODE_IDIE);
}
void bc_pdm_stop(void)
{
  
#if (defined(HANDWARE_1_18_1) || defined(HANDWARE_1_17_1) || defined(HANDWARE_1_19_1) || defined(HANDWARE_1_23_1))
	nrfx_pdm_stop();
	nrfx_pdm_uninit();
#else
   pdm_dual_mic_stop();
#endif    

 
	BC_LOG_INFO("pdm_seq:%d \r\n",pdm_seq);
	bc_queue_clear(BC_QUEUE_TYPE_PDM_COLLECTION_DATA);
	pdm_status = PDM_IDIE;
}
struct app_package_basic pdm_touch_cmd = {0};

void app_pdm_touch_start(void)
{
#if defined(HANDWARE_1_23_2_ONE_SEC)
  /* 优先级判断：如果已有同级或更高优先级录音在运行，不启动 */
  if(pdm_status != PDM_IDIE)
  {
    if(PDM_PRIO_TOUCH <= current_record_prio)
    {
      BC_LOG_INFO("pdm touch start: blocked by higher priority (cur=%d, touch=%d)\r\n", 
                  current_record_prio, PDM_PRIO_TOUCH);
      return ;
    }
    else
    {
      /* 双击录音抢占低优先级（定时录音） */
      BC_LOG_INFO("pdm touch start: preempt low priority (cur=%d, touch=%d)\r\n", 
                  current_record_prio, PDM_PRIO_TOUCH);
      /* 停止定时录音循环（定时录音时内部会调用app_pdm_recording_stop） */
      if(current_record_prio == PDM_PRIO_TIMER)
      {
        app_timer_record_stop();
        /* 标记定时录音被抢占（必须在stop之后设置，避免stop内部清除标志） */
        timer_record_preempted = true;
      }
      else if(app_pdm_mode_get() == PDM_MODE_OFFLINE)
      {
        /* 低优先级离线录音：直接停止录音 */
        app_pdm_recording_stop();
      }
    }
  }
  
  /* 充电时禁止录音 */
  if(bc_pmic_get_charge_status() == PMIC_CHARGED_ING)
  {
    BC_LOG_INFO("pdm touch start blocked: charging\r\n");
    return ;
  }
#else
  if(pdm_status != PDM_IDIE)
	{
		return ;
	}
#endif

  memset((uint8_t*)&pdm_ble_package,0,sizeof(pdm_ble_package));
  pdm_ble_package.package_basic.cmd = 0x71;
  pdm_ble_package.package_basic.frame_id = 9;
  pdm_ble_package.package_basic.frame_type = 0;
  pdm_ble_package.package_basic.subcmd = 0x08; //bc_device_info_get_audio_up_mode();
//	memcpy((uint8_t*)&pdm_ble_package,(uint8_t*)pack,4);
	pdm_status = PDM_WORK;

	pdm_stop_flag = false;
	BC_LOG_INFO("pdm_seq:%d \r\n",pdm_seq);
#if(defined(HANDWARE_1_23_1))  
#if defined(HANDWARE_1_23_2_ONE_SEC)
  // HANDWARE_1_23_2_ONE_SEC: 双击录音显示红灯慢闪（与离线录音LED一致）
  bc_ic_led_mic_offline_recording_on();
#else
  bc_ic_led_mic_online_recording_capture_on();
#endif
#if (defined(HANDWARE_1_23_3))   
    app_linear_motor_ic_start(1);
#elif defined(HANDWARE_1_23_2)
    app_pdm_motor_start_by_config();
#else 
#ifndef HANDWARE_1_23_4    
  bc_linear_motor_start(LINEAR_MOTOR_MIC_START);
#endif
#endif
#endif 

#if defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_2_ONE_SEC)
#if defined(HANDWARE_1_23_2_ONE_SEC)
  // 在线录音同时保存到本地Flash，打开录音文件
  uint8_t sts = lk_app_ppg_file_open(PPG_FILE_TYPE_16K_2_MIC_ADPCM);
  if(!sts)
#else
  if(true == lk_app_ppg_file_open(PPG_FILE_TYPE_16K_2_MIC_ADPCM))
#endif
  {
      BC_LOG_INFO("app_pdm_touch_start file_open ok\r\n");
      pdm_online_save_to_flash = true;
  }
  else
  {
      BC_LOG_INFO("app_pdm_touch_start file_open fail\r\n");
      pdm_online_save_to_flash = false;
  }
#endif

  app_pdm_open();  
  
  app_pdm_mode_set(PDM_MODE_ONLINE);
	pdm_seq = 0;
	app_ble_conn_time_audio_set();
	
#if defined(HANDWARE_1_23_2_ONE_SEC)
	current_record_prio = PDM_PRIO_TOUCH;
#endif
}

void app_pdm_touch_stop(void)
{
  pdm_poll_flag = false;	
	pdm_stop_flag = true; 

	bc_ldo_mic_power_off();
	bc_delay_ms(20);
  app_pdm_close(); 
//	BC_LOG_INFO("pdm_seq:%d    err:%d \r\n",pdm_seq,err);
//	bc_ic_led_pdm_off();
	if(app_ble_connect_status())
	{
//		bc_pdm_led_on();
	}
#if(defined(HANDWARE_1_23_1))  
  bc_ic_led_mic_online_recording_capture_off();
  /* 同时清除离线录音LED状态，避免蓝牙断开后错误恢复绿灯 */
  bc_ic_led_mic_offline_recording_off();
#if defined(HANDWARE_1_23_4)
  /* 清除暂停状态和暂停LED */
  if(pdm_paused)
  {
    pdm_paused = false;
    bc_ic_led_recording_pause_off();
  }
#endif
#if defined (HANDWARE_1_23_3)
    app_linear_motor_ic_start(2);
#elif defined(HANDWARE_1_23_2)
    app_pdm_motor_stop_by_config();
#else
#ifndef HANDWARE_1_23_4
  bc_linear_motor_start(LINEAR_MOTOR_MIC_STOP);
#endif
#endif
#endif 	

#if defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_2_ONE_SEC)
  // 关闭在线录音同时保存的本地文件
  if(pdm_online_save_to_flash)
  {
      if(lk_app_ppg_file_close())
      {
          BC_LOG_INFO("app_pdm_touch_stop file_close ok\r\n");
      }
      else
      {
          BC_LOG_INFO("app_pdm_touch_stop file_close fail\r\n");
      }
      pdm_online_save_to_flash = false;
  }
#endif

	pdm_seq = 0;

	bc_queue_clear(BC_QUEUE_TYPE_PDM_COLLECTION_DATA);
  app_pdm_mode_set(PDM_MODE_IDIE);
	app_ble_conn_time_audio_reset();
	pdm_status = PDM_IDIE;
	app_package_pdm_upload_over();
	
#if defined(HANDWARE_1_23_2_ONE_SEC)
	current_record_prio = PDM_PRIO_NONE;
	/* 如果定时录音被抢占，恢复定时录音 */
	if(timer_record_preempted)
	{
		BC_LOG_INFO("pdm touch stop: restore timer record after preempted\r\n");
		timer_record_preempted = false;
		app_timer_record_start();
	}
#endif
}


bool app_pdm_work_status(void)
{
	if(pdm_status == PDM_WORK)
	{
		return true;
	}
	return false;
}

#if defined(HANDWARE_1_23_2_ONE_SEC)
uint8_t app_pdm_get_record_priority(void)
{
    return (uint8_t)current_record_prio;
}

void app_pdm_set_next_record_priority(uint8_t prio)
{
    next_record_prio = (enum pdm_record_priority)prio;
}
#endif

void app_pdm_audio_discooenct_stop(void)
{
  if(app_pdm_work_status())
  {
    app_pdm_touch_stop();
  }
}

void app_pdm_mode_change_to_online(void)
{
    if(app_pdm_mode_get() == PDM_MODE_OFFLINE)
    {
       app_package_mic_recording_stop();
    }
}

bool app_pdm_switch_online_to_offline(void)
{
    if(app_pdm_mode_get() != PDM_MODE_ONLINE)
    {
        return false;
    }
    if(pdm_status != PDM_WORK)
    {
        return false;
    }
    
    BC_LOG_INFO("switch online to offline recording\r\n");
    
#if(defined(HANDWARE_1_23_1))
    bc_ic_led_mic_online_recording_off();
    bc_ic_led_mic_offline_recording_on();
#if defined(HANDWARE_1_23_3)
    app_linear_motor_ic_start(1);
#elif defined(HANDWARE_1_23_2)
    app_pdm_motor_start_by_config();
#else
#ifndef HANDWARE_1_23_4
    bc_linear_motor_start(LINEAR_MOTOR_MIC_START);
#endif
#endif
#endif

#if(defined(HANDWARE_1_19_1) || defined(HANDWARE_1_23_2) || defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))
#if defined(HANDWARE_1_23_2_ONE_SEC)
    uint8_t filests = lk_app_ppg_file_open(PPG_FILE_TYPE_16K_2_MIC_ADPCM);
    if(0 == filests)
#else
    if(true == lk_app_ppg_file_open(PPG_FILE_TYPE_16K_2_MIC_ADPCM))
#endif
#else
    if(app_ppg_file_open(PPG_FILE_TYPE_8K_1_MIC_ADPCM))
#endif
    {
        BC_LOG_INFO("file_open ok, switch to offline mode\r\n");
    }
    else
    {
        BC_LOG_INFO("file_open fail, stop recording\r\n");
#if(defined(HANDWARE_1_23_1))
        bc_ic_led_mic_offline_recording_off();
#if defined(HANDWARE_1_23_3)
        app_linear_motor_ic_start(2);
#elif defined(HANDWARE_1_23_2)
        app_pdm_motor_stop_by_config();
#else
#ifndef HANDWARE_1_23_4
        bc_linear_motor_start(LINEAR_MOTOR_MIC_STOP);
#endif
#endif
#endif
        pdm_poll_flag = false;
        pdm_stop_flag = true;
        bc_ldo_mic_power_off();
        bc_delay_ms(20);
        app_pdm_close();
        pdm_seq = 0;
        bc_queue_clear(BC_QUEUE_TYPE_PDM_COLLECTION_DATA);
        app_pdm_mode_set(PDM_MODE_IDIE);
        pdm_status = PDM_IDIE;
        return false;
    }

    app_pdm_mode_set(PDM_MODE_OFFLINE);
    app_touch_pdm_key_flag_set(false);
    return true;
}

#if defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_2_ONE_SEC)
/*******************************************************************************
 * Function Name     : app_pdm_ble_connect_set_tx_delay
 * Description       : 蓝牙连接时设置PDM BLE发送延迟，等待GATT协商完成
 *                     防止重连后立即发送大量数据导致SoftDevice断言
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 
 * Modified Date:    : 
 *******************************************************************************/
void app_pdm_ble_connect_set_tx_delay(void)
{
    // 延迟约50帧（约1.3秒），等MTU交换、数据长度更新等GATT协商完成
    pdm_ble_tx_delay_count = 50;
}

#if defined(HANDWARE_1_23_3)
bool app_pdm_offline_on_get(void)
{
    return offline_pdm_on;
}
#endif
#endif

void app_pdm_thread_create(void)
{	
  bc_base_type_t x_return = bc_pdPASS;
	for(uint8_t i = 0; i < PDM_TASK_TYPE_NUM; i++)
	{
		x_return  = bc_rtos_thread_create((TaskFunction_t )thread_struct[i].thread_task_code,     	
                                     (const char*    )thread_struct[i].thread_name,   	
                                     (uint16_t       )thread_struct[i].thread_stack_depth, 
                                     (void*          )&thread_struct[i].thread_parameters,				
                                     (UBaseType_t    )thread_struct[i].thread_priority,	
                                     (TaskHandle_t*  )&thread_struct[i].thread_handler); 
		if(x_return != NULL)
		{
			BC_LOG_INFO("create %s succeed \r\n",thread_struct[i].thread_name);
		}
		else
		{
			BC_LOG_ERROR("create  %s fail",thread_struct[i].thread_name);
		}	
	}
   ble_calss = bc_ble_new();
   
#if defined(HANDWARE_1_23_2_ONE_SEC)
   /* 启动时恢复定时录音配置 */
   {
       uint8_t enable = 0;
       uint32_t interval = 0;
       uint32_t duration = 0;
       bc_device_info_timer_record_config_get(&enable, &interval, &duration);
       if(enable == 1 && interval > 0)
       {
           BC_LOG_INFO("restore timer record: interval=%us, duration=%us\r\n", interval, duration);
           app_timer_record_start();
       }
   }
#endif
}

#if defined(HANDWARE_1_23_2_ONE_SEC)
/* 定时录音状态 */
static TimerHandle_t timer_record_interval_handle = NULL;  // 间隔定时器（单次）
static TimerHandle_t timer_record_duration_handle = NULL;  // 录音时长定时器（单次）
static bool timer_record_running = false;                   // 定时录音是否运行中
static uint32_t timer_record_interval = 60;                 // 缓存：间隔时间（秒）
static uint32_t timer_record_duration = 30;                 // 缓存：录音时长（秒）

/* 间隔定时器回调：间隔时间到，开始录音 */
static void timer_record_interval_callback(TimerHandle_t xTimer)
{
    BC_LOG_INFO("timer record: interval timeout, start recording\r\n");
    
    /* 设置定时录音为最低优先级 */
    app_pdm_set_next_record_priority(PDM_PRIO_TIMER);
    
    /* 开始离线录音 */
    app_pdm_recording_start();
    
    /* 启动录音时长定时器 */
    if(timer_record_duration_handle != NULL)
    {
        bc_rtos_timer_stop(timer_record_duration_handle, 0);
        bc_rtos_timer_change_period(timer_record_duration_handle, timer_record_duration * 1000, 0);
        bc_rtos_timer_start(timer_record_duration_handle, 0);
    }
}

/* 时长定时器回调：录音时间到，停止录音，重新启动间隔定时器 */
static void timer_record_duration_callback(TimerHandle_t xTimer)
{
    BC_LOG_INFO("timer record: duration timeout, stop recording\r\n");
    
    /* 停止录音 */
    app_pdm_recording_stop();
    
    /* 重新启动间隔定时器（间隔时间不含录音时间） */
    if(timer_record_interval_handle != NULL && timer_record_running)
    {
        bc_rtos_timer_stop(timer_record_interval_handle, 0);
        bc_rtos_timer_start(timer_record_interval_handle, 0);
    }
}

/* 启动定时录音 */
void app_timer_record_start(void)
{
    uint8_t enable = 0;
    
    bc_device_info_timer_record_config_get(&enable, &timer_record_interval, &timer_record_duration);
    
    if(enable != 1)
    {
        return;
    }
    
    if(timer_record_interval == 0)
    {
        return;
    }
    
    /* 最小间隔时间为60秒 */
    if(timer_record_interval < 60)
    {
        timer_record_interval = 60;
    }
    
    if(timer_record_running)
    {
        /* 已经在运行，先停止再重启 */
        app_timer_record_stop();
    }
    
    BC_LOG_INFO("timer record start: interval=%us, duration=%us\r\n", timer_record_interval, timer_record_duration);
    
    /* 创建间隔定时器（单次模式：录音结束后手动重启） */
    if(timer_record_interval_handle == NULL)
    {
        timer_record_interval_handle = bc_rtos_timer_create(
                                            "timer_record_interval",
                                            timer_record_interval * 1000,
                                            pdFALSE,  // 单次
                                            (void *)0,
                                            timer_record_interval_callback);
    }
    else
    {
        /* 重新设置周期并启动 */
        bc_rtos_timer_stop(timer_record_interval_handle, 0);
        bc_rtos_timer_change_period(timer_record_interval_handle, timer_record_interval * 1000, 0);
        bc_rtos_timer_start(timer_record_interval_handle, 0);
    }
    
    /* 创建时长定时器（单次） */
    if(timer_record_duration_handle == NULL)
    {
        timer_record_duration_handle = bc_rtos_timer_create(
                                            "timer_record_duration",
                                            timer_record_duration * 1000,
                                            pdFALSE, // 单次
                                            (void *)0,
                                            timer_record_duration_callback);
    }
    
    timer_record_running = true;
}

/* 停止定时录音 */
void app_timer_record_stop(void)
{
    BC_LOG_INFO("timer record stop\r\n");
    
    timer_record_running = false;
    timer_record_preempted = false; /* 清除抢占标志 */
    
    if(timer_record_interval_handle != NULL)
    {
        bc_rtos_timer_stop(timer_record_interval_handle, 0);
    }
    
    if(timer_record_duration_handle != NULL)
    {
        bc_rtos_timer_stop(timer_record_duration_handle, 0);
    }
    
    /* 如果正在录音，停止录音 */
    if(app_pdm_work_status())
    {
        BC_LOG_INFO("timer record stop: stop current recording\r\n");
        app_pdm_recording_stop();
    }
}
#endif



















