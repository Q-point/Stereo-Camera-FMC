/*
AUTHOR: dhq     

DESCRIPTION:

CHANGE HISTORY:
23 Aug 2020		dhq
	Initial version

MIT License

Copyright (c) 2020 dhq 

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */
 
 
#include "xparameters.h"

#include "Delay.h"


#if !defined(USE_DEFAULT_USLEEP)


#include "sleep.h"
#include "xparameters.h"
#include "xpseudo_asm.h"
#include "xreg_cortexa9.h"

#define COUNTS_PER_SECOND          (XPAR_CPU_CORTEXA9_CORE_CLOCK_FREQ_HZ)

int xusleep(unsigned int useconds)
{
	unsigned long tEnd, tCur;
	unsigned int reg;
	// debug
	unsigned long tCur1;
	unsigned int iterations = 0;
	unsigned int invalid = 0;

	/* check requested delay for out of range */

	if (useconds == 0) {
		return 0;
	}

	if (((COUNTS_PER_SECOND / 1000000) > 0) &&
	    (useconds > (0xFFFFFFFF / (COUNTS_PER_SECOND / 1000000)))) {
		return -1;
	}

	/* enable the counter */
	mtcp(XREG_CP15_PERF_MONITOR_CTRL, 1);
#ifdef __GNUC__
	reg = mfcp(XREG_CP15_COUNT_ENABLE_SET);
#else
	{ register unsigned int Reg __asm(XREG_CP15_COUNT_ENABLE_SET);
	  reg = Reg; }
#endif
	mtcp(XREG_CP15_COUNT_ENABLE_SET, reg | 0x80000000);

#ifdef __GNUC__
	tCur = mfcp(XREG_CP15_PERF_CYCLE_COUNTER);
#else
	{ register unsigned int Reg __asm(XREG_CP15_PERF_CYCLE_COUNTER);
	  tCur = Reg; }
#endif
	tEnd = tCur + (useconds * (COUNTS_PER_SECOND / 1000000));
	// debug
	tCur1 = tCur;
	if ( tEnd < tCur )
	{
		//xil_printf( "[usleep] WARNING : Wrap-around Condition (implementing bug fix)\r\n" );
		// wrap-around condition ... wait until counter wraps ...
		do {
#ifdef __GNUC__
			tCur = mfcp(XREG_CP15_PERF_CYCLE_COUNTER);
#else
			{ register unsigned int Reg __asm(XREG_CP15_PERF_CYCLE_COUNTER);
			  tCur = Reg; }
#endif
			// debug
			iterations++;
		} while (tCur > tCur1);
	}

	do {
#ifdef __GNUC__
		tCur = mfcp(XREG_CP15_PERF_CYCLE_COUNTER);
#else
		{ register unsigned int Reg __asm(XREG_CP15_PERF_CYCLE_COUNTER);
		  tCur = Reg; }
#endif
		// debug
		iterations++;
	} while (tCur < tEnd);

	if ( iterations < 40*useconds ) invalid = 1;
	//printf( "[usleep] useconds=%d, tCur1=%08X, tEnd=%08X, tCur=%08X, iterations=%d\r\n", useconds, tCur1, tEnd, tCur, iterations );
	if (invalid)
	{
       return -1;
	}

	return 0;
}

#endif // #if !defined(USE_DEFAULT_USLEEP)

void millisleep(unsigned int milliseconds)
{
	int i = 0;


	for (i = 0; i < milliseconds; i++)
	{
		usleep(1000);
	}
}

void del_sleep(unsigned int seconds)
{
	int i = 0;

	for (i = 0; i < seconds; i++)
	{
		//millisleep(1000);
		usleep(1000000);
	}
}

void delay_ms(unsigned int milliseconds)
{
	int i = 0;


	for (i = 0; i < milliseconds; i++)
	{
		usleep(1000);
	}
}



