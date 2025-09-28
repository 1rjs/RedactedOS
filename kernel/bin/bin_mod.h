#pragma once

#include "dev/driver_base.h"
#include "process/process.h"

#ifdef __cplusplus
extern "C" {
#endif

process_t* execute(const char* prog_name, int argc, const char* argv[]);

#ifdef __cplusplus
}
#endif

extern driver_module bin_module;