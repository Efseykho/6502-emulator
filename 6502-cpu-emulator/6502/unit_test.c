//this is the testing component of the emulator
//it will test out the functionality of the existing code



#include "unit_test.h"
#include "em_6502.h"
#include "definitions.h"


//on vista, calling assert fails spectacularly with a dialog box popup
//i could not figure out how to get rid of that so i hacked around it
//my apologies to anyone acutally dealing with this madness.
//
#define VISTA_HACK 0
#ifdef VISTA_HACK
	#define assert(exp) \
	if ((int)exp == (int)0) { printf("assert [%d] failed at %s:%d \n",exp, __FILE__,__LINE__); exit(-1);  }
#else
    #include "assert.h"
#endif


//include some helper assembly macros
//these simplify the testing
//they lay out memory in some common places to test different addressing modes
//
// this looks like: Memory[0x00FA] = 0x30, length=2
#define SETUP_ZP_MEMORY \
		0xA9, 0x30, /* LDA #$30, imm. load */ \
		0x85, 0xFA /* STA $FA, store in zero-page */
//
// this looks like: Memory[0x00FA] = 0x30, X=0x03, length=3
#define SETUP_ZP_INDEXED_X_MEMORY \
		0xA9, 0x30,  /* LDA #$30, imm. load */ \
		0x85, 0xFA,   /* STA $FA, store in zero-page */ \
		0xA2, 0x03  /* LDX $#03, imm load X */
//
// this looks like: Memory[0x00FA] = 0x30, Y=0x03, length=3
#define SETUP_ZP_INDEXED_Y_MEMORY \
		0xA9, 0x30,  /* LDA #$30, imm. load */ \
		0x85, 0xFA,   /* STA $FA, store in zero-page */ \
		0xA0, 0x03  /* LDY $#03, imm load Y */


//
// Memory[DA]=FA, Memory[DB]=EA, Memory[EAFA]=CC, X=0x27, length=7
#define SETUP_PRE_INDEXED_X_INDIRECT_MEMORY \
		0xA9, 0xFA,        /* LDA #$FA, imm. load */ \
		0x85, 0xDA,        /* STA $DA, store in zero-page */ \
		0xA9, 0xEA,        /* LDA #$EA, imm. load */ \
		0x85, 0xDB,        /* STA $DB, store in zero-page */ \
		0xA2, 0x27,   	   /*  LDX $#27, imm load X */ \
		0xA9, 0xCC,        /* LDA #$CC, imm. load */ \
		0x8D, 0xFA, 0xEA   /* STA $#EAFA */

//
// Memory[DC]=FB, Memory[DD]=EA, Memory[EB22]=CD, Y=0x27, length=7
#define SETUP_POST_INDEXED_Y_INDIRECT_MEMORY \
		0xA9, 0xFB,        /* LDA #$FB, imm. load */ \
		0x85, 0xDC,        /* STA $DC, store in zero-page */ \
		0xA9, 0xEA,        /* LDA #$EA, imm. load */ \
		0x85, 0xDD,        /* STA $DD, store in zero-page */ \
		0xA0, 0x27,   	   /*  LDY $#27, imm load Y */ \
		0xA9, 0xCD,        /* LDA #$CD, imm. load */ \
		0x8D, 0x22, 0xEB   /* STA $#EB22 */

//
//  Memory[EB23]=FC, A=0xFC, length=2
#define SETUP_EXTENDED_DIRECT_MEMORY \
		0xA9, 0xFC,        /* LDA #$FC, imm. load */ \
		0x8D, 0x23, 0xEB   /* STA $#EB23 */

//
// Memory[EB24]=FD, Y=0x27, length=3
#define SETUP_ABSOLUTE_INDEXED_Y_MEMORY \
		0xA9, 0xFD,        /* LDA #$FD, imm. load */ \
		0x8D, 0x24, 0xEB,  /* STA $24,$EB  */ \
		0xA0, 0x27 	       /*  LDY $#27, imm load Y */ \


//
// Memory[EB25]=FE, X=0x27, length=3
#define SETUP_ABSOLUTE_INDEXED_X_MEMORY \
		0xA9, 0xFE,        /* LDA #$FD, imm. load */ \
		0x8D, 0x25, 0xEB,  /* STA $25,$EB  */ \
		0xA2, 0x27 	       /*  LDX $#27, imm load X */






//helper macro for all the boring stuff here:
#define SETUP_UNIT_TEST(strName) \
	em6502 emulator; \
	printf("running %s...\n", strName); \
    initialize_em6502( &emulator); \
    create_simple_memory_map( &emulator ); \
	load_program( &emulator, &program, sizeof(program), 0)



//define our unit test sigs here
void test_init();
void test_no_flags();
void test_memory_copied();
void test_long_program_load();
void test_ld__instr();
void test_sta_instr();
void test_stx_instr();
void test_sty_instr();
void test_non_imm_lda_instr();
void test_non_imm_ldx_instr();
void test_non_imm_ldy_instr();
void test_V_flags();
void test_C_flags();
void test_hw_flags();
void test_adc_instr();
void test_and_instr();
void test_bit_instr();
void test_cmp_instr();
void test_eor_instr();
void test_ora_instr();
void test_sbc_instr();
void test_inc_instr();
void test_dec_instr();
void test_cpx_instr();
void test_cpy_instr();
void test_rol_instr();
void test_ror_instr();
void test_asl_intsr();
void test_lsr_instr();
void test_jmp_instr();
void test_bxx_instr();
void test_txx_instr();
void test_inx_instr();
void test_dex_instr();
void test_phx_instr();
void test_cli_instr();
void test_nop_instr();
void test_brk_instr();
void test_jsr_instr();

//start testing real programs
void test_program_1();

//this is a simple non-looping program
//compiled from: http://e-tradition.net/bytes/6502/assembler.html
unsigned char testProgram_no_loop[] = {
0xA9, 0x0F, 0x8D, 0x00, 0x00, 0xA9, 0x04, 0x8D,
0x01, 0x00, 0xA9, 0x01, 0x8D, 0x02, 0x00, 0xAD,
0x00, 0x00, 0x8D, 0x03, 0x00, 0xAD, 0x01, 0x00,
0x8D, 0x04, 0x00
};


//this is a simple looping program
//compiled from: http://e-tradition.net/bytes/6502/assembler.html
unsigned char testProgram_loop[] = {
0xA9, 0x0F, 0x8D, 0x00, 0x00, 0xA9, 0x04, 0x8D,
0x01, 0x00, 0xA9, 0x01, 0x8D, 0x02, 0x00, 0xAD,
0x00, 0x00, 0x8D, 0x03, 0x00, 0xAD, 0x01, 0x00,
0x8D, 0x04, 0x00, 0x4C, 0x0F, 00
};



/**************************************
 * Name:  run_test_harness
 * Inputs:  None
 * Outputs: None
 * Function: runs the battery of unit tests
 *
***************************************/
void run_test_harness()
{
	printf("running unit tests...\n\n");


	test_init();
	test_no_flags();
	test_memory_copied();
	test_long_program_load();
	test_ld__instr();
	test_sta_instr();
	test_non_imm_lda_instr();
	test_non_imm_ldx_instr();
	test_non_imm_ldy_instr();

	test_stx_instr();
	test_sty_instr();

	test_V_flags();
	test_C_flags();
	test_hw_flags();

	test_adc_instr();
	test_and_instr();
	test_bit_instr();
	test_cmp_instr();
	test_eor_instr();
	test_ora_instr();
	test_sbc_instr();
	test_inc_instr();
	test_dec_instr();
	test_cpx_instr();
	test_cpy_instr();
	test_rol_instr();
	test_ror_instr();
	test_asl_intsr();
	test_lsr_instr();
	test_jmp_instr();
	test_bxx_instr();
	test_txx_instr();
	test_inx_instr();
	test_dex_instr();
	test_phx_instr();
	test_cli_instr();
	test_nop_instr();
	test_brk_instr();
	test_jsr_instr();

	test_program_1();

	printf("\n...finished unit tests!\n");
}


/**************************************
 * Name:  test_init
 * Inputs:  None
 * Outputs: None
 * Function: unit tests for initialization
 *
***************************************/
void test_init()
{
    em6502 emulator;

	 printf("running test_init...\n");

    initialize_em6502( &emulator);

    assert( emulator.Acc == 0) ;
    assert( emulator.P == 0 );
    assert( emulator.PC == 0 );
    assert( emulator.S == 0xFF );
    assert( emulator.X == 0 );
    assert( emulator.Y == 0 );
}

/**************************************
 * Name:  test_no_flags
 * Inputs:  None
 * Outputs: None
 * Function: unit tests for no flags set in a freshly created emulator
 *
***************************************/
void test_no_flags()
{
    em6502 emulator;
	printf("running test_no_flags...\n");
    initialize_em6502( &emulator);

    assert( NONE_SET(emulator.P) );
//    assert(!(CARRY_GET(emulator.P)) );
//    assert(! (ZERO_GET(emulator.P)) );
//    assert(!(IRQ_DISABLE_GET(emulator.P)));
//    assert(!(DECIMAL_MODE_GET(emulator.P)));
//    assert(!(BRK_GET(emulator.P)));
//    assert(!(OVERFLOW_GET(emulator.P)));
//    assert(!(NEG_GET(emulator.P)));
}

/**************************************
 * Name:  test_memory_copied
 * Inputs:  None
 * Outputs: None
 * Function: unit tests for copying memory correctly
***************************************/
void test_memory_copied()
{
	em6502 emulator;
	unsigned char Memory[5];

	printf("running test_memory_copied...\n");

	memset(&Memory, 'C', sizeof(unsigned char)*5) ;

	initialize_em6502( &emulator);
	create_simple_memory_map( &emulator );
	load_program( &emulator, &Memory, 5, 0);

//	assert(emulator.Memory[0] == Memory[0] );
//	assert(emulator.Memory[1] == Memory[1] );
//	assert(emulator.Memory[2] == Memory[2] );
//	assert(emulator.Memory[3] == Memory[3] );
//	assert(emulator.Memory[4] == Memory[4] );
	assert(emulator._memory[0] == Memory[0] );
	assert(emulator._memory[1] == Memory[1] );
	assert(emulator._memory[2] == Memory[2] );
	assert(emulator._memory[3] == Memory[3] );
	assert(emulator._memory[4] == Memory[4] );

	//also test getting same memory via the read_mem method
	assert(read_mem(&emulator,0) == Memory[0] );
	assert(read_mem(&emulator,1) == Memory[1] );
	assert(read_mem(&emulator,2) == Memory[2] );
	assert(read_mem(&emulator,3) == Memory[3] );
	assert(read_mem(&emulator,4) == Memory[4] );
}

void test_long_program_load()
{
	unsigned char program[768];  //should occupy 3 pages: 0(h=255),1(h=511),2(h=767)
	int i;
	for ( i = 0; i < 768; i++)
	{
		program[i] = i % 255;
	}
	//memset(&program, 99, sizeof(unsigned char)*768);

	SETUP_UNIT_TEST("test_long_program_load") ;

	//simple test for memory correspondence
	assert(memcmp(&emulator._memory,program,768) );

	//now figure out if pages lined up as expected
	assert(emulator.page_table[0]->data[0] == program[0]);
	assert(emulator.page_table[0]->data[255] == program[255]);
	assert(emulator.page_table[1]->data[0] == program[256]);
	assert(emulator.page_table[1]->data[255] == program[511]);
	assert(emulator.page_table[2]->data[0] == program[512]);
	assert(emulator.page_table[2]->data[255] == program[767]);
}

void test_ld__instr()
{
	//this tests out all the lda instructions
	//it just cycles through all the possible lda combinations, 1 instr at a time
	unsigned char program[] =
	{
	 0xA9, 0x0E,  //LDA #$0E
	 0xA9, 0x00,  //LDA #$00
	 0xA9, 0xFE,  //LDA #$FE
	 0xA2, 0xFE,  //LDX #$FE
	 0xA0, 0x00   //LDY #$00
	} ;

	SETUP_UNIT_TEST("test_ld__instr") ;

	run_program(&emulator, 1); //LDA #$0E
	//check results
	assert(emulator.Acc == 0x0E);	//we should load that value into acc
	assert( !(ZERO_GET(emulator.P)) );  //zero flag should NOT be set
	assert( !(NEG_GET(emulator.P)) );  //negative flag should NOT be set

	run_program(&emulator, 1); //LDA #$00
	//check results
	assert(emulator.Acc == 0x00);	//we should load that value into acc
	assert( (ZERO_GET(emulator.P)) );  //zero flag should be set
	assert( !(NEG_GET(emulator.P)) );  //negative flag should NOT be set

	run_program(&emulator, 1); //LDA #$FE
	//check results
	assert(emulator.Acc == 0xFE);	//we should load that value into acc
	assert( !(ZERO_GET(emulator.P)) );  //zero flag should NOT be set
	assert( (NEG_GET(emulator.P)) );  //negative flag should be set


	run_program(&emulator, 1); //LDX #$FE
	//check results
	assert(emulator.X == 0xFE);	//we should load that value into acc
	assert( !(ZERO_GET(emulator.P)) );  //zero flag should NOT be set
	assert( (NEG_GET(emulator.P)) );  //negative flag should be set

	run_program(&emulator, 1); //LDY #$00
	//check results
	assert(emulator.Y == 0x00);	//we should load that value into acc
	assert( (ZERO_GET(emulator.P)) );  //zero flag should be set
	assert( !(NEG_GET(emulator.P)) );  //negative flag should NOT be set
}



void test_sty_instr()
{
	//this tests out all the sty instructions
	//it just cycles through all the possible sty combinations, 1 instr at a time
	unsigned char program[] =
	{
		0xA0, 0x20, //LDY #$20
		0x84, 0x35, //STY $#0x35

		0xA2, 0x02, //LDX $02
		0x94, 0x35, //STY $#0x35
		//the memory addr constructed is: (0035) + 0x02(Y)

		0x8C, 0x01, 0x30 //STY $01 $30
		//the memory addr constructed is: (0001)low, (0030)high
	};

	SETUP_UNIT_TEST("test_sty_instr") ;

	run_program(&emulator, 2); //LDY $20, STY $35
	assert(emulator._memory[0x35] == 0x20);

	run_program(&emulator, 2); //LDX $02, STY $35
	assert(emulator._memory[0x35 + 0x02 ] == 0x20);

	run_program(&emulator, 1); //STY $01 $30
	assert(emulator._memory[0x3001] == 0x20);
}

void test_stx_instr()
{
	//this tests out all the stx instructions
	//it just cycles through all the possible stx combinations, 1 instr at a time
	unsigned char program[] =
	{
		0xA2, 0x20, //LDX #$20
		0x86, 0x35, //STX $#0x35

		0xA0, 0x02, //LDY $02
		0x96, 0x35, //STX $#0x35
		//the memory addr constructed is: (0035) + 0x02(Y)

		0x8E, 0x01, 0x30 //STX $01 $30
		//the memory addr constructed is: (0001)low, (0030)high
	};

	SETUP_UNIT_TEST("test_stx_instr") ;

	run_program(&emulator, 2); //LDX $20, STX $35
	assert(emulator._memory[0x35] == 0x20);

	run_program(&emulator, 2); //LDY $02, STX $35
	assert(emulator._memory[0x35 + 0x02 ] == 0x20);

	run_program(&emulator, 1); //STX $01 $30
	assert(emulator._memory[0x3001] == 0x20);
}

void test_sta_instr()
{
	//this tests out all the sta instructions
	//it just cycles through all the possible sta combinations, 1 instr at a time
	unsigned char program[] =
	{
		// this looks like: Memory[0x00FA] = 0x30, X=0x03, length=3
		SETUP_ZP_INDEXED_X_MEMORY,
		0xA9, 0x12,  /* LDA #$12, imm. load */
		0x95, 0xF7, //STA $F7,X

		// Memory[DA]=FA, Memory[DB]=EA, Memory[EAFA]=CC, X=0x27, length=7
      SETUP_PRE_INDEXED_X_INDIRECT_MEMORY,
		0xA9, 0x12,  /* LDA #$12, imm. load */
		0x81, 0xB3,  //STA ($B3,X)

		// Memory[DC]=FB, Memory[DD]=EA, Memory[EB22]=CD, Y=0x27, length=7
      SETUP_POST_INDEXED_Y_INDIRECT_MEMORY,
	   0xA9, 0x12,  /* LDA #$12, imm. load */
	   0x91, 0xDC,   //STA ($DC),Y

		// Memory[EB23]=FC, A=0xFC, length=2
	   SETUP_EXTENDED_DIRECT_MEMORY,
		0xA9, 0x12,  /* LDA #$12, imm. load */
		0x8D, 0x23, 0xEB,  /* STA $23,$EB */

		// Memory[EB24]=FD, Y=0x27, length=3
      SETUP_ABSOLUTE_INDEXED_Y_MEMORY,
		0xA9, 0x12,  /* LDA #$12, imm. load */
		0x99, 0xFD, 0xEA, //STA $#0xEAFD,Y

		// Memory[EB25]=FE, X=0x27, length=3
      SETUP_ABSOLUTE_INDEXED_X_MEMORY,
		0xA9, 0x12,  /* LDA #$12, imm. load */
		0x9D, 0xFE, 0xEA //STA $#EAFE,X

	} ;

	SETUP_UNIT_TEST("test_sta_instr") ;

	run_program(&emulator, 5);
	assert(emulator._memory[0x00FA] == 0x12);

	run_program(&emulator, 9);
	assert(emulator._memory[0xEAFA] == 0x12);

	run_program(&emulator, 9);
	assert(emulator._memory[0xEB22] == 0x12);

	run_program(&emulator, 4);
	assert(emulator._memory[0xEB23] == 0x12);

	run_program(&emulator, 5);
	assert(emulator._memory[0xEB24] == 0x12);

	run_program(&emulator, 5);
	assert(emulator._memory[0xEB25] == 0x12);
}


void test_non_imm_lda_instr()
{
	//this tests out all the non-immidiate lda instructions
	//it just cycles through all the possible sta combinations, 1 instr at a time
	//this is broken up in such a stupid way because the book i use has a seperate section
	//for all the imm. memory load/sta and i dont want to refactor code now, blah!
	unsigned char program[] =
	{
		// this looks like: Memory[0x00FA] = 0x30, length=2
		SETUP_ZP_MEMORY,
		0xA5, 0xFA, //LDA $FA

		// this looks like: Memory[0x00FA] = 0x30, X=0x03, length=3
		SETUP_ZP_INDEXED_X_MEMORY,
		0xB5, 0xF7, //LDA 0xF7,X

		// Memory[EAFA]=CC, X=0x27, length=7
		SETUP_PRE_INDEXED_X_INDIRECT_MEMORY,
		0xA9, 0x12,  /* LDA #$12, imm. load */
		0xA1, 0xB3,   //LDA ($B3,X)

		// Memory[DC]=FB, Memory[DD]=EA, Memory[EB22]=CD, Y=0x27, length=7
		SETUP_POST_INDEXED_Y_INDIRECT_MEMORY,
		0xB1, 0xDC,    //LDA ($DC),Y

		// Memory[EB23]=FC, A=0xFC, length=2
		SETUP_EXTENDED_DIRECT_MEMORY,
		0xAD, 0x23, 0xEB,

		// Memory[EB24]=FD, Y=0x27, length=3
		SETUP_ABSOLUTE_INDEXED_Y_MEMORY,
		0xB9, 0xFD, 0xEA, //LDA $#0xEAFD,Y

		// Memory[EB25]=FE, X=0x27, length=3
		SETUP_ABSOLUTE_INDEXED_X_MEMORY,
		0xBD, 0xFE, 0xEA //LDA $#EAFE,X

	};

   SETUP_UNIT_TEST("test_non_imm_lda_instr") ;

	run_program(&emulator, 3);
	assert(emulator.Acc == 0x30);

	run_program(&emulator, 4);
	assert(emulator.Acc == 0x30);

	run_program(&emulator, 9);
	assert(emulator.Acc == 0xCC);

	run_program(&emulator, 8);
	assert(emulator.Acc == 0xCD);

	run_program(&emulator, 3);
	assert(emulator.Acc == 0xFC);

	run_program(&emulator, 4);
	assert(emulator.Acc == 0xFD);

	run_program(&emulator, 4);
	assert(emulator.Acc == 0xFE);
}

void test_non_imm_ldx_instr()
{

	//this tests out all the non-immidiate ldx instructions
	//it just cycles through all the possible sta combinations, 1 instr at a time
	//this is broken up in such a stupid way because the book i use has a seperate section
	//for all the imm. memory load/sta and i dont want to refactor code now, blah!
	unsigned char program[] =
	{
		// this looks like: Memory[0x00FA] = 0x30, length=2
		SETUP_ZP_MEMORY,
		0xA6, 0xFA, //LDX $FA

		// this looks like: Memory[0x00FA] = 0x30, Y=0x03, length=3
		SETUP_ZP_INDEXED_Y_MEMORY,
		0xB6, 0xF7,  //LDX ($F7),Y

		// Memory[EB23]=FC, A=0xFC, length=2
		SETUP_EXTENDED_DIRECT_MEMORY,
		0xAE, 0x23, 0xEB,  //LDX $23,$EB

		// Memory[EB24]=FD, Y=0x27, length=3
		SETUP_ABSOLUTE_INDEXED_Y_MEMORY,
		0xBE, 0xFD, 0xEA, //LDX $#0xEAFD,Y
	};

	SETUP_UNIT_TEST("test_non_imm_ldx_instr") ;

	run_program(&emulator, 3);
	assert(emulator.X == 0x30);

	run_program(&emulator, 4);
	assert(emulator.X == 0x30);

	run_program(&emulator, 3);
	assert(emulator.X == 0xFC);

	run_program(&emulator, 4);
	assert(emulator.X == 0xFD);

}



//TODO: refactor this madness
void test_non_imm_ldy_instr()
{
	//this tests out all the non-immidiate ldy instructions
	//it just cycles through all the possible sta combinations, 1 instr at a time
	//this is broken up in such a stupid way because the book i use has a seperate section
	//for all the imm. memory load/sta and i dont want to refactor code now, blah!
	unsigned char program[] =
	{
		// this looks like: Memory[0x00FA] = 0x30, length=2
		SETUP_ZP_MEMORY,
		0xA4, 0xFA, //LDY $FA

		// this looks like: Memory[0x00FA] = 0x30, X=0x03, length=3
        SETUP_ZP_INDEXED_X_MEMORY,
		0xB4, 0xF7,  //LDY ($F7),X

	    // Memory[EB23]=FC, A=0xFC, length=2
	    SETUP_EXTENDED_DIRECT_MEMORY,
		0xAC, 0x23, 0xEB,  //LDY $23,$EB

		// Memory[EB25]=FE, X=0x27, length=3
        SETUP_ABSOLUTE_INDEXED_X_MEMORY,
		0xBC, 0xFE, 0xEA, //LDY $#0xEAFE,X
	};

	SETUP_UNIT_TEST("test_non_imm_ldy_instr") ;

	run_program(&emulator, 3);
	assert(emulator.Y == 0x30);

	run_program(&emulator, 4);
	assert(emulator.Y == 0x30);

	run_program(&emulator, 3);
	assert(emulator.Y == 0xFC);

	run_program(&emulator, 4);
	assert(emulator.Y == 0xFE);
}

//This is the debug version i've used to develop the TEST_AND_SET_V macros
//it should not be needed again
/*
	int debug_test_overflow_subtract( unsigned char P, signed char CH1, signed char CH2)
	{

		if ( ((int)NEG_GET(CH1)) != ((int)NEG_GET(CH2)) )
		{
			if ( (signed short)CH1 - (signed short)CH2 >127 || (signed short)CH1 - (signed short)CH2 < -128 )
				return 1 ;
			else return 0 ;
		}
		else return 0;
	}
*/

void test_V_flags()
{
	//this is a very bareboned test to figure out the logic for
	//setting the vflag
	//the tests are coming from:
	//http://www.6502.org/tutorials/vflag.html

	//      V  =  OVERFLOW. Set if the addition of two like-signed numbers or the
   //          subtraction of two unlike-signed numbers produces a result
   //          greater than +127 or less than -128.
	//via: http://www.geocities.com/oneelkruns/asm1step.html

	unsigned char program[] =
	{
	};

	unsigned char P;
	unsigned char ch1;
	unsigned char ch2;

	SETUP_UNIT_TEST("test_V_flags") ;

	//testing additions
	P = 0;
	ch1 = 1;
	ch2 = 1;
	TEST_AND_SET_V_OVERFLOW_ADDITION(P, ch1, ch2) ;
	assert( (int)(OVERFLOW_GET(P)) == 0 );

	P = 0;
	ch1 = 1;
	ch2 = -1;
   TEST_AND_SET_V_OVERFLOW_ADDITION(P, ch1, ch2) ;
	assert( (int)(OVERFLOW_GET(P)) == 0 );

	P = 0;
	ch1 = 127;
	ch2 = 1;
	TEST_AND_SET_V_OVERFLOW_ADDITION(P, ch1, ch2) ;
	assert( (int)(OVERFLOW_GET(P)) != 0 );

	P = 0;
	ch1 = -128;
	ch2 = -1;
	TEST_AND_SET_V_OVERFLOW_ADDITION(P, ch1, ch2) ;
	assert( (int)(OVERFLOW_GET(P)) != 0 );

	//testing subtractions
	P = 0;
	ch1 = 0;
	ch2 = 1;
	//ret = test_overflow_subtract(P,ch1,ch2);
	TEST_AND_SET_V_OVERFLOW_SUBTRACTION(P, ch1, ch2) ;
	assert( (int)(OVERFLOW_GET(P)) == 0 );

	P = 0;
	ch1 = -128;
	ch2 = 1;
	//ret = test_overflow_subtract(P,ch1,ch2);
	//assert( ret == 1 );
	//ret = debug_test_overflow_subtract(P, ch1, ch2) ;
	TEST_AND_SET_V_OVERFLOW_SUBTRACTION(P, ch1, ch2) ;
	assert( (int)(OVERFLOW_GET(P)) != 0 );

	P = 0;
	ch1 = 127;
	ch2 = -1;
	//ret = test_overflow_subtract(P,ch1,ch2);
	//assert( ret == 1 );
	TEST_AND_SET_V_OVERFLOW_SUBTRACTION(P, ch1, ch2) ;
	assert( (int)(OVERFLOW_GET(P)) != 0 );
}


void test_C_flags()
{
	 //this is a very bareboned test to figure out the logic for
	 //setting the carry flag
	 //the tests are coming from:
	 //http://www.6502.org/tutorials/vflag.html


   //      C  =  CARRY. Set if the add produced a carry, or if the subtraction
   //           produced a borrow.  Also holds bits after a logical shift.
   //http://www.geocities.com/oneelkruns/asm1step.html
	unsigned char program[] =
	{
	};

	unsigned char P;
	unsigned char ch1;
	unsigned char ch2;

	SETUP_UNIT_TEST("test_C_flags") ;

	//printf("%d %d\n", ch1,  ch2 );
	//printf("%d %d\n", (int)ch1,  (int)ch2 );
	//printf("%d\n", (int)ch1 + (int)ch2 );

	P = 0;
	ch1 = 1;
	ch2 = 1;
	TEST_AND_SET_CARRY_ADDITION(P, ch1, ch2) ;
	assert( (int)(CARRY_GET(P)) == 0 );

	P = 0;
	ch1 = 1;
	ch2 = -1;
	TEST_AND_SET_CARRY_ADDITION(P, ch1, ch2) ;
	assert( (int)(CARRY_GET(P)) != 0 );

	P = 0;
	ch1 = 127;
	ch2 = 1;
	TEST_AND_SET_CARRY_ADDITION(P, ch1, ch2) ;
	assert( (int)(CARRY_GET(P)) == 0 );

	P = 0;
	ch1 = -128;
	ch2 = -1;
	TEST_AND_SET_CARRY_ADDITION(P, ch1, ch2) ;
	assert( (int)(CARRY_GET(P)) != 0 );
}


void test_hw_flags()
{
	 //this is the hw test of all teh possible flag configurations
	 //this unit test is probably extraneous

	unsigned char program[] =
	{
		0x18, //CLC
		0x38, //SEC
		0xD8, //CLD
		0xF8, //SED
		0xB8 //CLV
	};

	SETUP_UNIT_TEST("test_hw_flags") ;

	run_program(&emulator, 1); //CLC
	assert((int)(CARRY_GET(emulator.P)) == 0 );

	run_program(&emulator, 1); //SEC
	assert((int)(CARRY_GET(emulator.P)) != 0 );

	run_program(&emulator, 1); //CLD
	assert((int)(DECIMAL_MODE_GET(emulator.P)) == 0 );

	run_program(&emulator, 1); //SED
	assert((int)(DECIMAL_MODE_GET(emulator.P)) != 0 );

	run_program(&emulator, 1); //CLV
	assert((int)(OVERFLOW_GET(emulator.P)) == 0 );
}

void test_adc_instr()
{
	 //this tests the adc instruction

	unsigned char program[] =
	{
		0xA9, 0x01, //LDA #$01, imm. load
		0x69, 0x40, //ADC $#40

	   SETUP_ZP_MEMORY,  //2 memory cycles, Memory[0x00FA] = 0x30
		0xA9, 0x01, //LDA #$01, imm. load
		0x18,       //CLC, clear carry
		0x65, 0xFA,  //ADC $#FA, add A to contents of 0x00FA

		SETUP_ZP_INDEXED_X_MEMORY, //3 instr. Memory[0x00FA] = 0x30, X=0x03
		0xA9, 0x01, //LDA #$01, imm. load
		0x18,       //CLC, clear carry
		0x75, 0xF7, //ADC $#4F,X: memory generated= 0xF7 +0x03(X) = 0xFA
						//A = 1(A) + 0x30(Memory[0x00FA]) + 0(Carry)

		// Memory[DA]=FA, Memory[DB]=EA, Memory[EAFA]=CC, X=0x27, length=7
	    SETUP_PRE_INDEXED_X_INDIRECT_MEMORY,
		0xA9, 0x01, //LDA #$01, imm. load
		0x18,       //CLC, clear carry
		0x61, 0xB3, //ADC ($#B3,X): memory generated= 0xB3 + 0x27(X) = 0xDA(low), 0xDB(high)
					//from there, memory generated= 0xEAFA
					//result is 1(A) + 0(C) + 0xCC = 0xCD

		// Memory[DC]=FB, Memory[DD]=EA, Memory[EB22]=CD, Y=0x27, length=7
        SETUP_POST_INDEXED_Y_INDIRECT_MEMORY,
		0xA9, 0x01, //LDA #$01, imm. load
		0x18,       //CLC, clear carry
		0x71, 0xDC,  //ADC ($DC),Y

		// Memory[EB23]=FC, A=0xFC, length=2
	    SETUP_EXTENDED_DIRECT_MEMORY,
		0xA9, 0x01,        //LDA #$01, imm. load
		0x18,              //CLC, clear carry
		0x6D, 0x23, 0xEB,  //ADC $EB, $23

		// Memory[EB24]=FD, Y=0x27, length=3
        SETUP_ABSOLUTE_INDEXED_Y_MEMORY,
		0xA9, 0x01,       //LDA #$01, imm. load
		0x18,             //CLC, clear carry
		0x79, 0xFD, 0xEA,  //ADC $EAFD,Y

		// Memory[EB25]=FE, X=0x27, length=3
        SETUP_ABSOLUTE_INDEXED_X_MEMORY,
		0xA9, 0x01,       //LDA #$01, imm. load
		0x18,             //CLC, clear carry
		0x7D, 0xFE, 0xEA  //ADC $EAFE,X

	};

	SETUP_UNIT_TEST("test_adc_instr") ;

	run_program(&emulator, 2);
	assert(emulator.Acc == 0x41 );

	//test run of SETUP_ZP_MEMORY macro
	run_program(&emulator, 5);
	assert(emulator.Acc == 0x31 );

	//test run of SETUP_ZP_INDEXED_X_MEMORY macro
	run_program(&emulator, 6);
	assert(emulator.Acc == 0x31 );

	//test run of SETUP_PRE_INDEXED_X_INDIRECT_MEMORY macro
	run_program(&emulator, 10);
	assert(emulator.Acc == 0xCD );

	//test run of SETUP_POST_INDEXED_Y_INDIRECT_MEMORY macro
	run_program(&emulator, 10);
	assert(emulator.Acc == 0xCE);

	//test run of SETUP_EXTENDED_DIRECT_MEMORY macro
	run_program(&emulator, 5);
	assert(emulator.Acc == 0xFD);

	//test run of SETUP_ABSOLUTE_INDEXED_Y_MEMORY macro
	run_program(&emulator, 6);
	assert(emulator.Acc == 0xFE);

	//test run of SETUP_ABSOLUTE_INDEXED_X_MEMORY macro
	run_program(&emulator, 6);
	assert(emulator.Acc == 0xFF);
}


void test_and_instr()
{
		 //this tests the and instruction
	unsigned char program[] =
	{
		0xA9, 0x2A, //LDA #$2A, imm. load
		0x29, 0x0A, //AND $#0A,

	    SETUP_ZP_MEMORY,  //2 memory cycles, Memory[0x00FA] = 0x30
		0xA9, 0x10, //LDA #$10, imm. load
		0x18,       //CLC, clear carry
		0x25, 0xFA,  //AND $#10 and contents of Memory[0x00FA]

		SETUP_ZP_INDEXED_X_MEMORY, //3 instr. Memory[0x00FA] = 0x30, X=0x03
		0xA9, 0x10, //LDA #$10, imm. load
		0x18,       //CLC, clear carry
		0x35, 0xF7, //AND $#F7,X

		// Memory[DA]=FA, Memory[DB]=EA, Memory[EAFA]=CC, X=0x27, length=7
		SETUP_PRE_INDEXED_X_INDIRECT_MEMORY,
		0xA9, 0x0C, //LDA #$0C, imm. load
		0x18,       //CLC, clear carry
		0x21, 0xB3, //AND ($B3,X)

		// Memory[DC]=FB, Memory[DD]=EA, Memory[EB22]=CD, Y=0x27, length=7
		SETUP_POST_INDEXED_Y_INDIRECT_MEMORY,
		0xA9, 0x0D, //LDA #$01, imm. load
		0x18,       //CLC, clear carry
		0x31, 0xDC,  //AND ($DC),Y

		// Memory[EB23]=FC, A=0xFC, length=2
		SETUP_EXTENDED_DIRECT_MEMORY,
		0xA9, 0x0C,        //LDA #$01, imm. load
		0x18,              //CLC, clear carry
		0x2D, 0x23, 0xEB,  //AND $EB,$23

		// Memory[EB24]=FD, Y=0x27, length=3
		SETUP_ABSOLUTE_INDEXED_Y_MEMORY,
		0xA9, 0x0D,       //LDA #$01, imm. load
		0x18,             //CLC, clear carry
		0x39, 0xFD, 0xEA,  //AND $EAFD,Y

		// Memory[EB25]=FE, X=0x27, length=3
		SETUP_ABSOLUTE_INDEXED_X_MEMORY,
		0xA9, 0x0E,       //LDA #$01, imm. load
		0x18,             //CLC, clear carry
		0x3D, 0xFE, 0xEA  //AND $EAFE,X

	};

	SETUP_UNIT_TEST("test_and_instr") ;

	run_program(&emulator, 2);
	assert(emulator.Acc == 0x0A );

	//test run of SETUP_ZP_MEMORY macro
	run_program(&emulator, 5);
	assert(emulator.Acc == 0x10 );

	//test run of SETUP_ZP_INDEXED_X_MEMORY macro
	run_program(&emulator, 6);
	assert(emulator.Acc == 0x10 );

	//test run of SETUP_PRE_INDEXED_X_INDIRECT_MEMORY macro
	run_program(&emulator, 10);
	assert(emulator.Acc == 0x0C );

	//test run of SETUP_POST_INDEXED_Y_INDIRECT_MEMORY macro
	run_program(&emulator, 10);
	assert(emulator.Acc == 0x0D);

	//test run of SETUP_EXTENDED_DIRECT_MEMORY macro
	run_program(&emulator, 5);
	assert(emulator.Acc == 0x0C);

	//test run of SETUP_ABSOLUTE_INDEXED_Y_MEMORY macro
	run_program(&emulator, 6);
	assert(emulator.Acc == 0x0D);

	//test run of SETUP_ABSOLUTE_INDEXED_X_MEMORY macro
	run_program(&emulator, 6);
	assert(emulator.Acc == 0x0E);
}

void test_bit_instr()
{
		 //this tests the bit instruction
	unsigned char program[] =
	{
	   SETUP_ZP_MEMORY,  //2 memory cycles, Memory[0x00FA] = 0x30
		0xA9, 0x10, //LDA #$10, imm. load
		0x18,       //CLC, clear carry
		0x24, 0xFA,  //BIT $#10 and contents of Memory[0x00FA]

		0xA9, 0xFF, //LDA #$FF, imm. load
		0x85, 0xFA,     /* STA $FA, store in zero-page */ \
		0xA9, 0x00, //LDA #$00, imm. load
		0x24, 0xFA,  //BIT $#00 and contents of Memory[0x00FA]

		// Memory[EB23]=FC, A=0xFC, length=2
		SETUP_EXTENDED_DIRECT_MEMORY,
		0xA9, 0x0C,        //LDA #$0C, imm. load
		0x18,              //CLC, clear carry
		0x2C, 0x23, 0xEB,  //BIT $EB,$23 and contents of Memory[0x00FA]

	};

	SETUP_UNIT_TEST("test_bit_instr") ;

	//test run of SETUP_ZP_MEMORY macro
	run_program(&emulator, 5);
	assert(emulator.Acc == 0x10 );
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );
	assert( (int)(OVERFLOW_GET(emulator.P)) == 0x00 );

	emulator.P = 0;
	run_program(&emulator, 4);
	assert(emulator.Acc == 0x00 );
	assert( (int)(ZERO_GET(emulator.P)) != 0x00 );
	assert( (int)(NEG_GET(emulator.P)) != 0x00 );
	assert( (int)(OVERFLOW_GET(emulator.P)) != 0x00 );

	//test run of SETUP_EXTENDED_DIRECT_MEMORY macro
	run_program(&emulator, 5);
	assert(emulator.Acc == 0x0C );
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) != 0x00 );
	assert( (int)(OVERFLOW_GET(emulator.P)) != 0x00 );

}


void test_cmp_instr()
{
		 //this tests the cmp instruction
	unsigned char program[] =
	{
		0xA9, 0x10, //LDA $#10, imm. load
		0xC9, 0x05, //CMP $#05

	    SETUP_ZP_MEMORY,  //2 memory cycles, Memory[0x00FA] = 0x30
		0xA9, 0x10, //LDA #$10, imm. load
		0x18,       //CLC, clear carry
		0xC5, 0xFA,  //CMP $#10 and contents of Memory[0x00FA]

		0x18,       //CLC, clear carry
		0xA9, 0x40, //LDA #$40, imm. load
		0xC5, 0xFA,  //CMP $#40 and contents of Memory[0x00FA]

		SETUP_ZP_INDEXED_X_MEMORY, //3 instr. Memory[0x00FA] = 0x30, X=0x03
		0xA9, 0x10, //LDA #$10, imm. load
		0x18,       //CLC, clear carry
		0xD5, 0xF7, //CMP $#F7,X

		// Memory[DA]=FA, Memory[DB]=EA, Memory[EAFA]=CC, X=0x27, length=7
		SETUP_PRE_INDEXED_X_INDIRECT_MEMORY,
		0xA9, 0x0C, //LDA #$0C, imm. load
		0x18,       //CLC, clear carry
		0xC1, 0xB3, //CMP ($B3,X)

		// Memory[DC]=FB, Memory[DD]=EA, Memory[EB22]=CD, Y=0x27, length=7
		SETUP_POST_INDEXED_Y_INDIRECT_MEMORY,
		0xA9, 0x0D, //LDA #$0D, imm. load
		0x18,       //CLC, clear carry
		0xD1, 0xDC,  //CMP ($DC),Y

		// Memory[EB23]=FC, A=0xFC, length=2
		SETUP_EXTENDED_DIRECT_MEMORY,
		0xA9, 0x0C,        //LDA #$0C, imm. load
		0x18,              //CLC, clear carry
		0xCD, 0x23, 0xEB,  //CMP $EB,$23

		// Memory[EB24]=FD, Y=0x27, length=3
		SETUP_ABSOLUTE_INDEXED_Y_MEMORY,
		0xA9, 0x0D,       //LDA #$0D, imm. load
		0x18,             //CLC, clear carry
		0xD9, 0xFD, 0xEA,  //CMP $EAFD,Y

		// Memory[EB25]=FE, X=0x27, length=3
		SETUP_ABSOLUTE_INDEXED_X_MEMORY,
		0xA9, 0x0E,       //LDA #$0E, imm. load
		0x18,             //CLC, clear carry
		0xDD, 0xFE, 0xEA  //CMP $EAFE,X

	};

	SETUP_UNIT_TEST("test_cmp_instr") ;

	run_program(&emulator, 2);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) != 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 )

	//test run of SETUP_ZP_MEMORY macro
	run_program(&emulator, 5);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) != 0x00 );

	run_program(&emulator, 3);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) != 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );

	//test run of SETUP_ZP_INDEXED_X_MEMORY macro
	run_program(&emulator, 6);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) != 0x00 );

	//test run of SETUP_PRE_INDEXED_X_INDIRECT_MEMORY macro
	run_program(&emulator, 10);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );

	//test run of SETUP_POST_INDEXED_Y_INDIRECT_MEMORY macro
	run_program(&emulator, 10);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );

	//test run of SETUP_EXTENDED_DIRECT_MEMORY macro
	run_program(&emulator, 5);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );

	//test run of SETUP_ABSOLUTE_INDEXED_Y_MEMORY macro
	run_program(&emulator, 6);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );

	//test run of SETUP_ABSOLUTE_INDEXED_X_MEMORY macro
	run_program(&emulator, 6);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );
}


void test_eor_instr()
{
		 //this tests the xor instruction
	unsigned char program[] =
	{
		0xA9, 0x10, //LDA #$10, imm. load
		0x49, 0x55, //EOR $#55

	   SETUP_ZP_MEMORY,  //2 memory cycles, Memory[0x00FA] = 0x30
		0xA9, 0x10, //LDA #$10, imm. load
		0x18,       //CLC, clear carry
		0x45, 0xFA,  //EOR $#10 and contents of Memory[0x00FA]

		SETUP_ZP_INDEXED_X_MEMORY, //3 instr. Memory[0x00FA] = 0x30, X=0x03
		0xA9, 0x10, //LDA #$10, imm. load
		0x18,       //CLC, clear carry
		0x55, 0xF7, //EOR $#F7,X

		// Memory[DA]=FA, Memory[DB]=EA, Memory[EAFA]=CC, X=0x27, length=7
		SETUP_PRE_INDEXED_X_INDIRECT_MEMORY,
		0xA9, 0x0C, //LDA #$0C, imm. load
		0x18,       //CLC, clear carry
		0x41, 0xB3, //EOR ($B3,X)

		// Memory[DC]=FB, Memory[DD]=EA, Memory[EB22]=CD, Y=0x27, length=7
		SETUP_POST_INDEXED_Y_INDIRECT_MEMORY,
		0xA9, 0x0D, //LDA #$01, imm. load
		0x18,       //CLC, clear carry
		0x51, 0xDC,  //EOR ($DC),Y

		// Memory[EB23]=FC, A=0xFC, length=2
		SETUP_EXTENDED_DIRECT_MEMORY,
		0xA9, 0x0C,        //LDA #$01, imm. load
		0x18,              //CLC, clear carry
		0x4D, 0x23, 0xEB,  //EOR $EB,$23

		// Memory[EB24]=FD, Y=0x27, length=3
		SETUP_ABSOLUTE_INDEXED_Y_MEMORY,
		0xA9, 0x0D,       //LDA #$01, imm. load
		0x18,             //CLC, clear carry
		0x59, 0xFD, 0xEA,  //EOR $EAFD,Y

		// Memory[EB25]=FE, X=0x27, length=3
		SETUP_ABSOLUTE_INDEXED_X_MEMORY,
		0xA9, 0x0E,       //LDA #$01, imm. load
		0x18,             //CLC, clear carry
		0x5D, 0xFE, 0xEA  //EOR $EAFE,X

	};

	SETUP_UNIT_TEST("test_eor_instr") ;

	run_program(&emulator, 2);
	assert(emulator.Acc == 0x45 );

	//test run of SETUP_ZP_MEMORY macro
	run_program(&emulator, 5);
	assert(emulator.Acc == 0x20 );

	//test run of SETUP_ZP_INDEXED_X_MEMORY macro
	run_program(&emulator, 6);
	assert(emulator.Acc == 0x20 );

	//test run of SETUP_PRE_INDEXED_X_INDIRECT_MEMORY macro
	run_program(&emulator, 10);
	assert(emulator.Acc == 0xC0 );

	//test run of SETUP_POST_INDEXED_Y_INDIRECT_MEMORY macro
	run_program(&emulator, 10);
	assert(emulator.Acc == 0xC0);

	//test run of SETUP_EXTENDED_DIRECT_MEMORY macro
	run_program(&emulator, 5);
	assert(emulator.Acc == 0xF0);

	//test run of SETUP_ABSOLUTE_INDEXED_Y_MEMORY macro
	run_program(&emulator, 6);
	assert(emulator.Acc == 0xF0);

	//test run of SETUP_ABSOLUTE_INDEXED_X_MEMORY macro
	run_program(&emulator, 6);
	assert(emulator.Acc == 0xF0);
}



void test_ora_instr()
{
		 //this tests the ora instruction
	unsigned char program[] =
	{
		0xA9, 0x10, //LDA #$10, imm. load
		0x09, 0x01, //ORA $#01

	   SETUP_ZP_MEMORY,  //2 memory cycles, Memory[0x00FA] = 0x30
		0xA9, 0x10, //LDA #$10, imm. load
		0x18,       //CLC, clear carry
		0x05, 0xFA,  //ORA $#10 and contents of Memory[0x00FA]

		SETUP_ZP_INDEXED_X_MEMORY, //3 instr. Memory[0x00FA] = 0x30, X=0x03
		0xA9, 0x10, //LDA #$10, imm. load
		0x18,       //CLC, clear carry
		0x15, 0xF7, //ORA $#F7,X

		// Memory[DA]=FA, Memory[DB]=EA, Memory[EAFA]=CC, X=0x27, length=7
		SETUP_PRE_INDEXED_X_INDIRECT_MEMORY,
		0xA9, 0x0C, //LDA #$0C, imm. load
		0x18,       //CLC, clear carry
		0x01, 0xB3, //ORA ($B3,X)

		// Memory[DC]=FB, Memory[DD]=EA, Memory[EB22]=CD, Y=0x27, length=7
		SETUP_POST_INDEXED_Y_INDIRECT_MEMORY,
		0xA9, 0x0D, //LDA #$01, imm. load
		0x18,       //CLC, clear carry
		0x11, 0xDC,  //ORA ($DC),Y

		// Memory[EB23]=FC, A=0xFC, length=2
		SETUP_EXTENDED_DIRECT_MEMORY,
		0xA9, 0x0C,        //LDA #$01, imm. load
		0x18,              //CLC, clear carry
		0x0D, 0x23, 0xEB,  //ORA $EB,$23

		// Memory[EB24]=FD, Y=0x27, length=3
		SETUP_ABSOLUTE_INDEXED_Y_MEMORY,
		0xA9, 0x0D,       //LDA #$01, imm. load
		0x18,             //CLC, clear carry
		0x19, 0xFD, 0xEA,  //ORA $EAFD,Y

		// Memory[EB25]=FE, X=0x27, length=3
		SETUP_ABSOLUTE_INDEXED_X_MEMORY,
		0xA9, 0x0E,       //LDA #$01, imm. load
		0x18,             //CLC, clear carry
		0x1D, 0xFE, 0xEA  //ORA $EAFE,X

	};

	SETUP_UNIT_TEST("test_ora_instr") ;

	run_program(&emulator, 2);
	assert(emulator.Acc == 0x11 );

	//test run of SETUP_ZP_MEMORY macro
	run_program(&emulator, 5);
	assert(emulator.Acc == 0x30 );

	//test run of SETUP_ZP_INDEXED_X_MEMORY macro
	run_program(&emulator, 6);
	assert(emulator.Acc == 0x30 );

	//test run of SETUP_PRE_INDEXED_X_INDIRECT_MEMORY macro
	run_program(&emulator, 10);
	assert(emulator.Acc == 0xCC );

	//test run of SETUP_POST_INDEXED_Y_INDIRECT_MEMORY macro
	run_program(&emulator, 10);
	assert(emulator.Acc == 0xCD);

	//test run of SETUP_EXTENDED_DIRECT_MEMORY macro
	run_program(&emulator, 5);
	assert(emulator.Acc == 0xFC);

	//test run of SETUP_ABSOLUTE_INDEXED_Y_MEMORY macro
	run_program(&emulator, 6);
	assert(emulator.Acc == 0xFD);

	//test run of SETUP_ABSOLUTE_INDEXED_X_MEMORY macro
	run_program(&emulator, 6);
	assert(emulator.Acc == 0xFE);
}

void test_sbc_instr()
{
	 //this tests the sbc instruction

	unsigned char program[] =
	{
		0xA9, 0x20, //LDA #$20, imm. load
		0x18,       //CLC, clear carry
		0xE9, 0x10, //SBC $#10

	    SETUP_ZP_MEMORY,  //2 memory cycles, Memory[0x00FA] = 0x30
		0xA9, 0x01, //LDA #$01, imm. load
		0x18,       //CLC, clear carry
		0xE5, 0xFA,  //SBC $#FA, subtract A from contents of 0x00FA

		SETUP_ZP_INDEXED_X_MEMORY, //3 instr. Memory[0x00FA] = 0x30, X=0x03
		0xA9, 0x01, //LDA #$01, imm. load
		0x18,       //CLC, clear carry
		0xF5, 0xF7, //SBC $#4F,X: memory generated= 0xF7 +0x03(X) = 0xFA
						//A = 1(A) - 0x30(Memory[0x00FA]) - 1(Carry')

		// Memory[DA]=FA, Memory[DB]=EA, Memory[EAFA]=CC, X=0x27, length=7
	    SETUP_PRE_INDEXED_X_INDIRECT_MEMORY,
		0xA9, 0x01, //LDA #$01, imm. load
		0x18,       //CLC, clear carry
		0xE1, 0xB3, //SBC ($#B3,X): memory generated= 0xB3 + 0x27(X) = 0xDA(low), 0xDB(high)
					//from there, memory generated= 0xEAFA
					//result is 1(A) - 1(C') - 0xCC

		// Memory[DC]=FB, Memory[DD]=EA, Memory[EB22]=CD, Y=0x27, length=7
        SETUP_POST_INDEXED_Y_INDIRECT_MEMORY,
		0xA9, 0x01, //LDA #$01, imm. load
		0x18,       //CLC, clear carry
		0xF1, 0xDC,  //SBC ($DC),Y

		// Memory[EB23]=FC, A=0xFC, length=2
	    SETUP_EXTENDED_DIRECT_MEMORY,
		0xA9, 0x01,        //LDA #$01, imm. load
		0x18,              //CLC, clear carry
		0xED, 0x23, 0xEB,  //SBC $EB, $23

		// Memory[EB24]=FD, Y=0x27, length=3
        SETUP_ABSOLUTE_INDEXED_Y_MEMORY,
		0xA9, 0x01,       //LDA #$01, imm. load
		0x18,             //CLC, clear carry
		0xF9, 0xFD, 0xEA,  //SBC $EAFD,Y

		// Memory[EB25]=FE, X=0x27, length=3
        SETUP_ABSOLUTE_INDEXED_X_MEMORY,
		0xA9, 0x01,       //LDA #$01, imm. load
		0x18,             //CLC, clear carry
		0xFD, 0xFE, 0xEA  //SBC $EAFE,X

	};

	SETUP_UNIT_TEST("test_sbc_instr") ;

	run_program(&emulator, 3);
	assert(emulator.Acc == 0x0F );
	assert( (int)(CARRY_GET(emulator.P)) != 0x00 );
	assert( (int)(OVERFLOW_GET(emulator.P)) == 0x00 );

	//test run of SETUP_ZP_MEMORY macro
	run_program(&emulator, 5);
	assert(emulator.Acc == 0xD0 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );
	assert( (int)(OVERFLOW_GET(emulator.P)) == 0x00 );

	//test run of SETUP_ZP_INDEXED_X_MEMORY macro
	run_program(&emulator, 6);
	assert(emulator.Acc == 0xD0 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );
	assert( (int)(OVERFLOW_GET(emulator.P)) == 0x00 );

	//test run of SETUP_PRE_INDEXED_X_INDIRECT_MEMORY macro
	run_program(&emulator, 10);
	assert(emulator.Acc == 0x34 );


	//test run of SETUP_POST_INDEXED_Y_INDIRECT_MEMORY macro
	run_program(&emulator, 10);
	assert(emulator.Acc == 0x33);

	//test run of SETUP_EXTENDED_DIRECT_MEMORY macro
	run_program(&emulator, 5);
	assert(emulator.Acc == 0x04);

	//test run of SETUP_ABSOLUTE_INDEXED_Y_MEMORY macro
	run_program(&emulator, 6);
	assert(emulator.Acc == 0x03);

	//test run of SETUP_ABSOLUTE_INDEXED_X_MEMORY macro
	run_program(&emulator, 6);
	assert(emulator.Acc == 0x02);
}

void test_inc_instr()
{
	 //this tests the inc instruction

	unsigned char program[] =
	{
		SETUP_ZP_MEMORY,  //2 memory cycles, Memory[0x00FA] = 0x30
		0xE6, 0xFA,

		SETUP_ZP_INDEXED_X_MEMORY, //3 instr. Memory[0x00FA] = 0x30, X=0x03
		0xF6, 0xF7,

		// Memory[EB23]=FC, A=0xFC, length=2
	    SETUP_EXTENDED_DIRECT_MEMORY,
		0xEE, 0x23, 0xEB,

		// Memory[EB25]=FE, X=0x27, length=3
        SETUP_ABSOLUTE_INDEXED_X_MEMORY,
		0xFE, 0xFE, 0xEA
	};

	SETUP_UNIT_TEST("test_inc_instr") ;

	//test run of SETUP_ZP_MEMORY macro
	run_program(&emulator, 3);
	assert(emulator._memory[0x00FA] == 0x31);
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );

	//test run of SETUP_ZP_INDEXED_X_MEMORY macro
	run_program(&emulator, 4);
	assert(emulator._memory[0x00FA] == 0x31);

	//test run of SETUP_EXTENDED_DIRECT_MEMORY macro
	run_program(&emulator, 3);
	assert(emulator._memory[0xEB23] == 0xFD);
	assert( (int)(NEG_GET(emulator.P)) != 0x00 );

	//test run of SETUP_ABSOLUTE_INDEXED_X_MEMORY macro
	run_program(&emulator, 4);
	assert(emulator._memory[0xEB25] == 0xFF);

}

void test_dec_instr()
{
	 //this tests the dec instruction

	unsigned char program[] =
	{
		SETUP_ZP_MEMORY,  //2 memory cycles, Memory[0x00FA] = 0x30
		0xC6, 0xFA,

		SETUP_ZP_INDEXED_X_MEMORY, //3 instr. Memory[0x00FA] = 0x30, X=0x03
		0xD6, 0xF7,

		// Memory[EB23]=FC, A=0xFC, length=2
		SETUP_EXTENDED_DIRECT_MEMORY,
		0xCE, 0x23, 0xEB,

		// Memory[EB25]=FE, X=0x27, length=3
		SETUP_ABSOLUTE_INDEXED_X_MEMORY,
		0xDE, 0xFE, 0xEA
	};

	SETUP_UNIT_TEST("test_dec_instr") ;

	//test run of SETUP_ZP_MEMORY macro
	run_program(&emulator, 3);
	assert(emulator._memory[0x00FA] == 0x2F);
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );

	//test run of SETUP_ZP_INDEXED_X_MEMORY macro
	run_program(&emulator, 4);
	assert(emulator._memory[0x00FA] == 0x2F);

	//test run of SETUP_EXTENDED_DIRECT_MEMORY macro
	run_program(&emulator, 3);
	assert(emulator._memory[0xEB23] == 0xFB);
	assert( (int)(NEG_GET(emulator.P)) != 0x00 );

	//test run of SETUP_ABSOLUTE_INDEXED_X_MEMORY macro
	run_program(&emulator, 4);
	assert(emulator._memory[0xEB25] == 0xFD);
}


void test_cpx_instr()
{
	//this tests the cpx instruction
		unsigned char program[] =
	{
		0xA2, 0x07,  //LDX $#07
		0xE0, 0x01, //CPX $#01

		SETUP_ZP_MEMORY,  //2 memory cycles, Memory[0x00FA] = 0x30
		0xA2, 0x07,  //LDX $#07
		0xE4, 0xFA,   //CPX $#FA

		// Memory[EB23]=FC, A=0xFC, length=2
		SETUP_EXTENDED_DIRECT_MEMORY,
		0xA2, 0xFF,  //LDX $#FF
		0xEC, 0x23, 0xEB //CPX $#EB23
	};

	SETUP_UNIT_TEST("test_cpx_instr") ;

	run_program(&emulator, 2);
	assert(emulator.X == 0x07);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) != 0x00 );

	//test run of SETUP_ZP_MEMORY macro
	run_program(&emulator, 4);
	assert(emulator._memory[0x00FA] == 0x30);
	assert(emulator.X == 0x07);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) != 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );

	//test run of SETUP_EXTENDED_DIRECT_MEMORY macro
	run_program(&emulator, 4);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) != 0x00 );
}

void test_cpy_instr()
{
	//this tests the cpy instruction
	unsigned char program[] =
	{
		0xA0, 0x07,  //LDY $#07
		0xC0, 0x30, //CPY $#01

		SETUP_ZP_MEMORY,  //2 memory cycles, Memory[0x00FA] = 0x30
		0xA0, 0x07,  //LDY $#07
		0xC4, 0xFA,   //CPY $#FA

		// Memory[EB23]=FC, A=0xFC, length=2
		SETUP_EXTENDED_DIRECT_MEMORY,
		0xA0, 0xFF,  //LDY $#FF
		0xCC, 0x23, 0xEB //CPY $#EB23
	};

	SETUP_UNIT_TEST("test_cpy_instr") ;

	run_program(&emulator, 2);
	assert(emulator.Y == 0x07);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) != 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );

	//test run of SETUP_ZP_MEMORY macro
	run_program(&emulator, 4);
	assert(emulator._memory[0x00FA] == 0x30);
	assert(emulator.Y == 0x07);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) != 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );

	//test run of SETUP_EXTENDED_DIRECT_MEMORY macro
	run_program(&emulator, 4);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) != 0x00 );
}


void test_rol_instr()
{
	//this tests the rol instruction
	unsigned char program[] =
	{
		SETUP_ZP_MEMORY,  //2 memory cycles, Memory[0x00FA] = 0x30
		0x18, //CLC
		0x26, 0xFA, //ROL $#FA

		SETUP_ZP_INDEXED_X_MEMORY, //3 instr. Memory[0x00FA] = 0x30, X=0x03
		0x38, //SEC
		0x36, 0xF7, //ROL $#F7,X

		// Memory[EB23]=FC, A=0xFC, length=2
		SETUP_EXTENDED_DIRECT_MEMORY,
		0x18, //CLC
		0x2E, 0x23, 0xEB, //ROL $#EB23

		// Memory[EB25]=FE, X=0x27, length=3
		SETUP_ABSOLUTE_INDEXED_X_MEMORY,
		0x18,  //CLC
		0x3E, 0xFE, 0xEA,  //ROL $#EAFE,X

		0xA9, 0xFE,        /* LDA #$FE, imm. load */ \
		0x18, //CLC
		0x2A
	};

	SETUP_UNIT_TEST("test_rol_instr") ;

	//test run of SETUP_ZP_MEMORY macro
	run_program(&emulator, 4);
	assert(emulator._memory[0x00FA] == 0x60);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );

	//test run of SETUP_ZP_INDEXED_X_MEMORY macro
	run_program(&emulator, 5);
	assert(emulator._memory[0x00FA] == 0x61);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );


	//test run of SETUP_EXTENDED_DIRECT_MEMORY macro
	run_program(&emulator, 4);
	assert(emulator._memory[0xEB23] == 0xF8);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) != 0x00 );
	assert( (int)(NEG_GET(emulator.P)) != 0x00 );

	//test run of SETUP_ABSOLUTE_INDEXED_X_MEMORY macro
	run_program(&emulator, 5);
	assert(emulator._memory[0xEB25] == 0xFC);

	//test run of accumulator wraparound
	run_program(&emulator, 3);
	assert(emulator.Acc == 0xFC);
}


void test_ror_instr()
{
	//this tests the ror instruction
	unsigned char program[] =
	{
		SETUP_ZP_MEMORY,  //2 memory cycles, Memory[0x00FA] = 0x30
		0x18, //CLC
		0x66, 0xFA, //ROR $#FA

		SETUP_ZP_INDEXED_X_MEMORY, //3 instr. Memory[0x00FA] = 0x30, X=0x03
		0x38, //SEC
		0x76, 0xF7, //ROR $#F7,X

		// Memory[EB23]=FC, A=0xFC, length=2
		SETUP_EXTENDED_DIRECT_MEMORY,
		0x18, //CLC
		0x6E, 0x23, 0xEB, //ROR $#EB23

		// Memory[EB25]=FE, X=0x27, length=3
		SETUP_ABSOLUTE_INDEXED_X_MEMORY,
		0x18,  //CLC
		0x7E, 0xFE, 0xEA,  //ROR $#EAFE,X

		0xA9, 0xFE,        /* LDA #$FE, imm. load */ \
		0x18, //CLC
		0x6A
	};

	SETUP_UNIT_TEST("test_ror_instr") ;

	//test run of SETUP_ZP_MEMORY macro
	run_program(&emulator, 4);
	assert(emulator._memory[0x00FA] == 0x18);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );

	//test run of SETUP_ZP_INDEXED_X_MEMORY macro
	run_program(&emulator, 5);
	assert(emulator._memory[0x00FA] == 0x98);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) != 0x00 );

	//test run of SETUP_EXTENDED_DIRECT_MEMORY macro
	run_program(&emulator, 4);
	assert(emulator._memory[0xEB23] == 0x7E);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );

	//test run of SETUP_ABSOLUTE_INDEXED_X_MEMORY macro
	run_program(&emulator, 5);
	assert(emulator._memory[0xEB25] == 0x7F);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );

	//test run of accumulator wraparound
	run_program(&emulator, 3);
	assert(emulator.Acc == 0x7F);
}


void test_asl_intsr()
{
	//this tests the asl instruction
	unsigned char program[] =
	{
		SETUP_ZP_MEMORY,  //2 memory cycles, Memory[0x00FA] = 0x30
		0x18, //CLC
		0x06, 0xFA, //ASL $#FA

		SETUP_ZP_INDEXED_X_MEMORY, //3 instr. Memory[0x00FA] = 0x30, X=0x03
		0x38, //SEC
		0x16, 0xF7, //ASL $#F7,X

		// Memory[EB23]=FC, A=0xFC, length=2
		SETUP_EXTENDED_DIRECT_MEMORY,
		0x18, //CLC
		0x0E, 0x23, 0xEB, //ASL $#EB23

		// Memory[EB25]=FE, X=0x27, length=3
		SETUP_ABSOLUTE_INDEXED_X_MEMORY,
		0x18,  //CLC
		0x1E, 0xFE, 0xEA,  //ASL $#EAFE,X

		0xA9, 0xFE,        /* LDA #$FE, imm. load */ \
		0x18, //CLC
		0x0A		//ASL, shift accumulator
	};

	SETUP_UNIT_TEST("test_asl_instr") ;

	//test run of SETUP_ZP_MEMORY macro
	run_program(&emulator, 4);
	assert(emulator._memory[0x00FA] == 0x60);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );

	//test run of SETUP_ZP_INDEXED_X_MEMORY macro
	run_program(&emulator, 5);
	assert(emulator._memory[0x00FA] == 0x60);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );

	//test run of SETUP_EXTENDED_DIRECT_MEMORY macro
	run_program(&emulator, 4);
	assert(emulator._memory[0xEB23] == 0xF8);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) != 0x00 );
	assert( (int)(NEG_GET(emulator.P)) != 0x00 );

	//test run of SETUP_ABSOLUTE_INDEXED_X_MEMORY macro
	run_program(&emulator, 5);
	assert(emulator._memory[0xEB25] == 0xFC);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) != 0x00 );
	assert( (int)(NEG_GET(emulator.P)) != 0x00 );

	//test run of accumulator addressing
	run_program(&emulator, 3);
	assert(emulator.Acc == 0xFC);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) != 0x00 );
	assert( (int)(NEG_GET(emulator.P)) != 0x00 );
}


void test_lsr_instr()
{
	//this tests the lsr instruction
	unsigned char program[] =
	{
		SETUP_ZP_MEMORY,  //2 memory cycles, Memory[0x00FA] = 0x30
		0x18, //CLC
		0x46, 0xFA, //LSR $#FA

		SETUP_ZP_INDEXED_X_MEMORY, //3 instr. Memory[0x00FA] = 0x30, X=0x03
		0x38, //SEC
		0x56, 0xF7, //LSR $#F7,X

		// Memory[EB23]=FC, A=0xFC, length=2
		SETUP_EXTENDED_DIRECT_MEMORY,
		0x18, //CLC
		0x4E, 0x23, 0xEB, //LSR $#EB23

		// Memory[EB25]=FE, X=0x27, length=3
		SETUP_ABSOLUTE_INDEXED_X_MEMORY,
		0x18,  //CLC
		0x5E, 0xFE, 0xEA,  //LSR $#EAFE,X

		0xA9, 0xFE,        /* LDA #$FE, imm. load */ \
		0x18, //CLC
		0x4A		//LSR, shift accumulator
	};

	SETUP_UNIT_TEST("test_lsr_instr") ;

	//test run of SETUP_ZP_MEMORY macro
	run_program(&emulator, 4);
	assert(emulator._memory[0x00FA] == 0x18);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );

	//test run of SETUP_ZP_INDEXED_X_MEMORY macro
	run_program(&emulator, 5);
	assert(emulator._memory[0x00FA] == 0x18);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );

	//test run of SETUP_EXTENDED_DIRECT_MEMORY macro
	run_program(&emulator, 4);
	assert(emulator._memory[0xEB23] == 0x7E);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );

	//test run of SETUP_ABSOLUTE_INDEXED_X_MEMORY macro
	run_program(&emulator, 5);
	assert(emulator._memory[0xEB25] == 0x7F);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );

	//test run of accumulator addressing
	run_program(&emulator, 3);
	assert(emulator.Acc == 0x7F);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(CARRY_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );
}

void test_jmp_instr()
{
	//this tests the jmp instruction
	unsigned char program[] =
	{
		// Memory[EB23]=FC, A=0xFC, length=2
		SETUP_EXTENDED_DIRECT_MEMORY,
		0x18, //CLC
		0x4C, 0x23, 0xEB, //JMP $#EB23

		// Memory[DC]=FB, Memory[DD]=EA, Memory[EB22]=CD, Y=0x27, length=7
        SETUP_POST_INDEXED_Y_INDIRECT_MEMORY,
		0x6C, 0xDC, 0x00

	};

	SETUP_UNIT_TEST("test_jmp_instr") ;

	//test run of SETUP_EXTENDED_DIRECT_MEMORY macro
	run_program(&emulator, 4);
	assert(emulator.PC == 0xEB23);

	//must reset the PC to continue execution
	emulator.PC = 0x09;

	//test run of SETUP_POST_INDEXED_Y_INDIRECT_MEMORY macro
	run_program(&emulator, 8);
	assert(emulator.PC == 0xEAFB);
}

void test_bxx_instr()
{
	//this tests the bxx instruction
	unsigned char program[] =
	{
		//BCC******************
		0x38, //SEC
		0x90, 0x45,  //BCC #$45

		0x18, //CLC
		0x90, 0x45,  //BCC #$45

		//BCS******************
		0x38, //SEC
		0xB0, 0x45,  //BCS #$45

		0x18, //CLC = 9th instr
		0xB0, 0x45,  //BCS #$45

		//BEQ*****************
		0xA9, 0x00, //LDA #$00, imm. load
		0xF0, 0x45,  //BEQ $#45

		//BMI*****************
		0xA9, 0xFA, //LDA #$FA, imm. load, 0x10th instr
		0x30, 0x45,  //BMI $#45

		//BNE*****************
		0xA9, 0x0F, //LDA #$0F, imm. load, 0x14th instr
		0xD0, 0x45,  //BNE $#45

		//BPL*****************
		0xA9, 0x0F, //LDA #$0F, imm. load, 0x18th instr
		0x10, 0x45,  //BPL $#45

		//BVC*****************
		0xA9, 0xAB, //LDA #$AB, imm. load, 0x1Cth instr
		0x50, 0x45,  //BVC $#45

		//BSV*****************
		0xA9, 0xAB, //LDA #$AB, imm. load, 0x20th instr
		0x70, 0x45  //BVS $#45

	};

	SETUP_UNIT_TEST("test_bxx_instr") ;

	run_program(&emulator, 2);
	assert(emulator.PC == 0x03);

	run_program(&emulator, 2);
	assert(emulator.PC == 0x4B);

	//must reset the PC to continue execution
	emulator.PC = 0x06;

	run_program(&emulator, 2);
	assert(emulator.PC == 0x4E);

	//must reset the PC to continue execution
	emulator.PC = 0x09;

	run_program(&emulator, 2);
	assert(emulator.PC == 0x0C);

	run_program(&emulator, 2);
	assert(emulator.PC == 0x55);

	//must reset the PC to continue execution
	emulator.PC = 0x10;

	run_program(&emulator, 2);
	assert(emulator.PC == 0x59);

	//must reset the PC to continue execution
	emulator.PC = 0x14 ;

	run_program(&emulator, 2);
	assert(emulator.PC == 0x5D);

	//must reset the PC to continue execution
	emulator.PC = 0x18;

	run_program(&emulator, 2);
	assert(emulator.PC == 0x61);

	//must reset the PC to continue execution
	emulator.PC = 0x1C;

	run_program(&emulator, 2);
	assert(emulator.PC == 0x65);

	//must reset the PC to continue execution
	emulator.PC = 0x20;

	run_program(&emulator, 2);
	assert(emulator.PC == 0x24);
}


void test_txx_instr()
{
	//this tests the txx instruction
	unsigned char program[] =
	{
		// Memory[EB23]=FC, A=0xFC, length=2
		SETUP_EXTENDED_DIRECT_MEMORY,
		0xAA,

		SETUP_ZP_INDEXED_X_MEMORY, //3 instr. Memory[0x00FA] = 0x30, X=0x03
		0x8A,

		// Memory[EB23]=FC, A=0xFC, length=2
		SETUP_EXTENDED_DIRECT_MEMORY,
		0xA8,

		//this looks like: Memory[0x00FA] = 0x30, Y=0x03, length=3
		SETUP_ZP_INDEXED_Y_MEMORY,
		0x98,

		0xBA,

		SETUP_ZP_INDEXED_X_MEMORY, //3 instr. Memory[0x00FA] = 0x30, X=0x03
		0x9A
	};

	SETUP_UNIT_TEST("test_txx_instr") ;

	run_program(&emulator, 3);
	assert(emulator.X == 0xFC);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) != 0x00 );

	run_program(&emulator, 4);
	assert(emulator.Acc == 0x03);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );

	run_program(&emulator, 3);
	assert(emulator.Y == 0xFC);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) != 0x00 );

	run_program(&emulator, 4);
	assert(emulator.Acc == 0x03);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );

	//reset stack pointer
	emulator.S = 0xFF;

	run_program(&emulator, 1);
	assert(emulator.X == 0xFF);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) != 0x00 );

	run_program(&emulator, 4);
	assert(emulator.S == 0x03);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) == 0x00 );
}

void test_inx_instr()
{
	//this tests the inx instruction
	unsigned char program[] =
	{
		SETUP_ZP_INDEXED_X_MEMORY, //3 instr. Memory[0x00FA] = 0x30, X=0x03
		0xE8,

		//this looks like: Memory[0x00FA] = 0x30, Y=0x03, length=3
		SETUP_ZP_INDEXED_Y_MEMORY,
		0xC8
	};

	SETUP_UNIT_TEST("test_inx_instr") ;

	run_program(&emulator, 4);
	assert( emulator.X == 0x04);

	run_program(&emulator, 4);
	assert( emulator.Y == 0x04);
}

void test_dex_instr()
{
	//this tests the dex instruction
	unsigned char program[] =
	{
		SETUP_ZP_INDEXED_X_MEMORY, //3 instr. Memory[0x00FA] = 0x30, X=0x03
		0xCA,

		//this looks like: Memory[0x00FA] = 0x30, Y=0x03, length=3
		SETUP_ZP_INDEXED_Y_MEMORY,
		0x88
	};

	SETUP_UNIT_TEST("test_dex_instr") ;

	run_program(&emulator, 4);
	assert( emulator.X == 0x02);

	run_program(&emulator, 4);
	assert( emulator.Y == 0x02);
}


void test_phx_instr()
{
	//this tests the phx instruction
	unsigned char program[] =
	{
		0xA9, 0xCC,        /* LDA #$CC, imm. load */ \
		0x48, //PHA

		0xA9, 0x1F,        /* LDA #$1F, imm. load */ \
		0x68, //PLA

		0x08,  //PLP

		0x28
	};

	SETUP_UNIT_TEST("test_phx_instr") ;

	run_program(&emulator, 2);
	assert( emulator.S == 0xFE);
	assert( emulator._memory[0x01FF] == 0xCC);

	run_program(&emulator, 2);
	assert( emulator.S == 0xFF);
	assert( emulator.Acc == 0xCC);
	assert( (int)(ZERO_GET(emulator.P)) == 0x00 );
	assert( (int)(NEG_GET(emulator.P)) != 0x00 );

	//first set status to something norml
	emulator.P = 0xDE;
	run_program(&emulator, 1);
	assert( emulator._memory[0x01FF] == 0xDE);
	assert( emulator.S == 0xFE);

	//now reset status
	emulator.P = 0x00;
	run_program(&emulator, 1);
	assert( emulator.P == 0xDE);
	assert( emulator.S == 0xFF);
}


void test_cli_instr()
{
	//this tests the cli instruction
	unsigned char program[] =
	{
		0x58,  //CLI

		0x78  //SEI

	};

	SETUP_UNIT_TEST("test_cli_instr") ;

	run_program(&emulator, 1);
	assert( (int)(IRQ_DISABLE_GET(emulator.P)) == 0x00 );

	run_program(&emulator, 1);
	assert( (int)(IRQ_DISABLE_GET(emulator.P)) != 0x00 );
}

void test_nop_instr()
{
	//this tests the nop instruction
	unsigned char program[] =
	{
		0xEA, //NOP
		0xEA, //NOP
		0xEA //NOP
	};

	SETUP_UNIT_TEST("test_nop_instr") ;

	run_program(&emulator, 3);
	assert( emulator.PC == 0x03 );

}

void test_brk_instr()
{
	//this tests the brk instruction
	unsigned char program[] =
	{
		0xEA, //NOP
		0xEA, //NOP
		0xEA, //NOP

		0x00, 0x88, //BRK (with brk-signature of 0x88)

		0xEA, //NOP
		0xEA, //NOP
		0xEA //NOP
	};

	SETUP_UNIT_TEST("test_brk_instr") ;

	run_program(&emulator, 3);
	assert( emulator.PC == 0x03 );

	//first set our isr to something sensical, like 0x1234
	emulator._memory[0xFFFF] = 0x12;
	emulator._memory[0xFFFE] = 0x34;

	run_program(&emulator, 1);
	assert( emulator.PC == 0x1234 );
	assert( emulator._memory[0x01FF]  == 0x00 );  //high bit for the stack ptr
	assert( emulator._memory[0x01FE]  == 0x05 );  //low bit for the stack ptr
	assert( emulator._memory[0x01FD]  == 0x10 );  //P register with break set
	assert( (int)(IRQ_DISABLE_GET(emulator.P)) != 0x00 );  //interrupts enabled

	//now monkey-patch in an isr to bounce us back out to somewhere sane
	emulator._memory[0x1234] = 0x38; //SEC
	emulator._memory[0x1235] = 0x40; //RTI

	run_program(&emulator, 3);
	assert( emulator.PC == 0x06 );
	assert( emulator.P == 0x10 );
	assert( (int)(IRQ_DISABLE_GET(emulator.P)) == 0x00 );  //interrupts disabled
}


void test_jsr_instr()
{
	//this tests the jsr instruction
	unsigned char program[] =
	{
		0xEA, //NOP
		0xEA, //NOP
		0xEA, //NOP

		0x20, 0x34, 0x12, //JSR $#1234

		0xEA, //NOP
		0xEA, //NOP
		0xEA //NOP
	};

	SETUP_UNIT_TEST("test_jsr_instr") ;

	run_program(&emulator, 3);
	assert( emulator.PC == 0x03 );

	run_program(&emulator, 1);
	assert( emulator.PC == 0x1234 );
	assert( emulator._memory[0x01FF]  == 0x00 );  //high bit for the stack ptr
	assert( emulator._memory[0x01FE]  == 0x05 );  //low bit for the stack ptr

	//now monkey-patch in an isr to bounce us back out to somewhere sane
	emulator._memory[0x1234] = 0x38; //SEC
	emulator._memory[0x1235] = 0x60; //RTS

	run_program(&emulator, 3);
	assert( emulator.PC == 0x07 );
	assert( (int)(CARRY_GET(emulator.P)) != 0x00 );  //flags should get preserved across subroutines
}


void test_program_1()
{
	//this runs a random looping program
	unsigned char program[] =
	{
	0xA9, 0x0F, 0x8D, 0x00, 0x00, 0xA9, 0x04, 0x8D,
	0x01, 0x00, 0xA9, 0x01, 0x8D, 0x02, 0x00, 0xAD,
	0x00, 0x00, 0x8D, 0x03, 0x00, 0xAD, 0x01, 0x00,
	0x8D, 0x04, 0x00, 0xAD, 0x02, 0x00, 0xC9, 0x00,
	0xD0, 0x06, 0xEE, 0x00, 0x00, 0x4C, 0x2B, 0x00,
	0xCE, 0x00, 0x00, 0xAE, 0x02, 0x00, 0xAD, 0x00,
	0x00, 0xC9, 0x1F, 0xD0, 0x05, 0xA2, 0x01, 0x4C,
	0x40, 0x00, 0xC9, 0x00, 0xD0, 0x02, 0xA2, 0x00,
	0x8E, 0x02, 0x00, 0xA9, 0x01, 0xA2, 0x00, 0x81,
	0x00, 0xA9, 0x00, 0xA2, 0x00, 0x81, 0x03, 0x4C,
	0x0F, 0x00
	};

	SETUP_UNIT_TEST("test_program_1") ;

	run_program(&emulator, 100);
}


