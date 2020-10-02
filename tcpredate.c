#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <getopt.h>
#include <sys/time.h>
#include <pcap/pcap.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <memory.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include "tcpredate.h"


struct timeval
tcpredate(FILE *fp, struct timeval newstart, double factor, FILE *outfp, int first)
{
	struct timeval startoffset, origstart, ourtime, ouroffset;
	struct timeval lasttime;
	pcap_hdr_t pcapfile;
	pcaprec_hdr_t pcaphdr;
	u_int8_t *buffer;
	char buf1[50], buf2[50];
	struct timeval nulltime={0,0};

	/* Read header; write only the first time */
	fread(&pcapfile, sizeof(pcap_hdr_t), 1, fp);
	if (pcapfile.magic_number != 0xa1b2c3d4) {
		fprintf(stderr, "Bad magic number %x\n", pcapfile.magic_number);
		return nulltime;
	}
	if (pcapfile.version_major > 2 || pcapfile.version_minor > 4) {
		fprintf(stderr, "Unknown version %d.%d\n",
			pcapfile.version_major, pcapfile.version_minor);
		return nulltime;
	}
	if (pcapfile.snaplen < BUFSIZ) pcapfile.snaplen=BUFSIZ;
	buffer = (u_int8_t *)malloc(pcapfile.snaplen);
	if (first)
		fwrite(&pcapfile, sizeof(pcap_hdr_t), 1, outfp);

	/* Determine offset from first packet */
	fread(&pcaphdr, sizeof(pcaprec_hdr_t), 1, fp);
	origstart = totimeval(pcaphdr.ts);

	if (newstart.tv_sec || newstart.tv_usec) {
		startoffset = timevalsub(newstart, origstart);
		fprintf(stderr, "offset is %s seconds\n",
			sprinttimeval(startoffset, buf1));
		pcaphdr.ts = totimeval32(newstart);
	} else {
		newstart = origstart;
		memset(&startoffset, 0, sizeof(startoffset));
	}
	fwrite(&pcaphdr, sizeof(pcaprec_hdr_t), 1, outfp);
	fread(buffer, 1, pcaphdr.incl_len, fp);
	fwrite(buffer, 1, pcaphdr.incl_len, outfp);
	lasttime = totimeval(pcaphdr.ts);


	while (1) {
		fread(&pcaphdr, sizeof(pcaprec_hdr_t), 1, fp);
		if (feof(fp)) {
			break;
		}
		/* new time = newstart + (origtime-origstart)*factor */

		if (factor != 1.0) {
			ourtime = timevaladd(
				timevalmultx(
					timevalsub(totimeval(pcaphdr.ts),
						origstart),
							factor),
							newstart);
		} else {
			ourtime = timevaladd(
				timevalsub(totimeval(pcaphdr.ts),
					origstart),
						newstart);
		}
		pcaphdr.ts = totimeval32(ourtime);
		fwrite(&pcaphdr, sizeof(pcaprec_hdr_t), 1, outfp);
		fread(buffer, 1, pcaphdr.incl_len, fp);
		fwrite(buffer, 1, pcaphdr.incl_len, outfp);
		lasttime = ourtime;
	}

	free((void *)buffer);
	return lasttime;
}

/* getdate(3) assumes DATEMSK has been set up, but often it hasn't.
 * This attempts to handle that situation.
 */
static char *
tryfile(char *file)
{
	struct stat dummy;
	static char longpath[PATH_MAX];
	char *home;

	/* Try the straight pathname */
	if (!stat(file, &dummy))
		return file;
	/* Check for homedir. Note I'm not bothering with ~user */
	if (*file == '~') {
		/* There's the possibility of a hack, using a bogus
		 * environmental variable. Why someone would hack
		 * themselves this way is beyond me, but ...
		 */
		home = getenv("HOME");
		if (!home) return NULL;
		if ((strlen(home) + strlen(file)) >= PATH_MAX)
			return NULL;
		sprintf(longpath, "%s%s", home, file+1);
		if (!stat(longpath, &dummy))
			return longpath;
	}
	return NULL;
}


void
getdatesetup(void)
{
	static char *datemsklist[] = {
		"./datemsk",
		"~/datemsk",
		"/etc/datemsk",
		"/tmp/datemsk",
		NULL
	};
	char *file;
	/* @@ Sloppy with lengths */
	char datebuf[PATH_MAX+20];
	int i;

	if (!getenv("DATEMSK")) {
		/* Look for it */
		for (i=0; datemsklist[i]; ++i) {
			if ((file=tryfile(datemsklist[i]))) {
				sprintf(datebuf, "DATEMSK=%s", file);
				putenv(datebuf);
				return;
			}
		}
	}
	/* @@ Last ditch - write out our default list to /tmp and
	 * use that instead.
	 */
	{
	static char defaultlist[] =
"%a %B %d %H:%M:%S %Y\n"
"%a %B %d %H:%M:%S %y\n"
"%A %B %d, %Y %H:%M:%S\n"
"%A\n"
"%B\n"
"%m/%d/%y %I %p\n"
"%d,%m,%Y %H:%M\n"
"%m/%d/%y\n"
"%H:%M\n"
"%m/%d/%y %H:%M\n"
"%B %Y\n"
"%a %d %b %Y\n"
"%y/%m/%d %H:%M:%S\n"
"%Y/%m/%d %H:%M:%S\n"
"%y/%m/%d %I:%M %p\n"
"%Y/%m/%d %I:%M %p\n"
"%y/%m/%d\n"
"%Y/%m/%d\n"
"%y-%m-%d %H:%M:%S\n"
"%y-%m-%d %H:%M.%S\n"
"%Y-%m-%d %H:%M:%S\n"
"%Y-%m-%d %H:%M.%S\n"
"%y-%m-%d %I:%M %p\n"
"%Y-%m-%d %I:%M %p\n"
"%y-%m-%d\n"
"%Y-%m-%d\n"
"%m/%d/%y %I:%M:%S %p\n"
"%m/%d/%Y %I:%M:%S %p\n"
"%m/%d/%y %H:%M:%S\n"
"%m/%d/%Y %H:%M:%S\n"
"%m/%d/%y %I:%M %p\n"
"%m/%d/%Y %I:%M %p\n"
"%m/%d/%y %H:%M\n"
"%m/%d/%Y %H:%M\n"
"%m/%d/%y\n"
"%m/%d/%Y\n"
"%b %d, %Y %I:%M:%S %p\n"
"%b %d, %Y %H:%M:%S\n"
"%B %d, %Y %I:%M:%S %p\n"
"%B %d, %Y %H:%M:%S\n"
"%b %d, %Y %I:%M %p\n"
"%b %d, %Y %H:%M\n"
"%B %d, %Y %I:%M %p\n"
"%B %d, %Y %H:%M\n"
"%b %d, %Y\n"
"%B %d, %Y\n"
"%b %d\n"
"%B %d\n"
"%m%d%H%M%y\n"
"%m%d%H%M%Y\n"
"%m%d%H%M\n"
"%m%d\n";
	FILE *datefp;

	datefp = fopen("/tmp/datemsk", "w");
	if (!datefp)
		return;
	fwrite(defaultlist, 1, strlen(defaultlist), datefp);
	fclose(datefp);
	sprintf(datebuf, "DATEMSK=/tmp/datemsk");
	putenv(datebuf);
	}
}

void
main(int argc, char **argv)
{
	int i;
	FILE *infp=stdin;
	FILE *outfp=stdout;
	struct timeval starttime, newstart={0,0};
	struct timeval gap={0,1};
	struct timezone tz;
	int c;
	struct tm *tmdate = NULL;
	int first=1;
	double factor=1.0;
	char buf1[50];

	timevalsetup();
	getdatesetup();

	while ((c=getopt(argc, argv, "s:nf:g:o:"))>=0) {
		switch(c) {
		case 's':
			tmdate = getdate(optarg);
			if (!tmdate) {
				fprintf(stderr, "Can't interpret %s\n",
					optarg);
				exit(1);
			}
			starttime.tv_sec = mktime(tmdate);
			starttime.tv_usec = 0;
			newstart = starttime;
			break;
		case 'n':
			gettimeofday(&starttime, &tz);
			newstart = starttime;
			break;
		case 'f':
			factor = atof(optarg);
			if (factor <= 0.0) {
				fprintf(stderr, "Factor must be > 0\n");
				exit(1);
			}
			break;
		case 'g':
			gap.tv_usec = atoi(optarg);
			if (gap.tv_usec < 0) {
				fprintf(stderr, "Gap must be > 0\n");
				exit(1);
			}
			gap = timevalnormalize(gap);
			break;
		case 'o':
			outfp = fopen(optarg, "w");
			if (!outfp) {
				perror(optarg);
				exit(1);
			}
			break;
		}
	}
	
	if (optind < argc) {
		for (i=optind; i < argc; ++i) {
			infp = fopen(argv[i], "r");
			if (!infp) {
				perror(argv[i]);
				exit(1);
			}
			newstart = tcpredate(infp, newstart, factor,
				outfp, first);
			first = 0;
			fclose(infp);
			/* Increment the start time slightly for the
			 * next go-around.
			 */
			newstart = timevaladd(newstart, gap);
		}
	} else {
		newstart = tcpredate(stdin, newstart, factor, outfp, first);
	}
	fclose(outfp);
	exit(0);
}
