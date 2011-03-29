#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <simics/api.h>
#include <simics/alloc.h>
#include <simics/utils.h>

#include "uart-sampler.h"

#define DEVICE_NAME "uart-sampler-conf"

static conf_object_t *
new_instance(parse_object_t *parse_obj)
{
    uart_sampler_conf_t *c = MM_ZALLOC(1, uart_sampler_conf_t);
    SIM_log_constructor(&c->log, parse_obj);

    return &c->log.obj;
}

static void
finalize_instance(conf_object_t *self)
{
}

#define GETSET(_name, _type)                                        \
    static attr_value_t                                             \
    get_##_name(void          *arg,                                 \
                conf_object_t *self,                                \
                attr_value_t  *idx)                                 \
    {                                                               \
        uart_sampler_conf_t *conf = (uart_sampler_conf_t *)self;    \
        return SIM_make_attr_##_type(conf->_name);                  \
    }                                                               \
    static set_error_t                                              \
    set_##_name(void          *arg,                                 \
                conf_object_t *self,                                \
                attr_value_t  *val,                                 \
                attr_value_t  *idx)                                 \
    {                                                               \
        uart_sampler_conf_t *conf = (uart_sampler_conf_t *)self;    \
        conf->_name = val->u._type;                                 \
        return Sim_Set_Ok;                                          \
    }

GETSET(sample_period,  integer)
GETSET(burst_period,   integer)
GETSET(burst_size,     integer)
GETSET(line_size_lg2,  integer)
GETSET(master,         integer)


#define GETSET_STR(_name, _len)                                     \
    static attr_value_t                                             \
    get_##_name(void          *arg,                                 \
                conf_object_t *self,                                \
                attr_value_t  *idx)                                 \
    {                                                               \
        uart_sampler_conf_t *conf = (uart_sampler_conf_t *)self;    \
        return SIM_make_attr_string(conf->_name);                   \
    }                                                               \
    static set_error_t                                              \
    set_##_name(void          *arg,                                 \
                conf_object_t *self,                                \
                attr_value_t  *val,                                 \
                attr_value_t  *idx)                                 \
    {                                                               \
        uart_sampler_conf_t *conf = (uart_sampler_conf_t *)self;    \
        strncpy(conf->_name, val->u.string, _len);                  \
        return Sim_Set_Ok;                                          \
    }

GETSET_STR(file_base_name,  FILE_BASE_NAME_LEN)
GETSET_STR(sample_rnd_type, RND_TYPE_NAME_LEN)
GETSET_STR(burst_rnd_type,  RND_TYPE_NAME_LEN)

void
conf_init_local(void)
{
    class_data_t funcs;
    conf_class_t *class;

    memset(&funcs, 0, sizeof(class_data_t));
    funcs.new_instance = new_instance;
    funcs.finalize_instance = finalize_instance;
    funcs.description = "";
    class = SIM_register_class(DEVICE_NAME, &funcs);

#define REGISTER(_name, _type, _doc)                    \
    SIM_register_typed_attribute(class, #_name,         \
                                 get_##_name, NULL,     \
                                 set_##_name, NULL,     \
                                 Sim_Attr_Optional,     \
                                 _type, NULL,           \
                                 _doc)                  \

    REGISTER(sample_period,   "i", "XXX");
    REGISTER(burst_period,    "i", "XXX");
    REGISTER(burst_size,      "i", "XXX");
    REGISTER(line_size_lg2,   "i", "XXX");
    REGISTER(master,          "b", "XXX");
    REGISTER(file_base_name,  "s", "XXX");
    REGISTER(sample_rnd_type, "s", "XXX");
    REGISTER(burst_rnd_type,  "s", "XXX");
}
