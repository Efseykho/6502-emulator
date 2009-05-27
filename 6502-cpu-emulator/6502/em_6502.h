#ifndef EM_6502_H
#define EM_6502_H

#include <string.h>

#include "definitions.h"



/* Define macros to check the P-register  */
#define CARRY_GET(P) \
        P & 0x01
#define ZERO_GET(P) \
        P & 0x02
#define IRQ_DISABLE_GET(P) \
        P & 0x04
#define DECIMAL_MODE_GET(P) \
        P & 0x08
#define BRK_GET(P) \
        P & 0x10
// bit at position 0x20 is NOT USED
//
#define OVERFLOW_GET(P) \
        P & 0x40
#define NEG_GET(P) \
        P & 0x80
#define NONE_SET(P) \
        P == 0

/* Define macros to set the P-register */
#define CARRY_SET(P) \
        P = P | 0x01
#define ZERO_SET(P) \
        P = P | 0x02
#define IRQ_DISABLE_SET(P) \
        P = P | 0x04
#define DECIMAL_MODE_SET(P) \
        P = P | 0x08
#define BRK_SET(P) \
        P = P | 0x10
#define OVERFLOW_SET(P) \
        P = P | 0x40
#define NEG_SET(P) \
        P = P | 0x80

//define macros to clear P register
/* Define macros to set the P-register */
#define CARRY_CLEAR(P) \
        P = P & 0xFE
#define ZERO_CLEAR(P) \
        P = P & 0xFD
#define IRQ_DISABLE_CLEAR(P) \
        P = P & 0xFB
#define DECIMAL_MODE_CLEAR(P) \
        P = P & 0xF7
#define BRK_CLEAR(P) \
        P = P & 0xEF
#define OVERFLOW_CLEAR(P) \
        P = P & 0xBF
#define NEG_CLEAR(P) \
        P = P & 0x7F


// The P register is composed of following flags:
// s - sign flag (1=7th bit set)
// v - overflow flag
// d - not used/decimal mode flag
// i - break status (1=break used)
// z - zero flag (1=zero value, 0=non-zero value)
// c - carry flag
//
//Note: I dont understand the OVERFLOW -  v bit
//here's what 6502.org has to say about it:
//  This is where V comes in. V indicates whether the result of an addition or subraction is outside the
//  range -128 to 127, i.e. whether there is a twos complement overflow.
//  http://www.6502.org/tutorials/vflag.html
//FOR SBC/ADC, we'll go with that.
//It behaves differently for BIT:
/*
 On the 6502, BIT has two possible addressing modes: zero page or absolute. On the 65C02 (and 65816), BIT has three additional addressing modes: zero page indexed by X -- i.e. zp,X --, absolute indexed by X -- i.e. abs,X -- and immediate.
The immediate addressing mode (65C02 or 65816 only) does not affect V!
The remaining addressing modes affect V as follows: after a BIT instruction, V will contain bit 6 of the byte at the memory location specified in the operand.
For example, if memory location $1000 contains the (hexadecimal) value $AB, then a BIT $1000 instruction will clear V, since bit 6 of $AB is 0. On the other hand, if memory location $1000 contains $40, then BIT $1000 will set V, since bit 6 of $40 is 1.
 **/
#define TEST_AND_SET_ZERO(P,VAL) \
		  if ( VAL == 0x00) ZERO_SET(P); \
		  else  ZERO_CLEAR(P);

#define TEST_AND_SET_NEG(P,VAL) \
		   if ( VAL & 0x80 ) NEG_SET(P); \
			else NEG_CLEAR(P);

//#define TEST_AND_SET_CARRY(P,SHORT) \
//		   if ( SHORT > 0xFF || ) CARRY_SET(P); \
//			else CARRY_CLEAR(P);
//Here's what 6502.org has to say about the carry flag:
//http://www.6502.org/tutorials/vflag.html
/*
  When the addition result is 0 to 255, the carry is cleared.
  When the addition result is greater than 255, the carry is set.
  When the subtraction result is 0 to 255, the carry is set.
  When the subtraction result is less than 0, the carry is cleared.
 *
 *       C  =  CARRY. Set if the add produced a carry, or if the subtraction
 *            produced a borrow.  Also holds bits after a logical shift.
 *       http://www.geocities.com/oneelkruns/asm1step.html
 **/

#define TEST_AND_SET_CARRY_ADDITION(P,CH1,CH2) \
			if ( (int)((int)CH1 + (int)CH2) > 0xFF ) CARRY_SET(P); \
			else CARRY_CLEAR(P);

#define TEST_AND_SET_CARRY_SUBTRACTION(P,CH1,CH2) \
			if ( (int)((int)CH1 - (int)CH2) < 0x00 ) CARRY_CLEAR(P); \
			else CARRY_SET(P);



//use this macro for SBC/ADC ops
//
//      V  =  OVERFLOW. Set if the addition of two like-signed numbers or the
//          subtraction of two unlike-signed numbers produces a result
//          greater than +127 or less than -128.
//via: http://www.geocities.com/oneelkruns/asm1step.html


#define TEST_AND_SET_V_OVERFLOW_ADDITION(P,CH1,CH2) \
	if ( ((int)NEG_GET(CH1)) == ((int)NEG_GET(CH2)) ) \
	{ \
		if ( (signed short)(signed char)CH1 + (signed short)(signed char)CH2 > 127 || (signed short)(signed char)CH1 + (signed short)(signed char)CH2 < -128 ) \
			OVERFLOW_SET(P) ; \
		else OVERFLOW_CLEAR(P) ; \
	} \
	else OVERFLOW_CLEAR(P)

	//printf("here0\n"); \
	//    printf("res0 %d\n",(signed short)(signed char)CH1 ); \
	//	 printf("res1 %d\n",(signed short)(signed char)CH2 ); \
	//	 printf("res2 %d\n",(signed char)CH1 - (signed char)CH2 ); \

#define TEST_AND_SET_V_OVERFLOW_SUBTRACTION(P,CH1,CH2) \
	if ( ((int)NEG_GET(CH1)) != ((int)NEG_GET(CH2)) ) \
	{   \
		if ( (signed short)(signed char)CH1 - (signed short)(signed char)CH2 > 127 || (signed short)(signed char)CH1 - (signed short)(signed char)CH2 < -128 ) \
			OVERFLOW_SET(P); \
		else OVERFLOW_CLEAR(P) ; \
	} \
	else OVERFLOW_CLEAR(P)


//use this macro for BIT ops
#define TEST_SIXTH_MEMORY_BIT(P,MEM_LOC) \
			if ( OVERFLOW_GET(MEM_LOC) ) OVERFLOW_SET(P); \
			else OVERFLOW_CLEAR(P);

//this is to mark those instructions that access memory and do not go through the read/write_mem
//interface. in the future, these might have to be changed
#define STUB_OUT_MEM_ACCESS_IFACES 1
#ifndef STUB_OUT_MEM_ACCESS_IFACES
	assert(0); //this fails because its not going through read/write_mem iface
#endif


//this is to mark those instructions that deal with interrups
//in the future, these might have to be changed
#define STUB_OUT_INTERRUPTS_IFACES 1
#ifndef STUB_OUT_INTERRUPTS_IFACES
	assert(0); //this fails because its not doing the interrupts correctly
#endif


//what defines a memory region
//for mem-mapped io services
typedef struct {
	unsigned short low;
	unsigned short high;
}mem_region;





typedef struct {
        unsigned char Acc; //accumulator
        unsigned char X; //X register
        unsigned char Y; //Y register
        unsigned char P; //status/P register
        unsigned char S; //stack pointer
        unsigned short PC; //program counter
        unsigned char Memory[MEMORY_SIZE];

		#ifdef ALLOW_MAX_INSTR_COUNT
		  unsigned int instr_count;  //how many instructions we executed
		#endif

		#ifdef ENABLE_MEM_MAP_DEVICES
		  void (*mem_write_listeners[MAX_MEMORY_WRITER_LISTENERS])(unsigned short);
		  mem_region regions[MAX_MEMORY_WRITER_LISTENERS]; //where we store the corresponding mem_region
		  int num_wlisteners; //number of thusly registered listeners

		#endif
}em6502;

/**************************************
 * Name:  initialize_em6502
 * Inputs:  em6502 * - the 6502 object to init
 * Outputs: None
 * Function: initializes the values to a known state
 *
***************************************/
void initialize_em6502( em6502 *);

/**************************************
 * Name:  load_program
 * Inputs:  em6502 * - the 6502 object to load program
 *	 			void * - the memory containing the program; gets copied into em6502->Memory
 *				size_t - size of program to copy
 *				unsigned int -  offset from start
 * Outputs: None
 * Function: copies the memory into the memory space of the em6502 object
 *
***************************************/
void load_program( em6502 *, void *, size_t, unsigned int);


/**************************************
 * Name:  run_program
 * Inputs:  em6502 * - the 6502 object to execute
 *				unsigned int - max number of instructions to execute, -1 for all
 * Outputs: None
 * Function: executes the previosuly loaded program
 *
***************************************/
void run_program( em6502 *, unsigned int );


/**************************************
 * Name:  add_memory_write_listener
 * Inputs:  em6502 * - the 6502 object to execute
 * 			mem_region - memory region to watch
 *			void (*cb_mem_write)(unsigned int) - callback function ptr to invoke
 * Outputs: None
 * Function: registers a memory write listener at given memory region
 *
***************************************/
void add_memory_write_listener( em6502 *, mem_region, void (*cb_mem_write)(unsigned short) );

#endif  /* EM_6502_H */
