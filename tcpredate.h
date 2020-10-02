#ifndef _TCPREDATE_H
#define _TCPREDATE_H

#ifndef _TIMEVAL32_H
#include "timeval32.h"
#endif

/* This is for "pcap classic." For some reason, I couldn't find a header
 * file with these defined anywhere.
 */

/* Pcap file header */
typedef struct pcap_hdr_s {
	u_int32_t magic_number;   /* magic number */
	u_int16_t version_major;  /* major version number */
	u_int16_t version_minor;  /* minor version number */
	int32_t  thiszone;       /* GMT to local correction */
	u_int32_t sigfigs;        /* accuracy of timestamps */
	u_int32_t snaplen;        /* max length of captured packets, in octets */
	u_int32_t network;        /* data link type */
} pcap_hdr_t;

/* Pcap record header */
typedef struct pcaprec_hdr_s {
	struct timeval32 ts;
	u_int32_t incl_len;       /* number of octets of packet saved in file */
	u_int32_t orig_len;       /* actual length of packet */
} pcaprec_hdr_t;

/* Read a pcap file, adjust it to start at a given time, and 
 * stretch/compress its timing by a given factor. Return the
 * (new) time of the last packet in the file.
 *
 * Write a new file header only when first is true.
 */
struct timeval tcpredate(FILE *fp, struct timeval starttime, double factor, FILE *outfp, int first);

#endif
