#include "wear_check.h"
#include "stdio.h"
#include "bc_delay.h"

//DC检测返回数据初始化
ZS_WEAR_STSTUE_RET deinit_ZS_WEAR_STSTUE_RET(ZS_WEAR_STSTUE_RET set_DC_ret)
{
	set_DC_ret.current_wearing_condition = WEAR_UNDETECTED;
	set_DC_ret.dc_average = 0;
	
	return set_DC_ret;
}

//佩戴检测程序返回数据初始化
ZS_WEAR_STSTUE_FULL_RET deinit_ZS_WEAR_STSTUE_FULL_RET(ZS_WEAR_STSTUE_FULL_RET set_WEAR_CHECK_ret)
{
	set_WEAR_CHECK_ret.ac_wearing_condition = WEAR_UNDETECTED;
	set_WEAR_CHECK_ret.dc_wearing_condition =  WEAR_UNDETECTED;
	set_WEAR_CHECK_ret.result_wear_statue =  WEAR_UNDETECTED;
	set_WEAR_CHECK_ret.dc_average = 0;
	
	return set_WEAR_CHECK_ret;
}
/******************************************************/
//函数名称：uint8_t wear_check__process_step_1(uint8_t* wear_data_temp,uint8_t wear_data_num,uint8_t wear_data_bit_width,uint8_t repeat_tsx,uint8_t Equipment_type)
//函数功能：程序运行前的佩戴检测程序，2级佩戴检测的第一级，根据当前DC值判定设备是否已佩戴   
//功能原理：通过比对采样数据与设定的未佩戴阈值的大小，来判定当前信号是否过小或对空
//
//函数说明：该函数所检测的数据不需要算法的输出结果，最好作为前级检测来用,即该程序单独应用于调用算法之前时，应将数据调整为小端模式；
//
//形式参数说明：				uint8_t* 	wear_data_temp				采样数据的数组地址  从FIFO直接读取的数据
//											uint16_t 	wear_data_num					采样数据的数组内当前可用的数据数量(字节数)
//											uint8_t 	wear_data_bit_width		每组数据的数据位数
//											uint8_t 	repeat_tsx						每TS重复采样次数，对应我司产品 寄存器0X23 的低8位写入值
//											uint8_t 	Equipment_type				设备类型：当前两种选择：ZSPD_TYPE &  ZSBM_TYPE
//
//返回值：当前穿戴状态：	WEAR_NONE：未佩戴		WEAR_POORLY：未正常佩戴			WEAR_NICE：正常佩戴			异常进入：0
/******************************************************/
ZS_WEAR_STSTUE_RET wear_check__process_step_1(uint8_t* wear_data_temp,uint16_t wear_data_num,uint8_t wear_data_bit_width,uint8_t repeat_tsx,uint8_t Equipment_type)
{
	ZS_WEAR_STSTUE_RET step1_wear_check;
	uint8_t byte_num;
	static uint16_t i,j,compute_data_num = 0;
	static int wear_thres,get_data_value = 0;
	
	step1_wear_check = deinit_ZS_WEAR_STSTUE_RET(step1_wear_check);
	
	if((Equipment_type != ZSPD_TYPE ) && (Equipment_type != ZSBM_TYPE))		//异常进入，直接返回，数据不进行处理
		return step1_wear_check;
		
	
	wear_thres = (repeat_tsx+1)*WEAR_CHECK_THRESHOLD;			//计算穿戴检测阈值
	byte_num = wear_data_bit_width/8;		//计算每个数据的字节数
	compute_data_num = 0;				//初始化均值计数
	
	if((Equipment_type == ZSPD_TYPE) && (wear_data_num >byte_num))			//ZSPD_TYPE佩戴验证程序			//字节数不足一格数据的时候不进行比较
	{
		for(i = 0;i<=(wear_data_num-byte_num);i+=byte_num)			//每个数据进行一次比较		//防止指针溢出
		{
			//数据组合
			get_data_value = 0;
			for(j = byte_num;j>0;j--)			//调整数据格式
			{
				get_data_value <<= 8;
				if(i+j-1 >=wear_data_num)
				{
					while(1)
					{
						printf("data_num is  %d   i+j-1 is: %d\r\n  ,i is %d\r\n ", wear_data_num,i+j-1,i);
						bc_delay_ms(1000);
					}
				}
				get_data_value += wear_data_temp[i+j-1];
			}
			
//			for(n = 0;n<byte_num;n++)			//读取每个数据的真实值
//			{
//				get_data_value <<=8;
//				get_data_value += wear_temp[byte_num - 1 - n];
//			}
			compute_data_num++;
			step1_wear_check.dc_average += get_data_value;
			if((get_data_value <= wear_thres) || (get_data_value > (0x7fff * (repeat_tsx+1)))	)	//读取数据的值小于穿戴检测阈值，则判定没有佩戴
			{
				step1_wear_check.current_wearing_condition = WEAR_NONE;
				step1_wear_check.dc_average = get_data_value;			
				
//				LOG_OUT("DC LAST DATA: %d \r\n   ",get_data_value);
				return step1_wear_check;
			}
		}
		
		step1_wear_check.current_wearing_condition = WEAR_NICE;
		step1_wear_check.dc_average = step1_wear_check.dc_average/compute_data_num;				//计算本次穿戴校验的均值DC
//		LOG_OUT("DC LAST DATA: %d \r\n   ",get_data_value);
		return step1_wear_check;	//每组值都大于检测阈值，判定为正常佩戴
				
	
	}
	
//	if(Equipment_type == ZSBM_TYPE)			//ZSBM_TYPE佩戴验证程序
//	{
//		for(i = 0;i<wear_data_num;i+=byte_num)
//		{
//			for(n = 0;n<byte_num;n++)
//			{
//				get_data_value <<=8;
//				get_data_value += wear_temp[byte_num - 1 - n];
//			}
//			if(get_data_value <= wear_thres)			//读取数据的值小于穿戴检测阈值，则判定没有佩戴
//			{
//				return WEAR_NONE;
//			}
//		
//		}
//		return WEAR_NICE;				//每组值都大于检测阈值，判定为正常佩戴
//	
//	}
	
	return step1_wear_check;
}

/******************************************************/
//函数名称：uint8_t wear_check__process(ZSBM_ALGO_OUTPUT_DATA *algo_out,uint8_t* wear_data_temp,uint16_t wear_data_num,uint8_t wear_data_bit_width,uint8_t repeat_tsx,uint8_t Equipment_type)
//函数功能：佩戴检测总函数，直接调用该函数后直接读取当前的佩戴状态
//功能原理：通过算法给出的信号状态来判定当前佩戴状态
//
//函数说明：该函数需要调用算法的输出数据，因此，该程序应用在调用算法之后
//
//形式参数说明：			
//											ZSBM_ALGO_INIT_PARAMETERS *algo_init		当前算法初始化状态结构体指针									
//											ZSBM_ALGO_INPUT_DATA *algo_in						当前算法的输入数据结构体指针
//											ZSBM_ALGO_OUTPUT_DATA *algo_out					当前算法的状态输出数据结构体指针
//											uint8_t 	repeat_tsx										每TS重复采样次数，对应我司产品 寄存器0X23 的低8位写入值+1
//											uint8_t 	Equipment_type								设备类型：当前两种选择：ZSPD_TYPE &  ZSBM_TYPE										
//
//返回值：int32_t  	dc_average;      							// 本次穿戴校验的平均DC值
//  			uint8_t		dc_wearing_condition;    			// DC穿戴检测结果
//				uint8_t		ac_wearing_condition;    			// AC穿戴检测结果
//				uint8_t		result_wear_statue;   				// 本次穿戴判定结果
/******************************************************/
ZS_WEAR_STSTUE_FULL_RET wear_check__process(ZSBM_ALGO_INIT_PARAMETERS *algo_init,ZSBM_ALGO_INPUT_DATA *algo_in ,ZSBM_ALGO_OUTPUT_DATA *algo_out,uint8_t repeat_tsx,uint8_t Equipment_type)
{
	ZS_WEAR_STSTUE_RET current_wear_ret;	
	ZS_WEAR_STSTUE_FULL_RET last_wear_statue_ret;
	static uint8_t step1_wear_last_flg = DC_CHECK_INIT_STATUE, algo_wear_check_flg =AC_CHECK_INIT_STATUE,step2_NG_wear_check_num = 0,step2_WE_wear_check_num = 0,current_mode = 0xff,step2_wear_statue = AC_CHECK_INIT_STATUE;
	static  uint32_t data_judge_num_NG = 0,data_judge_num_WE = 0;
	static uint32_t wear2nice_num = 0,wear2none_num = 0;

	static uint16_t	hm_ac_threshold = 0;									//活体检测目标阈值
	
	//初始化输出状态
	last_wear_statue_ret = deinit_ZS_WEAR_STSTUE_FULL_RET(last_wear_statue_ret);
	
	
	if(((algo_in->data_length)<((algo_init->bit_width_ppg)/8)) ||((Equipment_type != ZSPD_TYPE ) && (Equipment_type != ZSBM_TYPE)))		//异常状态判定
	{
		return last_wear_statue_ret;		//异常进入，直接跳出，不输出穿戴状态
	} 
	
	if(!(algo_in->ptr_x &0x07))			//绿光 红光 红外无数据时跳出
	{
		return last_wear_statue_ret;
	}
		
	
	if(algo_init->algo_type != current_mode)			//更换模式后重新计算各阶段目标阈值	并重新初始化数据
	{
		step1_wear_last_flg = DC_CHECK_INIT_STATUE;
		algo_wear_check_flg =AC_CHECK_INIT_STATUE;
		step2_wear_statue = AC_CHECK_INIT_STATUE;
		current_mode = algo_init->algo_type;
		
		//计算状态更改所需的总数据组
		hm_ac_threshold = repeat_tsx>1?HUMAN_AC_SIGNEL_THRESHOLD*repeat_tsx*7/10:HUMAN_AC_SIGNEL_THRESHOLD*repeat_tsx;		//根据单周期采样次数计算活体检测信号阈值，2ADC_PULSE以下不进行阈值衰减
		
		wear2nice_num = WEAR_NICE_CNT_TIME*(algo_init->sample_rate_ppg)*((algo_init->bit_width_ppg)/8)/10;				//单TS模式计算更改到已佩戴状态所需的数据量(字节数)
		wear2none_num = WEAR_NONE_CNT_TIME*(algo_init->sample_rate_ppg)*((algo_init->bit_width_ppg)/8)/10;				//单TS模式计算更改到未佩戴状态所需的数据量(字节数)
		
//		else
//		{
//			wear2nice_num = WEAR_NICE_CNT_TIME*(algo_init->sample_rate_ppg)*((algo_init->bit_width_ppg)/8)/10 *2;				//双TS模式计算更改到已佩戴状态所需的数据量(字节数)
//			wear2none_num = WEAR_NONE_CNT_TIME*(algo_init->sample_rate_ppg)*((algo_init->bit_width_ppg)/8)/10 *2;				//双TS相关模式计算更改到未佩戴状态所需的数据量(字节数)
//		}
		printf("wear2nice_num %d   ,WEAR_NICE_CNT_TIME: %d,  sample_rate_ppg: %d,  bit_widthis: %d\r\n   ",wear2nice_num,WEAR_NICE_CNT_TIME,algo_init->sample_rate_ppg,algo_init->bit_width_ppg);
	}
	
	
	/******************STEP1 初始DC检测，当算法未被调用时，该项作为佩戴检测的最高优先级输出项，同时该项为未佩戴的最高优先级输出项**************************/
	//根据输入的光源类型数据来判定调用哪一组数据进行DC值检测
	
	if(algo_in->ptr_x &0x01)					//绿光
	{
		current_wear_ret = wear_check__process_step_1(algo_in->green_data,algo_in->data_length,algo_init->bit_width_ppg,repeat_tsx,Equipment_type);
	}
	else if(algo_in->ptr_x &0x02)		//红光			//红光红外都有数据时，使用红光的数据进行判断
	{
		current_wear_ret = wear_check__process_step_1(algo_in->red_data,algo_in->data_length,algo_init->bit_width_ppg,repeat_tsx,Equipment_type);
	}
	else if(algo_in->ptr_x &0x04)		//红外
	{
		current_wear_ret = wear_check__process_step_1(algo_in->ir_data,algo_in->data_length,algo_init->bit_width_ppg,repeat_tsx,Equipment_type);
	}
	else
	{
		return last_wear_statue_ret;			//数据异常，返回未检测
	}
	
	last_wear_statue_ret.dc_average = current_wear_ret.dc_average;
//	LOG_OUT("dc_average,current %d   ,current_wearing_condition %d\r\n   ", current_wear_ret.dc_average,current_wear_ret.current_wearing_condition);
	
	
	if(step1_wear_last_flg != current_wear_ret.current_wearing_condition)					//当前穿戴检测状态改变后才会开始计数
	{
		if(current_wear_ret.current_wearing_condition == WEAR_NONE)
		{
			data_judge_num_NG += algo_in->data_length;
			data_judge_num_WE = 0;
			printf("data_judge_num_NG is: %d ,wear2none_num is: %d\r\n",data_judge_num_NG,wear2none_num);
			if(data_judge_num_NG >= wear2none_num)				//连续多次DC值小于设定阈值，判定为未穿戴
			{
				data_judge_num_NG = 0;
				step1_wear_last_flg = current_wear_ret.current_wearing_condition;
				last_wear_statue_ret.dc_wearing_condition = current_wear_ret.current_wearing_condition;
				step2_wear_statue = WEAR_UNDETECTED; //DC检测未过，判定重新进入算法判定
			}
				
		}
		else						//STEP1判定为穿戴状态
		{	
			data_judge_num_NG = 0;
			data_judge_num_WE += algo_in->data_length;
			printf("data_judge_num_WE is: %d ,wear2nice_num is: %d\r\n",data_judge_num_WE,wear2nice_num);
			if(data_judge_num_WE >= wear2nice_num) 				//连续多次DC值大于设定阈值，且当前未进入算法状态，直接修改佩戴状态为已佩戴
			{
				data_judge_num_WE = 0;
				step1_wear_last_flg = current_wear_ret.current_wearing_condition;
				last_wear_statue_ret.dc_wearing_condition = current_wear_ret.current_wearing_condition;
				
			}
		}
	}
	else									//STEP1校验状态与当前状态一致，清两种字节计数累加器
	{
		data_judge_num_NG = 0;
		data_judge_num_WE =0;
		last_wear_statue_ret.dc_wearing_condition = step1_wear_last_flg;
	}
	

	/******************STEP2 算法佩戴检测，当算法被被调用后才会开始进行下列程序**************************/	
	if(step1_wear_last_flg != WEAR_NONE)			//在已穿戴状态下才会进行算法判定
	{
		/******************AC信号活体检测部分**************************/	
		if(algo_out->ppg_signal_strength_refreshed)				//判定是否已进入算法
		{
			printf("ppg_signal_strength is: %d ,hm_ac_threshold is: %d\r\n",algo_out->ppg_signal_strength,hm_ac_threshold);
			
			if (algo_out->ppg_signal_strength >= hm_ac_threshold) 			//AC信号大于活体检测阈值 判定为已佩戴
			{
				algo_wear_check_flg =  WEAR_NICE;	
			}		
			else																			//AC信号小于活体检测阈值 判定为已佩戴
			{
				algo_wear_check_flg = WEAR_NONE;	
			}	


			if(current_wear_ret.current_wearing_condition == WEAR_NICE)			//STEP1的DC值判定合格的情况
			{
				switch(algo_wear_check_flg)
				{
					case WEAR_NICE:
					{
						if(step2_wear_statue != WEAR_NICE)
						{
							printf("step2_NG_wear_check_num is: %d ,wear2nice_num is: %d\r\n",step2_WE_wear_check_num,WEAR_CHECK_STEP2_WE_CHECK_NUM);
							if(step2_WE_wear_check_num >=WEAR_CHECK_STEP2_WE_CHECK_NUM)
							{
								step2_wear_statue = WEAR_NICE;
								step2_NG_wear_check_num = 0;
								step2_WE_wear_check_num = 0;
								break;
							}
							step2_WE_wear_check_num ++;
						}
						
						break;
					}
					case WEAR_NONE:
					{
						if(step2_wear_statue != WEAR_NONE)
						{
							printf("step2_NG_wear_check_num is: %d ,wear2none_num is: %d\r\n",step2_NG_wear_check_num,WEAR_CHECK_STEP2_NG_CHECK_NUM);
							if(step2_NG_wear_check_num >=WEAR_CHECK_STEP2_NG_CHECK_NUM)
							{
								step2_NG_wear_check_num = 0;
								step2_WE_wear_check_num = 0;
								step2_wear_statue = WEAR_NONE;
								break;
							}
							step2_NG_wear_check_num ++;
						}
						
						break;
					}
					default:
						break;

				}
			}
		
		}
	
	}
	last_wear_statue_ret.dc_wearing_condition = step1_wear_last_flg;
	last_wear_statue_ret.ac_wearing_condition = step2_wear_statue;
	if((last_wear_statue_ret.dc_wearing_condition == WEAR_NICE) && ((last_wear_statue_ret.ac_wearing_condition == WEAR_NICE) || (last_wear_statue_ret.ac_wearing_condition == WEAR_UNDETECTED)))
		last_wear_statue_ret.result_wear_statue = WEAR_NICE;
	else
		last_wear_statue_ret.result_wear_statue = WEAR_NONE;
		
	
	return last_wear_statue_ret;	//返回函数内部状态器记录的状态
}

/******************************************************/
//函数名称：uint8_t wear_check__process(ZSBM_ALGO_OUTPUT_DATA *algo_out,uint8_t* wear_data_temp,uint16_t wear_data_num,uint8_t wear_data_bit_width,uint8_t repeat_tsx,uint8_t Equipment_type)
//函数功能：佩戴检测总函数，直接调用该函数后直接读取当前的佩戴状态
//功能原理：通过算法给出的信号状态来判定当前佩戴状态
//
//函数说明：该函数需要调用算法的输出数据，因此，该程序应用在调用算法之后
//
//形式参数说明：			
//											ZSBM_ALGO_INIT_PARAMETERS *algo_in		当前算法初始化输入数据									
//											ZSBM_ALGO_INPUT_DATA *algo_in					当前算法的输入数据
//											ZSBM_ALGO_OUTPUT_DATA *algo_out				当前算法的状态输出数据
//											uint8_t 	repeat_tsx									每TS重复采样次数，对应我司产品 寄存器0X23 的低8位写入值
//											uint8_t 	Equipment_type							设备类型：当前两种选择：ZSPD_TYPE &  ZSBM_TYPE										
//
//返回值：int32_t  	dc_average;      							// 本次穿戴校验的平均DC值
//  			uint8_t		dc_wearing_condition;    			// DC穿戴检测结果
//				uint8_t		ac_wearing_condition;    			// AC穿戴检测结果
//				uint8_t		result_wear_statue;   				// 本次穿戴判定结果
/******************************************************/
ZS_WEAR_STSTUE_RET Low_Fs_DC_wear_check__process(uint8_t* wear_data_temp,uint16_t wear_data_num,uint8_t wear_data_bit_width,uint8_t repeat_tsx,uint8_t Equipment_type)
{
		
	
	ZS_WEAR_STSTUE_RET low_fs_DC_wear_ret;
	
	low_fs_DC_wear_ret = wear_check__process_step_1(wear_data_temp,wear_data_num, wear_data_bit_width,repeat_tsx,Equipment_type);
	
	


}



