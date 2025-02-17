/// @file tm.cpu.h

#pragma once
#include <tm.common.h>

/* Public Constants ***********************************************************/

#define TM_ERROR_STRLEN 512

/* Typedefs and Forward Declarations ******************************************/

typedef struct tm_cpu tm_cpu_t;
typedef bool (*tm_bus_read)     (addr_t, long_t*);
typedef bool (*tm_bus_write)    (addr_t, long_t);
typedef bool (*tm_cycle)        ();

/* Public Functions ***********************************************************/

tm_cpu_t* tm_create_cpu (tm_bus_read p_read, tm_bus_write p_write, tm_cycle p_cycle);
void tm_init_cpu (tm_cpu_t* p_cpu);
void tm_destroy_cpu (tm_cpu_t* p_cpu);

/* Public Functions - CPU Registers *******************************************/

bool tm_read_cpu_register (tm_cpu_t* p_cpu, enum_t p_type, long_t* p_value);
bool tm_write_cpu_register (tm_cpu_t* p_cpu, enum_t p_type, long_t p_value);

/* Public Functions - Bus Read ************************************************/

bool tm_read_byte (tm_cpu_t* p_cpu, addr_t p_address, long_t* p_value);
bool tm_read_word (tm_cpu_t* p_cpu, addr_t p_address, long_t* p_value);
bool tm_read_long (tm_cpu_t* p_cpu, addr_t p_address, long_t* p_value);

/* Public Functions - Bus Write ***********************************************/

bool tm_write_byte (tm_cpu_t* p_cpu, addr_t p_address, long_t p_value);
bool tm_write_word (tm_cpu_t* p_cpu, addr_t p_address, long_t p_value);
bool tm_write_long (tm_cpu_t* p_cpu, addr_t p_address, long_t p_value);

/* Public Functions - Interrupts **********************************************/

void tm_request_interrupt (tm_cpu_t* p_cpu, byte_t p_id);

/* Public Functions - Cycle and Step ******************************************/

bool tm_cycle_cpu (tm_cpu_t* p_cpu, size_t p_cycle_count);
bool tm_advance_cpu (tm_cpu_t* p_cpu, size_t p_cycle_count);
bool tm_step_cpu (tm_cpu_t* p_cpu);

/* Public Functions - Error Checking ******************************************/

bool tm_has_error (tm_cpu_t* p_cpu);
const char* tm_get_error (tm_cpu_t* p_cpu);
