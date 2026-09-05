#ifndef __LIB_EMOTION_H__
#define __LIB_EMOTION_H__

#ifdef __cplusplus
extern "C"{
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//#define SAMPLERATE 81		// sampling rate
#define SAMPLERATE 100
#define SAMPLETIME 5		// sampling time (seconds) data of which will be processed every time
#define EMOTIONTIME 60
#define BPTIME 30
#define DDTIME 20			// measure duration of drinking degree
#define SINGLETIME 5
#define MAX_SLEEP_STATE 50


/* Integer data types */
#if ((defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)) && \
     (!defined(__ICC8051__) || (__ICC8051__ == 0)))
#include <stdint.h>
#else
	typedef signed char int8_t;
	typedef unsigned char uint8_t;
	typedef signed short int16_t;
	typedef unsigned short uint16_t;
	typedef signed long int32_t;
	typedef unsigned long uint32_t;
	typedef unsigned long long uint64_t;
#endif



void request_validation(
	const unsigned char fact[], const unsigned int fact_len,
	const unsigned char imei[], const unsigned int imei_len,
	unsigned char req_code[], unsigned int* req_code_len);

int activate_algorithm(
	const unsigned char req_code[], const unsigned int req_code_len,
	const unsigned char key_code[], const unsigned int key_code_len);

void version_get_string(char* string, unsigned int* str_len);


#ifdef __cplusplus  
}
#endif


#endif
