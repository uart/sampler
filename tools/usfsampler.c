/*
 * Copyright (C) 2009-2011, David Eklöv
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <stdarg.h>
#include <strings.h>

#include <uart/usf.h>
#include <uart/sampler.h>

typedef struct {
    char *i_file_name;
    char *o_file_name;

    unsigned long   sample_period;
    char	   *sample_rnd;
    unsigned long   burst_period;
    char	   *burst_rnd;
    unsigned long   burst_size;
    unsigned short  line_size_lg2;
    unsigned int    random_seed;
    int             log_level;
} args_t;

#define USF_CHECK_E(_x) do {                    \
        usf_error_t error = _x;                 \
        USF_E(error);                           \
    } while (0)

#define USF_E(_e)                                       \
    if ((_e) != USF_ERROR_OK) {                         \
        fprintf(stderr, "%s\n", usf_strerror(error));   \
        goto error_out;                                 \
    }

static void
usage(char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    if (fmt)
        vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "Usage: usfsampler [OPTION...]\n");
    fprintf(stderr, "   --help,          -h             Print this\n");
    fprintf(stderr, "   --infile,        -i FILE        Input file\n");
    fprintf(stderr, "   --outfile,       -o FILE        Output file base name\n");
    fprintf(stderr, "   --sample-period, -s NUM         Average time between samples\n");
    fprintf(stderr, "   --sample-rnd,    -S STR         Random generator exp/const\n");
    fprintf(stderr, "   --burst-period,  -b NUM         Average time between bursts\n");
    fprintf(stderr, "   --burst-rnd,     -B STR         Random generator exp/const\n");
    fprintf(stderr, "   --burst-size,    -z NUM         Size of the bursts\n");
    fprintf(stderr, "   --line-size,     -l NUM         Line size\n");
    fprintf(stderr, "   --seed,          -r NUM         Random seed\n");
    fprintf(stderr, "   --verbose,       -v NUM         Verbosity\n");
}

static int
parse_args(int argc, char **argv, args_t *args)
{
    int c;
    int opt_idx = 0;

    bzero(args, sizeof(*args));
    args->line_size_lg2 = 6;
    args->sample_rnd    = "exp";
    args->burst_rnd     = "exp";

    static struct option long_opts[] = {
        {"help",           no_argument,       NULL, 'h'},
        {"infile",         required_argument, NULL, 'i'},
        {"outfile",        required_argument, NULL, 'o'},
        {"sample-period",  required_argument, NULL, 's'},
        {"sample-rnd",     required_argument, NULL, 'S'},
        {"burst-period",   required_argument, NULL, 'b'},
        {"burst-rnd",      required_argument, NULL, 'B'},
        {"burst-size",     required_argument, NULL, 'z'},
        {"line-size",      required_argument, NULL, 'l'},
        {"seed",           required_argument, NULL, 'r'},
        {"verbose",        required_argument, NULL, 'v'},

    };

    while ((c = getopt_long(argc, argv, "hi:o:s:S:b:B:z:l:r:v:",
                            long_opts, &opt_idx)) != -1) {
        switch (c) {
        case 'i':
            args->i_file_name = optarg;
            break;
        case 'o':
            args->o_file_name = optarg;
            break;
        case 's':
            args->sample_period = atol(optarg);
            break;
        case 'S':
            args->sample_rnd = optarg;
            break;
        case 'b':
            args->burst_period = atol(optarg);
            break;
        case 'B':
            args->burst_rnd = optarg;
            break;
        case 'z':
            args->burst_size = atol(optarg);
            break;
        case 'l':
            args->line_size_lg2 = log(atoi(optarg));
            break;
        case 'r':
            args->random_seed = atoi(optarg);
            break;
        case 'v':
            args->log_level = atoi(optarg);
            break;
        case 'h':
        default:
            usage(NULL);
            return 1;
        }
    }
    
    if (!args->i_file_name) {
        usage("Error: --infile must be specified.\n");
        return 1;
    }

    if (!args->o_file_name) {
        usage("Error: --outfile must be specified.\n");
        return 1;
    }

    return 0;
}

static int
my_sampler_init(sampler_t *sampler, args_t *args)
{
    int err;

    err = sampler_init(sampler);
    if (err)
        return err;

    sampler->usf_base_path   = args->o_file_name;
    sampler->sample_period   = args->sample_period;
    sampler->burst_period    = args->burst_period;
    sampler->burst_size      = args->burst_size;
    sampler->line_size_lg2   = args->line_size_lg2;
    sampler->seed            = args->random_seed;
    sampler->log_level       = args->log_level;

    srand((unsigned int)sampler->seed);

    if (!strncmp(args->burst_rnd, "const", 5)) {
        sampler->burst_rnd = sampler_rnd_const;
    } else {
        sampler->burst_rnd = sampler_rnd_exp;
    }

    if (!strncmp(args->sample_rnd, "const", 5)) {
        sampler->sample_rnd = sampler_rnd_const;
    } else {
        sampler->sample_rnd = sampler_rnd_exp;
    }

    if (!sampler->burst_size) {
        err = sampler_burst_begin(sampler, 0);
        if (err)
            return err;
    }

    return 0;
}


int
main(int argc, char **argv)
{
    args_t args;
    sampler_t sampler;
    usf_file_t *usf_i_file;
    usf_header_t *header;

    if (parse_args(argc, argv, &args))
        return 1;

    USF_CHECK_E(usf_open(&usf_i_file, args.i_file_name));
    USF_CHECK_E(usf_header((const usf_header_t **)&header, usf_i_file));

    if (!(header->flags & USF_FLAG_TRACE)) {
        fprintf(stderr, "%s: is not a trace file.\n", args.i_file_name);
        return 1;
    }

    if (my_sampler_init(&sampler, &args)) {
        fprintf(stderr, "Error initializing sampler.\n");
        return 1;
    }

    do {
        usf_error_t error;
        usf_event_t event;
        
        error = usf_read(usf_i_file, &event);
        if (error == USF_ERROR_EOF)
            break;
        USF_E(error);

        if (event.type != USF_EVENT_TRACE)
            USF_E(USF_ERROR_FILE);

        switch (event.u.trace.access.type) {
        case USF_ATYPE_RD: case USF_ATYPE_WR: case USF_ATYPE_RW: break;
        default: continue;
        }

        if (sampler_ref(&sampler, &event.u.trace.access)) {
            fprintf(stderr, "Sampler error: exiting\n");
            goto error_out;
        }
    } while (1);
// Clean return
    sampler_fini(&sampler);
    USF_CHECK_E(usf_close(usf_i_file));
    return 0;

error_out:
    sampler_fini(&sampler);
    USF_CHECK_E(usf_close(usf_i_file));
    return 1;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "k&r"
 * End:
 */
