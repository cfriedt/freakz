/*******************************************************************
    Copyright (C) 2009 FreakLabs
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. Neither the name of the the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.
    4. This software is subject to the additional restrictions placed on the
       Zigbee Specification's Terms of Use.

    THIS SOFTWARE IS PROVIDED BY THE THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.

    Originally written by Christopher Wang aka Akiba.
    Modifications made for GNURadio / UDP by Christopher Friedt, <chrisfriedt@gmail.com>
    Please post support questions to the FreakLabs forum.

*******************************************************************/
/*******************************************************************
    Author: Christopher Friedt

    Title:

    Description:
*******************************************************************/
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "test_sim.h"
#include "test_app.h"
#include "contiki-main.h"
#include "freakz.h"
#include "sim_drvr.h"
#include "test_app.h"

#define DEFAULT_UDP_HOST "localhost"
#define DEFAULT_UDP_PORT 52001

extern process_event_t event_data_in;
extern process_event_t event_cmd_rcvd;

static sim_node_t node;             // node struct that holds info related to node communications

static int udp_socket = -1;
static struct sockaddr_in udp_sockaddr = {};

FILE *fp = NULL;
FILE *fout = NULL;

/* Threads to handle the pipe communications */

/*
 * This thread is used to monitor the data_in pipe for
 * incoming frames. Threads should only call reentrant
 * functions or else bad things can happen.
 */
static void *sim_data_in_thread(void *dummy)
{
	(void) dummy;
	static char msg[BUFSIZE];
	int recvfrom_r = -1;

	for( ;; ) {
		// XXX: use pselect!
		recvfrom_r = recvfrom( udp_socket, msg, BUFSIZE, 0, (struct sockaddr *) &udp_sockaddr, (socklen_t *) &udp_sockaddr.sin_len );
		if ( -1 == recvfrom_r ) {
			perror("sim_data_in_thread");
		} else {
			/*
			 * write the received data into the input buffer
			 * of the stack and trigger the isr
			 */
			drvr_write_rx_buf( (U8 *)msg, recvfrom_r );
			drvr_rx_isr();
		}
	}
	return NULL;
}

/* Send the tx to the data_out pipe */
static void sim_pipe_data_out(U8 *data, U8 len) {
	int sendto_r = -1;

	sendto_r = sendto( udp_socket, data, len, 0, (struct sockaddr *) &udp_sockaddr, (socklen_t) udp_sockaddr.sin_len );
	if ( -1 == sendto_r ) {
		perror("sim_data_in_thread");
	}
}

/* Take care of the signals that come in */
static void sigint_handler()
{
	switch (errno) {
	case EEXIST:
		perror("node exists");
		break;
	case EINTR:
		pthread_cancel(node.data_in.thread);
		pthread_cancel(node.cmd_in.thread);
		pthread_join(node.cmd_in.thread, NULL);
		pthread_join(node.data_in.thread, NULL);
		close(node.data_in.pipe);
		close(node.data_out.pipe);
		close(node.cmd_in.pipe);
		close(node.cmd_out.pipe);
		fclose(fp);
		exit(EXIT_SUCCESS);
		break;
	default:
		perror("node other");
		break;
	}
}

/* Get a pointer to the sim node structure */
sim_node_t *node_get()
{
	return &node;
}

static void sigkill_handler()
{
	pthread_cancel(node.data_in.thread);
	pthread_cancel(node.cmd_in.thread);
	pthread_join(node.cmd_in.thread, NULL);
	pthread_join(node.data_in.thread, NULL);
	close(node.data_in.pipe);
	close(node.data_out.pipe);
	fclose(fp);
	exit(EXIT_SUCCESS);
}

#ifndef format_cmd_str
#define format_cmd_str(x)
#endif

static void usage( const char *progname ) {
	printf(
		"usage:"
		"\t%s [arguments...]\n\n"
		"where [arguments...] is one or more of the following:\n\n"
		"\t[<--host|-H> localhost]        hostname or IP address of IEEE 802.15.4 MAC layer\n"
		"\t[<--port|-p> 52001]            UDP port of IEEE 802.15.4 MAC layer\n"
		,
		progname
	);
}

static int process_args( int argc, char *argv[] ) {

}

int main(int argc, char *argv[])
{
	int idx;
	int retval = EXIT_SUCCESS;
	int i;
	char *udp_host = DEFAULT_UDP_HOST;
	int udp_port = DEFAULT_UDP_PORT;
	bool have_help = false;
	bool arg_error = false;
	bool missing_arg = false;

	for( i=1; i<argc; i++ ) {
		if ( 0 == strcmp( argv[i], "-H" ) || 0 == strcmp( argv[i], "--host" ) ) {
			if ( i+1 < argc ) {
				udp_host = argv[ ++i ];
			} else {
				missing_arg = true;
			}
		} else if ( 0 == strcmp( argv[i], "-p" ) || 0 == strcmp( argv[i], "--port" ) ) {
			if ( i+1 < argc ) {
				udp_port = atoi( argv[ ++i ] );
			} else {
				missing_arg = true;
			}
		} else if ( 0 == strcmp( argv[i], "-h" ) || 0 == strcmp( argv[i], "--help" ) ) {
			have_help = true;
			break;
		} else {
			arg_error = true;
			break;
		}
	}
	if ( have_help || arg_error || missing_arg ) {
		goto just_return;
	}

	char msg[BUFSIZE];
	FILE *errfile;

	sprintf(msg, "./log/node_%03d.txt", idx );
	fp = fopen(msg, "w");

	sprintf(msg, "./log/node_%03d_data.txt", idx );
	fout = fopen(msg, "w");

	sprintf(msg, "./log/node_%03d_err.txt", idx );
	errfile = freopen(msg, "w", stderr);
	if(!errfile)
		perror("Fail to open the err log file");

	const struct addrinfo hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_DGRAM,
	};

	struct addrinfo *addrinfo = NULL;
	int addrinfo_retval = getaddrinfo( udp_host, NULL, &hints, &addrinfo );
	if ( 0 != addrinfo_retval ) {
		if ( EAI_SYSTEM == addrinfo_retval ) {
			perror( "getaddrinfo" );
			retval = errno;
		} else {
			printf( "getaddrinfo: %s\n", gai_strerror( addrinfo_retval ) );
			retval = EINVAL;
		}
		goto just_return;
	}

	memcpy( &udp_sockaddr, addrinfo[0].ai_addr, sizeof( struct sockaddr ) );
	udp_sockaddr.sin_port = udp_port;

	freeaddrinfo( addrinfo );
	addrinfo = NULL;

	udp_socket = socket(AF_INET,SOCK_DGRAM,0);
	if ( -1 == udp_socket ) {
		retval = errno;
		perror( "socket" );
		goto just_return;
	}

	/* initialize the communication pipes */
	snprintf( node.data_in.name, NAMESIZE, "%s:udp:%u", udp_host, udp_port );
	snprintf( node.data_out.name, NAMESIZE, "%s:udp:%u", udp_host, udp_port );

	node.data_in.pipe = udp_socket;
	node.data_out.pipe = udp_socket;

	/* start a thread to wait for incoming messages */
	if ( 0 != pthread_create( &node.data_in.thread, NULL, sim_data_in_thread, NULL ) ) {
		perror("pthread_create");
	}

	/* register the signal handler */
	signal( SIGINT, sigint_handler );
	signal( SIGTERM, sigkill_handler );

	/* set up node parameters */
	node.pid = getpid();
	node.index = 0;

	freakz_register_data_sink( (data_sink_t *) sim_pipe_data_out );

	/* jump to the main contiki loop */
	contiki_main();

	freakz_deregister_data_sink();

just_return:
	if ( 0 != retval || have_help || arg_error || missing_arg ) {
		if ( ! have_help ) {
			if ( missing_arg ) {
				printf( "option '%s' requires an argument\n", argv[ i ] );
			}
			if ( arg_error ) {
				printf( "unrecognized argument '%s'\n", argv[i] );
			}
			retval = EINVAL;
		}
		usage( argv[0] );
	}
	return retval;
}
