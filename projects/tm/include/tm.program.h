/// @file tm.program.h
/// @brief Structure representing a program to be run by the TM virtual CPU.

#pragma once
#include <tm.common.h>

/* Program Structure **********************************************************/

/**
 * @brief Structure representing a program to be run by the TM virtual CPU.
 */
typedef struct tm_program
{
    char    m_name[TM_PROGRAM_NAME_SIZE];      ///< Program name.
    char    m_author[TM_PROGRAM_AUTHOR_SIZE];  ///< Program author.
    byte_t* m_rom;                             ///< Program ROM.
    size_t  m_rom_size;                        ///< Program ROM size.
} tm_program_t;

/* Public Functions ***********************************************************/

tm_api tm_program_t* tm_create_program  (const char* p_filename);
tm_api bool          tm_init_program    (tm_program_t* p_program, const char* p_filename);
tm_api void          tm_destroy_program (tm_program_t* p_program);
tm_api bool          tm_read_rom_byte   (tm_program_t* p_program, addr_t p_address, byte_t* p_byte);
tm_api bool          tm_write_rom_byte  (tm_program_t* p_program, addr_t p_address, byte_t byte);
