/* timevaltest.c - some simple tests of the timeval routines */

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

#ifdef TEST1
/* A simple test of time conversions, going forward and backward. */
int
main(int argc, char **argv)
{
	struct timeval now, a, b, c;
	struct timezone tz;
	int i;
	double factor;
	long long s;
	char buf[50];

	timevalsetup();

	gettimeofday(&now, &tz);
	printf("Now: %s", ctime(&now.tv_sec));
	for (factor = .125; factor < 100000; factor *= 2.0) {
		a = now;
		c = timevalmult(a, factor);
		printf("%8.3f: %s", factor, ctime(&c.tv_sec));
		c = timevalmultx(a, factor);
		printf("%8.3f: %s", factor, ctime(&c.tv_sec));
	}
	for (s = 1; s > 0; s <<= 1) {
		b.tv_sec = 0;
		b.tv_usec = s;
		b = timevalnormalize(b);
		a = now;
		c = timevaladd(a, b);
		printf("+%s: %s", sprinttimeval(b, buf), ctime(&c.tv_sec));
		a = now;
		c = timevalsub(a, b);
		printf("-%s: %s", sprinttimeval(b, buf), ctime(&c.tv_sec));
	}
	/* This will (currently) fail for s >= 2^56, i.e. around 2 billion
	 * years before or after now, because of ctime limitations. Though
	 * the day will be significantly (on the order of hours) longer by
	 * then, so the results would already be way off. Plus, humanity
	 * will likely be long extinct.
	 */
	for (s = 1; s > 0; s <<= 1) {
		b.tv_sec = s;
		b.tv_usec = 0;
		b = timevalnormalize(b);
		a = now;
		c = timevaladd(a, b);
		printf("+%s: %s", sprinttimeval(b, buf), ctime(&c.tv_sec));
		a = now;
		c = timevalsub(a, b);
		printf("-%s: %s", sprinttimeval(b, buf), ctime(&c.tv_sec));
	}
	return 0;
}
#endif /* TEST1 */

#ifdef TEST2
/* Simple interactive test of the various operations */
int
main(int argc, char **argv)
{
	struct timeval a, b, c;
	long double lda, ldb, ldc;
	double eps = 1.5/(double)MILLION;
	char bufa[50], bufb[50];

	timevalsetup();

	while (!feof(stdin)) {
		printf("a? ");
		scanf("%lld.%ld", &a.tv_sec, &a.tv_usec);
		lda = timevaltold(a);
		printf("a %s vs %llf\n",
			sprinttimeval(a, bufa), lda);
		printf("b? ");
		scanf("%lld.%ld", &b.tv_sec, &b.tv_usec);
		ldb = timevaltold(b);
		printf("b %s vs %llf\n",
			sprinttimeval(b, bufb), ldb);

		c = timevaladd(a, b);
		ldc = timevaltold(c);
		printf("a + b = %s vs %llf, diff %llf\n",
			sprinttimeval(c, bufa), lda+ldb,
			lda+ldb-ldc);

		c = timevalsub(a, b);
		ldc = timevaltold(c);
		printf("a - b = %s vs %llf, diff %llf\n",
			sprinttimeval(c, bufa), lda-ldb,
			lda-ldb-ldc);

		c = timevalmult(a, ldb);
		ldc = timevaltold(c);
		printf("a * b = %s vs %llf, diff %llf ... ",
			sprinttimeval(c, bufa), lda*ldb,
			lda*ldb-ldc);
		c = timevalmultx(a, ldb);
		ldc = timevaltold(c);
		printf("vs %s, diff %llf\n", 
			sprinttimeval(c, bufb),
			lda*ldb-ldc);
		c = timevalmult(a, 1.0/ldb);
		ldc = timevaltold(c);
		printf("a / b = %s vs %llf, diff %llf ... ",
			sprinttimeval(c, bufa), lda/ldb,
			lda/ldb-ldc);
		c = timevalmultx(a, 1.0/ldb);
		ldc = timevaltold(c);
		printf("vs %s, diff %llf\n", 
			sprinttimeval(c, bufb),
			lda/ldb-ldc);
	}
	return 0;
}
#endif /* TEST2 */

#ifdef TEST3
/* This does random tests of all the various fixed-point arithmetic
 * operations, and compares them to what floating point versions
 * would give. It's mostly for regression testing, to make sure that
 * "improvements" to the code don't wreck it.
 */
int
randsign(void)
{
	static int signsource;
	int sign;

	if (!signsource) signsource = random();
	sign = 1 - (signsource&0x2);
	signsource >>= 1;
	return sign;
}

int
main(int argc, char **argv)
{
	struct timeval a, b, c;
	double da, db, dc;
	long double lda, ldb, ldc;
	double eps15 = 1.5/(double)MILLION;
	double eps3 = 3.0/(double)MILLION;
	double eps4 = 4.0/(double)MILLION;
	long double eps;
	double delta;
	long double ldelta;
	char bufa[50], bufb[50];
	int i=0, billions=0;

	timevalsetup();
	srandom(time(NULL));

	while (1) {
		++i;
		if (i > BILLION) {
			i = 0;
			++billions;
			fprintf(stderr, "%d billion\n", billions);
		}
		/* Choose random values and random operations */
		a.tv_sec = random()*randsign();
		a.tv_usec = random()*randsign();
		a = timevalnormalize(a);
		c = a;
		b.tv_sec = random()*randsign();
		b.tv_usec = random()*randsign();
		b = timevalnormalize(b);
		da = timevaltod(a);
		db = timevaltod(b);

		switch (random()&0x3) {
		case 0:		/* do addition */
			c = timevaladd(c, b);
			dc = timevaltod(c);
			delta = fabs(dc - (da + db));
			if (delta > eps15) {
					printf("%s + %s:\n",
						sprinttimeval(a, bufa),
						sprinttimeval(b, bufb));
					printf("\tgot %s\n",
						sprinttimeval(c, NULL));
					printf("\texpected %lf + %lf = %lf\n",
						da, db, da+db);
					printf("\tdelta %lf\n", delta);
					exit(1);
			}
			break;
		case 1:		/* do subtraction */
			c = timevalsub(c, b);
			dc = timevaltod(c);
			delta = fabs(dc - (da - db));
			if (delta > eps15) {
					printf("%s - %s:\n",
						sprinttimeval(a, bufa),
						sprinttimeval(b, bufb));
					printf("\tgot %s\n",
						sprinttimeval(c, NULL));
					printf("\texpected %lf - %lf = %lf\n",
						da, db, da-db);
					printf("\tdelta %lf\n", delta);
					exit(1);
			}
			break;
		case 2:		/* do scale up */
			/* Avoid overflow - use long doubles to compare */
			c = timevalmultx(c, db);
			ldc = timevaltold(c);
			lda = da; ldb = db;
			/* Error threshold is around one part in 2^51.
			 * Here, the fixed-point version will normally be
			 * more accurate than the floating point one.
			 */
			eps = fabsl(ldc)/((long double)((long long)1<<51));
			/* Except ... */
			if (eps < eps4) eps = eps4;
			ldelta = fabsl(ldc - (lda*ldb));
			if (ldelta > eps) {
					printf("%s * %s:\n",
						sprinttimeval(a, bufa),
						sprinttimeval(b, bufb));
					printf("\tgot %s\n",
						sprinttimeval(c, NULL));
					printf("\texpected %lf * %lf = %llf\n",
						da, db, lda*ldb);
					printf("\teps %llf diff %llf (delta ratio %llg)\n",
						eps, ldelta, ldelta/ldc);
					printf("\tbitspace %d\n",
						bitspace(a.tv_sec, ldb));
					exit(1);
			}
			break;
		case 3:		/* do scale down */
			/* Avoid divide by zero! */
			if (b.tv_sec == 0 && b.tv_usec == 0)
				++b.tv_usec;
			db = timevaltod(b);
			db = 1.0/db;
			c = timevalmultx(c, db);
			ldc = timevaltold(c);
			lda = da; ldb = db;
			/* For factors < 1, the fixed-point version can be
			 * less accurate than the floating-point one, at least
			 * the way I do them here. I'll make the error
			 * threshold twice the above.
			 */
			eps = fabsl(ldc)/((long double)((long long)1<<50));
			/* Except ... */
			if (eps < eps4) eps = eps4;
			ldelta = fabsl(ldc - (lda*ldb));
			if ( ldelta > eps) {
					printf("%s / %s:\n",
						sprinttimeval(a, bufa),
						sprinttimeval(b, bufb));
					printf("\tgot %s\n",
						sprinttimeval(c, NULL));
					printf("\texpected %llf / %llf = %llf\n",
						lda, 1.0/ldb, lda*ldb);
					printf("\teps %llf diff %llf (delta ratio %llg)\n",
						eps, ldelta, ldelta/ldc);
					printf("\tbitspace %d\n",
						bitspace(a.tv_sec, ldb));
					exit(1);
			}
			break;

		}
	}
	return 0;
}
#endif /* TEST3 */

#ifdef TEST4
/* Like the above, but this one is tuned to probe edge cases */
int
main(int argc, char **argv)
{
	struct timeval a, b, c;
	double da, db, dc;
	long double lda, ldb, ldc;
	double eps15 = 1.5/(double)MILLION;
	double eps3 = 3.0/(double)MILLION;
	double eps4 = 4.0/(double)MILLION;
	long double eps;
	double delta;
	long double ldelta;
	char bufa[50], bufb[50];
	int bitsa, bitsb;
	int i=0, billions=0;

	timevalsetup();
	srandom(time(NULL));

	while (1) {
		++i;
		if (i > BILLION) {
			i = 0;
			++billions;
			fprintf(stderr, "%d billion\n", billions);
		}
		/* Choose random values and random operations */
		a.tv_sec = random();
		bitsa = (random()%33);
		a.tv_sec &= (1<<bitsa)-1;
		a.tv_usec = random();
		a = timevalnormalize(a);
		c = a;
		b.tv_sec = random();
		bitsb = (random()%33);
		b.tv_sec &= (1<<bitsb)-1;
		b.tv_usec = random();
		b = timevalnormalize(b);
		da = timevaltod(a);
		db = timevaltod(b);

		switch (random()&0x3) {
		case 0:		/* do addition */
			c = timevaladd(c, b);
			dc = timevaltod(c);
			delta = fabs(dc - (da + db));
			if (delta > eps15) {
					printf("%s + %s:\n",
						sprinttimeval(a, bufa),
						sprinttimeval(b, bufb));
					printf("\tgot %s\n",
						sprinttimeval(c, NULL));
					printf("\texpected %lf + %lf = %lf\n",
						da, db, da+db);
					printf("\tdelta %lf\n", delta);
					exit(1);
			}
			break;
		case 1:		/* do subtraction */
			c = timevalsub(c, b);
			dc = timevaltod(c);
			delta = fabs(dc - (da - db));
			if ( (dc - (da - db)) > eps15
				|| (dc - (da - db)) < -eps15) {
					printf("%s - %s:\n",
						sprinttimeval(a, bufa),
						sprinttimeval(b, bufb));
					printf("\tgot %s\n",
						sprinttimeval(c, NULL));
					printf("\texpected %lf - %lf = %lf\n",
						da, db, da-db);
					printf("\tdelta %lf\n", delta);
					exit(1);
			}
			break;
		case 2:		/* do scale up */
			/* Avoid overflow - use long doubles to compare */
			c = timevalmultx(c, db);
			ldc = timevaltold(c);
			lda = da; ldb = db;
			/* Error threshold is around one part in 2^51.
			 * Here, the fixed-point version will normally be
			 * more accurate than the floating point one.
			 */
			eps = fabsl(ldc)/((long double)((long long)1<<51));
			/* Except ... */
			if (eps < eps4) eps = eps4;
			ldelta = fabsl(ldc - (lda*ldb));
			if (ldelta > eps) {
					printf("%s * %s:\n",
						sprinttimeval(a, bufa),
						sprinttimeval(b, bufb));
					printf("\tgot %s\n",
						sprinttimeval(c, NULL));
					printf("\texpected %lf * %lf = %llf\n",
						da, db, lda*ldb);
					printf("\teps %llf diff %llf (delta ratio %llg)\n",
						eps, ldelta, ldelta/ldc);
					printf("\tbitspace %d\n",
						bitspace(a.tv_sec, ldb));
					exit(1);
			}
			break;
		case 3:		/* do scale down */
			/* Avoid divide by zero! */
			if (b.tv_sec == 0 && b.tv_usec == 0)
				++b.tv_usec;
			db = timevaltod(b);
			db = 1.0/db;
			c = timevalmultx(c, db);
			ldc = timevaltold(c);
			lda = da; ldb = db;
			/* For factors < 1, the fixed-point version can be
			 * less accurate than the floating-point one, at least
			 * the way I do them here. I'll make the error
			 * threshold twice the above.
			 */
			eps = fabsl(ldc)/((long double)((long long)1<<50));
			/* Except ... */
			if (eps < eps4) eps = eps4;
			ldelta = fabsl(ldc - (lda*ldb));
			if ( ldelta > eps) {
					printf("%s / %s:\n",
						sprinttimeval(a, bufa),
						sprinttimeval(b, bufb));
					printf("\tgot %s\n",
						sprinttimeval(c, NULL));
					printf("\texpected %llf / %llf = %llf\n",
						lda, 1.0/ldb, lda*ldb);
					printf("\teps %llf diff %llf (delta ratio %llg)\n",
						eps, ldelta, ldelta/ldc);
					printf("\tbitspace %d\n",
						bitspace(a.tv_sec, ldb));
					exit(1);
			}
			break;

		}
	}
	return 0;
}
#endif /* TEST4 */

#ifdef TEST5

/* This is a quick but systematic probing of various ranges */

int
main(int argc, char **argv)
{
	struct timeval a, b, c;
	double da, db, dc;
	long double lda, ldb, ldc;
	double eps15 = 1.5/(double)MILLION;
	double eps3 = 3.0/(double)MILLION;
	double eps4 = 4.0/(double)MILLION;
	long double eps;
	double delta;
	long double ldelta;
	char bufa[50], bufb[50];
	int bitsa, bitsb;
	int i=0, billions=0;
	long long asec, bsec;
	int ausec, busec;

	timevalsetup();

	/* We go up to 2^31.5 ... */
	for (asec = 0; asec <= 3037000499L;
		asec = truncl(1.01*(long double)asec), asec++) {

	  for (bsec = 0; bsec <= 3037000499L;
	  	bsec = truncl(1.01*(long double)bsec), bsec++) {
	    for (ausec=0; ausec < MILLION;
	    	  ausec =truncl(1.4*(long double)ausec), ausec++) {
	      for (busec=0; busec < MILLION;
	    	    busec =truncl(1.4*(long double)busec), busec++) {
		a.tv_sec = asec;
		a.tv_usec = ausec;
		a = timevalnormalize(a);
		b.tv_sec = bsec;
		b.tv_usec = busec;
		b = timevalnormalize(b);
		da = timevaltod(a);
		db = timevaltod(b);
		++i;
		if (i > BILLION) {
		      i = 0;
		      ++billions;
		      fprintf(stderr, "%d billion\n", billions);
		      fprintf(stderr, "\ta %s b %s\n",
		      	sprinttimeval(a, bufa),
		      	sprinttimeval(b, bufb));
		}

		/* do addition */
		c = timevaladd(a, b);
		dc = timevaltod(c);
		delta = fabs(dc - (da + db));
		if (delta > eps15) {
				printf("%s + %s:\n",
					sprinttimeval(a, bufa),
					sprinttimeval(b, bufb));
				      printf("\tgot %s\n",
					      sprinttimeval(c, NULL));
				      printf("\texpected %lf + %lf = %lf\n",
					      da, db, da+db);
				      printf("\tdelta %lf\n", delta);
				      exit(1);
		      }
		/* do subtraction */
		c = timevalsub(a, b);
		dc = timevaltod(c);
		delta = fabs(dc - (da - db));
		if ( (dc - (da - db)) > eps15
			|| (dc - (da - db)) < -eps15) {
				printf("%s - %s:\n",
					sprinttimeval(a, bufa),
					sprinttimeval(b, bufb));
				printf("\tgot %s\n",
					sprinttimeval(c, NULL));
				printf("\texpected %lf - %lf = %lf\n",
					da, db, da-db);
				printf("\tdelta %lf\n", delta);
				exit(1);
		}
		/* do scale up */
		/* Avoid overflow - use long doubles to compare */
		c = timevalmultx(a, db);
		ldc = timevaltold(c);
		lda = da; ldb = db;
		/* Error threshold is around one part in 2^51.
		 * Here, the fixed-point version will normally be
		 * more accurate than the floating point one.
		 */
		eps = fabsl(ldc)/((long double)((long long)1<<51));
		/* Except ... */
		if (eps < eps4) eps = eps4;
		ldelta = fabsl(ldc - (lda*ldb));
		if (ldelta > eps) {
				printf("%s * %s:\n",
					sprinttimeval(a, bufa),
					sprinttimeval(b, bufb));
				printf("\tgot %s\n",
					sprinttimeval(c, NULL));
				printf("\texpected %lf * %lf = %llf\n",
					da, db, lda*ldb);
				printf("\teps %llf diff %llf (delta ratio %llg)\n",
					eps, ldelta, ldelta/ldc);
				printf("\tbitspace %d\n",
					bitspace(a.tv_sec, ldb));
				exit(1);
		}
		/* do scale down */
		/* Avoid divide by zero! */
		if (b.tv_sec == 0 && b.tv_usec == 0)
			++b.tv_usec;
		db = timevaltod(b);
		db = 1.0/db;
		c = timevalmultx(a, db);
		ldc = timevaltold(c);
		lda = da; ldb = db;
		/* For factors < 1, the fixed-point version can be
		 * less accurate than the floating-point one, at least
		 * the way I do them here. I'll make the error
		 * threshold twice the above.
		 */
		eps = fabsl(ldc)/((long double)0.9*((long long)1<<50));
		/* Except ... */
		if (eps < eps4) eps = eps4;
		ldelta = fabsl(ldc - (lda*ldb));
		if ( ldelta > eps) {
				printf("%s / %s:\n",
					sprinttimeval(a, bufa),
					sprinttimeval(b, bufb));
				printf("\tgot %s\n",
					sprinttimeval(c, NULL));
				printf("\texpected %llf / %llf = %llf\n",
					lda, 1.0/ldb, lda*ldb);
				printf("\teps %llf diff %llf (delta ratio %llg)\n",
					eps, ldelta, ldelta/ldc);
				printf("\tbitspace %d\n",
					bitspace(a.tv_sec, ldb));
				exit(1);
		  }
	      }
	    }
	  }
	}
	fprintf(stderr, "%d.%09d billion\n", billions, i);
	fprintf(stderr, "\ta %s b %s\n",
		sprinttimeval(a, bufa),
		sprinttimeval(b, bufb));
	return 0;
}
#endif /* TEST5 */
