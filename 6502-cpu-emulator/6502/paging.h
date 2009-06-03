/*
 * paging.h
 * This will implement the paging system for our memory
 * Read/Write pages, etc
 * Eventually we want to be able to figure out cache hits/misses here as well
 *
 *
 *  Created on: May 27, 2009
 *      Author: Efseykho
 */

#ifndef PAGING_H_
#define PAGING_H_

#include "definitions.h"

//it seems the 6502 pages are 0xFF bytes in size
//the total addressable memory is 0xFFFF bytes
//
typedef struct{
	unsigned char *data;  //ptr to physical memory

	unsigned short page_addr; //first address of this page
	unsigned char flag; //any flags we might need

	//a listener that's configured to watch accesses to this addr
	//mode should be one of: {READ,WRITE}
	//possible execute for accesses by PC (FUTURE)
	void (*cb_mem_listener)(unsigned short addr, unsigned char val, unsigned char mode);
}page_t;


//flag bit values:
//(high) |x| |x| |x| |x| |listener| |execute| |write| |read| (low)
#define READ 1
#define WRITE 2
#define EXECUTE 4
#define LISTENER 8

//public accessor methods
//
#define SET_READ(P) \
	P = P | READ
#define SET_WRITE(P) \
	P = P | WRITE
#define SET_EXECUTE(P) \
	P = P | EXECUTE
#define SET_LISTENER(P) \
	P = P | LISTENER

#define GET_READ(P) \
	P & READ
#define GET_WRITE(P) \
	P & WRITE
#define GET_EXECUTE(P) \
	P & EXECUTE
#define GET_LISTENER(P) \
	P & LISTENER



#endif /* PAGING_H_ */
