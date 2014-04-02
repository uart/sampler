/*
 * Copyright (C) 2009-2011, Andreas Sandberg
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <stdlib.h>

#include "pin.H"
#include <uart/sampler.h>

using namespace std;

KNOB<string> knob_smp_base(KNOB_MODE_WRITEONCE, "pintool", "o", "sample",
			   "Output file base name.");

KNOB<unsigned long> knob_smp_period(KNOB_MODE_WRITEONCE, "pintool", "p", "100000",
				    "Average time between samples");

KNOB<string> knob_smp_rnd(KNOB_MODE_WRITEONCE, "pintool", "r", "exp",
			  "Random generator exp/const");

KNOB<unsigned long> knob_burst_period(KNOB_MODE_WRITEONCE, "pintool", "P", "0",
				      "Average time between bursts");

KNOB<string> knob_burst_rnd(KNOB_MODE_WRITEONCE, "pintool", "R", "exp",
			    "Random generator exp/const");

KNOB<unsigned long> knob_burst_size(KNOB_MODE_WRITEONCE, "pintool", "b", "0",
				    "Size of bursts");

KNOB<unsigned> knob_smp_line_size_lg2(KNOB_MODE_WRITEONCE, "pintool", "l", "6",
				      "Line size (log 2)");

KNOB<unsigned> knob_seed(KNOB_MODE_WRITEONCE, "pintool", "s", "0",
			 "Random seed");

KNOB<int> knob_log_level(KNOB_MODE_WRITEONCE, "pintool", "v", "0",
			 "Log level");


sampler_t sampler;
usf_atime_t access_counter = 0;


static VOID
trace_mem(ADDRINT ip, ADDRINT addr, UINT32 size, THREADID tid, UINT32 ref_type)
{
    usf_access_t access = {
	(usf_addr_t)ip,
	(usf_addr_t)addr,
	access_counter,
	(usf_tid_t) tid,
	(usf_alen_t) size,
	(usf_atype_t)ref_type
    };

    sampler_ref(&sampler, &access);
}

static VOID PIN_FAST_ANALYSIS_CALL
trace_update_access_counter(THREADID tid)
{
    ++access_counter;
}

static VOID
instrument(INS ins, VOID *v)
{
    BOOL rd = INS_IsMemoryRead(ins);
    BOOL wr = INS_IsMemoryWrite(ins);

    if (!rd && !wr)
	return;

    UINT32 no_ops = INS_MemoryOperandCount(ins);

    for (UINT32 op = 0; op < no_ops; op++) {
        const UINT32 size = INS_MemoryOperandSize(ins, op);
	const bool is_rd = INS_MemoryOperandIsRead(ins, op);
	const bool is_wr = INS_MemoryOperandIsWritten(ins, op);
	const UINT32 atype =
	    is_rd && is_wr ? USF_ATYPE_RW :
	    (is_wr ? USF_ATYPE_WR : USF_ATYPE_RD);

	INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)trace_mem,
		       IARG_INST_PTR,
		       IARG_MEMORYOP_EA, op,
		       IARG_UINT32, size,
		       IARG_THREAD_ID,
		       IARG_UINT32, atype,
		       IARG_END); 

	INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)trace_update_access_counter,
				 IARG_FAST_ANALYSIS_CALL,
				 IARG_THREAD_ID,
				 IARG_END);
    }
}

static int
init()
{
    if (sampler_init(&sampler))
        return 1;

    sampler.usf_base_path   = (char *)knob_smp_base.Value().c_str();
    sampler.sample_period   = knob_smp_period;
    sampler.burst_period    = knob_burst_period;
    sampler.burst_size      = knob_burst_size;
    sampler.line_size_lg2   = knob_smp_line_size_lg2;
    sampler.seed            = knob_seed;
    sampler.log_level       = knob_log_level;

    if (knob_burst_rnd.Value() == "const")
        sampler.burst_rnd = sampler_rnd_const;
    else if (knob_burst_rnd.Value() == "exp")
        sampler.burst_rnd = sampler_rnd_exp;
    else {
	cerr << "Illegal burst random generator specified." << endl;
	return 1;
    }

    if (knob_smp_rnd.Value() == "const")
        sampler.sample_rnd = sampler_rnd_const;
    else if (knob_smp_rnd.Value() == "exp")
        sampler.sample_rnd = sampler_rnd_exp;
    else {
	cerr << "Illegal burst random generator specified." << endl;
	return 1;
    }

    if (!sampler.burst_size)
        if (sampler_burst_begin(&sampler, 0))
            return 1;

    return 0;
}

static VOID
fini(INT32 code, VOID *v)
{
    sampler_fini(&sampler);
}

static void
usage()
{
    cerr << "This tool samples an application's memory accesses." << endl
	 << endl;
    cerr << KNOB_BASE::StringKnobSummary();
    cerr << endl;
}

int
main(int argc, char *argv[])
{
    if (PIN_Init(argc, argv)) {
	usage();
        return 1;
    }

    if (init())
	return 1;

    INS_AddInstrumentFunction(instrument, 0);
    PIN_AddFiniFunction(fini, 0);

    PIN_StartProgram();
    return 0;
}
