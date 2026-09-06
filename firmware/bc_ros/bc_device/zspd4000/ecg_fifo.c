#include "ecg_fifo.h"

void fifo_init(fifo_t *fifo, uint8_t *buf)
{
	fifo->head = 0;
	fifo->tail = 0;
	fifo->buf = buf;
}


uint8_t fifo_put(fifo_t *fifo, uint8_t c)
{
	int next;
	
	// check if FIFO has room
	next = (fifo->head + 1) % ECG_FIFO_SIZE;
	if (next == fifo->tail) {
		// full
		return 0;
	}
	
	fifo->buf[fifo->head] = c;
	fifo->head = next;
	
	return 1;
}


uint8_t fifo_get(fifo_t *fifo, uint8_t *pc)
{
	int next;
	
	// check if FIFO has data
	if (fifo->head == fifo->tail) {
		return 0;
	}
	
	next = (fifo->tail + 1) % ECG_FIFO_SIZE;
	
	*pc = fifo->buf[fifo->tail];
	fifo->tail = next;

	return 1;
}


int fifo_avail(fifo_t *fifo)
{
	return (ECG_FIFO_SIZE + fifo->head - fifo->tail) % ECG_FIFO_SIZE;
}


int fifo_free(fifo_t *fifo)
{
	return (ECG_FIFO_SIZE - 1 - fifo_avail(fifo));
}
