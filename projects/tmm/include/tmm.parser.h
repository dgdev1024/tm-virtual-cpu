/// @file   tmm.parser.h
/// @brief  contains functions for generating an abstract syntax tree (AST) from
///         a list of tokens extracted by the lexer.

#pragma once
#include <tmm.syntax.h>

/* Public Functions ***********************************************************/

void tmm_init_parser ();
void tmm_shutdown_parser ();
bool tmm_parse_tokens (tmm_syntax_block_t* p_block);
