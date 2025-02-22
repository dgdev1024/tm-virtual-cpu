/// @file tmm.token.c

#include <tmm.token.h>

/* Public Functions ***********************************************************/

const char* tmm_stringify_token_type (tmm_token_type_t p_type)
{
    switch (p_type)
    {
        case TMM_TOKEN_UNKNOWN: return "unknown";
        case TMM_TOKEN_KEYWORD: return "keyword";
        case TMM_TOKEN_IDENTIFIER: return "identifier";
        case TMM_TOKEN_NUMBER: return "number";
        case TMM_TOKEN_BINARY: return "binary";
        case TMM_TOKEN_OCTAL: return "octal";
        case TMM_TOKEN_HEXADECIMAL: return "hexadecimal";
        case TMM_TOKEN_STRING: return "string";
        case TMM_TOKEN_CHARACTER: return "character";
        case TMM_TOKEN_ADD: return "add";
        case TMM_TOKEN_SUBTRACT: return "subtract";
        case TMM_TOKEN_MULTIPLY: return "multiply";
        case TMM_TOKEN_DIVIDE: return "divide";
        case TMM_TOKEN_MODULO: return "modulo";
        case TMM_TOKEN_EXPONENT: return "exponent";
        case TMM_TOKEN_BITWISE_AND: return "bitwise_and";
        case TMM_TOKEN_BITWISE_OR: return "bitwise_or";
        case TMM_TOKEN_BITWISE_XOR: return "bitwise_xor";
        case TMM_TOKEN_BITWISE_NOT: return "bitwise_not";
        case TMM_TOKEN_BITWISE_LSHIFT: return "bitwise_lshift";
        case TMM_TOKEN_BITWISE_RSHIFT: return "bitwise_rshift";
        case TMM_TOKEN_LOGICAL_AND: return "logical_and";
        case TMM_TOKEN_LOGICAL_OR: return "logical_or";
        case TMM_TOKEN_LOGICAL_NOT: return "logical_not";
        case TMM_TOKEN_EQUAL: return "equal";
        case TMM_TOKEN_NOT_EQUAL: return "not_equal";
        case TMM_TOKEN_LESS: return "less";
        case TMM_TOKEN_LESS_EQUAL: return "less_equal";
        case TMM_TOKEN_GREATER: return "greater";
        case TMM_TOKEN_GREATER_EQUAL: return "greater_equal";
        case TMM_TOKEN_ASSIGN: return "assign";
        case TMM_TOKEN_ADD_ASSIGN: return "add_assign";
        case TMM_TOKEN_SUB_ASSIGN: return "sub_assign";
        case TMM_TOKEN_MUL_ASSIGN: return "mul_assign";
        case TMM_TOKEN_DIV_ASSIGN: return "div_assign";
        case TMM_TOKEN_MOD_ASSIGN: return "mod_assign";
        case TMM_TOKEN_EXP_ASSIGN: return "exp_assign";
        case TMM_TOKEN_AND_ASSIGN: return "and_assign";
        case TMM_TOKEN_OR_ASSIGN: return "or_assign";
        case TMM_TOKEN_XOR_ASSIGN: return "xor_assign";
        case TMM_TOKEN_LSHIFT_ASSIGN: return "lshift_assign";
        case TMM_TOKEN_RSHIFT_ASSIGN: return "rshift_assign";
        case TMM_TOKEN_COMMA: return "comma";
        case TMM_TOKEN_SEMICOLON: return "semicolon";
        case TMM_TOKEN_COLON: return "colon";
        case TMM_TOKEN_PERIOD: return "period";
        case TMM_TOKEN_QUESTION: return "question";
        case TMM_TOKEN_OPEN_PAREN: return "open_paren";
        case TMM_TOKEN_CLOSE_PAREN: return "close_paren";
        case TMM_TOKEN_OPEN_BRACKET: return "open_bracket";
        case TMM_TOKEN_CLOSE_BRACKET: return "close_bracket";
        case TMM_TOKEN_OPEN_BRACE: return "open_brace";
        case TMM_TOKEN_CLOSE_BRACE: return "close_brace";
        case TMM_TOKEN_ARROW: return "arrow";
        case TMM_TOKEN_EOF: return "end_of_file";
        case TMM_TOKEN_EOL: return "end_of_line";
    }
}

const char* tmm_stringify_token (const tmm_token_t* p_token)
{
    tm_expect(p_token != nullptr, "tm: null token!\n");
    if (p_token->m_name[0] != '\0')
    {
        return p_token->m_name;
    }
    else
    {
        return tmm_stringify_token_type(p_token->m_type);
    }
}

bool tmm_is_number_token (const tmm_token_t* p_token)
{
    tm_expect(p_token != nullptr, "tm: null token!\n");
    switch (p_token->m_type)
    {
        case TMM_TOKEN_NUMBER:
        case TMM_TOKEN_BINARY:
        case TMM_TOKEN_OCTAL:
        case TMM_TOKEN_HEXADECIMAL:
            return true;
        default:
            return false;
    }
}

bool tmm_is_arithmetic_operator_token (const tmm_token_t* p_token)
{
    tm_expect(p_token != nullptr, "tm: null token!\n");
    switch (p_token->m_type)
    {
        case TMM_TOKEN_ADD:
        case TMM_TOKEN_SUBTRACT:
        case TMM_TOKEN_MULTIPLY:
        case TMM_TOKEN_DIVIDE:
        case TMM_TOKEN_MODULO:
        case TMM_TOKEN_EXPONENT:
            return true;
        default:
            return false;
    }
}

bool tmm_is_bitwise_operator_token (const tmm_token_t* p_token)
{
    tm_expect(p_token != nullptr, "tm: null token!\n");
    switch (p_token->m_type)
    {
        case TMM_TOKEN_BITWISE_AND:
        case TMM_TOKEN_BITWISE_OR:
        case TMM_TOKEN_BITWISE_XOR:
        case TMM_TOKEN_BITWISE_NOT:
        case TMM_TOKEN_BITWISE_LSHIFT:
        case TMM_TOKEN_BITWISE_RSHIFT:
            return true;
        default:
            return false;
    }
}

bool tmm_is_logical_operator_token (const tmm_token_t* p_token)
{
    tm_expect(p_token != nullptr, "tm: null token!\n");
    switch (p_token->m_type)
    {
        case TMM_TOKEN_LOGICAL_AND:
        case TMM_TOKEN_LOGICAL_OR:
        case TMM_TOKEN_LOGICAL_NOT:
            return true;
        default:
            return false;
    }
}

bool tmm_is_comparison_operator_token (const tmm_token_t* p_token)
{
    tm_expect(p_token != nullptr, "tm: null token!\n");
    switch (p_token->m_type)
    {
        case TMM_TOKEN_EQUAL:
        case TMM_TOKEN_NOT_EQUAL:
        case TMM_TOKEN_LESS:
        case TMM_TOKEN_LESS_EQUAL:
        case TMM_TOKEN_GREATER:
        case TMM_TOKEN_GREATER_EQUAL:
            return true;
        default:
            return false;
    }
}

bool tmm_is_assignment_operator_token (const tmm_token_t* p_token)
{
    tm_expect(p_token != nullptr, "tm: null token!\n");
    switch (p_token->m_type)
    {
        case TMM_TOKEN_ASSIGN:
        case TMM_TOKEN_ADD_ASSIGN:
        case TMM_TOKEN_SUB_ASSIGN:
        case TMM_TOKEN_MUL_ASSIGN:
        case TMM_TOKEN_DIV_ASSIGN:
        case TMM_TOKEN_MOD_ASSIGN:
        case TMM_TOKEN_EXP_ASSIGN:
        case TMM_TOKEN_AND_ASSIGN:
        case TMM_TOKEN_OR_ASSIGN:
        case TMM_TOKEN_XOR_ASSIGN:
        case TMM_TOKEN_LSHIFT_ASSIGN:
        case TMM_TOKEN_RSHIFT_ASSIGN:
            return true;
        default:
            return false;
    }
}
