/// @file tm.common.h

#pragma once

/* Include Files **************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <errno.h>

/* Helper Macros - Shared Library Import/Export *******************************/

#define tm_api

/* Helper Macros - Logging ****************************************************/

#define tm_printf(...) fprintf(stdout, __VA_ARGS__)
#define tm_errorf(...) fprintf(stderr, __VA_ARGS__)
#define tm_perror(msg) perror(msg)
#define tm_perrorf(...) \
{ \
    int _errno = errno; \
    tm_errorf(__VA_ARGS__); \
    tm_errorf(": %s\n", strerror(_errno)); \
}
#define tm_trace() \
    tm_errorf(" - in function '%s'.\n", __FUNCTION__); \
    tm_errorf(" - in file '%s:%d'.\n", __FILE__, __LINE__)

/* Helper Macros - Assertions *************************************************/

#define tm_die() exit(EXIT_FAILURE)
#define tm_hard_assert(clause) assert(clause)
#define tm_assert(clause) \
    if (!(clause)) \
    { \
        tm_errorf("assertion failure: '%s'!\n", #clause); \
        tm_trace(); \
        tm_die(); \
    }
#define tm_expect(clause, ...) \
    if (!(clause)) \
    { \
        tm_errorf(__VA_ARGS__); \
        tm_trace(); \
        tm_die(); \
    }
#define tm_expect_p(clause, ...) \
    if (!(clause)) \
    { \
        tm_perrorf(__VA_ARGS__); \
        tm_trace(); \
        tm_die(); \
    }

/* Helper Macros - Bitwise Macros *********************************************/

#define tm_check_bit(value, bit) \
    ((value >> bit) & 0b1)
#define tm_check_nibble(value, nibble) \
    ((value >> (nibble * 4)) & 0xF)
#define tm_check_byte(value, byte) \
    ((value >> (byte * 8)) & 0xFF)
#define tm_set_bit(value, bit, on) \
    if (on) { value = (value | (1 << bit)); } \
    else { value = (value & ~(1 << bit)); }

/* Helper Macros - Allocation Macros ******************************************/

#define nullptr NULL
#define tm_malloc(count, type) \
    (type*) malloc(count * sizeof(type))
#define tm_realloc(ptr, count, type) \
    (type*) realloc(ptr, count * sizeof(type))
#define tm_calloc(count, type) \
    (type*) calloc(count, sizeof(type))
#define tm_free(ptr) \
    if (ptr != nullptr) { free(ptr); ptr = nullptr; }

/* Helper Macros - Memory Map Constants ***************************************/

#define TM_ROM_START            0x00000000
#define TM_ROM_END              0x7FFFFFFF
#define TM_ROM_SIZE             0x80000000
#define TM_METADATA_START       0x00000000
#define TM_METADATA_END         0x00000FFF
#define TM_METADATA_SIZE        0x00001000
#define TM_RST_START            0x00001000
#define TM_RST_END              0x00001FFF
#define TM_RST_SIZE             0x00001000
#define TM_INT_START            0x00002000
#define TM_INT_END              0x00002FFF
#define TM_INT_SIZE             0x00001000
#define TM_PROGRAM_START        0x00003000
#define TM_PROGRAM_END          0x7FFFFFFF
#define TM_PROGRAM_SIZE         0x7FFFD000
#define TM_RAM_START            0x80000000
#define TM_RAM_END              0xFFFCFFFF
#define TM_RAM_SIZE             0x7FFD0000
#define TM_XRAM_START           0xC0000000
#define TM_XRAM_END             0xFFFCFFFF
#define TM_XRAM_SIZE            0x3FFD0000
#define TM_STACK_START          0xFFFD0000
#define TM_STACK_END            0xFFFDFFFF
#define TM_STACK_SIZE           0x00010000
#define TM_CALL_STACK_START     0xFFFE0000
#define TM_CALL_STACK_END       0xFFFEFFFF
#define TM_CALL_STACK_SIZE      0x00010000
#define TM_QRAM_START           0xFFFF0000
#define TM_QRAM_END             0xFFFFFFFF
#define TM_QRAM_SIZE            0x00010000
#define TM_IO_START             0xFFFFFF00
#define TM_IO_END               0xFFFFFFFF
#define TM_IO_SIZE              0x00000100

/* Helper Macros - Metadata Constants *****************************************/

#define TM_MAGIC_NUMBER             0x38304D54      // `TM08`
#define TM_MAGIC_NUMBER_ADDRESS     0x00000000
#define TM_PROGRAM_NAME_ADDRESS     0x00000004
#define TM_PROGRAM_NAME_SIZE        123             // Program name in bytes, not including null terminator.
#define TM_PROGRAM_AUTHOR_ADDRESS   0x00000080
#define TM_PROGRAM_AUTHOR_SIZE      127             // Program author string in bytes, not including null terminator.
#define TM_PROGRAM_ROM_SIZE_ADDRESS 0x00000160      // Program's expected ROM size, in bytes.

/* Common Typedefs ************************************************************/

typedef int         enum_t;
typedef size_t      handle_t;
typedef uint8_t     byte_t;
typedef uint16_t    word_t;
typedef uint32_t    long_t;
typedef uint32_t    addr_t;

/* Common Enumerations ********************************************************/

enum tm_cpu_register_type
{
    TM_REGISTER_A   = 0b0000,
    TM_REGISTER_AW  = 0b0001,
    TM_REGISTER_AH  = 0b0010,
    TM_REGISTER_AL  = 0b0011,
    TM_REGISTER_B   = 0b0100,
    TM_REGISTER_BW  = 0b0101,
    TM_REGISTER_BH  = 0b0110,
    TM_REGISTER_BL  = 0b0111,
    TM_REGISTER_C   = 0b1000,
    TM_REGISTER_CW  = 0b1001,
    TM_REGISTER_CH  = 0b1010,
    TM_REGISTER_CL  = 0b1011,
    TM_REGISTER_D   = 0b1100,
    TM_REGISTER_DW  = 0b1101,
    TM_REGISTER_DH  = 0b1110,
    TM_REGISTER_DL  = 0b1111,
};

enum tm_cpu_flag_type
{
    TM_FLAG_Z,
    TM_FLAG_N,
    TM_FLAG_H,
    TM_FLAG_C,
    TM_FLAG_O,
    TM_FLAG_U,
    TM_FLAG_L,
    TM_FLAG_S,
};

enum tm_cpu_condition_type
{
    TM_CONDITION_N,
    TM_CONDITION_CS,
    TM_CONDITION_CC,
    TM_CONDITION_ZS,
    TM_CONDITION_ZC,
    TM_CONDITION_OS,
    TM_CONDITION_US,
};

enum tm_cpu_instruction_type
{
    TM_INSTRUCTION_NOP = 0x00,
    TM_INSTRUCTION_STOP = 0x01,
    TM_INSTRUCTION_HALT = 0x02,
    TM_INSTRUCTION_SEC = 0x03,
    TM_INSTRUCTION_CEC = 0x04,
    TM_INSTRUCTION_DI = 0x05,
    TM_INSTRUCTION_EI = 0x06,
    TM_INSTRUCTION_DAA = 0x07,
    TM_INSTRUCTION_CPL = 0x08,
    TM_INSTRUCTION_CPW = 0x09,
    TM_INSTRUCTION_CPB = 0x0A,
    TM_INSTRUCTION_SCF = 0x0B,
    TM_INSTRUCTION_CCF = 0x0C,
    TM_INSTRUCTION_LD = 0x10,
    TM_INSTRUCTION_LDQ = 0x13,
    TM_INSTRUCTION_LDH = 0x15,
    TM_INSTRUCTION_ST = 0x17,
    TM_INSTRUCTION_STQ = 0x19,
    TM_INSTRUCTION_STH = 0x1B,
    TM_INSTRUCTION_MV = 0x1D,
    TM_INSTRUCTION_PUSH = 0x1E,
    TM_INSTRUCTION_POP = 0x1F,
    TM_INSTRUCTION_JMP = 0x20,
    TM_INSTRUCTION_JPB = 0x22,
    TM_INSTRUCTION_CALL = 0x23,
    TM_INSTRUCTION_RST = 0x24,
    TM_INSTRUCTION_RET = 0x25,
    TM_INSTRUCTION_RETI = 0x26,
    TM_INSTRUCTION_INC = 0x30,
    TM_INSTRUCTION_DEC = 0x32,
    TM_INSTRUCTION_ADD = 0x34,
    TM_INSTRUCTION_ADC = 0x37,
    TM_INSTRUCTION_SUB = 0x3A,
    TM_INSTRUCTION_SBC = 0x3D,
    TM_INSTRUCTION_AND = 0x40,
    TM_INSTRUCTION_OR = 0x43,
    TM_INSTRUCTION_XOR = 0x46,
    TM_INSTRUCTION_CMP = 0x49,
    TM_INSTRUCTION_SLA = 0x50,
    TM_INSTRUCTION_SRA = 0x52,
    TM_INSTRUCTION_SRL = 0x54,
    TM_INSTRUCTION_RL = 0x56,
    TM_INSTRUCTION_RLC = 0x58,
    TM_INSTRUCTION_RR = 0x5A,
    TM_INSTRUCTION_RRC = 0x5C,
    TM_INSTRUCTION_BIT = 0x60,
    TM_INSTRUCTION_SET = 0x62,
    TM_INSTRUCTION_RES = 0x64,
    TM_INSTRUCTION_SWAP = 0x66,
    TM_INSTRUCTION_JPS = 0xFF
};

enum tm_cpu_error_type
{
    TM_ERROR_OK = 0x00,
    TM_ERROR_HARDWARE,
    TM_ERROR_BUS_READ,
    TM_ERROR_BUS_WRITE,
    TM_ERROR_INVALID_OPCODE,
    TM_ERROR_READ_ACCESS_VIOLATION,
    TM_ERROR_WRITE_ACCESS_VIOLATION,
    TM_ERROR_EXECUTE_ACCESS_VIOLATION,
    TM_ERROR_DATA_STACK_OVERFLOW,
    TM_ERROR_DATA_STACK_UNDERFLOW,
    TM_ERROR_CALL_STACK_OVERFLOW,
    TM_ERROR_CALL_STACK_UNDERFLOW
};
