#ifndef __ECG_FIFO_H__
#define __ECG_FIFO_H__

#include <stdint.h>

#define ECG_FIFO_SIZE	600

typedef struct {
	int		head;
	int 	tail;
	uint8_t		*buf;
} fifo_t;

void fifo_init(fifo_t *fifo, uint8_t *buf);
uint8_t fifo_put(fifo_t *fifo, uint8_t c);
uint8_t fifo_get(fifo_t *fifo, uint8_t *pc);
int  fifo_avail(fifo_t *fifo);
int	 fifo_free(fifo_t *fifo);
#endif

