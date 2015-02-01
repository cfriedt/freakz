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
#include <arpa/inet.h>

#include "test_sim.h"
#include "test_app.h"
#include "contiki-main.h"
#include "freakz.h"
#include "sim_drvr.h"
#include "test_app.h"

#define DEFAULT_UDP_HOST "localhost"
#define DEFAULT_UDP_PORT 52001

#define D( fmt, args... ) \
	do { \
		fprintf( stderr, "D/ %s():%d: " fmt "\n", __FUNCTION__, __LINE__, ##args ); \
		fflush( stderr ); \
	} while( 0 )

#define E( fmt, args... ) \
	do { \
		if ( errno ) { \
			fprintf( stderr, "E/ %s():%d: %s: " fmt "\n", __FUNCTION__, __LINE__, strerror(errno), ##args ); \
		} else { \
			fprintf( stderr, "E/ %s():%d: " fmt "\n", __FUNCTION__, __LINE__, ##args ); \
		} \
		fflush( stderr ); \
	} while( 0 )

extern process_event_t event_data_in;
extern process_event_t event_cmd_rcvd;

static sim_node_t node;             // node struct that holds info related to node communications
static struct pipe_t pp;                   // public pipe used to initially communicate with sim shell
static char cmd[BUFSIZE];

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

	D( "entering rx loop..." );

	for( ;; ) {
		// XXX: use pselect!
		D( "calling recvfrom" );
		recvfrom_r = recvfrom( udp_socket, msg, BUFSIZE, 0, (struct sockaddr *) &udp_sockaddr, (socklen_t *) &udp_sockaddr.sin_len );
		if ( -1 == recvfrom_r ) {
			E("recvfrom");
		} else {
			/*
			 * write the received data into the input buffer
			 * of the stack and trigger the isr
			 */
			D( "received buffer '%s'", msg );
			drvr_write_rx_buf( (U8 *)msg, recvfrom_r );
			D( "calling rx_isr()" );
			drvr_rx_isr();
		}
	}
	return NULL;
}

/*
 * This thread is used to monitor the cmd_in pipe for incoming
 * commands issued from the simulator shell. When the commands
 * appear, it will save them in a static variable and post an
 * event to the zdo cmd process with a pointer to the cmd string.
 */
static void *sim_cmd_in_thread(void *dummy)
{
	while (1)
	{
		if (read(node.cmd_in.pipe, cmd, sizeof(cmd)) == -1)
			perror("sim_cmd_in_thread");
		test_app_parse(cmd);
	}
	return NULL;
}

static char data_out_buf[256];
static char *u8array_to_string( const char *data, uint8_t len ) {

}

/* Send the tx to the data_out pipe */
void sim_pipe_data_out(U8 *data, U8 len) {
	int sendto_r = -1;

	D( "calling sendto..." );

	sendto_r = sendto( udp_socket, data, len, 0, (struct sockaddr *) &udp_sockaddr, (socklen_t) udp_sockaddr.sin_len );
	if ( -1 == sendto_r ) {
		E("sim_data_in_thread");
	} else {
		//D( "sending message '%s'", data );
	}
}

/* Send the cmd to the cmd_out pipe */
void sim_pipe_cmd_out(U8 *data, U8 len)
{
	if (write(node.cmd_out.pipe, data, len) == -1)
		perror("sim_pipe_cmd_out");
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
	close(node.cmd_in.pipe);
	close(node.cmd_out.pipe);
	close(pp.pipe);
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

	D( "checking arguments..." );
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

	strcpy(pp.name, "/tmp/PUBLIC");
	D( "opening public fifo %s...", pp.name );
	pp.pipe = open(pp.name, O_WRONLY);
	if (pp.pipe == -1) {
		E("open(%s)", pp.name);
	}

	/* sending pid to public fifo */
	sprintf(msg, "%d", getpid());
	D("sending pid to public fifo");
	if (write(pp.pipe, msg, strlen(msg) + 1) == -1) {
		E("write");
	}

//	sprintf(msg, "/tmp/node_%03d.txt", idx );
//	D( "opening %s...", msg );
//	fp = fopen(msg, "w");
//	if ( NULL == fp ) {
//		E("fopen(%s)", msg);
//	}
	fp = stdout;

//	sprintf(msg, "/tmp/node_%03d_data.txt", idx );
//	D( "opening %s...", msg );
//	fout = fopen(msg, "w");
//	if ( NULL == fout ) {
//		E("fopen(%s)", msg);
//	}
	fout = stdout;

//	sprintf(msg, "/tmp/node_%03d_err.txt", idx );
//	D( "opening %s...", msg );
//	errfile = freopen(msg, "w", stderr);
//	if ( NULL == errfile ) {
//		E("freopen(%s, stderr)", msg);
//	}
	errfile = stderr;

	const struct addrinfo hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_DGRAM,
	};

	D( "getaddrinfo( %s )", udp_host );
	struct addrinfo *addrinfo = NULL;
	int addrinfo_retval = getaddrinfo( udp_host, NULL, &hints, &addrinfo );
	if ( 0 != addrinfo_retval ) {
		if ( EAI_SYSTEM == addrinfo_retval ) {
			retval = errno;
			E( "getaddrinfo" );
		} else {
			E( "getaddrinfo: %s\n", gai_strerror( addrinfo_retval ) );
			retval = EINVAL;
		}
		goto just_return;
	}

	D( "setting port to %d", udp_port );
	memcpy( &udp_sockaddr, addrinfo[0].ai_addr, sizeof( struct sockaddr ) );
	udp_sockaddr.sin_port = htons(udp_port);

	D( "freeaddrinfo()" );
	freeaddrinfo( addrinfo );
	addrinfo = NULL;

	D( "opening UDP socket" );
	udp_socket = socket(AF_INET,SOCK_DGRAM,0);
	if ( -1 == udp_socket ) {
		retval = errno;
		E( "socket" );
		goto just_return;
	}
	D( "opened UDP socket as fd %d", udp_socket );

	/* initialize the communication pipes */
	snprintf( node.data_in.name, NAMESIZE, "%s:udp:%u", udp_host, udp_port );
	snprintf( node.data_out.name, NAMESIZE, "%s:udp:%u", udp_host, udp_port );

	node.data_in.pipe = udp_socket;
	node.data_out.pipe = udp_socket;

//	sprintf(node.cmd_in.name, "/tmp/fifo_cmd_in_%d", getpid());
	sprintf(node.cmd_in.name, "stdin");
//	D( "making fifo '%s'", node.cmd_in.name );
//	int mkfifo_r = mkfifo( node.cmd_in.name, S_IFIFO | S_IRWXU | S_IRWXG );
//	if ( -1 == mkfifo_r ) {
//		E("mkfifo");
//		goto close_socket;
//	}
	D( "opening cmd_in '%s'", node.cmd_in.name );
//	if ((node.cmd_in.pipe   = open(node.cmd_in.name,    O_RDONLY)) == -1) {
//		E("open cmd_in pipe");
//		goto close_socket;
//	}
	node.cmd_in.pipe = fileno( stdin );

//	sprintf(node.cmd_out.name, "/tmp/fifo_cmd_out_%d", getpid());
	sprintf(node.cmd_out.name, "stdout");
//	D( "making fifo '%s'", node.cmd_out.name );
//	mkfifo_r = mkfifo( node.cmd_out.name, S_IFIFO | S_IRWXU | S_IRWXG );
//	if ( -1 == mkfifo_r ) {
//		E("mkfifo");
//		goto close_socket;
//	}
	D( "opening cmd_out '%s'", node.cmd_out.name );
//	if ( -1 == (node.cmd_out.pipe = open(node.cmd_out.name, O_WRONLY)) ) {
//		E("open cmd_out pipe");
//		goto close_socket;
//	}
	node.cmd_out.pipe = fileno( stdout );

	D( "starting data_in thread..." );
	/* start a thread to wait for incoming messages */
	if ( 0 != pthread_create( &node.data_in.thread, NULL, sim_data_in_thread, NULL ) ) {
		E("pthread_create");
		goto close_socket;
	}

	D("starting cmd_in thread");
	if (pthread_create(&node.cmd_in.thread, NULL, sim_cmd_in_thread, NULL) > 0) {
		E("pthread_crate");
		goto close_socket;
	}

	D("registering signal handlers for SIGINT and SIGTERM");
	/* register the signal handler */
//	signal( SIGINT, sigint_handler );
//	signal( SIGTERM, sigkill_handler );

	/* set up node parameters */
	node.pid = getpid();
	node.index = 0;

	/* jump to the main contiki loop */
	sprintf(msg, "node %d added\n", node.index);
	format_cmd_str((U8 *)msg);
	sim_pipe_cmd_out((U8 *)msg, strlen(msg) + 1);

	D( "calling contiki_main()" );
	/* jump to the main contiki loop */
	contiki_main();

close_socket:
	D( "closing socket %d", udp_socket );
	close( udp_socket );
	udp_socket = -1;

just_return:
	if ( 0 != retval || have_help || arg_error || missing_arg ) {
		if ( ! have_help ) {
			if ( missing_arg ) {
				E( "option '%s' requires an argument\n", argv[ i ] );
			}
			if ( arg_error ) {
				E( "unrecognized argument '%s'\n", argv[i] );
			}
			retval = EINVAL;
		}
		usage( argv[0] );
	}
	D( "returning %d", retval );
	return retval;
}
