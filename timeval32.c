/* timeval32.c - timeval-related routines, mostly for pcap timing. This
 * also includes some arithmetic routines treating the timeval as a
 * fixed-point decimal structure.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <fenv.h>
#include "timeval32.h"

/* So I get the number of zeroes right ... */
#define BILLION		1000000000
#define MILLION		1000000
#define THOUSAND	1000

/* Of course, this could truncate ...*/
struct timeval32
totimeval32(struct timeval a)
{
	struct timeval32 answer;

	answer.tv_sec = a.tv_sec;
	answer.tv_usec = a.tv_usec;
	return answer;
}

struct timeval
totimeval(struct timeval32 a)
{
	struct timeval answer;

	answer.tv_sec = a.tv_sec;
	answer.tv_usec = a.tv_usec;
	return answer;
}

struct timeval32
dtotimeval32(double a)
{
	struct timeval32 answer;

	answer.tv_sec = trunc(a);
	answer.tv_usec = (long)nearbyint((a-answer.tv_sec)*MILLION);
	answer.tv_sec += answer.tv_usec/MILLION;
	answer.tv_usec %= MILLION;
	return answer;
}

struct timeval
dtotimeval(double a)
{
	struct timeval answer;

	answer.tv_sec = trunc(a);
	answer.tv_usec = (long)nearbyint((a-answer.tv_sec)*MILLION);
	answer.tv_sec += answer.tv_usec/MILLION;
	answer.tv_usec %= MILLION;
	return answer;
}

struct timeval32
ldtotimeval32(long double a)
{
	struct timeval32 answer;

	answer.tv_sec = truncl(a);
	answer.tv_usec = (long)nearbyintl((a-answer.tv_sec)*MILLION);
	answer.tv_sec += answer.tv_usec/MILLION;
	answer.tv_usec %= MILLION;
	return answer;
}

struct timeval
ldtotimeval(long double a)
{
	struct timeval answer;

	answer.tv_sec = truncl(a);
	answer.tv_usec = (long)nearbyintl((a-answer.tv_sec)*MILLION);
	answer.tv_sec += answer.tv_usec/MILLION;
	answer.tv_usec %= MILLION;
	return answer;
}

double
timeval32tod(struct timeval32 a)
{
	return (double)a.tv_sec + (double)a.tv_usec/MILLION;
}

double
timevaltod(struct timeval a)
{
	return (double)a.tv_sec + (double)a.tv_usec/MILLION;
}

long double
timeval32told(struct timeval32 a)
{
	return (long double)a.tv_sec + (long double)a.tv_usec/MILLION;
}

long double
timevaltold(struct timeval a)
{
	return (long double)a.tv_sec + (long double)a.tv_usec/MILLION;
}


#define mysign(a) ((a) < 0 ? -1 : ((a) == 0 ? 0 : 1))

/* The timeval structure uses two signed integers to allow for easier
 * arithmetic computations. But we need to make sure the signs are
 * consistent in the end.
 */
struct timeval 
timevalnormalize(struct timeval a)
{
	/* Move excess microseconds into the seconds place */
	a.tv_sec += a.tv_usec/MILLION;
	a.tv_usec = a.tv_usec%MILLION;

	/* Make sure the signs are consistent, doing a carry/borrow
	 * if needed.
	 */
	if (a.tv_usec < 0) {
		if (a.tv_sec > 0) {
			--a.tv_sec;
			a.tv_usec = MILLION + a.tv_usec;
		}
	} else {
		if (a.tv_sec < 0) {
			++a.tv_sec;
			a.tv_usec = a.tv_usec - MILLION;
		}
	}
if (mysign(a.tv_sec)*mysign(a.tv_usec) < 0) abort();
	return a;
}

/* Yes, polymorphism would be helpful here .... */
struct timeval32
timeval32normalize(struct timeval32 a)
{
	if (a.tv_usec < 0) {
		if (a.tv_sec <= 0) {
			a.tv_sec += a.tv_usec/MILLION;
			a.tv_usec = a.tv_usec%MILLION;
		} else {
			a.tv_sec += a.tv_usec/MILLION - 1;
			a.tv_usec = MILLION + a.tv_usec%MILLION;
		}
	} else {
		if (a.tv_sec < 0) {
			a.tv_sec += a.tv_usec/MILLION + 1;
			a.tv_usec = a.tv_usec%MILLION - MILLION;
		} else {
			a.tv_sec += a.tv_usec/MILLION;
			a.tv_usec = a.tv_usec%MILLION;
		}
	}
	return a;
}

/* The sign of the timeval is that of the seconds if nonzero, otherwise
 * that of the microseconds.
 */
char *
sprinttimeval(struct timeval a, char *buffer)
{
	static char answer[50];

	if (!buffer)
		buffer = answer;
	if (a.tv_sec < 0) {
		sprintf(buffer, "%ld.%06d", a.tv_sec, -a.tv_usec);
	} else if (a.tv_sec == 0 && a.tv_usec < 0) {
		sprintf(buffer, "-%ld.%06d", a.tv_sec, -a.tv_usec);
	} else {
		sprintf(buffer, "%ld.%06d", a.tv_sec, a.tv_usec);
	}
	return buffer;
}

/* sys/time.h has macros for these, but they don't really do any
 * error checking, so I decided to write my own.
 */
struct timeval
timevaladd(struct timeval a, struct timeval b)
{
	long usec;

	usec = a.tv_usec + b.tv_usec;
	a.tv_sec = a.tv_sec + b.tv_sec + usec/MILLION;
	a.tv_usec = usec%MILLION;
	return timevalnormalize(a);
}

struct timeval
timevalsub(struct timeval a, struct timeval b)
{
	a.tv_sec = a.tv_sec - b.tv_sec;
	a.tv_usec = a.tv_usec - b.tv_usec;
	return timevalnormalize(a);
}

/*
 * This version uses floating point arithmetic.
 */
struct timeval
timevalmult(struct timeval a, long double b)
{
	long double dsec, dusec;

	/* To be honest, ordinary doubles would be more than enough,
	 * but using a long double for intermediate results should make
	 * this good enough even if we switch to timespecs (nanosecond
	 * precision).
	 */

	dsec = a.tv_sec*b + a.tv_usec*b/MILLION;
	dusec = a.tv_sec*b*MILLION + a.tv_usec*b;
	a.tv_sec = truncl(dsec);
	a.tv_usec = truncl(dusec);
	a.tv_usec = a.tv_usec%MILLION;
	return timevalnormalize(a);
}

/*
 * This version only uses integer arithmetic (except for converting b).
 * It's a bit fussy to deal with rounding/overflow/underflow issues.
 * It does truncate any fractional microseconds rather than rounding,
 * so can be off by almost 1 usec. If b is small, it scales it up
 * first, to try to get as many significant digits in the fixed-point
 * format as possible.
 *
 * Since the fixed-point representation can effectively have up to around
 * 84 bits of precision (64 bit seconds, ~20 bits for microseconds), it
 * can actually be more precise than even long doubles. For very small
 * multiplication factors, though, the long double wins. The differences
 * are trivially small in either case, though.
 */

/* Kind of dumb, but this is a tabular representation of the largest
 * number of the form (2^i)*(10^j), 0<=i<= 3, which is less than
 * (2^63 - 1)/(2^(63-k)), where 2^(63-k) is the ceiling for a*b.  This
 * is the factor we use to scale up the multiplicand b before doing the
 * fixed-point calculation.
 *
 * There's not really a strong reason to restrict the factors to this
 * form other than that it made things easier to interpret when I was
 * debugging this. And it works well enough in practice that I saw no
 * reason to try to be more precise.
 *
 * We only go up to 8*10^18, as 10^19 would overflow the signed 64-bit int.
 */
long long scaletable[] = {
	1,
	/*0*/ 1,
	/*1*/ 1,
	/*2*/ 2,
	/*3*/ 4,
	/*4*/ 8,
	/*5*/ 10,
	/*6*/ 20,
	/*7*/ 40,
	/*8*/ 100,
	/*9*/ 200,
	/*10*/ 400,
	/*11*/ 1000,
	/*12*/ 2000,
	/*13*/ 4000,
	/*14*/ 8000,
	/*15*/ 10000,
	/*16*/ 20000,
	/*17*/ 40000,
	/*18*/ 100000,
	/*19*/ 200000,
	/*20*/ 400000,
	/*21*/ 1000000,
	/*22*/ 2000000,
	/*23*/ 4000000,
	/*24*/ 8000000,
	/*25*/ 10000000,
	/*26*/ 20000000,
	/*27*/ 40000000,
	/*28*/ 100000000,
	/*29*/ 200000000,
	/*30*/ 400000000,
	/*31*/ 1000000000,
	/*32*/ 2000000000,
	/*33*/ 4000000000,
	/*34*/ 8000000000,
	/*35*/ 10000000000,
	/*36*/ 20000000000,
	/*37*/ 40000000000,
	/*38*/ 100000000000,
	/*39*/ 200000000000,
	/*40*/ 400000000000,
	/*41*/ 1000000000000,
	/*42*/ 2000000000000,
	/*43*/ 4000000000000,
	/*44*/ 8000000000000,
	/*45*/ 10000000000000,
	/*46*/ 20000000000000,
	/*47*/ 40000000000000,
	/*48*/ 100000000000000,
	/*49*/ 200000000000000,
	/*50*/ 400000000000000,
	/*51*/ 1000000000000000,
	/*52*/ 2000000000000000,
	/*53*/ 4000000000000000,
	/*54*/ 8000000000000000,
	/*55*/ 10000000000000000,
	/*56*/ 20000000000000000,
	/*57*/ 40000000000000000,
	/*58*/ 100000000000000000,
	/*59*/ 200000000000000000,
	/*60*/ 400000000000000000,
	/*61*/ 1000000000000000000,
	/*62*/ 2000000000000000000,
	/*63*/ 8000000000000000000,
};

long long
scalefactor(int bits)
{
	if (bits <= 0) return 1;
	if (bits > 63) bits = 63;
	return scaletable[bits];
}

int
bitspace(long long all, long double bll)
{
	union {
		long double b;
		u_int16_t ui[5];
	} bui;
	int bitsa, bitsb, bits;

	bui.b = bll;
	/* In little-endian IEEE 754 format, the fifth word
	 * contains the exponent, biased by 16383. Hence, the
	 * number of (>0) bits required is one more than that.
	 *
	 * Meanwhile, the clz functions return the number of
	 * leading zero bits * in an integer representation.
	 *
	 * Using these together, we can estimate the size of a*b
	 * as the sum of the (>0) bits each will require.
	 *
	 * Note that this code is dependent on both endianness and
	 * floating point representation!
	 */
	bitsa = all > 0 ? 64 - __builtin_clzll(all) :
			64 - __builtin_clzll(-all);
	bitsb = bui.b > 0 ? bui.ui[4]-16382 : 
			bui.ui[4]-(32768+16382);
	bits = bitsa + bitsb;

	return 64-bits;
}

struct timeval
timevalmultx(struct timeval a, long double b)
{
	struct timeval c;
	long long lsec, lusec;
	int bits;
	long long factor;

	if (b == 0.0 || (a.tv_sec == 0 && a.tv_usec == 0)) {
		c.tv_sec = 0;
		c.tv_usec = 0;
		return c;
	}
	/* To avoid underflow ... This isn't the maximum precision
	 * possible, which would scale up b just shy of having
	 * a*b overflow, but it should be more than good enough
	 * for the intended purpose.
	 */
	bits = bitspace(a.tv_sec, b);
	factor = scalefactor(bits);
	if (factor > 1) {
		/* It would be mildly more efficient not to make this
		 * recursive, of course.
		 */
		c = timevalmultx(a, b*(long double)factor);
		lsec = c.tv_sec/factor;
		if (factor > MILLION)
			lusec = (long long)c.tv_usec/factor +
				(c.tv_sec%factor)/(factor/MILLION);
		else 
			lusec = (long long)c.tv_usec/factor
				+ (MILLION*(c.tv_sec%factor))/factor;
		c.tv_sec = lsec;
		c.tv_usec = lusec;
		return timevalnormalize(c);
	} else {
		c = ldtotimeval(b);
		return timevalmultxx(a, c);
	}
}

/* This version is strictly fixed point */
struct timeval
timevalmultxx(struct timeval a, struct timeval b)
{
	struct timeval c;
	long long ab, abm;

	/* Messy, but avoids (unnecessary) overflow */
	abm =
	   (a.tv_sec/MILLION)*b.tv_usec + (a.tv_sec%MILLION*b.tv_usec)/MILLION +
	   (b.tv_sec/MILLION)*a.tv_usec + (b.tv_sec%MILLION*a.tv_usec)/MILLION;
	c.tv_sec = a.tv_sec*b.tv_sec + abm
		/*+(a.tv_usec*b.tv_usec)/(MILLION*MILLION) no use*/;
	c.tv_usec = ((a.tv_sec%MILLION)*b.tv_usec)%MILLION +
		((b.tv_sec%MILLION)*a.tv_usec)%MILLION +
		(a.tv_usec*b.tv_usec)/MILLION;
	return timevalnormalize(c);
}


/* This is probably unnecessary, but let's just make sure rounding
 * is going in the most advantageous direction for us. Where I need
 * truncation instead, I ask for it specifically.
 */
static int wasround;

void
timevalsetup(void)
{
	wasround = fegetround();
	fesetround(FE_TONEAREST);
}

void
timevalunsetup(void)
{
	fesetround(wasround);
}

