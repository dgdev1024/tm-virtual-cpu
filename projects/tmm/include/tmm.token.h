/// @file   tmm.token.h
/// @brief  contains a structure for a token extracted from a source file by
//          the lexer.

#pragma once
#include <tmm.keyword.h>

/* Constants ******************************************************************/

#define TMM_TOKEN_STRLEN 64

/* Token Type Enumeration *****************************************************/

typedef enum tmm_token_type
{
    TMM_TOKEN_UNKNOWN,      ///< Unknown token type.

    // Keyword and Identifier Tokens
    TMM_TOKEN_KEYWORD,      ///< Keyword token type.
    TMM_TOKEN_IDENTIFIER,   ///< Identifier token type.

    // Number Tokens
    TMM_TOKEN_NUMBER,       ///< Number token type.
    TMM_TOKEN_BINARY,       ///< Binary number token type.
    TMM_TOKEN_OCTAL,        ///< Octal number token type.
    TMM_TOKEN_HEXADECIMAL,  ///< Hexadecimal number token type.
    TMM_TOKEN_STRING,       ///< String token type.
    TMM_TOKEN_CHARACTER,    ///< Character token type.

    // Arithmetic Operator Tokens
    TMM_TOKEN_ADD,          ///< Addition token type.
    TMM_TOKEN_SUBTRACT,     ///< Subtraction token type.
    TMM_TOKEN_MULTIPLY,     ///< Multiplication token type.
    TMM_TOKEN_DIVIDE,       ///< Division token type.
    TMM_TOKEN_MODULO,       ///< Modulo token type.
    TMM_TOKEN_EXPONENT,     ///< Exponent token type.

    // Bitwise Operator Tokens
    TMM_TOKEN_BITWISE_AND,      ///< Bitwise AND token type.
    TMM_TOKEN_BITWISE_OR,       ///< Bitwise OR token type.
    TMM_TOKEN_BITWISE_XOR,      ///< Bitwise XOR token type.
    TMM_TOKEN_BITWISE_NOT,      ///< Bitwise NOT token type.
    TMM_TOKEN_BITWISE_LSHIFT,   ///< Bitwise left shift token type.
    TMM_TOKEN_BITWISE_RSHIFT,   ///< Bitwise right shift token type.

    // Logical Operator Tokens
    TMM_TOKEN_LOGICAL_AND,      ///< Logical AND token type.
    TMM_TOKEN_LOGICAL_OR,       ///< Logical OR token type.
    TMM_TOKEN_LOGICAL_NOT,      ///< Logical NOT token type.

    // Comparison Operator Tokens
    TMM_TOKEN_EQUAL,            ///< Equal token type.
    TMM_TOKEN_NOT_EQUAL,        ///< Not equal token type.
    TMM_TOKEN_LESS,             ///< Less than token type.
    TMM_TOKEN_LESS_EQUAL,       ///< Less than or equal token type.
    TMM_TOKEN_GREATER,          ///< Greater than token type.
    TMM_TOKEN_GREATER_EQUAL,    ///< Greater than or equal token type.

    // Assignment Operator Tokens
    TMM_TOKEN_ASSIGN,           ///< Assignment token type.
    TMM_TOKEN_ADD_ASSIGN,       ///< Addition assignment token type.
    TMM_TOKEN_SUB_ASSIGN,       ///< Subtraction assignment token type.
    TMM_TOKEN_MUL_ASSIGN,       ///< Multiplication assignment token type.
    TMM_TOKEN_DIV_ASSIGN,       ///< Division assignment token type.
    TMM_TOKEN_MOD_ASSIGN,       ///< Modulo assignment token type.
    TMM_TOKEN_EXP_ASSIGN,       ///< Exponent assignment token type.
    TMM_TOKEN_AND_ASSIGN,       ///< Bitwise AND assignment token type.
    TMM_TOKEN_OR_ASSIGN,        ///< Bitwise OR assignment token type.
    TMM_TOKEN_XOR_ASSIGN,       ///< Bitwise XOR assignment token type.
    TMM_TOKEN_LSHIFT_ASSIGN,    ///< Bitwise left shift assignment token type.
    TMM_TOKEN_RSHIFT_ASSIGN,    ///< Bitwise right shift assignment token type.

    // Delimiter Tokens
    TMM_TOKEN_COMMA,            ///< Comma token type.
    TMM_TOKEN_SEMICOLON,        ///< Semicolon token type.
    TMM_TOKEN_COLON,            ///< Colon token type.
    TMM_TOKEN_PERIOD,           ///< Period token type.
    TMM_TOKEN_QUESTION,         ///< Question mark token type.

    // Grouping Tokens
    TMM_TOKEN_OPEN_PAREN,       ///< Open parenthesis token type.
    TMM_TOKEN_CLOSE_PAREN,      ///< Close parenthesis token type.
    TMM_TOKEN_OPEN_BRACKET,     ///< Open bracket token type.
    TMM_TOKEN_CLOSE_BRACKET,    ///< Close bracket token type.
    TMM_TOKEN_OPEN_BRACE,       ///< Open brace token type.
    TMM_TOKEN_CLOSE_BRACE,      ///< Close brace token type.
    TMM_TOKEN_ARROW,            ///< Arrow token type.

    // Signal Tokens
    TMM_TOKEN_EOF,              ///< End of file token type.
    TMM_TOKEN_EOL,              ///< End of line token type.
} tmm_token_type_t;

/* Token Structure ************************************************************/

typedef struct tmm_token
{
    char                m_name[TMM_TOKEN_STRLEN];   ///< Token name.
    tmm_token_type_t    m_type;                     ///< Token type.
    const char*         m_source_file;              ///< Source file name.
    size_t              m_line;                     ///< Line number.
} tmm_token_t;

/* Public Functions ***********************************************************/

const char* tmm_stringify_token_type (tmm_token_type_t p_type);
const char* tmm_stringify_token (const tmm_token_t* p_token);
bool tmm_is_number_token (const tmm_token_t* p_token);
bool tmm_is_arithmetic_operator_token (const tmm_token_t* p_token);
bool tmm_is_bitwise_operator_token (const tmm_token_t* p_token);
bool tmm_is_logical_operator_token (const tmm_token_t* p_token);
bool tmm_is_comparison_operator_token (const tmm_token_t* p_token);
bool tmm_is_assignment_operator_token (const tmm_token_t* p_token);
