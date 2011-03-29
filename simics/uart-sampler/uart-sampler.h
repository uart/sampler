#ifndef UART_SAMPLER_H
#define UART_SAMPLER_H
#include <uart/sampler.h>

#define FILE_BASE_NAME_LEN  256
#define RND_TYPE_NAME_LEN   32

typedef struct {
    log_object_t   log;
    char           file_base_name[FILE_BASE_NAME_LEN];

    unsigned long  sample_period;
    char           sample_rnd_type[RND_TYPE_NAME_LEN];
    
    unsigned long  burst_period;
    char           burst_rnd_type[RND_TYPE_NAME_LEN];
    
    unsigned long  burst_size;
    unsigned short line_size_lg2;

    int            master;
} uart_sampler_conf_t;

typedef struct {
    log_object_t   log;

    unsigned long  time;
    conf_object_t *conf;
    int            active;

    sampler_t      sampler;
    unsigned long  burst_begin;
    unsigned long  burst_end;
    unsigned long  next_sample;
} uart_sampler_t;

void conf_init_local(void);

#endif /* UART_SAMPLER_H */
