#include <stdlib.h>
#include <assert.h>

#include "em_6502.h"
#include "unit_test.h"




int main()
{
	   //this is the tester code for the V-flag
		signed short c;
		signed short d;
		signed short e;
		
		unsigned char c1;
		unsigned char c2;

//    printf("sizeof(char) == %d\n", sizeof(char));
    printf("sizeof(unsigned short) == %d\n", sizeof(unsigned short));
//    printf("sizeof(int) == %d\n", sizeof(int));
//    printf("sizeof(long) == %d\n", sizeof(long));
//    printf("sizeof(long long) == %d\n", sizeof(long long));

	
	c1 = -121;
	c2 = 12;
	printf("result = %d\n", (unsigned char)(c1 - c2));


    #ifdef RUN_UNIT_TEST
       run_test_harness();
    #endif

    return 0;
}


