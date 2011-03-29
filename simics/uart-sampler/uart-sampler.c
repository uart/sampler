#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <simics/api.h>
#include <simics/alloc.h>
#include <simics/utils.h>

#include "uart-sampler.h"

#define DEVICE_NAME "uart-sampler"

#define E_IF(cond, msg, ret) do {                       \
    if (cond) {                                         \
        SIM_frontend_exception(SimExc_General, msg);    \
        return ret;                                     \
    }                                                   \
} while (0)

#define E_VOID

static hap_type_t hap_burst_begin;
static hap_type_t hap_burst_end;

static void hap_cb_begin(void *not_used, conf_object_t *self);
static void hap_cb_end(void *not_used, conf_object_t *self);


static inline uint64_t
eip(conf_object_t *cpu)
{
    attr_value_t attr;

    attr = SIM_get_attribute(cpu, "eip");
    return (uint64_t)attr.u.integer;
}

static inline int
cpuid(conf_object_t *cpu)
{
    attr_value_t attr;

    attr = SIM_get_attribute(cpu, "cpuid_physical_apic_id");
    return (int)attr.u.integer;
}

static conf_object_t *
new_instance(parse_object_t *parse_obj)
{
    uart_sampler_t *s = MM_ZALLOC(1, uart_sampler_t);
    SIM_log_constructor(&s->log, parse_obj);

    return &s->log.obj;
}

static void
finalize_instance(conf_object_t *self)
{
    int err;
    uart_sampler_t *s = (uart_sampler_t *)self;
    uart_sampler_conf_t *c = (uart_sampler_conf_t *)s->conf;

    err = sampler_init(&s->sampler);
    E_IF(err, "sampler_init", E_VOID);

    s->sampler.usf_base_path = c->file_base_name;
    s->sampler.sample_period = c->sample_period;
    s->sampler.burst_period = c->burst_period;
    s->sampler.burst_size = c->burst_size;
    s->sampler.line_size_lg2 = c->line_size_lg2;
    s->sampler.seed = 0;

    if (!strncmp(c->burst_rnd_type, "const", 5)) {
        s->sampler.burst_rnd = sampler_rnd_const;
    } else {
        s->sampler.burst_rnd = sampler_rnd_exp;
    }

    if (!strncmp(c->sample_rnd_type, "const", 5)) {
        s->sampler.sample_rnd = sampler_rnd_const;
    } else {
        s->sampler.sample_rnd = sampler_rnd_exp;
    }
    
    if (!s->sampler.burst_size) {
        err = sampler_burst_begin(&s->sampler, 0);
        E_IF(err, "sampler_burst_begin", E_VOID);
    }

    if (!c->master) {
        SIM_hap_add_callback("Uart_Sampler_Burst_Begin", hap_cb_begin, s);
        SIM_hap_add_callback("Uart_Sampler_Burst_End", hap_cb_end, s);
    }
}


static int
operate_master(uart_sampler_t *s, uart_sampler_conf_t *c, usf_access_t *ref)
{
    int           err;
    unsigned long time = s->time;

    assert(time == ref->time);

    err = sampler_watchpoint_lookup(&s->sampler, ref);
    E_IF(err, "sampler_watchpoint_lookup", 0);

    if (c->burst_size) {
        if (s->burst_end == time) {
            err = sampler_burst_end(&s->sampler, time);
            E_IF(err, "sampler_burst_end", 0);

            s->burst_begin = time + 0; //XXX Should be random

            SIM_c_hap_occurred(hap_burst_end, (conf_object_t *)s, 0);
        }

        if (s->burst_begin == time) {
            err = sampler_burst_begin(&s->sampler, time);
            E_IF(err, "sampler_burst_begin", 0);

            s->next_sample = time; 
            s->burst_end   = time + c->burst_size;
            
            SIM_c_hap_occurred(hap_burst_begin, (conf_object_t *)s, 0);
        }
    }

    if (sampler_burst_active(&s->sampler) && s->next_sample == time) {
        err = sampler_watchpoint_insert(&s->sampler, ref);
        E_IF(err, "sampler_watchpoint_insert", 0);

        s->next_sample = time + 1; //XXX Should be random
    }

    return 0;
}


static void
hap_cb_begin(void *self, conf_object_t *obj)
{
    int err;
    uart_sampler_t *s = (uart_sampler_t *)self;

    err = sampler_burst_begin(&s->sampler, s->time);
    E_IF(err, "sampler_burst_begin", E_VOID);

    s->next_sample = s->time;
}

static void
hap_cb_end(void *self, conf_object_t *obj)
{
    int err;
    uart_sampler_t *s = (uart_sampler_t *)self;

    err = sampler_burst_end(&s->sampler, s->time);
    E_IF(err, "sampler_burst_end", E_VOID);
}

static int
operate_slave(uart_sampler_t *s, uart_sampler_conf_t *c, usf_access_t *ref)
{
    int           err;

    err = sampler_watchpoint_lookup(&s->sampler, ref);
    E_IF(err, "sampler_watchpoint_lookup", 0);

    if (sampler_burst_active(&s->sampler) && s->next_sample == s->time) {
        err = sampler_watchpoint_insert(&s->sampler, ref);
        E_IF(err, "sampler_burst_active", 0);

        s->next_sample = s->time + 1; //XXX
    }
    return 0;
}


static cycles_t
operate(conf_object_t         *self,
        conf_object_t         *mem_space,
        map_list_t            *map_list,
        generic_transaction_t *mem_op)
{
    uart_sampler_t *s = (uart_sampler_t *)self;
    uart_sampler_conf_t *c = (uart_sampler_conf_t *)s->conf;
    usf_access_t ref;

    if (!s->active)
        return 0;

    if (SIM_mem_op_is_prefetch(mem_op)) {
        SIM_log_info(4, &s->log, 0, "Ignoring prefetch");
        return 0;
    }
    if (SIM_mem_op_is_control(mem_op)) {
        SIM_log_info(4, &s->log, 0, "Ignoring control");
        return 0;
    }
    assert(SIM_mem_op_is_data(mem_op));
    assert(SIM_mem_op_is_from_cpu(mem_op));
   
    ref.pc   = eip((mem_op)->ini_ptr);
    ref.addr = mem_op->physical_address;
    ref.time = s->time;
    ref.tid  = cpuid((mem_op)->ini_ptr);
    ref.len  = mem_op->size;
    ref.type = SIM_mem_op_is_read(mem_op) ? USF_ATYPE_RD : USF_ATYPE_WR;
 
    if (c->master)
        operate_master(s, c, &ref);
    else
        operate_slave(s, c, &ref);

    s->time++;
    return 0;
}

static attr_value_t
get_conf(void          *arg,
         conf_object_t *self,
         attr_value_t  *idx)
{
    uart_sampler_t *s = (uart_sampler_t *)self;
    return SIM_make_attr_object(((uart_sampler_t *)s)->conf);
}

static set_error_t
set_conf(void          *arg,
         conf_object_t *self,
         attr_value_t  *val,
         attr_value_t  *idx)
{
    uart_sampler_t *s = (uart_sampler_t *)self;
    s->conf = val->u.object;
    return Sim_Set_Ok;
}

static attr_value_t
get_start(void          *arg,
          conf_object_t *self,
          attr_value_t  *idx)
{
    ((uart_sampler_t *)self)->active = 1;
    return SIM_make_attr_nil();
}

static attr_value_t
get_stop(void          *arg,
         conf_object_t *self,
         attr_value_t  *idx)
{
    ((uart_sampler_t *)self)->active = 0;
    return SIM_make_attr_nil();
}

static attr_value_t
get_close(void          *arg,
          conf_object_t *self,
          attr_value_t  *idx)
{
    uart_sampler_t *s = (uart_sampler_t *)self;

    s->active = 0;
    if (sampler_fini(&s->sampler)) {
        SIM_frontend_exception(SimExc_General,
                               "Error flusing the dangling samples.");
    }
    return SIM_make_attr_nil();
}

static set_error_t
set_null(void          *arg,
         conf_object_t *self,
         attr_value_t  *val,
         attr_value_t  *idx)
{
    return Sim_Set_Ok;
}

void
init_local(void)
{
    static class_data_t funcs;
    static conf_class_t *class;
    static timing_model_interface_t ifc;

    memset(&funcs, 0, sizeof(class_data_t));
    funcs.new_instance = new_instance;
    funcs.finalize_instance = finalize_instance;
    funcs.description = "";
    class = SIM_register_class(DEVICE_NAME, &funcs);

    ifc.operate = operate;
    SIM_register_interface(class, "timing-model", &ifc);

    SIM_register_typed_attribute(class, "conf",
                                 get_conf, NULL,
                                 set_conf, NULL,
                                 Sim_Attr_Required,
                                 "o", NULL,
                                 "XXX: doc");
 
    SIM_register_typed_attribute(class, "start",
                                 get_start, NULL,
                                 set_null,  NULL,
                                 Sim_Attr_Optional,
                                 NULL, NULL,
                                 "start sampling");

    SIM_register_typed_attribute(class, "stop",
                                 get_stop, NULL,
                                 set_null, NULL,
                                 Sim_Attr_Optional,
                                 NULL, NULL,
                                 "stop sampling");

    SIM_register_typed_attribute(class, "close",
                                 get_close, NULL,
                                 set_null,  NULL,
                                 Sim_Attr_Pseudo,
                                 NULL, NULL,
                                 "close the usf files");

    hap_burst_begin = SIM_hap_add_type("Uart_Sampler_Burst_Begin",
                                       "I", "start_time", NULL, "XXX", 0);
    hap_burst_end   = SIM_hap_add_type("Uart_Sampler_Burst_End",
                                       "I", "start_time", NULL, "XXX", 0);

    conf_init_local();
}
