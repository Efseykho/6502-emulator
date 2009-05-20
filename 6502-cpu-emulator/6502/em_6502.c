/* This is the implementation of the 6502 emulator  */

#include <stdlib.h>  //- for malloc'ing
#include <string.h>
#include "assert.h"


#include "em_6502.h"


/*
 * Rules for when to use read_mem/write_mem functions:
 * 	The reason why we need them is for mem-mapped io. We have to trap
 *	some memory accesses to certain regions in memory and make special things happen.
 * However we dont need to worry about ALL memory accesses. So the rules are simple:
 * trap ONLY st* and ld* instructions cause those affect memory directly. All others are
 * read-only. I think this should be good enough.
 *
 * In particular, trapping for every read instr is silly when we constantly increment
 * PC.
 *
 *
 *
 */
/**************************************
 * Name:  read_mem
 * Inputs:  em6502 * - the 6502 chip whose memory we want to read
 *				unsigned short  - the addr to get
 * Outputs: unsigned char - the value at that memory location
 * Function: returns memory at given addr
 *
***************************************/
inline unsigned char read_mem( em6502 *em, unsigned short addr )
{
	//currently, we're not using mem-mapped IO so we're fine using this
	return em->Memory[addr];
}

/**************************************
 * Name:  write_mem
 * Inputs:  em6502 * - the 6502 chip whose memory we want to write
 *			unsigned short  - the addr to write to
 *			unsigned char - the value to write
 * Outputs: none
 * Function: writes memory to given addr
 *
***************************************/
inline void write_mem( em6502 *emu, unsigned short addr, unsigned char val )
{
	emu->Memory[addr] = val;

	//check to figure out if we're using mem-mapped io
	#ifdef ENABLE_MEM_MAP_DEVICES
		int i;
		for ( i = 0; i < emu->num_wlisteners; i++ ) //check any registered mem-write-listeners
		{
			if ( addr >= emu->regions[i].low && addr <= emu->regions[i].high )
			{
				//(*pt2Function) (12, 'a', 'b');
				(* emu->mem_write_listeners[i])(addr);
				break;
			}
		}
	#endif
}

//This is a convenience macro for getting the next argument for the PC
#define GET_FIRST_ARG emu->Memory[emu->PC+1]

//This is a convenience macro for getting the 2nd next argument for the PC
#define GET_SECOND_ARG emu->Memory[emu->PC+2]


//These are convinience macros for commonly used addressing modes
//they each return the addr for the memory access
#define IMMIDIATE_ACCESS GET_FIRST_ARG
#define ZP_DIRECT_ACCESS (unsigned char)GET_FIRST_ARG //we must not allow overflow to short type on zero-page addrs
#define ZP_INDEXED_X_ACCESS (unsigned char) (GET_FIRST_ARG + emu->X) //we must not allow overflow to short type on zero-page addrs
#define ZP_INDEXED_Y_ACCESS (unsigned char)(GET_FIRST_ARG + emu->Y) //we must not allow overflow to short type on zero-page addrs
#define PRE_INDEXED_X_INDIRECT_ACCESS generate_addr( emu->Memory[GET_FIRST_ARG + emu->X], emu->Memory[GET_FIRST_ARG + emu->X + 1] )
#define POST_INDEXED_Y_INDIRECT_ACCESS generate_addr( emu->Memory[GET_FIRST_ARG], emu->Memory[GET_FIRST_ARG + 1] ) + emu->Y
#define EXTENDED_DIRECT_ACCESS generate_addr(GET_FIRST_ARG, GET_SECOND_ARG)
#define ABSOLUTE_INDEXED_Y_ACCESS generate_addr(GET_FIRST_ARG, GET_SECOND_ARG)+emu->Y
#define ABSOLUTE_INDEXED_X_ACCESS generate_addr(GET_FIRST_ARG, GET_SECOND_ARG)+emu->X

//this is a special case of pre/post indexed indirect addressing with index=0
//only ever used by the JMP instr
#define ABSOLUTE_INDIRECT_JMP_ACCESS generate_addr( emu->Memory[generate_addr( GET_FIRST_ARG, GET_SECOND_ARG )], \
													emu->Memory[generate_addr( GET_FIRST_ARG + 1, GET_SECOND_ARG )] )


//this is the default addr that the stack starts a
//defined by chip, i guess
#define STACK_HIGH_ADDR 0x01

//this is the location of the ISR in 6502
#define ISR_HIGH_ADDR 0xFF
#define ISR_LOW_ADDR  ISR_HIGH_ADDR - 1

/**************************************
 * Name:  generate_addr
 * Inputs:  unsigned char  - the low char memory addr
 *				unsigned char - the high char memory addr
 * Outputs: unsigned short
 * Function: the memory addr formed by the low,high bits passed in
 *
***************************************/
inline unsigned short generate_addr(unsigned char low, unsigned char high )
{
	unsigned short ret = 0;
	unsigned char *ptr = (unsigned char *)&ret;
	(*ptr) = low;
	(*(ptr+1)) = high;
	return ret;
}




/**************************************
 * Name:  initialize_em6502
 * Inputs:  em6502 * - the 6502 chip to init
 * Outputs: None
 * Function: initializes the values to a known state
 *
***************************************/
void initialize_em6502(em6502 * emu)
{
	int i;

	emu->Acc = 0;
	emu->X = 0;
	emu->Y = 0;
	emu->P = 0;
	emu->S = 0xFF;  //stack confined to: $0100-$01FF, starts at $01FF
	emu->PC = 0;

	//OVERFLOW_SET(emu->P) ; //we start out with this flag set, who knows why?

	memset(emu->Memory, -1, MEMORY_SIZE );

	#ifdef ALLOW_MAX_INSTR_COUNT
	emu->instr_count = 0;
	#endif

	//setup the function pointers to memory listeners, if available
	#ifdef ENABLE_MEM_MAP_DEVICES
		for ( i = 0; i < MAX_MEMORY_WRITER_LISTENERS; i++ )
		{
			emu->mem_write_listeners[i] = 0;
		}
		emu->num_wlisteners = 0;
	#endif


}

/**************************************
 * Name:  load_program
 * Inputs:  em6502 * - the 6502 object to load program
 *	 			void * - the memory containing the program; gets copied into em6502->Memory
 *				size_t - size of program to copy
 *				unsigned int -  offset from start
 * Outputs: None
 * Function: copies the memory into the memory space of the em6502 object. This allows us
 * to use relocatable code and copy it anywhere in the memory space
 *
***************************************/
void load_program( em6502 *emu, void *prog, size_t size, unsigned int offset)
{
	memcpy( emu->Memory+offset, prog, size );

	//also, we must fix the PC to point to start of memory region, for supporting relocatable code
	emu->PC = offset;
}


/**************************************
 * Name:  add_memory_write_listener
 * Inputs:  em6502 * - the 6502 object to execute
 * 			mem_region - memory region to watch
 *			void (*cb_mem_write)(unsigned int) - callback function ptr to invoke
 * Outputs: None
 * Function: registers a memory write listener at given memory region
 *
***************************************/
void add_memory_write_listener( em6502 *emu, mem_region region, void (*cb_mem_write)(unsigned short) )
{
	emu->mem_write_listeners[emu->num_wlisteners] = cb_mem_write;
	emu->regions[emu->num_wlisteners] = region;

	emu->num_wlisteners++;

	//printf("registered region %d-%d with listener\n", region.low,region.high);
}


/**************************************
 * Name:  run_program
 * Inputs:  em6502 * - the 6502 object to execute
 *				unsigned int - max number of instructions to execute, -1 for all
 * Outputs: None
 * Function: executes the previosuly loaded program
 *
***************************************/
void run_program( em6502 *emu, unsigned int max_instr_count )
{
	unsigned char ch1;
	unsigned char ch2;
	unsigned char res;


	#ifdef ALLOW_MAX_INSTR_COUNT
		while (max_instr_count--)
	#elif
		while(1)
	#endif
	{
		//process a single instruction here
		//this defines the main logic loop that implements the instruction set for the 6502 chip
		switch( emu->Memory[emu->PC] )
		{

			//***********************>>>LDA INSTRUCTIONS<<<*************************
			case 0xA9: // LDA data : A<- data, immidiate addressing mode
				emu->Acc = read_mem(emu, emu->PC+1 ); //emu->Memory[emu->PC+1];
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0xA5: // LDA data : A<- [addr], zero-page direct addressing mode
				emu->Acc = read_mem(emu, ZP_DIRECT_ACCESS );
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0xB5: //LDA data : A<- [addr+X] , zero-page indexed addressing mode
				emu->Acc = read_mem(emu, ZP_INDEXED_X_ACCESS );
			    emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

		   case 0xA1: // LDA data : A<- [[addr+X]], pre-indexed, indirect addressing mode
				emu->Acc = read_mem(emu, PRE_INDEXED_X_INDIRECT_ACCESS );
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0xB1: // LDA data : A<- [[addr+1,addr]+Y], post-indexed, indirect addressing mode
				emu->Acc = read_mem(emu, POST_INDEXED_Y_INDIRECT_ACCESS );
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0xAD: //LDA data : A<- [addr16] , extended direct addressing mode
				emu->Acc = read_mem(emu, EXTENDED_DIRECT_ACCESS );
				emu->PC+=3;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0xB9: //LDA data : A<- [addr16+Y], absolute indexed addressing mode
				emu->Acc = read_mem(emu, ABSOLUTE_INDEXED_Y_ACCESS );
				emu->PC+=3;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0xBD: //LDA data : A<- [addr16+X], absolute indexed addressing mode
				emu->Acc = read_mem(emu, ABSOLUTE_INDEXED_X_ACCESS );
				emu->PC+=3;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;


			//***********************>>>LDY INSTRUCTIONS<<<*************************
			case 0xA0: // LDY data : Y<- data, immidiate addressing mode
				emu->Y = read_mem(emu, emu->PC+1); //emu->Memory[emu->PC+1];
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Y) ;
				TEST_AND_SET_NEG(emu->P, emu->Y) ;
				break;

		  case 0xA4: // LDY data : Y<- [data], zero-page direct addressing mode
				emu->Y = read_mem(emu, ZP_DIRECT_ACCESS );
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Y) ;
				TEST_AND_SET_NEG(emu->P, emu->Y) ;
				break;

		  case 0xB4: // LDY data : Y<- [data+X], zero-page indexed addressing mode
				emu->Y = read_mem(emu, ZP_INDEXED_X_ACCESS );
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Y) ;
				TEST_AND_SET_NEG(emu->P, emu->Y) ;
				break;

		 case 0xAC: // LDY data : Y<- [data16], extended direct addressing mode
				emu->Y = read_mem(emu, EXTENDED_DIRECT_ACCESS );
				emu->PC+=3;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Y) ;
				TEST_AND_SET_NEG(emu->P, emu->Y) ;
				break;

		case 0xBC: // LDY data : Y<- [data16+X], absolute in addressing mode
				emu->Y = read_mem(emu, ABSOLUTE_INDEXED_X_ACCESS );
				emu->PC+=3;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Y) ;
				TEST_AND_SET_NEG(emu->P, emu->Y) ;
				break;


			//***********************>>>LDX INSTRUCTIONS<<<*************************
		   case 0xA2: // LDX data : X<- data, immidiate addressing mode
				emu->X = read_mem(emu, emu->PC+1); //emu->Memory[emu->PC+1];
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->X) ;
				TEST_AND_SET_NEG(emu->P, emu->X) ;
				break;

		  case 0xA6: // LDX data : X<- [data], zero-page direct addressing mode
				emu->X = read_mem(emu, ZP_DIRECT_ACCESS );
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->X) ;
				TEST_AND_SET_NEG(emu->P, emu->X) ;
				break;

		  case 0xB6: // LDX data : X<- [data+Y], zero-page indexed addressing mode
				emu->X = read_mem(emu, ZP_INDEXED_Y_ACCESS );
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->X) ;
				TEST_AND_SET_NEG(emu->P, emu->X) ;
				break;

		 case 0xAE: // LDX data : X<- [data16], extended direct addressing mode
				emu->X = read_mem(emu, EXTENDED_DIRECT_ACCESS );
				emu->PC+=3;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->X) ;
				TEST_AND_SET_NEG(emu->P, emu->X) ;
				break;

		case 0xBE: // LDX data : X<- [data16+Y], absolute in addressing mode
				emu->X = read_mem(emu, ABSOLUTE_INDEXED_Y_ACCESS );
				emu->PC+=3;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->X) ;
				TEST_AND_SET_NEG(emu->P, emu->X) ;
				break;


			//***********************>>>STA INSTRUCTIONS<<<*************************
			case 0x85: //STA addr : [addr]<- A, zero-page direct addressing mode
				write_mem(emu, ZP_DIRECT_ACCESS, emu->Acc  );
			   emu->PC+=2;
				//affects no flags
				break;

			case 0x95: //STA addr : [addr+X]<- A, zero-page indexed addressing mode
				write_mem(emu, ZP_INDEXED_X_ACCESS, emu->Acc  );
			   emu->PC+=2;
				//affects no flags
				break;

			case 0x81: //STA addr : [[addr+X]]<- A, pre-indexed indirect addressing mode
				write_mem(emu, PRE_INDEXED_X_INDIRECT_ACCESS, emu->Acc  );
			   emu->PC+=2;
				//affects no flags
				break;

			case 0x91: //STA addr : [[addr+1, addr]+ Y]<- A, post-indexed indirect addressing mode
				write_mem( emu, POST_INDEXED_Y_INDIRECT_ACCESS, emu->Acc );
				emu->PC+=2;
				//affects no flags
				break;

		  case 0x8D: //STA addr : [addr16]<- A, extended direct addressing mode
				write_mem( emu, EXTENDED_DIRECT_ACCESS, emu->Acc );
				emu->PC+=3;
				//affects no flags
				break;

			case 0x99: //STA addr : [addr16+Y]<- A, absolute indexed addressing mode
				write_mem( emu, ABSOLUTE_INDEXED_Y_ACCESS, emu->Acc );
				emu->PC+=3;
				//affects no flags
				break;

			case 0x9D: //STA addr : [addr16+X]<- A, absolute indexed addressing mode
				write_mem( emu, ABSOLUTE_INDEXED_X_ACCESS, emu->Acc );
				emu->PC+=3;
				//affects no flags
				break;


			//***********************>>>STX INSTRUCTIONS<<<*************************
			case 0x86: //STX addr : [addr]<- X, zero page, direct addressing mode
				write_mem(emu, ZP_DIRECT_ACCESS, emu->X  );
				emu->PC+=2;
				//affects no flags
				break;

			case 0x96: //STX addr : [addr+Y]<- X, zero-page indexed addressing mode
				write_mem(emu, ZP_INDEXED_Y_ACCESS, emu->X  );
				emu->PC+=2;
				//affects no flags
				break;

			case 0x8E: //STX addr : [addr16]<- X, extended direct addressing mode
				write_mem( emu, EXTENDED_DIRECT_ACCESS, emu->X );
				emu->PC+=3;
				//affects no flags
				break;


			//***********************>>>STY INSTRUCTIONS<<<*************************
			case 0x84: //STY addr : [addr]<- Y, zero page, direct addressing mode
				write_mem(emu, ZP_DIRECT_ACCESS, emu->Y  );
				emu->PC+=2;
				//affects no flags
				break;

			case 0x94: //STY addr : [addr+X]<- Y, zero-page indexed addressing mode
				write_mem(emu, ZP_INDEXED_X_ACCESS, emu->Y  );
				emu->PC+=2;
				//affects no flags
				break;

			case 0x8C: //STY addr : [addr16]<- Y, extended direct addressing mode
				write_mem( emu, EXTENDED_DIRECT_ACCESS, emu->Y );
				emu->PC+=3;
				//affects no flags
				break;


			//***********************>>>FLAG INSTRUCTIONS<<<*************************
			case 0x18:  //CLC : C<- 0, clear carry flag
				CARRY_CLEAR(emu->P);
				emu->PC+=1;
				//affects no flags
				break;

			case 0x38:  //SEC : C<- 1, set carry flag
				CARRY_SET(emu->P);
				emu->PC+=1;
				//affects no flags
				break;

			case 0xD8:  //CLD : D<- 0, clear decimal flag
				DECIMAL_MODE_CLEAR(emu->P);
				emu->PC+=1;
				//affects no flags
				break;

			case 0xF8:  //SED : D<- 1, set decimal flag
				DECIMAL_MODE_SET(emu->P);
				emu->PC+=1;
				//affects no flags
				break;

			case 0xB8:  //CLV : V<- 1, clear overflow flag
				OVERFLOW_CLEAR(emu->P);
				emu->PC+=1;
				//affects no flags
				break;


			//***********************>>>ADC INSTRUCTIONS<<<*************************
			case 0x69: //ADC addr : A<- A + IMM + C
			{
				ch1 = emu->Acc;
				ch2 = IMMIDIATE_ACCESS;

				emu->Acc = (ch1 + ch2) + (unsigned char)(CARRY_GET(emu->P));
				emu->PC+=2;
				//affects s,z,v,c flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				TEST_AND_SET_CARRY_ADDITION(emu->P, ch1, ch2 ) ;
				TEST_AND_SET_V_OVERFLOW_ADDITION(emu->P, ch1, ch2) ;
				break;
			}

			case 0x65: //ADC addr : A<- A + [addr] + C
			{
				ch1 = emu->Acc;
				ch2 = emu->Memory[ZP_DIRECT_ACCESS];

				emu->Acc = (ch1 + ch2) + (unsigned char)(CARRY_GET(emu->P));
				emu->PC+=2;
				//affects s,z,v,c flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				TEST_AND_SET_CARRY_ADDITION(emu->P, ch1, ch2 ) ;
				TEST_AND_SET_V_OVERFLOW_ADDITION(emu->P, ch1, ch2) ;
				break;
			}

			case 0x75: //ADC addr : A<- A + [addr+X] + C
			{
				ch1 = emu->Acc;
				ch2 = emu->Memory[ZP_INDEXED_X_ACCESS];

				emu->Acc = (ch1 + ch2) + (unsigned char)(CARRY_GET(emu->P));
				emu->PC+=2;
				//affects s,z,v,c flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				TEST_AND_SET_CARRY_ADDITION(emu->P, ch1, ch2 ) ;
				TEST_AND_SET_V_OVERFLOW_ADDITION(emu->P, ch1, ch2) ;
				break;
			}

		    case 0x61: //ADC addr : A<- A + [[addr+X]] + C
			{
				ch1 = emu->Acc;
				ch2 = emu->Memory[PRE_INDEXED_X_INDIRECT_ACCESS];

				emu->Acc = (ch1 + ch2) + (unsigned char)(CARRY_GET(emu->P));
				emu->PC+=2;
				//affects s,z,v,c flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				TEST_AND_SET_CARRY_ADDITION(emu->P, ch1, ch2 ) ;
				TEST_AND_SET_V_OVERFLOW_ADDITION(emu->P, ch1, ch2) ;
				break;
			}

			case 0x71: //ADC addr : A<- A+ [[addr+1, addr]+ Y] + C, post-indexed indirect addressing mode
			{
				ch1 = emu->Acc;
				ch2 = emu->Memory[POST_INDEXED_Y_INDIRECT_ACCESS];

				emu->Acc = (ch1 + ch2) + (unsigned char)(CARRY_GET(emu->P));
				emu->PC+=2;
				//affects s,z,v,c flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				TEST_AND_SET_CARRY_ADDITION(emu->P, ch1, ch2 ) ;
				TEST_AND_SET_V_OVERFLOW_ADDITION(emu->P, ch1, ch2) ;
				break;
			}

			case 0x6D: //ADC addr : A<- A+ [addr16] + C, extended direct addressing mode
			{
				ch1 = emu->Acc;
				ch2 = emu->Memory[EXTENDED_DIRECT_ACCESS];

				emu->Acc = (ch1 + ch2) + (unsigned char)(CARRY_GET(emu->P));
				emu->PC+=3;
				//affects s,z,v,c flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				TEST_AND_SET_CARRY_ADDITION(emu->P, ch1, ch2 ) ;
				TEST_AND_SET_V_OVERFLOW_ADDITION(emu->P, ch1, ch2) ;
				break;
			}

			case 0x79: //ADC addr: A<- A+ [addr16+Y] + C, absolute indexed addressing mode
			{
				ch1 = emu->Acc;
				ch2 = emu->Memory[ABSOLUTE_INDEXED_Y_ACCESS];

				emu->Acc = (ch1 + ch2) + (unsigned char)(CARRY_GET(emu->P));
				emu->PC+=3;
				//affects s,z,v,c flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				TEST_AND_SET_CARRY_ADDITION(emu->P, ch1, ch2 ) ;
				TEST_AND_SET_V_OVERFLOW_ADDITION(emu->P, ch1, ch2) ;
				break;
			}

			case 0x7D: //ADC addr: A<- A+ [addr16+X] + C, absolute indexed addressing mode
			{
				ch1 = emu->Acc;
				ch2 = emu->Memory[ABSOLUTE_INDEXED_X_ACCESS];

				emu->Acc = (ch1 + ch2) + (unsigned char)(CARRY_GET(emu->P));
				emu->PC+=3;
				//affects s,z,v,c flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				TEST_AND_SET_CARRY_ADDITION(emu->P, ch1, ch2 ) ;
				TEST_AND_SET_V_OVERFLOW_ADDITION(emu->P, ch1, ch2) ;
				break;
			}


			//***********************>>>AND INSTRUCTIONS<<<*************************
			case 0x29: //AND addr : A<- A AND IMM
				emu->Acc = emu->Acc & IMMIDIATE_ACCESS ;
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0x25: //AND addr : A<- A AND [addr]
				emu->Acc = emu->Acc & emu->Memory[ZP_DIRECT_ACCESS];
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0x35: //AND addr : A<- A AND [addr+X]
				emu->Acc = emu->Acc & emu->Memory[ZP_INDEXED_X_ACCESS];
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0x21: //AND addr : A<- A AND [[addr+X]]
				emu->Acc = emu->Acc & emu->Memory[PRE_INDEXED_X_INDIRECT_ACCESS];
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0x31: //AND addr : A<- A AND [[addr+1,addr] +Y]
				emu->Acc = emu->Acc & emu->Memory[POST_INDEXED_Y_INDIRECT_ACCESS];
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0x2D: //AND addr : A<- A AND [addr16]
				emu->Acc = emu->Acc & emu->Memory[EXTENDED_DIRECT_ACCESS];
				emu->PC+=3;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0x39: //AND addr : A<- A AND [addr16+Y]
				emu->Acc = emu->Acc & emu->Memory[ABSOLUTE_INDEXED_Y_ACCESS];
				emu->PC+=3;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0x3D: //AND addr : A<- A AND [addr16+X]
				emu->Acc = emu->Acc & emu->Memory[ABSOLUTE_INDEXED_X_ACCESS];
				emu->PC+=3;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;


			//***********************>>>BIT INSTRUCTIONS<<<*************************
			case 0x24: //BIT addr : A AND [addr], sets s,z,v flags only
				//affects s,z,v flags
				TEST_AND_SET_ZERO(emu->P, (unsigned char)(emu->Acc & emu->Memory[ZP_DIRECT_ACCESS]) );
				TEST_AND_SET_NEG(emu->P, emu->Memory[ZP_DIRECT_ACCESS] );
				TEST_SIXTH_MEMORY_BIT(emu->P, emu->Memory[ZP_DIRECT_ACCESS] );

				emu->PC+=2;
				break;

			case 0x2C: //BIT addr : A AND [addr16], sets s,z,v flags only
				//affects s,z,v flags
				TEST_AND_SET_ZERO(emu->P, (unsigned char)(emu->Acc & emu->Memory[EXTENDED_DIRECT_ACCESS]) );
				TEST_AND_SET_NEG(emu->P, emu->Memory[EXTENDED_DIRECT_ACCESS] );
				TEST_SIXTH_MEMORY_BIT(emu->P, emu->Memory[EXTENDED_DIRECT_ACCESS] );

				emu->PC+=3;
				break;


			//***********************>>>CMP INSTRUCTIONS<<<*************************
			case 0xC9: //CMP addr : A - IMM, sets s,z,c flags only
			{
				ch1 = emu->Acc;
				ch2 = IMMIDIATE_ACCESS ;
				res = ch1 - ch2;

				emu->PC+=2;

				//affects s,z,c flags
				TEST_AND_SET_ZERO(emu->P, res) ;
				TEST_AND_SET_NEG(emu->P, res) ;
				TEST_AND_SET_CARRY_SUBTRACTION(emu->P, ch1, ch2 ) ;
				break;
			}

			case 0xC5: //CMP addr : A - [addr], sets s,z,c flags only
			{
				ch1 = emu->Acc;
				ch2 = emu->Memory[ZP_DIRECT_ACCESS];
				res = ch1 - ch2;

				emu->PC+=2;

				//affects s,z,c flags
				TEST_AND_SET_ZERO(emu->P, res) ;
				TEST_AND_SET_NEG(emu->P, res) ;
				TEST_AND_SET_CARRY_SUBTRACTION(emu->P, ch1, ch2 ) ;
				break;
			}

			case 0xD5: //CMP addr : A - [addr+X], sets s,z,c flags only
			{
				ch1 = emu->Acc;
				ch2 = emu->Memory[ZP_INDEXED_X_ACCESS];
				res = ch1 - ch2;

				emu->PC+=2;

				//affects s,z,c flags
				TEST_AND_SET_ZERO(emu->P, res) ;
				TEST_AND_SET_NEG(emu->P, res) ;
				TEST_AND_SET_CARRY_SUBTRACTION(emu->P, ch1, ch2 ) ;
				break;
			}

			case 0xC1: //CMP addr : A - [[addr+X]], sets s,z,c flags only
			{
				ch1 = emu->Acc;
				ch2 = emu->Memory[PRE_INDEXED_X_INDIRECT_ACCESS];
				res = ch1 - ch2;

				emu->PC+=2;

				//affects s,z,c flags
				TEST_AND_SET_ZERO(emu->P, res) ;
				TEST_AND_SET_NEG(emu->P, res) ;
				TEST_AND_SET_CARRY_SUBTRACTION(emu->P, ch1, ch2 ) ;
				break;
			}

			case 0xD1: //CMP addr : A - [[addr+1,addr]+Y], sets s,z,c flags only
			{
				ch1 = emu->Acc;
				ch2 = emu->Memory[POST_INDEXED_Y_INDIRECT_ACCESS];
				res = ch1 - ch2;

				emu->PC+=2;

				//affects s,z,c flags
				TEST_AND_SET_ZERO(emu->P, res) ;
				TEST_AND_SET_NEG(emu->P, res) ;
				TEST_AND_SET_CARRY_SUBTRACTION(emu->P, ch1, ch2 ) ;
				break;
			}

			case 0xCD: //CMP addr : A - [addr16], sets s,z,c flags only
			{
				ch1 = emu->Acc;
				ch2 = emu->Memory[EXTENDED_DIRECT_ACCESS];
				res = ch1 - ch2;

				emu->PC+=3;

				//affects s,z,c flags
				TEST_AND_SET_ZERO(emu->P, res) ;
				TEST_AND_SET_NEG(emu->P, res) ;
				TEST_AND_SET_CARRY_SUBTRACTION(emu->P, ch1, ch2 ) ;
				break;
			}

			case 0xD9: //CMP addr : A - [addr16+Y], sets s,z,c flags only
			{
				ch1 = emu->Acc;
				ch2 = emu->Memory[ABSOLUTE_INDEXED_Y_ACCESS];
				res = ch1 - ch2;

				emu->PC+=3;

				//affects s,z,c flags
				TEST_AND_SET_ZERO(emu->P, res) ;
				TEST_AND_SET_NEG(emu->P, res) ;
				TEST_AND_SET_CARRY_SUBTRACTION(emu->P, ch1, ch2 ) ;
				break;
			}

			case 0xDD: //CMP addr : A - [addr16+X], sets s,z,c flags only
			{
				ch1 = emu->Acc;
				ch2 = emu->Memory[ABSOLUTE_INDEXED_X_ACCESS];
				res = ch1 - ch2;

				emu->PC+=3;

				//affects s,z,c flags
				TEST_AND_SET_ZERO(emu->P, res) ;
				TEST_AND_SET_NEG(emu->P, res) ;
				TEST_AND_SET_CARRY_SUBTRACTION(emu->P, ch1, ch2 ) ;
				break;
			}


			//***********************>>>EOR INSTRUCTIONS<<<*************************
			case 0x49: //EOR addr : A<- A ^ IMM
				emu->Acc = emu->Acc ^ IMMIDIATE_ACCESS ;
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0x45: //EOR addr : A<- A ^ [addr]
				emu->Acc = emu->Acc ^ emu->Memory[ZP_DIRECT_ACCESS];
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0x55: //EOR addr : A<- A ^ [addr+X]
				emu->Acc = emu->Acc ^ emu->Memory[ZP_INDEXED_X_ACCESS];
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0x41: //EOR addr : A<- A ^ [[addr+X]]
				emu->Acc = emu->Acc ^ emu->Memory[PRE_INDEXED_X_INDIRECT_ACCESS];
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0x51: //EOR addr : A<- A ^ [[addr+1,addr] +Y]
				emu->Acc = emu->Acc ^ emu->Memory[POST_INDEXED_Y_INDIRECT_ACCESS];
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0x4D: //EOR addr : A<- A ^ [addr16]
				emu->Acc = emu->Acc ^ emu->Memory[EXTENDED_DIRECT_ACCESS];
				emu->PC+=3;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0x59: //EOR addr : A<- A ^ [addr16+Y]
				emu->Acc = emu->Acc ^ emu->Memory[ABSOLUTE_INDEXED_Y_ACCESS];
				emu->PC+=3;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0x5D: //EOR addr : A<- A ^ [addr16+X]
				emu->Acc = emu->Acc ^ emu->Memory[ABSOLUTE_INDEXED_X_ACCESS];
				emu->PC+=3;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;


			//***********************>>>ORA INSTRUCTIONS<<<*************************
			case 0x09: //ORA addr : A<- A | IMM
				emu->Acc = emu->Acc | IMMIDIATE_ACCESS ;
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0x05: //ORA addr : A<- A | [addr]
				emu->Acc = emu->Acc | emu->Memory[ZP_DIRECT_ACCESS];
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0x15: //ORA addr : A<- A | [addr+X]
				emu->Acc = emu->Acc | emu->Memory[ZP_INDEXED_X_ACCESS];
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0x01: //ORA addr : A<- A | [[addr+X]]
				emu->Acc = emu->Acc | emu->Memory[PRE_INDEXED_X_INDIRECT_ACCESS];
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0x11: //ORA addr : A<- A | [[addr+1,addr] +Y]
				emu->Acc = emu->Acc | emu->Memory[POST_INDEXED_Y_INDIRECT_ACCESS];
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0x0D: //ORA addr : A<- A | [addr16]
				emu->Acc = emu->Acc | emu->Memory[EXTENDED_DIRECT_ACCESS];
				emu->PC+=3;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0x19: //ORA addr : A<- A | [addr16+Y]
				emu->Acc = emu->Acc | emu->Memory[ABSOLUTE_INDEXED_Y_ACCESS];
				emu->PC+=3;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			case 0x1D: //ORA addr : A<- A | [addr16+X]
				emu->Acc = emu->Acc | emu->Memory[ABSOLUTE_INDEXED_X_ACCESS];
				emu->PC+=3;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;


			//***********************>>>SBC INSTRUCTIONS<<<*************************
			case 0xE9: //SBC addr : A<- A - IMM - C'
			{
				ch1 = emu->Acc;
				ch2 = IMMIDIATE_ACCESS + ((unsigned char)1 - (unsigned char)(CARRY_GET(emu->P)));

				emu->Acc = (ch1 - ch2);
				emu->PC+=2;
				//affects s,z,v,c flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				TEST_AND_SET_CARRY_SUBTRACTION(emu->P, ch1, ch2 ) ;
				TEST_AND_SET_V_OVERFLOW_SUBTRACTION(emu->P, ch1, ch2) ;
				break;
			}

			case 0xE5: //SBC addr : A<- A - [addr] - C'
			{
				ch1 = emu->Acc;
				ch2 = emu->Memory[ZP_DIRECT_ACCESS] + ((unsigned char)1 - (unsigned char)(CARRY_GET(emu->P)));

				emu->Acc = (ch1 - ch2);
				emu->PC+=2;
				//affects s,z,v,c flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				TEST_AND_SET_CARRY_SUBTRACTION(emu->P, ch1, ch2 ) ;
				TEST_AND_SET_V_OVERFLOW_SUBTRACTION(emu->P, ch1, ch2) ;
				break;
			}

			case 0xF5: //SBC addr : A<- A - [addr+X] - C'
			{
				ch1 = emu->Acc;
				ch2 = emu->Memory[ZP_INDEXED_X_ACCESS] + ((unsigned char)1 - (unsigned char)(CARRY_GET(emu->P)));

				emu->Acc = (ch1 - ch2);
				emu->PC+=2;
				//affects s,z,v,c flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				TEST_AND_SET_CARRY_SUBTRACTION(emu->P, ch1, ch2 ) ;
				TEST_AND_SET_V_OVERFLOW_SUBTRACTION(emu->P, ch1, ch2) ;
				break;
			}

		    case 0xE1: //SBC addr : A<- A - [[addr+X]] - C'
			{
				ch1 = emu->Acc;
				ch2 = emu->Memory[PRE_INDEXED_X_INDIRECT_ACCESS] + ((unsigned char)1 - (unsigned char)(CARRY_GET(emu->P)));

				emu->Acc = (ch1 - ch2);
				emu->PC+=2;
				//affects s,z,v,c flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				TEST_AND_SET_CARRY_SUBTRACTION(emu->P, ch1, ch2 ) ;
				TEST_AND_SET_V_OVERFLOW_SUBTRACTION(emu->P, ch1, ch2) ;
				break;
			}

			case 0xF1: //SBC addr : A<- A - [[addr+1, addr]+ Y] - C', post-indexed indirect addressing mode
			{
				ch1 = emu->Acc;
				ch2 = emu->Memory[POST_INDEXED_Y_INDIRECT_ACCESS] + ((unsigned char)1 - (unsigned char)(CARRY_GET(emu->P)));

				emu->Acc = (ch1 - ch2);
				emu->PC+=2;
				//affects s,z,v,c flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				TEST_AND_SET_CARRY_SUBTRACTION(emu->P, ch1, ch2 ) ;
				TEST_AND_SET_V_OVERFLOW_SUBTRACTION(emu->P, ch1, ch2) ;
				break;
			}

			case 0xED: //SBC addr : A<- A - [addr16] - C', extended direct addressing mode
			{
				ch1 = emu->Acc;
				ch2 = emu->Memory[EXTENDED_DIRECT_ACCESS] + ((unsigned char)1 - (unsigned char)(CARRY_GET(emu->P)));

				emu->Acc = (ch1 - ch2);
				emu->PC+=3;
				//affects s,z,v,c flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				TEST_AND_SET_CARRY_SUBTRACTION(emu->P, ch1, ch2 ) ;
				TEST_AND_SET_V_OVERFLOW_SUBTRACTION(emu->P, ch1, ch2) ;
				break;
			}

			case 0xF9: //SBC addr: A<- A - [addr16+Y] - C', absolute indexed addressing mode
			{
				ch1 = emu->Acc;
				ch2 = emu->Memory[ABSOLUTE_INDEXED_Y_ACCESS] + ((unsigned char)1 - (unsigned char)(CARRY_GET(emu->P)));

				emu->Acc = (ch1 - ch2);
				emu->PC+=3;
				//affects s,z,v,c flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				TEST_AND_SET_CARRY_SUBTRACTION(emu->P, ch1, ch2 ) ;
				TEST_AND_SET_V_OVERFLOW_SUBTRACTION(emu->P, ch1, ch2) ;
				break;
			}

			case 0xFD: //SBC addr: A<- A - [addr16+X] - C', absolute indexed addressing mode
			{
				ch1 = emu->Acc;
				ch2 = emu->Memory[ABSOLUTE_INDEXED_X_ACCESS] + ((unsigned char)1 - (unsigned char)(CARRY_GET(emu->P)));

				emu->Acc = (ch1 - ch2);
				emu->PC+=3;
				//affects s,z,v,c flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				TEST_AND_SET_CARRY_SUBTRACTION(emu->P, ch1, ch2 ) ;
				TEST_AND_SET_V_OVERFLOW_SUBTRACTION(emu->P, ch1, ch2) ;
				break;
			}


			//***********************>>>INC INSTRUCTIONS<<<*************************
			case 0xE6: //INC addr : [addr]<- [addr]+1
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Memory[ZP_DIRECT_ACCESS] + 1;

				emu->Memory[ZP_DIRECT_ACCESS] = ch1;

				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				TEST_AND_SET_NEG(emu->P, ch1) ;
				break;

			case 0xF6: //INC addr : [addr+X]<- [addr+X]+1
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Memory[ZP_INDEXED_X_ACCESS] + 1;

				emu->Memory[ZP_INDEXED_X_ACCESS] = ch1;
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				TEST_AND_SET_NEG(emu->P, ch1) ;
				break;

			case 0xEE: //INC addr : [addr16]<- [addr16]+1
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Memory[EXTENDED_DIRECT_ACCESS] + 1;

				emu->Memory[EXTENDED_DIRECT_ACCESS] = ch1;
				emu->PC+=3;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				TEST_AND_SET_NEG(emu->P, ch1) ;
				break;

			case 0xFE: //INC addr : [addr16+X]<- [addr16+X]+1
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Memory[ABSOLUTE_INDEXED_X_ACCESS] + 1;

				emu->Memory[ABSOLUTE_INDEXED_X_ACCESS] = ch1;
				emu->PC+=3;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				TEST_AND_SET_NEG(emu->P, ch1) ;
				break;


			//***********************>>>DEC INSTRUCTIONS<<<*************************
			case 0xC6: //DEC addr : [addr]<- [addr]-1
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Memory[ZP_DIRECT_ACCESS] - 1;

				emu->Memory[ZP_DIRECT_ACCESS] = ch1;
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				TEST_AND_SET_NEG(emu->P, ch1) ;
				break;

			case 0xD6: //DEC addr : [addr+X]<- [addr+X]-1
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Memory[ZP_INDEXED_X_ACCESS] - 1;

				emu->Memory[ZP_INDEXED_X_ACCESS] = ch1;
				emu->PC+=2;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				TEST_AND_SET_NEG(emu->P, ch1) ;
				break;

			case 0xCE: //DEC addr : [addr16]<- [addr16]+1
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Memory[EXTENDED_DIRECT_ACCESS] - 1;

				emu->Memory[EXTENDED_DIRECT_ACCESS] = ch1;
				emu->PC+=3;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				TEST_AND_SET_NEG(emu->P, ch1) ;
				break;

			case 0xDE: //DEC addr : [addr16+X]<- [addr16+X]-1
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Memory[ABSOLUTE_INDEXED_X_ACCESS] - 1;

				emu->Memory[ABSOLUTE_INDEXED_X_ACCESS] = ch1;
				emu->PC+=3;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				TEST_AND_SET_NEG(emu->P, ch1) ;
				break;


			//***********************>>>CPX INSTRUCTIONS<<<*************************
			case 0xE0: //CPX addr : X-IMM, sets s,z,c flags
				ch1 = emu->X;
				ch2 = IMMIDIATE_ACCESS ;
				res = ch1 - ch2;

				emu->PC+=2;

				//affects s,z,c flags
				TEST_AND_SET_ZERO(emu->P, res) ;
				TEST_AND_SET_NEG(emu->P, res) ;
				TEST_AND_SET_CARRY_SUBTRACTION(emu->P, ch1, ch2 ) ;
				break;

			case 0xE4: //CPX addr : X-[addr], sets s,z,c flags
				ch1 = emu->X;
				ch2 = emu->Memory[ZP_DIRECT_ACCESS];
				res = ch1 - ch2;

				emu->PC+=2;

				//affects s,z,c flags
				TEST_AND_SET_ZERO(emu->P, res) ;
				TEST_AND_SET_NEG(emu->P, res) ;
				TEST_AND_SET_CARRY_SUBTRACTION(emu->P, ch1, ch2 ) ;
				break;

			case 0xEC: //CPX addr : X-[addr16], sets s,z,c flags
				ch1 = emu->X;
				ch2 = emu->Memory[EXTENDED_DIRECT_ACCESS];
				res = ch1 - ch2;

				emu->PC+=3;

				//affects s,z,c flags
				TEST_AND_SET_ZERO(emu->P, res) ;
				TEST_AND_SET_NEG(emu->P, res) ;
				TEST_AND_SET_CARRY_SUBTRACTION(emu->P, ch1, ch2 ) ;
				break;


			//***********************>>>CPY INSTRUCTIONS<<<*************************
			case 0xC0: //CPY addr : Y-IMM, sets s,z,c flags
				ch1 = emu->Y;
				ch2 = IMMIDIATE_ACCESS ;
				res = ch1 - ch2;

				emu->PC+=2;

				//affects s,z,c flags
				TEST_AND_SET_ZERO(emu->P, res) ;
				TEST_AND_SET_NEG(emu->P, res) ;
				TEST_AND_SET_CARRY_SUBTRACTION(emu->P, ch1, ch2 ) ;
				break;

			case 0xC4: //CPY addr : Y-[addr], sets s,z,c flags
				ch1 = emu->Y;
				ch2 = emu->Memory[ZP_DIRECT_ACCESS];
				res = ch1 - ch2;

				emu->PC+=2;

				//affects s,z,c flags
				TEST_AND_SET_ZERO(emu->P, res) ;
				TEST_AND_SET_NEG(emu->P, res) ;
				TEST_AND_SET_CARRY_SUBTRACTION(emu->P, ch1, ch2 ) ;
				break;

			case 0xCC: //CPY addr : Y-[addr16], sets s,z,c flags
				ch1 = emu->Y;
				ch2 = emu->Memory[EXTENDED_DIRECT_ACCESS];
				res = ch1 - ch2;

				emu->PC+=3;

				//affects s,z,c flags
				TEST_AND_SET_ZERO(emu->P, res) ;
				TEST_AND_SET_NEG(emu->P, res) ;
				TEST_AND_SET_CARRY_SUBTRACTION(emu->P, ch1, ch2 ) ;
				break;


			//***********************>>>ROL INSTRUCTIONS<<<*************************
			case 0x2A: //ROL addr : addr, sets s,z flags, rotated through c flag
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Acc;
				ch2 = (((int)(NEG_GET(emu->P))) == 0x00)?0:1;
				res = (((int)(CARRY_GET(emu->P))) == 0x00)?0:1;

				ch1 = ch1 << 1;

				//rotate previous value of high-bit into carry flag
				if (((int)(ch2)) == 0x00)CARRY_CLEAR(emu->P);
				else CARRY_SET(emu->P);

				//rotate previous value of carry flag into low-bit
				if (((int)(res)) == 0x00)CARRY_CLEAR(ch1);
				else CARRY_SET(ch1);

				//now move back to accumulator
				emu->Acc = ch1;

				emu->PC+=1;

				//affects s,z flags, c flag already set
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				TEST_AND_SET_NEG(emu->P, ch1) ;
				break;


			case 0x26: //ROL addr : [addr], sets s,z flags, rotated through c flag
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Memory[ZP_DIRECT_ACCESS];
				ch2 = (((int)(NEG_GET(emu->P))) == 0x00)?0:1;
				res = (((int)(CARRY_GET(emu->P))) == 0x00)?0:1;

				ch1 = ch1 << 1;

				//rotate previous value of high-bit into carry flag
				if (((int)(ch2)) == 0x00)CARRY_CLEAR(emu->P);
				else CARRY_SET(emu->P);

				//rotate previous value of carry flag into low-bit
				if (((int)(res)) == 0x00)CARRY_CLEAR(ch1);
				else CARRY_SET(ch1);

				//now move back to memory
				emu->Memory[ZP_DIRECT_ACCESS] = ch1;

				emu->PC+=2;

				//affects s,z flags, c flag already set
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				TEST_AND_SET_NEG(emu->P, ch1) ;
				break;

			case 0x36: //ROL addr : [addr+X], sets s,z flags, rotated through c flag
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Memory[ZP_INDEXED_X_ACCESS];
				ch2 = (((int)(NEG_GET(emu->P))) == 0x00)?0:1;
				res = (((int)(CARRY_GET(emu->P))) == 0x00)?0:1;

				ch1 = ch1 << 1;

				//rotate previous value of high-bit into carry flag
				if (((int)(ch2)) == 0x00)CARRY_CLEAR(emu->P);
				else CARRY_SET(emu->P);

				//rotate previous value of carry flag into low-bit
				if (((int)(res)) == 0x00)CARRY_CLEAR(ch1);
				else CARRY_SET(ch1);

				//now move back to memory
				emu->Memory[ZP_INDEXED_X_ACCESS] = ch1;

				emu->PC+=2;

				//affects s,z flags, c flag already set
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				TEST_AND_SET_NEG(emu->P, ch1) ;
				break;

			case 0x2E: //ROL addr : [addr16], sets s,z flags, rotated through c flag
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Memory[EXTENDED_DIRECT_ACCESS];
				ch2 = (((int)(NEG_GET(emu->P))) == 0x00)?0:1;
				res = (((int)(CARRY_GET(emu->P))) == 0x00)?0:1;

				ch1 = ch1 << 1;

				//rotate previous value of high-bit into carry flag
				if (((int)(ch2)) == 0x00)CARRY_CLEAR(emu->P);
				else CARRY_SET(emu->P);

				//rotate previous value of carry flag into low-bit
				if (((int)(res)) == 0x00)CARRY_CLEAR(ch1);
				else CARRY_SET(ch1);

				//now move back to memory
				emu->Memory[EXTENDED_DIRECT_ACCESS] = ch1;

				emu->PC+=3;

				//affects s,z flags, c flag already set
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				TEST_AND_SET_NEG(emu->P, ch1) ;
				break;

			case 0x3E: //ROL addr : [addr16+X], sets s,z flags, rotated through c flag
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Memory[ABSOLUTE_INDEXED_X_ACCESS];
				ch2 = (((int)(NEG_GET(emu->P))) == 0x00)?0:1;
				res = (((int)(CARRY_GET(emu->P))) == 0x00)?0:1;

				ch1 = ch1 << 1;

				//rotate previous value of high-bit into carry flag
				if (((int)(ch2)) == 0x00)CARRY_CLEAR(emu->P);
				else CARRY_SET(emu->P);

				//rotate previous value of carry flag into low-bit
				if (((int)(res)) == 0x00)CARRY_CLEAR(ch1);
				else CARRY_SET(ch1);

				//now move back to memory
				emu->Memory[ABSOLUTE_INDEXED_X_ACCESS] = ch1;

				emu->PC+=3;

				//affects s,z flags, c flag already set
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				TEST_AND_SET_NEG(emu->P, ch1) ;
				break;



			//***********************>>>ROR INSTRUCTIONS<<<*************************
			case 0x6A: //ROR addr : addr, sets s,z flags, rotated through c flag
				//STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Acc;
				ch2 = (((int)(CARRY_GET(ch1))) == 0x00)?0:1;
				res = (((int)(CARRY_GET(emu->P))) == 0x00)?0:1;

				ch1 = ch1 >> 1;

				//rotate previous value of low-bit into carry flag
				if (((int)(ch2)) == 0x00)CARRY_CLEAR(emu->P);
				else CARRY_SET(emu->P);

				//rotate previous value of carry flag into high-bit
				if (((int)(res)) == 0x00)NEG_CLEAR(ch1);
				else NEG_SET(ch1);

				//now move back to memory
				emu->Acc = ch1;

				emu->PC+=1;

				//affects s,z flags, c flag already set
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				TEST_AND_SET_NEG(emu->P, ch1) ;
				break;

			case 0x66: //ROR addr : [addr], sets s,z flags, rotated through c flag
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Memory[ZP_DIRECT_ACCESS];
				ch2 = (((int)(CARRY_GET(ch1))) == 0x00)?0:1;
				res = (((int)(CARRY_GET(emu->P))) == 0x00)?0:1;

				ch1 = ch1 >> 1;

				//rotate previous value of low-bit into carry flag
				if (((int)(ch2)) == 0x00)CARRY_CLEAR(emu->P);
				else CARRY_SET(emu->P);

				//rotate previous value of carry flag into high-bit
				if (((int)(res)) == 0x00)NEG_CLEAR(ch1);
				else NEG_SET(ch1);

				//now move back to memory
				emu->Memory[ZP_DIRECT_ACCESS] = ch1;

				emu->PC+=2;

				//affects s,z flags, c flag already set
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				TEST_AND_SET_NEG(emu->P, ch1) ;
				break;

			case 0x76: //ROR addr : [addr+X], sets s,z flags, rotated through c flag
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Memory[ZP_INDEXED_X_ACCESS];
				ch2 = (((int)(CARRY_GET(ch1))) == 0x00)?0:1;
				res = (((int)(CARRY_GET(emu->P))) == 0x00)?0:1;

				ch1 = ch1 >> 1;

				//rotate previous value of low-bit into carry flag
				if (((int)(ch2)) == 0x00)CARRY_CLEAR(emu->P);
				else CARRY_SET(emu->P);

				//rotate previous value of carry flag into high-bit
				if (((int)(res)) == 0x00)NEG_CLEAR(ch1);
				else NEG_SET(ch1);

				//now move back to memory
				emu->Memory[ZP_INDEXED_X_ACCESS] = ch1;

				emu->PC+=2;

				//affects s,z flags, c flag already set
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				TEST_AND_SET_NEG(emu->P, ch1) ;
				break;


			case 0x6E: //ROR addr : [addr16], sets s,z flags, rotated through c flag
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Memory[EXTENDED_DIRECT_ACCESS];
				ch2 = (((int)(CARRY_GET(ch1))) == 0x00)?0:1;
				res = (((int)(CARRY_GET(emu->P))) == 0x00)?0:1;

				ch1 = ch1 >> 1;

				//rotate previous value of low-bit into carry flag
				if (((int)(ch2)) == 0x00)CARRY_CLEAR(emu->P);
				else CARRY_SET(emu->P);

				//rotate previous value of carry flag into high-bit
				if (((int)(res)) == 0x00)NEG_CLEAR(ch1);
				else NEG_SET(ch1);

				//now move back to memory
				emu->Memory[EXTENDED_DIRECT_ACCESS] = ch1;

				emu->PC+=3;

				//affects s,z flags, c flag already set
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				TEST_AND_SET_NEG(emu->P, ch1) ;
				break;


			case 0x7E: //ROR addr : [addr16+X], sets s,z flags, rotated through c flag
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Memory[ABSOLUTE_INDEXED_X_ACCESS];
				ch2 = (((int)(CARRY_GET(ch1))) == 0x00)?0:1;
				res = (((int)(CARRY_GET(emu->P))) == 0x00)?0:1;

				ch1 = ch1 >> 1;

				//rotate previous value of low-bit into carry flag
				if (((int)(ch2)) == 0x00)CARRY_CLEAR(emu->P);
				else CARRY_SET(emu->P);

				//rotate previous value of carry flag into high-bit
				if (((int)(res)) == 0x00)NEG_CLEAR(ch1);
				else NEG_SET(ch1);

				//now move back to memory
				emu->Memory[ABSOLUTE_INDEXED_X_ACCESS] = ch1;

				emu->PC+=3;

				//affects s,z flags, c flag already set
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				TEST_AND_SET_NEG(emu->P, ch1) ;
				break;


			//***********************>>>ASL INSTRUCTIONS<<<*************************
			case 0x0A: //ASL addr : addr, sets n,z flags, shifts to c flag
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Acc;
				ch2 = (((int)(NEG_GET(ch1))) == 0x00)?0:1;
				//res = (((int)(CARRY_GET(emu->P))) == 0x00)?0:1;

				ch1 = ch1 << 1; //shift bits and clear 0th bit
				CARRY_CLEAR(ch1);

				//rotate previous value of high-bit into carry flag
				if (((int)(ch2)) == 0x00)CARRY_CLEAR(emu->P);
				else CARRY_SET(emu->P);

				//now move back to memory
				emu->Acc = ch1;

				emu->PC+=1;

				//affects n,z flags, c flag already set
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				TEST_AND_SET_NEG(emu->P, ch1) ;
				break;

			case 0x06: //ASL addr : [addr], sets n,z flags, shifts to c flag
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Memory[ZP_DIRECT_ACCESS];
				ch2 = (((int)(NEG_GET(ch1))) == 0x00)?0:1;
				//res = (((int)(CARRY_GET(emu->P))) == 0x00)?0:1;

				ch1 = ch1 << 1; //shift bits and clear 0th bit
				CARRY_CLEAR(ch1);

				//rotate previous value of high-bit into carry flag
				if (((int)(ch2)) == 0x00)CARRY_CLEAR(emu->P);
				else CARRY_SET(emu->P);

				//now move back to memory
				emu->Memory[ZP_DIRECT_ACCESS] = ch1;

				emu->PC+=2;

				//affects n,z flags, c flag already set
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				TEST_AND_SET_NEG(emu->P, ch1) ;
				break;

			case 0x16: //ASL addr : [addr+x], sets n,z flags, shifts to c flag
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Memory[ZP_INDEXED_X_ACCESS];
				ch2 = (((int)(NEG_GET(ch1))) == 0x00)?0:1;
				//res = (((int)(CARRY_GET(emu->P))) == 0x00)?0:1;

				ch1 = ch1 << 1; //shift bits and clear 0th bit
				CARRY_CLEAR(ch1);

				//rotate previous value of high-bit into carry flag
				if (((int)(ch2)) == 0x00)CARRY_CLEAR(emu->P);
				else CARRY_SET(emu->P);

				//now move back to memory
				emu->Memory[ZP_INDEXED_X_ACCESS] = ch1;

				emu->PC+=2;

				//affects n,z flags, c flag already set
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				TEST_AND_SET_NEG(emu->P, ch1) ;
				break;

			case 0x0E: //ASL addr : [addr16], sets n,z flags, shifts to c flag
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Memory[EXTENDED_DIRECT_ACCESS];
				ch2 = (((int)(NEG_GET(ch1))) == 0x00)?0:1;
				//res = (((int)(CARRY_GET(emu->P))) == 0x00)?0:1;

				ch1 = ch1 << 1; //shift bits and clear 0th bit
				CARRY_CLEAR(ch1);

				//rotate previous value of high-bit into carry flag
				if (((int)(ch2)) == 0x00)CARRY_CLEAR(emu->P);
				else CARRY_SET(emu->P);

				//now move back to memory
				emu->Memory[EXTENDED_DIRECT_ACCESS] = ch1;

				emu->PC+=3;

				//affects n,z flags, c flag already set
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				TEST_AND_SET_NEG(emu->P, ch1) ;
				break;

			case 0x1E: //ASL addr : [addr16+X], sets n,z flags, shifts to c flag
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Memory[ABSOLUTE_INDEXED_X_ACCESS];
				ch2 = (((int)(NEG_GET(ch1))) == 0x00)?0:1;
				//res = (((int)(CARRY_GET(emu->P))) == 0x00)?0:1;

				ch1 = ch1 << 1; //shift bits and clear 0th bit
				CARRY_CLEAR(ch1);

				//rotate previous value of high-bit into carry flag
				if (((int)(ch2)) == 0x00)CARRY_CLEAR(emu->P);
				else CARRY_SET(emu->P);

				//now move back to memory
				emu->Memory[ABSOLUTE_INDEXED_X_ACCESS] = ch1;

				emu->PC+=3;

				//affects n,z flags, c flag already set
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				TEST_AND_SET_NEG(emu->P, ch1) ;
				break;


			//***********************>>>LSR INSTRUCTIONS<<<*************************
			case 0x4A: //LSR addr : addr, sets z flag, clears n flag, shifts to c flag
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Acc;
				ch2 = (((int)(CARRY_GET(ch1))) == 0x00)?0:1;
				//res = (((int)(CARRY_GET(emu->P))) == 0x00)?0:1;

				ch1 = ch1 >> 1; //shift bits and clear high bit
				NEG_CLEAR(ch1);

				//rotate previous value of low-bit into carry flag
				if (((int)(ch2)) == 0x00)CARRY_CLEAR(emu->P);
				else CARRY_SET(emu->P);

				//now move back to memory
				emu->Acc = ch1;

				emu->PC+=1;

				//affects z,n flags, c flag already set
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				NEG_CLEAR(emu->P);
				break;

			case 0x46: //LSR addr : [addr], sets z flag, clears n flag, shifts to c flag
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Memory[ZP_DIRECT_ACCESS];
				ch2 = (((int)(CARRY_GET(ch1))) == 0x00)?0:1;
				//res = (((int)(CARRY_GET(emu->P))) == 0x00)?0:1;

				ch1 = ch1 >> 1; //shift bits and clear high bit
				NEG_CLEAR(ch1);

				//rotate previous value of low-bit into carry flag
				if (((int)(ch2)) == 0x00)CARRY_CLEAR(emu->P);
				else CARRY_SET(emu->P);

				//now move back to memory
				emu->Memory[ZP_DIRECT_ACCESS] = ch1;

				emu->PC+=2;

				//affects z,n flags, c flag already set
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				NEG_CLEAR(emu->P);
				break;

			case 0x56: //LSR addr : [addr], sets z flag, clears n flag, shifts to c flag
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Memory[ZP_INDEXED_X_ACCESS];
				ch2 = (((int)(CARRY_GET(ch1))) == 0x00)?0:1;
				//res = (((int)(CARRY_GET(emu->P))) == 0x00)?0:1;

				ch1 = ch1 >> 1; //shift bits and clear high bit
				NEG_CLEAR(ch1);

				//rotate previous value of low-bit into carry flag
				if (((int)(ch2)) == 0x00)CARRY_CLEAR(emu->P);
				else CARRY_SET(emu->P);

				//now move back to memory
				emu->Memory[ZP_INDEXED_X_ACCESS] = ch1;

				emu->PC+=2;

				//affects z,n flags, c flag already set
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				NEG_CLEAR(emu->P);
				break;

			case 0x4E: //LSR addr : [addr16], sets z flag, clears n flag, shifts to c flag
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Memory[EXTENDED_DIRECT_ACCESS];
				ch2 = (((int)(CARRY_GET(ch1))) == 0x00)?0:1;
				//res = (((int)(CARRY_GET(emu->P))) == 0x00)?0:1;

				ch1 = ch1 >> 1; //shift bits and clear high bit
				NEG_CLEAR(ch1);

				//rotate previous value of low-bit into carry flag
				if (((int)(ch2)) == 0x00)CARRY_CLEAR(emu->P);
				else CARRY_SET(emu->P);

				//now move back to memory
				emu->Memory[EXTENDED_DIRECT_ACCESS] = ch1;

				emu->PC+=3;

				//affects z,n flags, c flag already set
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				NEG_CLEAR(emu->P);
				break;

			case 0x5E: //LSR addr : [addr16+X], sets z flag, clears n flag, shifts to c flag
				STUB_OUT_MEM_ACCESS_IFACES ;

				ch1 = emu->Memory[ABSOLUTE_INDEXED_X_ACCESS];
				ch2 = (((int)(CARRY_GET(ch1))) == 0x00)?0:1;
				//res = (((int)(CARRY_GET(emu->P))) == 0x00)?0:1;

				ch1 = ch1 >> 1; //shift bits and clear high bit
				NEG_CLEAR(ch1);

				//rotate previous value of low-bit into carry flag
				if (((int)(ch2)) == 0x00)CARRY_CLEAR(emu->P);
				else CARRY_SET(emu->P);

				//now move back to memory
				emu->Memory[ABSOLUTE_INDEXED_X_ACCESS] = ch1;

				emu->PC+=3;

				//affects z,n flags, c flag already set
				TEST_AND_SET_ZERO(emu->P, ch1) ;
				NEG_CLEAR(emu->P);
				break;


			//***********************>>>JMP INSTRUCTIONS<<<*************************
			case 0x4C: //JMP addr : PC<- addr16, no flags affected
				emu->PC = EXTENDED_DIRECT_ACCESS;

				//no flags affected
				break;

			case 0x6C: //JMP addr : PC<- [addr16], no flags affected
				emu->PC = ABSOLUTE_INDIRECT_JMP_ACCESS;

				//no flags affected
				break;


			//***********************>>>B** INSTRUCTIONS<<<*************************
			case 0x90: //BCC addr : C=0-> PC+= addr+2, else PC+=2, no flags affectd
				if ( (int)(CARRY_GET(emu->P)) == 0x00 )
				{
					emu->PC+= (signed char)(IMMIDIATE_ACCESS) ;
				}
				emu->PC +=2; //still have to add 2 to account for length of opcode executed

				//no flags affected
				break;

			case 0xB0: //BCS addr : C=1-> PC+= addr+2, else PC+=2, no flags affectd
				if ( (int)(CARRY_GET(emu->P)) != 0x00 )
				{
					emu->PC+= (signed char)(IMMIDIATE_ACCESS) ;
				}
				emu->PC +=2; //still have to add 2 to account for length of opcode executed

				//no flags affected
				break;

			case 0xF0: //BEQ addr : Z=1-> PC+= addr+2, else PC+=2, no flags affectd
				if ( (int)(ZERO_GET(emu->P)) != 0x00 )
				{
					emu->PC+= (signed char)(IMMIDIATE_ACCESS) ;
				}
				emu->PC +=2; //still have to add 2 to account for length of opcode executed

				//no flags affected
				break;

			case 0x30: //BMI addr : N=1-> PC+= addr+2, else PC+=2, no flags affectd
				if ( (int)(NEG_GET(emu->P)) != 0x00 )
				{
					emu->PC+= (signed char)(IMMIDIATE_ACCESS) ;
				}
				emu->PC +=2; //still have to add 2 to account for length of opcode executed

				//no flags affected
				break;

			case 0xD0: //BNE addr : Z=0-> PC+= addr+2, else PC+=2, no flags affectd
				if ( (int)(ZERO_GET(emu->P)) == 0x00 )
				{
					emu->PC+= (signed char)(IMMIDIATE_ACCESS) ;
				}
				emu->PC +=2; //still have to add 2 to account for length of opcode executed

				//no flags affected
				break;

			case 0x10: //BPL addr : N=0-> PC+= addr+2, else PC+=2, no flags affectd
				if ( (int)(NEG_GET(emu->P)) == 0x00 )
				{
					emu->PC+= (signed char)(IMMIDIATE_ACCESS) ;
				}
				emu->PC +=2; //still have to add 2 to account for length of opcode executed

				//no flags affected
				break;

			case 0x50: //BVC addr : V=0-> PC+= addr+2, else PC+=2, no flags affectd
				if ( (int)(OVERFLOW_GET(emu->P)) == 0x00 )
				{
					emu->PC+= (signed char)(IMMIDIATE_ACCESS) ;
				}
				emu->PC +=2; //still have to add 2 to account for length of opcode executed

				//no flags affected
				break;

			case 0x70: //BVS addr : V=1-> PC+= addr+2, else PC+=2, no flags affectd
				if ( (int)(OVERFLOW_GET(emu->P)) != 0x00 )
				{
					emu->PC+= (signed char)(IMMIDIATE_ACCESS) ;
				}
				emu->PC +=2; //still have to add 2 to account for length of opcode executed

				//no flags affected
				break;


			//***********************>>>T** INSTRUCTIONS<<<*************************
			case 0xAA: //TAX : X<- A, s,z flags affected
				emu->X = emu->Acc;

				emu->PC+=1;

				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->X);
				TEST_AND_SET_NEG(emu->P, emu->X);
				break;

			case 0x8A: //TXA : A<- X, s,z flags affected
				emu->Acc = emu->X;

				emu->PC+=1;

				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc);
				TEST_AND_SET_NEG(emu->P, emu->Acc);
				break;

			case 0xA8: //TAY : Y<- A, s,z flags affected
				emu->Y = emu->Acc;

				emu->PC+=1;

				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Y);
				TEST_AND_SET_NEG(emu->P, emu->Y);
				break;

			case 0x98: //TYA : A<- Y, s,z flags affected
				emu->Acc = emu->Y;

				emu->PC+=1;

				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc);
				TEST_AND_SET_NEG(emu->P, emu->Acc);
				break;

			case 0xBA: //TSX : X<- S, s,z flags affected
				emu->X = emu->S;

				emu->PC+=1;

				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->X);
				TEST_AND_SET_NEG(emu->P, emu->X);
				break;

			case 0x9A: //TXS : S<- X, no flags affected
				emu->S = emu->X;

				emu->PC+=1;

				//no flags affected
				break;


			//***********************>>>DEX INSTRUCTIONS<<<*************************
			case 0xCA: //DEX : X<- X - 1
				emu->X = emu->X - (unsigned char)1;

				emu->PC+=1;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->X) ;
				TEST_AND_SET_NEG(emu->P, emu->X) ;
				break;


			//***********************>>>DEY INSTRUCTIONS<<<*************************
			case 0x88: //DEY : Y<- Y - 1
				emu->Y = emu->Y - (unsigned char)1;

				emu->PC+=1;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Y) ;
				TEST_AND_SET_NEG(emu->P, emu->Y) ;
				break;

			//***********************>>>INX INSTRUCTIONS<<<*************************
			case 0xE8: //INX : X<- X + 1
				emu->X = emu->X + (unsigned char)1;

				emu->PC+=1;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->X) ;
				TEST_AND_SET_NEG(emu->P, emu->X) ;
				break;


			//***********************>>>INY INSTRUCTIONS<<<*************************
			case 0xC8: //INY : Y<- Y + 1
				emu->Y = emu->Y + (unsigned char)1;

				emu->PC+=1;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Y) ;
				TEST_AND_SET_NEG(emu->P, emu->Y) ;
				break;


			//***********************>>>PHA INSTRUCTIONS<<<*************************
			case 0x48:  //PHA : [stack]<- Acc, stack<- stack - 1
				STUB_OUT_MEM_ACCESS_IFACES ;

				emu->Memory[ generate_addr(emu->S, STACK_HIGH_ADDR) ] = emu->Acc;
				emu->S-=1; //remember that stack grows down

				emu->PC+=1;
				//affects no flags
				break;

			//***********************>>>PLA INSTRUCTIONS<<<*************************
			case 0x68:  //PLA : Acc<- [stack], stack<- stack - 1
				STUB_OUT_MEM_ACCESS_IFACES ;

				//first increment stack
				emu->S+=1; //remember that stack grows down

				emu->Acc =emu->Memory[ generate_addr(emu->S, STACK_HIGH_ADDR) ];

				emu->PC+=1;
				//affects s,z flags
				TEST_AND_SET_ZERO(emu->P, emu->Acc) ;
				TEST_AND_SET_NEG(emu->P, emu->Acc) ;
				break;

			//***********************>>>PHP INSTRUCTIONS<<<*************************
			case 0x08:  //PHP : [stack]<- P, stack<- stack - 1
				STUB_OUT_MEM_ACCESS_IFACES ;

				emu->Memory[ generate_addr(emu->S, STACK_HIGH_ADDR) ] = emu->P;
				emu->S-=1; //remember that stack grows down

				emu->PC+=1;
				//affects no flags
				break;

			//***********************>>>PLP INSTRUCTIONS<<<*************************
			case 0x28:  //PLP : P<- [stack], stack<- stack - 1
				STUB_OUT_MEM_ACCESS_IFACES ;

				//first increment stack
				emu->S+=1; //remember that stack grows down

				emu->P =emu->Memory[ generate_addr(emu->S, STACK_HIGH_ADDR) ];

				emu->PC+=1;
				//affects all flags, they all get replaced with new values
				//no need to set/test them right now
				break;

			//***********************>>>CLI INSTRUCTIONS<<<*************************
			case 0x58: //CLI : clear interrupts, dont think it does anything...yet
				STUB_OUT_INTERRUPTS_IFACES ;

				//affects IRQ/interrupts flag
				IRQ_DISABLE_CLEAR(emu->P);
				emu->PC+=1;
				break;

			//***********************>>>SEI INSTRUCTIONS<<<*************************
			case 0x78: //SEI : sets interrupts, dont think it does anything...yet
				STUB_OUT_INTERRUPTS_IFACES ;

				//affects IRQ/interrupts flag
				IRQ_DISABLE_SET(emu->P);
				emu->PC+=1;
				break;

			//***********************>>>NOP INSTRUCTIONS<<<*************************
			case 0xEA: //NOP : increments the PC by one, does nothing else
				emu->PC+=1;
				break;

			//***********************>>>BRK INSTRUCTIONS<<<*************************
			case 0x00: //BRK : programmed interrupt
				STUB_OUT_INTERRUPTS_IFACES ;

				//increment PC by 2, push onto stack
				//observe that the 2nd byte of the instr is the interrupt signature
				emu->Memory[ generate_addr(emu->S, STACK_HIGH_ADDR) ] = ((emu->PC)+2) >> 8;
				emu->S-=1;

				emu->Memory[ generate_addr(emu->S, STACK_HIGH_ADDR) ] = ((emu->PC)+2);
				emu->S-=1;

				//set the break flag and push onto stack
				BRK_SET(emu->P);

				emu->Memory[ generate_addr(emu->S, STACK_HIGH_ADDR) ] = emu->P;
				emu->S-=1;

				//disable interrupts
				IRQ_DISABLE_SET(emu->P);

				//set PC to point to ISR location
				//for historic reasons, the isr is located at the addr [0xFFFF,0xFFFE]
				/*
				printf("generate_addr_1 %d\n",generate_addr(ISR_HIGH_ADDR, ISR_HIGH_ADDR) );
				printf("generate_addr_2 %d\n",generate_addr(ISR_LOW_ADDR, ISR_HIGH_ADDR) );
				printf("generate_addr_3 %d\n",
					generate_addr(
						emu->Memory[ generate_addr( ISR_LOW_ADDR, ISR_HIGH_ADDR ) ],    //low bit
						emu->Memory[ generate_addr( ISR_HIGH_ADDR, ISR_HIGH_ADDR ) ]  //high bit
					)
				);
				*/

				emu->PC = generate_addr(
					emu->Memory[ generate_addr( ISR_LOW_ADDR, ISR_HIGH_ADDR ) ],    //low bit
					emu->Memory[ generate_addr( ISR_HIGH_ADDR, ISR_HIGH_ADDR ) ]  //high bit
				);
				break;

			//***********************>>>RTI INSTRUCTIONS<<<*************************
			case 0x40: //RTI : return from interrupt
				STUB_OUT_INTERRUPTS_IFACES ;

				//pull old P/status register from stack
				emu->S+=1;
				emu->P =emu->Memory[ generate_addr(emu->S, STACK_HIGH_ADDR) ];

				//pull 2bytes that make up our PC from stack
				emu->S+=1;
				emu->PC = generate_addr(
					emu->Memory[ generate_addr(emu->S, STACK_HIGH_ADDR) ],  //low byte
					emu->Memory[ generate_addr((emu->S)+1, STACK_HIGH_ADDR) ]  //high byte
				);
				emu->S+=1;

				//observe that this does not mess with IRQ status
				//whatever it was when it was pushed, that's what comes out
				//if you want to change it, the isr must modify it on the stack
				break;

			//***********************>>>JSR INSTRUCTIONS<<<*************************
			case 0x20: //JSR addr16 : jump to subroutine
				STUB_OUT_INTERRUPTS_IFACES ;

				//implemented same as BRK except the addr of the isr is in bytes 2,3 of the instr
				//also, no interrupts are handled in case of subroutine calls

				//increment PC by 2, push onto stack
				//PC will point to 3rd byte of JSR instr
				emu->Memory[ generate_addr(emu->S, STACK_HIGH_ADDR) ] = ((emu->PC)+2) >> 8;
				emu->S-=1;

				emu->Memory[ generate_addr(emu->S, STACK_HIGH_ADDR) ] = ((emu->PC)+2);
				emu->S-=1;

				emu->PC = generate_addr( GET_FIRST_ARG, GET_SECOND_ARG );

				//no flags affected
				break;


			//***********************>>>RTS INSTRUCTIONS<<<*************************
			case 0x60: //RTS : return from subroutine

				//implemented same as BRK except the addr of the isr is in bytes 2,3 of the instr
				//also, no interrupts are handled in case of subroutine calls

				//pull 2bytes that make up our PC from stack
				emu->S+=1;
				emu->PC = generate_addr(
					emu->Memory[ generate_addr(emu->S, STACK_HIGH_ADDR) ],  //low byte
					emu->Memory[ generate_addr((emu->S)+1, STACK_HIGH_ADDR) ]  //high byte
				);
				emu->S+=1;

				//increment PC again because it points to 3rd byte of previous JSR instr
				emu->PC+=1;
				break;

			default:
				printf("error: invalid object code 0x%hhx at P=%d\n", emu->Memory[emu->PC], emu->PC );
				printf("with %d remaining\n", max_instr_count);
				exit(-1);

				//assert(0);
		}


		#ifdef ALLOW_MAX_INSTR_COUNT
		  emu->instr_count++;  //each loop processes a single instruction
		#endif
	}

	return;
}
