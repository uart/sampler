from cli import *
from cli_impl import new_object_name

def new_uart_sampler_conf_cmd(name,
                              file_base_name,
                              sample_period,
                              sample_rnd_type,
                              burst_period,
                              burst_rnd_type,
                              burst_size,
                              line_size_lg2,
                              master):
    real_name = new_object_name(name, "sampler-conf")
    if real_name == None:
        print "An object called '%s' already exists." % name
        SIM_command_has_problem()
        return
    else:
        name = real_name

    if not sample_rnd_type:
        sample_rnd_type = "exp"
    if not burst_rnd_type:
        burst_rnd_type = "exp"

    conf = SIM_create_object("uart-sampler-conf", name, [])
    conf.file_base_name = file_base_name
    conf.sample_period = sample_period
    conf.sample_rnd_type = sample_rnd_type
    conf.burst_period = burst_period
    conf.burst_rnd_type = burst_rnd_type
    conf.burst_size = burst_size
    conf.line_size_lg2 = line_size_lg2
    conf.master = master
    return (conf,)

new_command("new-uart-sampler-conf", new_uart_sampler_conf_cmd,
            [arg(str_t, "name", "?", None),
             arg(str_t, "file_base_name"),
             arg(int_t, "sample_period", "?", 0),
             arg(str_t, "sample_rnd_type", "?", None),
             arg(int_t, "burst_period",  "?", 0),
             arg(str_t, "burst_rnd_type",  "?", None),
             arg(int_t, "burst_size",    "?", 0),
             arg(int_t, "line_size_lg2", "?", 6),
             arg(int_t, "master", "?", 1)],
            type = "",
            see_also = [],
            short = "create new uart-sampler-conf",
            doc = "")

def new_uart_sampler_cmd(name, conf):
    real_name = new_object_name(name, "sampler-conf")
    if real_name == None:
        print "An object called '%s' already exists." % name
        SIM_command_has_problem()
        return
    else:
        name = real_name

    SIM_create_object("uart-sampler", name, [["conf", conf]])

new_command("new-uart-sampler", new_uart_sampler_cmd,
            [arg(str_t, "name", "?", None),
             arg(obj_t("uart-sampler-conf"), "conf")],
            type = "",
            see_also = [],
            short = "create new uart-sampler",
            doc = "")

