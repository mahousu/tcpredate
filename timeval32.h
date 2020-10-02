#ifndef _TIMEVAL32_H
#define _TIMEVAL32_H
/* While I call this timeval32.h, most of the routines actually deal
 * with ordinary timeval structures. But since the ultimate intent is
 * to edit the timeval32 values stored in pcap files, I figure I'll
 * go with that name. Besides, there are already timeval.h header files.
 */
/* The ordinary (system) timeval structure may use 64 bit values, but the
 * pcap structure always uses 32 bit ones. So we define our own.
 *
 * The unsigned 32-bit second value makes this good until February, 2106.
 * Better than the 2038 barrier that a signed one would give, but it does
 * mean you can't use too big an expansion factor in rewriting packet times.
 * This is mostly an issue in stress-testing code that reads pcap files -
 * there's a limit to the stress you can apply.
 *
 * By contrast, the (signed 64-bit) system timeval structure is good for
 * the next billion years or so. I say "billion" rather than anything larger
 * because routines which use it seem to die on second values above 2^56.
 * I assume the intelligent insects who will have inherited the Earth by
 * then will fix this.
 */
struct timeval32 {
	u_int32_t tv_sec;
	u_int32_t tv_usec;
};

struct timeval32 timeval32normalize(struct timeval32 a);
struct timeval timevalnormalize(struct timeval a);

/* There are other versions of arithmetic routines for timeval structures,
 * but the ones I've seen don't do error checking. It's not really
 * necessary for their cases, but mine can take values from user input,
 * and so need to be able to handle oddities.
 */
struct timeval timevaladd(struct timeval a, struct timeval b);
struct timeval timevalsub(struct timeval a, struct timeval b);
/* The first of these does the calculation in floating point, while
 * the second converts b to fixed point and does the calculation
 * that way. The third is entirely in fixed point.
 */
struct timeval timevalmult(struct timeval a, long double b);
struct timeval timevalmultx(struct timeval a, long double b);
struct timeval timevalmultxx(struct timeval a, struct timeval b);

/* This one can take a work buffer */
char * sprinttimeval(struct timeval a, char *buf);

/* Conversion routines */
struct timeval32 totimeval32(struct timeval a);
struct timeval totimeval(struct timeval32 a);
struct timeval dtotimeval(double a);
double timevaltod(struct timeval a);
struct timeval ldtotimeval(long double a);
long double timevaltold(struct timeval a);
struct timeval32 dtotimeval32(double a);
double timeval32tod(struct timeval32 a);
struct timeval32 ldtotimeval32(long double a);
long double timeval32told(struct timeval32 a);

/* Miscellaneous */
int bitspace(long long all, long double bll);
void timevalsetup(void);
void timevalunsetup(void);


#endif
