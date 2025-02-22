/// @file   tmm.lexer.h
/// @brief  contains functions for lexing tokens from tmm assembly language
//          source files, and retrieving them.

#pragma once
#include <tmm.token.h>

/* Constants ******************************************************************/

#define TMM_LEXER_DEFAULT_CAPACITY  32

/* Public Functions ***********************************************************/

void tmm_init_lexer ();
void tmm_shutdown_lexer ();
bool tmm_lex_file (const char* p_filename);
void tmm_print_tokens ();
