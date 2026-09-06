#ifndef __BOS1921_H__
#define __BOS1921_H__




struct __attribute__((__packed__)) bos1921_slice_parameters
{
	
	unsigned int amplitude : 12;
	unsigned int :4;
	
	unsigned int  cycles: 8;
	unsigned int  frequency: 8;
	
	
	unsigned int  p180: 1;
	unsigned int  hcyc: 1;
	unsigned int  mode: 2;
	unsigned int  cont: 1;
	
	unsigned int  : 3;
	
	unsigned int  shapedn: 4;
	unsigned int  shapeup: 4;
	
	
};




#endif



