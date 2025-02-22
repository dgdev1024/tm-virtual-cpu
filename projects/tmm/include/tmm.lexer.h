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
bool tmm_has_more_tokens ();
const tmm_token_t* tmm_token_at (size_t p_index);
const tmm_token_t* tmm_advance_token ();
const tmm_token_t* tmm_advance_token_if_type (tmm_token_type_t p_type);
const tmm_token_t* tmm_advance_token_if_keyword (tmm_keyword_type_t p_type);
const tmm_token_t* tmm_peek_token (size_t p_offset);
const tmm_token_t* tmm_current_token ();
const tmm_token_t* tmm_previous_token ();
void tmm_print_tokens ();
void tmm_reset_lexer ();
