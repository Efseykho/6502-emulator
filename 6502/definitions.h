#ifndef DEFINITIONS_H
#define DEFINITIONS_H

/* Define flags that govern what our chip really is  
 * We should probably only emulate 1 of them
 */
#define EMULATE_6502 1 // original chip
#define EMULATE_65C02 0 // apple II variant with support for decimal flags
#define EMULATE_6507 0 // atari cutaway version (28 io pins available)

/* Define our memory size 
 * So far, they're all the same 
 */
#ifdef EMULATE_6502
	#define MEMORY_SIZE 65536
#elif EMULATE_65C02
	//#define MEMORY_SIZE 65536
	assert(0); //should not get here
#elif EMULATE_6507
	//#define MEMORY_SIZE 65536
	assert(0); //should not get here
#endif

#define ALLOW_MAX_INSTR_COUNT 1 //for debugging, we have a max instr counter


#endif  /* DEFINITIONS_H */
