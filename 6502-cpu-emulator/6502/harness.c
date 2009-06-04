#include <stdlib.h>
#include <assert.h>


#include "em_6502.h"
#include "unit_test.h"

#include <stdio.h>


//for running unit tests
//#define RUN_UNIT_TEST

void *test_memory_listener(unsigned short addr)
{
	printf("Called test_memory_listener with addr=%d\n",addr);
}


int main()
{
	FILE * pFile;
	long size;
	em6502 emulator;
	char * memblock;
	int i;
	mem_region region;

    printf("sizeof(char) == %d\n", sizeof(char));
	  printf("sizeof(unsigned short) == %d\n", sizeof(unsigned short));
    printf("sizeof(int) == %d\n", sizeof(int));
    printf("sizeof(long) == %d\n", sizeof(long));
    printf("sizeof(long long) == %d\n", sizeof(long long));


    #ifdef RUN_UNIT_TEST
       run_test_harness();
    #endif

	//lets try to run our own compiled file - 'disco.as'
	//as compiled by our asslink
	initialize_em6502( &emulator);
	create_simple_memory_map(&emulator);

	//pFile = fopen ("test/disco.as","rb");
	//pFile = fopen ("test/noise.as","rb"); //noise crashes: tries to return from main
	//pFile = fopen ("test/colors.as","rb");  //colors crashes; same as on 6502asm.com
	pFile = fopen ("test/alive.as","rb");

	if (pFile==NULL) { printf("Failed to open file"); exit(-1); }
	else
	{
	  fseek (pFile, 0, SEEK_END);
	  size = ftell (pFile);
	  rewind (pFile);

	  printf ("Size of file: %ld bytes.\n",size);
	}
	memblock = malloc(sizeof(char)* size);
	printf("read %d\n",fread(memblock,sizeof(char),size,pFile));

    for ( i = 0; i < size; i++)
    {
      printf("%d ",  ((unsigned char *)memblock)[i] );
    }
    printf("\n");

	fclose(pFile);

	load_program( &emulator, memblock, size, 0x0600);

	free(memblock);

//	//one more thing:
//	//as per documentation on 6502asm.com, 0x00FF is the key-pressed
//	//and 0x00FE is a random number
//	//or '0' if none
//	//so lets set that manually to disable this thing - otherwise it'll always read 0xFF
	write_mem(&emulator,15,0);
	write_mem(&emulator,16,0);

	//mem-mapped screen - set to all black=0x00 here
	memset(&emulator._memory[0x0200],0,sizeof(char)*1024);

//	//lets also register a test memory listener
//	region.low = 0x0200;
//	region.high = 0x0600;
//	add_memory_write_listener(&emulator,region, &test_memory_listener );

//	while(1)
//	{
		//launch this bad boy!
		run_program(&emulator, 9999);
//	}

	printf("Yes, success\n");



	return 0;
}


