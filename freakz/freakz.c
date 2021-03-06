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
    Please post support questions to the FreakLabs forum.

*******************************************************************/
/*!
    \file freakz.c
    \ingroup misc
    \brief Main FreakZ file - Does nothing

    Init the stack.
*/
/**************************************************************************/
#include "freakz.h"
#include "test_app.h"

enum {
	_mac_rx,
	_af_tx,
	_af_rx,
	_af_conf,
	_drvr_conf,
	_ed_bind_req,
	_ed_bind_match,
	_unbind_resp,
};

#define DECL_WEAK_PET(x) process_event_t event##x __attribute__((weak)) = x

DECL_WEAK_PET(_mac_rx);
DECL_WEAK_PET(_af_tx);
DECL_WEAK_PET(_af_rx);
DECL_WEAK_PET(_af_conf);
DECL_WEAK_PET(_drvr_conf);
DECL_WEAK_PET(_ed_bind_req);
DECL_WEAK_PET(_ed_bind_match);
DECL_WEAK_PET(_unbind_resp);

FILE *fp __attribute__((weak)) = NULL;

sim_node_t *node_get() __attribute__((weak));
sim_node_t *node_get() {
	DBG_PRINT("WEAKLY DEFINED node_get()!!!\n");
	return NULL;
}

void sim_pipe_cmd_out(U8 *data, U8 len) __attribute__((weak));
void sim_pipe_cmd_out(U8 *data, U8 len) {
	DBG_PRINT("WEAKLY DEFINED sim_pipe_cmd_out()!!!\n");
}
void test_app_init() __attribute__((weak));
void test_app_init() {
	DBG_PRINT("IN WEAKLY DEFINED test_app_init()!!!\n");
}

/* Dummy function that just initializes everybody. */
void freakz_init()
{
	drvr_init();
	mmem_init();
	ctimer_init();
	mac_init();
	nwk_init();
	aps_init();
	af_init();
	zdo_init();
	buf_init();
	slow_clock_init();
	test_app_init();
}


