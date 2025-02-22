/// @file tmm.parser.c

#include <tmm.lexer.h>
#include <tmm.parser.h>

/* Parser Context Structure ***************************************************/

static struct
{
    tmm_syntax_block_t* m_root; ///< Root node of the syntax tree.
} s_parser = {
    .m_root = nullptr,
};

/* Static Function Prototypes *************************************************/

static tmm_syntax_t* tmm_parse_expression ();
static tmm_syntax_t* tmm_parse_block ();
static tmm_syntax_t* tmm_parse_statement ();

/* Static Functions - Primary Expression Parsing ******************************/

static tmm_syntax_t* tmm_parse_primary_expression ()
{
    const tmm_token_t* l_token = tmm_advance_token();
    if (l_token == nullptr)
    {
        tm_errorf("tmm: unexpected end of file during parsing.\n");
        return nullptr;
    }

    switch (l_token->m_type)
    {
        case TMM_TOKEN_OPEN_PAREN:
        {
            tmm_syntax_t* l_expression = tmm_parse_expression();
            if (l_expression == nullptr)
            {
                tm_errorf("tmm:   while parsing parenthesis-encosed expression.\n");
                return nullptr;
            }

            const tmm_token_t* l_close_paren = tmm_advance_token_if_type(TMM_TOKEN_CLOSE_PAREN);
            if (l_close_paren == nullptr)
            {
                tm_errorf("tmm: expected closing parenthesis ')' in parenthesis-encosed expression.\n");
                tmm_destroy_syntax(l_expression);
                return nullptr;
            }

            return l_expression;
        } break;
        case TMM_TOKEN_OPEN_BRACKET:
        {
            tmm_syntax_t* l_expression = tmm_parse_expression();
            if (l_expression == nullptr)
            {
                tm_errorf("tmm:   while parsing pointer expression.\n");
                return nullptr;
            }

            const tmm_token_t* l_close_bracket = tmm_advance_token_if_type(TMM_TOKEN_CLOSE_BRACKET);
            if (l_close_bracket == nullptr)
            {
                tm_errorf("tmm: expected closing bracket ']' in pointer expression.\n");
                tmm_destroy_syntax(l_expression);
                return nullptr;
            }

            tmm_syntax_expression_pointer_t* l_pointer = 
                (tmm_syntax_expression_pointer_t*) tmm_create_syntax(TMM_SYNTAX_EXPRESSION_POINTER, l_token);
            l_pointer->m_expression = l_expression;
            return (tmm_syntax_t*) l_pointer;
        } break;
        case TMM_TOKEN_IDENTIFIER:
        {
            tmm_syntax_expression_identifier_t* l_identifier = 
                (tmm_syntax_expression_identifier_t*) tmm_create_syntax(TMM_SYNTAX_EXPRESSION_IDENTIFIER, l_token);
            strncpy(l_identifier->m_symbol, l_token->m_name, TMM_LITERAL_STRLEN);
            return (tmm_syntax_t*) l_identifier;
        } break;
        case TMM_TOKEN_STRING:
        {
            tmm_syntax_expression_string_literal_t* l_string = 
                (tmm_syntax_expression_string_literal_t*) tmm_create_syntax(TMM_SYNTAX_EXPRESSION_STRING_LITERAL, l_token);
            strncpy(l_string->m_value, l_token->m_name, TMM_LITERAL_STRLEN);
            return (tmm_syntax_t*) l_string;
        } break;
        case TMM_TOKEN_CHARACTER:
        {
            tmm_syntax_expression_numeric_literal_t* l_numeric = 
                (tmm_syntax_expression_numeric_literal_t*) tmm_create_syntax(TMM_SYNTAX_EXPRESSION_NUMERIC_LITERAL, l_token);
            l_numeric->m_value = strtoul(l_token->m_name, nullptr, 10);
            return (tmm_syntax_t*) l_numeric;
        } break;
        case TMM_TOKEN_NUMBER:
        {
            tmm_syntax_expression_numeric_literal_t* l_numeric = 
                (tmm_syntax_expression_numeric_literal_t*) tmm_create_syntax(TMM_SYNTAX_EXPRESSION_NUMERIC_LITERAL, l_token);
            l_numeric->m_value = strtod(l_token->m_name, nullptr);
            return (tmm_syntax_t*) l_numeric;
        } break;
        case TMM_TOKEN_HEXADECIMAL:
        {
            tmm_syntax_expression_numeric_literal_t* l_numeric = 
                (tmm_syntax_expression_numeric_literal_t*) tmm_create_syntax(TMM_SYNTAX_EXPRESSION_NUMERIC_LITERAL, l_token);
            l_numeric->m_value = strtoul(l_token->m_name, nullptr, 16);
            return (tmm_syntax_t*) l_numeric;
        } break;
        case TMM_TOKEN_BINARY:
        {
            tmm_syntax_expression_numeric_literal_t* l_numeric = 
                (tmm_syntax_expression_numeric_literal_t*) tmm_create_syntax(TMM_SYNTAX_EXPRESSION_NUMERIC_LITERAL, l_token);
            l_numeric->m_value = strtoul(l_token->m_name, nullptr, 2);
            return (tmm_syntax_t*) l_numeric;
        } break;
        case TMM_TOKEN_OCTAL:
        {
            tmm_syntax_expression_numeric_literal_t* l_numeric = 
                (tmm_syntax_expression_numeric_literal_t*) tmm_create_syntax(TMM_SYNTAX_EXPRESSION_NUMERIC_LITERAL, l_token);
            l_numeric->m_value = strtoul(l_token->m_name, nullptr, 8);
            return (tmm_syntax_t*) l_numeric;
        } break;
        case TMM_TOKEN_PLACEHOLDER:
        {
            tmm_syntax_expression_placeholder_literal_t* l_placeholder = 
                (tmm_syntax_expression_placeholder_literal_t*) tmm_create_syntax(TMM_SYNTAX_EXPRESSION_PLACEHOLDER_LITERAL, l_token);
            l_placeholder->m_index = strtoul(l_token->m_name, nullptr, 10);
            return (tmm_syntax_t*) l_placeholder;
        } break;
        case TMM_TOKEN_KEYWORD:
        {
            const tmm_keyword_t* l_keyword = tmm_lookup_keyword(l_token->m_name, TMM_KEYWORD_NONE);
            switch (l_keyword->m_type)
            {
                case TMM_KEYWORD_REGISTER:
                {
                    tmm_syntax_expression_register_literal_t* l_register = 
                        (tmm_syntax_expression_register_literal_t*) tmm_create_syntax(TMM_SYNTAX_EXPRESSION_REGISTER_LITERAL, l_token);
                    l_register->m_register = l_keyword->m_subtype;
                    return (tmm_syntax_t*) l_register;
                } break;
                case TMM_KEYWORD_CONDITION:
                {
                    tmm_syntax_expression_condition_literal_t* l_condition = 
                        (tmm_syntax_expression_condition_literal_t*) tmm_create_syntax(TMM_SYNTAX_EXPRESSION_CONDITION_LITERAL, l_token);
                    l_condition->m_condition = l_keyword->m_subtype;
                    return (tmm_syntax_t*) l_condition;
                } break;
                default:
                {
                    tm_errorf("tmm: unexpected keyword '%s' in primary expression.\n", l_token->m_name);
                    return nullptr;
                } break;
            }
        } break;
        default:
        {
            tm_errorf("tmm: unexpected '%s' token in primary expression", tmm_stringify_token_type(l_token->m_type));
            if (l_token->m_name[0] != '\0')
            {
                tm_errorf(" = '%s'", l_token->m_name);
            }
            tm_errorf(".\n");
            return nullptr;
        } break;
    }
}

/* Static Functions - Expression Parsing **************************************/

// Order of Operator Precedence:
// 1. Parenthesis / Primary Expressions
// 2. Unary Operators
// 3. Multiplication / Division / Modulus
// 4. Addition / Subtraction
// 5. Bitwise Shifts
// 6. Relational / Equality Operators
// 7. Bitwise AND
// 8. Bitwise XOR
// 9. Bitwise OR
// 10. Logical AND
// 11. Logical OR

static tmm_syntax_t* tmm_parse_unary_expression ()
{
    // Peek for a unary operator.
    const tmm_token_t* l_token = tmm_peek_token(0);
    if (!tmm_is_unary_operator_token(l_token))
    {
        return tmm_parse_primary_expression();
    }

    // Consume the unary operator.
    tmm_advance_token();

    // Parse the operand expression.
    tmm_syntax_t* l_operand = tmm_parse_unary_expression();
    if (l_operand == nullptr)
    {
        tm_errorf("tmm:   while parsing operand expression of unary operation.\n");
        return nullptr;
    }

    // Create the unary expression node.
    tmm_syntax_expression_unary_t* l_expression = 
        (tmm_syntax_expression_unary_t*) tmm_create_syntax(TMM_SYNTAX_EXPRESSION_UNARY, l_token);
    l_expression->m_operator = l_token->m_type;
    l_expression->m_operand = l_operand;
    return (tmm_syntax_t*) l_expression;
}

static tmm_syntax_t* tmm_parse_multiplicative_expression ()
{
    // Parse the lefthand expression.
    tmm_syntax_t* l_left = tmm_parse_unary_expression();
    if (l_left == nullptr)
    {
        return nullptr;
    }

    // Peek for a multiplicative operator.
    const tmm_token_t* l_token = tmm_peek_token(0);
    if (!tmm_is_multiplicative_operator_token(l_token))
    {
        return l_left;
    }

    // Consume the multiplicative operator.
    tmm_advance_token();

    // Parse the righthand expression.
    tmm_syntax_t* l_right = tmm_parse_multiplicative_expression();
    if (l_right == nullptr)
    {
        tm_errorf("tmm:   while parsing righthand expression of multiplicative operation.\n");
        tmm_destroy_syntax(l_left);
        return nullptr;
    }

    // Create the binary expression node.
    tmm_syntax_expression_binary_t* l_expression = 
        (tmm_syntax_expression_binary_t*) tmm_create_syntax(TMM_SYNTAX_EXPRESSION_BINARY, l_token);
    l_expression->m_operator = l_token->m_type;
    l_expression->m_left = l_left;
    l_expression->m_right = l_right;
    return (tmm_syntax_t*) l_expression;
}

static tmm_syntax_t* tmm_parse_additive_expression ()
{
    // Parse the lefthand expression.
    tmm_syntax_t* l_left = tmm_parse_multiplicative_expression();
    if (l_left == nullptr)
    {
        return nullptr;
    }

    // Peek for an additive operator.
    const tmm_token_t* l_token = tmm_peek_token(0);
    if (!tmm_is_additive_operator_token(l_token))
    {
        return l_left;
    }

    // Consume the additive operator.
    tmm_advance_token();

    // Parse the righthand expression.
    tmm_syntax_t* l_right = tmm_parse_additive_expression();
    if (l_right == nullptr)
    {
        tm_errorf("tmm:   while parsing righthand expression of additive operation.\n");
        tmm_destroy_syntax(l_left);
        return nullptr;
    }

    // Create the binary expression node.
    tmm_syntax_expression_binary_t* l_expression = 
        (tmm_syntax_expression_binary_t*) tmm_create_syntax(TMM_SYNTAX_EXPRESSION_BINARY, l_token);
    l_expression->m_operator = l_token->m_type;
    l_expression->m_left = l_left;
    l_expression->m_right = l_right;
    return (tmm_syntax_t*) l_expression;
}

static tmm_syntax_t* tmm_parse_shift_expression ()
{
    // Parse the lefthand expression.
    tmm_syntax_t* l_left = tmm_parse_additive_expression();
    if (l_left == nullptr)
    {
        return nullptr;
    }

    // Peek for a shift operator.
    const tmm_token_t* l_token = tmm_peek_token(0);
    if (!tmm_is_shift_operator_token(l_token))
    {
        return l_left;
    }

    // Consume the shift operator.
    tmm_advance_token();

    // Parse the righthand expression.
    tmm_syntax_t* l_right = tmm_parse_shift_expression();
    if (l_right == nullptr)
    {
        tm_errorf("tmm:   while parsing righthand expression of shift operation.\n");
        tmm_destroy_syntax(l_left);
        return nullptr;
    }

    // Create the binary expression node.
    tmm_syntax_expression_binary_t* l_expression = 
        (tmm_syntax_expression_binary_t*) tmm_create_syntax(TMM_SYNTAX_EXPRESSION_BINARY, l_token);
    l_expression->m_operator = l_token->m_type;
    l_expression->m_left = l_left;
    l_expression->m_right = l_right;
    return (tmm_syntax_t*) l_expression;
}

static tmm_syntax_t* tmm_parse_relational_expression ()
{
    // Parse the lefthand expression.
    tmm_syntax_t* l_left = tmm_parse_shift_expression();
    if (l_left == nullptr)
    {
        return nullptr;
    }

    // Peek for a relational operator.
    const tmm_token_t* l_token = tmm_peek_token(0);
    if (!tmm_is_relational_operator_token(l_token))
    {
        return l_left;
    }

    // Consume the relational operator.
    tmm_advance_token();

    // Parse the righthand expression.
    tmm_syntax_t* l_right = tmm_parse_relational_expression();
    if (l_right == nullptr)
    {
        tm_errorf("tmm:   while parsing righthand expression of relational operation.\n");
        tmm_destroy_syntax(l_left);
        return nullptr;
    }

    // Create the binary expression node.
    tmm_syntax_expression_binary_t* l_expression = 
        (tmm_syntax_expression_binary_t*) tmm_create_syntax(TMM_SYNTAX_EXPRESSION_BINARY, l_token);
    l_expression->m_operator = l_token->m_type;
    l_expression->m_left = l_left;
    l_expression->m_right = l_right;
    return (tmm_syntax_t*) l_expression;
}

static tmm_syntax_t* tmm_parse_bitwise_and_expression ()
{
    // Parse the lefthand expression.
    tmm_syntax_t* l_left = tmm_parse_relational_expression();
    if (l_left == nullptr)
    {
        return nullptr;
    }

    // Peek for the bitwise AND operator.
    const tmm_token_t* l_token = tmm_peek_token(0);
    if (l_token->m_type != TMM_TOKEN_BITWISE_AND)
    {
        return l_left;
    }

    // Consume the bitwise AND operator.
    tmm_advance_token();

    // Parse the righthand expression.
    tmm_syntax_t* l_right = tmm_parse_bitwise_and_expression();
    if (l_right == nullptr)
    {
        tm_errorf("tmm:   while parsing righthand expression of bitwise AND operation.\n");
        tmm_destroy_syntax(l_left);
        return nullptr;
    }

    // Create the binary expression node.
    tmm_syntax_expression_binary_t* l_expression = 
        (tmm_syntax_expression_binary_t*) tmm_create_syntax(TMM_SYNTAX_EXPRESSION_BINARY, l_token);
    l_expression->m_operator = l_token->m_type;
    l_expression->m_left = l_left;
    l_expression->m_right = l_right;
    return (tmm_syntax_t*) l_expression;
}

static tmm_syntax_t* tmm_parse_bitwise_xor_expression ()
{
    // Parse the lefthand expression.
    tmm_syntax_t* l_left = tmm_parse_bitwise_and_expression();
    if (l_left == nullptr)
    {
        return nullptr;
    }

    // Peek for the bitwise XOR operator.
    const tmm_token_t* l_token = tmm_peek_token(0);
    if (l_token->m_type != TMM_TOKEN_BITWISE_XOR)
    {
        return l_left;
    }

    // Consume the bitwise XOR operator.
    tmm_advance_token();

    // Parse the righthand expression.
    tmm_syntax_t* l_right = tmm_parse_bitwise_xor_expression();
    if (l_right == nullptr)
    {
        tm_errorf("tmm:   while parsing righthand expression of bitwise XOR operation.\n");
        tmm_destroy_syntax(l_left);
        return nullptr;
    }

    // Create the binary expression node.
    tmm_syntax_expression_binary_t* l_expression = 
        (tmm_syntax_expression_binary_t*) tmm_create_syntax(TMM_SYNTAX_EXPRESSION_BINARY, l_token);
    l_expression->m_operator = l_token->m_type;
    l_expression->m_left = l_left;
    l_expression->m_right = l_right;
    return (tmm_syntax_t*) l_expression;
}

static tmm_syntax_t* tmm_parse_bitwise_or_expression ()
{
    // Parse the lefthand expression.
    tmm_syntax_t* l_left = tmm_parse_bitwise_xor_expression();
    if (l_left == nullptr)
    {
        return nullptr;
    }

    // Peek for the bitwise OR operator.
    const tmm_token_t* l_token = tmm_peek_token(0);
    if (l_token->m_type != TMM_TOKEN_BITWISE_OR)
    {
        return l_left;
    }

    // Consume the bitwise OR operator.
    tmm_advance_token();

    // Parse the righthand expression.
    tmm_syntax_t* l_right = tmm_parse_bitwise_or_expression();
    if (l_right == nullptr)
    {
        tm_errorf("tmm:   while parsing righthand expression of bitwise OR operation.\n");
        tmm_destroy_syntax(l_left);
        return nullptr;
    }

    // Create the binary expression node.
    tmm_syntax_expression_binary_t* l_expression = 
        (tmm_syntax_expression_binary_t*) tmm_create_syntax(TMM_SYNTAX_EXPRESSION_BINARY, l_token);
    l_expression->m_operator = l_token->m_type;
    l_expression->m_left = l_left;
    l_expression->m_right = l_right;
    return (tmm_syntax_t*) l_expression;
}

static tmm_syntax_t* tmm_parse_logical_and_expression ()
{
    // Parse the lefthand expression.
    tmm_syntax_t* l_left = tmm_parse_bitwise_or_expression();
    if (l_left == nullptr)
    {
        return nullptr;
    }

    // Peek for the logical AND operator.
    const tmm_token_t* l_token = tmm_peek_token(0);
    if (l_token->m_type != TMM_TOKEN_LOGICAL_AND)
    {
        return l_left;
    }

    // Consume the logical AND operator.
    tmm_advance_token();

    // Parse the righthand expression.
    tmm_syntax_t* l_right = tmm_parse_logical_and_expression();
    if (l_right == nullptr)
    {
        tm_errorf("tmm:   while parsing righthand expression of logical AND operation.\n");
        tmm_destroy_syntax(l_left);
        return nullptr;
    }

    // Create the binary expression node.
    tmm_syntax_expression_binary_t* l_expression = 
        (tmm_syntax_expression_binary_t*) tmm_create_syntax(TMM_SYNTAX_EXPRESSION_BINARY, l_token);
    l_expression->m_operator = l_token->m_type;
    l_expression->m_left = l_left;
    l_expression->m_right = l_right;
    return (tmm_syntax_t*) l_expression;
}

static tmm_syntax_t* tmm_parse_logical_or_expression ()
{
    // Parse the lefthand expression.
    tmm_syntax_t* l_left = tmm_parse_logical_and_expression();
    if (l_left == nullptr)
    {
        return nullptr;
    }

    // Peek for the logical OR operator.
    const tmm_token_t* l_token = tmm_peek_token(0);
    if (l_token->m_type != TMM_TOKEN_LOGICAL_OR)
    {
        return l_left;
    }

    // Consume the logical OR operator.
    tmm_advance_token();

    // Parse the righthand expression.
    tmm_syntax_t* l_right = tmm_parse_logical_or_expression();
    if (l_right == nullptr)
    {
        tm_errorf("tmm:   while parsing righthand expression of logical OR operation.\n");
        tmm_destroy_syntax(l_left);
        return nullptr;
    }

    // Create the binary expression node.
    tmm_syntax_expression_binary_t* l_expression = 
        (tmm_syntax_expression_binary_t*) tmm_create_syntax(TMM_SYNTAX_EXPRESSION_BINARY, l_token);
    l_expression->m_operator = l_token->m_type;
    l_expression->m_left = l_left;
    l_expression->m_right = l_right;
    return (tmm_syntax_t*) l_expression;
}

tmm_syntax_t* tmm_parse_expression ()
{
    return tmm_parse_logical_or_expression();
}

/* Static Functions - Directive Parsing ***************************************/

static tmm_syntax_t* tmm_parse_org_directive ()
{
    // Peek the next token.
    const tmm_token_t* l_token = tmm_peek_token(0);

    // Parse the org's offset expression.
    tmm_syntax_t* l_expression = tmm_parse_expression();
    if (l_expression == nullptr)
    {
        tm_errorf("tmm:   while parsing org directive offset expression.\n");
        return nullptr;
    }

    // Create the org directive node.
    tmm_syntax_directive_org_t* l_directive = 
        (tmm_syntax_directive_org_t*) tmm_create_syntax(TMM_SYNTAX_DIRECTIVE_ORG, l_token);
    l_directive->m_expression = l_expression;
    return (tmm_syntax_t*) l_directive;
}

static tmm_syntax_t* tmm_parse_include_directive ()
{
    // Peek the next token.
    const tmm_token_t* l_token = tmm_peek_token(0);

    // Parse the include's filename expression.
    tmm_syntax_t* l_expression = tmm_parse_expression();
    if (l_expression == nullptr)
    {
        tm_errorf("tmm:   while parsing include directive filename expression.\n");
        return nullptr;
    }

    // Create the include directive node.
    tmm_syntax_directive_include_t* l_directive = 
        (tmm_syntax_directive_include_t*) tmm_create_syntax(TMM_SYNTAX_DIRECTIVE_INCLUDE, l_token);
    l_directive->m_expression = l_expression;
    return (tmm_syntax_t*) l_directive;
}

static tmm_syntax_t* tmm_parse_incbin_directive ()
{
    // Peek the next token.
    const tmm_token_t* l_token = tmm_peek_token(0);

    // The offset and length expressions are optional.
    tmm_syntax_t* l_offset = nullptr;
    tmm_syntax_t* l_length = nullptr;

    // Parse the incbin's filename expression.
    tmm_syntax_t* l_expression = tmm_parse_expression();
    if (l_expression == nullptr)
    {
        tm_errorf("tmm:   while parsing incbin directive filename expression.\n");
        return nullptr;
    }

    // Create the incbin directive node.
    tmm_syntax_directive_incbin_t* l_directive = 
        (tmm_syntax_directive_incbin_t*) tmm_create_syntax(TMM_SYNTAX_DIRECTIVE_INCBIN, l_token);
    l_directive->m_expression = l_expression;

    // If the next token is a comma, then an offset expression is present.
    if (tmm_advance_token_if_type(TMM_TOKEN_COMMA) != nullptr)
    {
        l_offset = tmm_parse_expression();
        if (l_offset == nullptr)
        {
            tm_errorf("tmm:   while parsing incbin directive offset expression.\n");
            tmm_destroy_syntax((tmm_syntax_t*) l_directive);
            return nullptr;
        }

        l_directive->m_offset = l_offset;

        // If the next token is a comma, then a length expression is present.
        if (tmm_advance_token_if_type(TMM_TOKEN_COMMA) != nullptr)
        {
            l_length = tmm_parse_expression();
            if (l_length == nullptr)
            {
                tm_errorf("tmm:   while parsing incbin directive length expression.\n");
                tmm_destroy_syntax((tmm_syntax_t*) l_directive);
                return nullptr;
            }

            l_directive->m_length = l_length;
        }
    }

    return (tmm_syntax_t*) l_directive;
}

static tmm_syntax_t* tmm_parse_define_directive ()
{
    // Peek the next token.
    const tmm_token_t* l_token = tmm_peek_token(0);

    // Parse the define's identifier expression.
    tmm_syntax_t* l_identifier = tmm_parse_expression();
    if (l_identifier == nullptr)
    {
        tm_errorf("tmm:   while parsing define directive identifier expression.\n");
        return nullptr;
    }

    // Parse the define's statement.
    tmm_syntax_t* l_statement = tmm_parse_statement();
    if (l_statement == nullptr)
    {
        tm_errorf("tmm:   while parsing define directive statement.\n");
        tmm_destroy_syntax(l_identifier);
        return nullptr;
    }

    // Create the define directive node.
    tmm_syntax_directive_define_t* l_directive = 
        (tmm_syntax_directive_define_t*) tmm_create_syntax(TMM_SYNTAX_DIRECTIVE_DEFINE, l_token);
    l_directive->m_identifier = l_identifier;
    l_directive->m_statement = l_statement;
    return (tmm_syntax_t*) l_directive;
}

static tmm_syntax_t* tmm_parse_undef_directive ()
{
    // Peek the next token.
    const tmm_token_t* l_token = tmm_peek_token(0);

    // Parse the undef's identifier expression.
    tmm_syntax_t* l_identifier = tmm_parse_expression();
    if (l_identifier == nullptr)
    {
        tm_errorf("tmm:   while parsing undef directive identifier expression.\n");
        return nullptr;
    }

    // Create the undef directive node.
    tmm_syntax_directive_undef_t* l_directive = 
        (tmm_syntax_directive_undef_t*) tmm_create_syntax(TMM_SYNTAX_DIRECTIVE_UNDEF, l_token);
    l_directive->m_identifier = l_identifier;
    return (tmm_syntax_t*) l_directive;
}

static tmm_syntax_t* tmm_parse_if_directive ()
{
    // Peek the next token.
    const tmm_token_t* l_token = tmm_peek_token(0);

    // Parse the if's condition expression.
    tmm_syntax_t* l_expression = tmm_parse_expression();
    if (l_expression == nullptr)
    {
        tm_errorf("tmm:   while parsing if directive condition expression.\n");
        return nullptr;
    }

    // Create the if directive node.
    tmm_syntax_directive_if_t* l_directive = 
        (tmm_syntax_directive_if_t*) tmm_create_syntax(TMM_SYNTAX_DIRECTIVE_IF, l_token);
    l_directive->m_expression = l_expression;
    return (tmm_syntax_t*) l_directive;
}

static tmm_syntax_t* tmm_parse_else_directive ()
{
    // Peek the next token.
    const tmm_token_t* l_token = tmm_peek_token(0);

    // Create the else directive node.
    tmm_syntax_directive_else_t* l_directive = 
        (tmm_syntax_directive_else_t*) tmm_create_syntax(TMM_SYNTAX_DIRECTIVE_ELSE, l_token);
    return (tmm_syntax_t*) l_directive;
}

static tmm_syntax_t* tmm_parse_endif_directive ()
{
    // Peek the next token.
    const tmm_token_t* l_token = tmm_peek_token(0);

    // Create the endif directive node.
    tmm_syntax_directive_endif_t* l_directive = 
        (tmm_syntax_directive_endif_t*) tmm_create_syntax(TMM_SYNTAX_DIRECTIVE_ENDIF, l_token);
    return (tmm_syntax_t*) l_directive;
}

static tmm_syntax_t* tmm_parse_byte_directive ()
{
    // Peek the next token.
    const tmm_token_t* l_token = tmm_peek_token(0);

    // Create the byte directive node.
    tmm_syntax_directive_byte_t* l_directive = 
        (tmm_syntax_directive_byte_t*) tmm_create_syntax(TMM_SYNTAX_DIRECTIVE_BYTE, l_token);

    // Parse the byte's syntax body. There should be at least one expression.
    while (tmm_has_more_tokens() == true)
    {
        tmm_syntax_t* l_expression = tmm_parse_expression();
        if (l_expression == nullptr)
        {
            tm_errorf("tmm:   while parsing byte directive expression.\n");
            tmm_destroy_syntax((tmm_syntax_t*) l_directive);
            return nullptr;
        }

        tmm_push_syntax(&l_directive->m_body, l_expression);

        // If the next token is a comma, then another expression is expected.
        if (tmm_advance_token_if_type(TMM_TOKEN_COMMA) == nullptr)
        {
            break;
        }
    }

    return (tmm_syntax_t*) l_directive;
}

static tmm_syntax_t* tmm_parse_word_directive ()
{
    // Peek the next token.
    const tmm_token_t* l_token = tmm_peek_token(0);

    // Create the word directive node.
    tmm_syntax_directive_word_t* l_directive = 
        (tmm_syntax_directive_word_t*) tmm_create_syntax(TMM_SYNTAX_DIRECTIVE_WORD, l_token);

    // Parse the word's syntax body. There should be at least one expression.
    while (tmm_has_more_tokens() == true)
    {
        tmm_syntax_t* l_expression = tmm_parse_expression();
        if (l_expression == nullptr)
        {
            tm_errorf("tmm:   while parsing word directive expression.\n");
            tmm_destroy_syntax((tmm_syntax_t*) l_directive);
            return nullptr;
        }

        tmm_push_syntax(&l_directive->m_body, l_expression);

        // If the next token is a comma, then another expression is expected.
        if (tmm_advance_token_if_type(TMM_TOKEN_COMMA) == nullptr)
        {
            break;
        }
    }

    return (tmm_syntax_t*) l_directive;
}

static tmm_syntax_t* tmm_parse_long_directive ()
{
    // Peek the next token.
    const tmm_token_t* l_token = tmm_peek_token(0);

    // Create the long directive node.
    tmm_syntax_directive_long_t* l_directive = 
        (tmm_syntax_directive_long_t*) tmm_create_syntax(TMM_SYNTAX_DIRECTIVE_LONG, l_token);

    // Parse the long's syntax body. There should be at least one expression.
    while (tmm_has_more_tokens() == true)
    {
        tmm_syntax_t* l_expression = tmm_parse_expression();
        if (l_expression == nullptr)
        {
            tm_errorf("tmm:   while parsing long directive expression.\n");
            tmm_destroy_syntax((tmm_syntax_t*) l_directive);
            return nullptr;
        }

        tmm_push_syntax(&l_directive->m_body, l_expression);

        // If the next token is a comma, then another expression is expected.
        if (tmm_advance_token_if_type(TMM_TOKEN_COMMA) == nullptr)
        {
            break;
        }
    }

    return (tmm_syntax_t*) l_directive;
}

static tmm_syntax_t* tmm_parse_directive ()
{
    const tmm_token_t* l_token = tmm_advance_token_if_type(TMM_TOKEN_KEYWORD);
    if (l_token == nullptr)
    {
        tm_errorf("tmm: expected keyword after '.' in directive.\n");
        return nullptr;
    }

    const tmm_keyword_t* l_keyword = tmm_lookup_keyword(l_token->m_name, TMM_KEYWORD_DIRECTIVE);
    if (l_keyword->m_type == TMM_KEYWORD_NONE)
    {
        tm_errorf("tmm: unexpected keyword '%s' after '.' in directive.\n", l_token->m_name);
        return nullptr;
    }

    switch (l_keyword->m_subtype)
    {
        case TMM_DIRECTIVE_ORG:      return tmm_parse_org_directive();
        case TMM_DIRECTIVE_INCLUDE:  return tmm_parse_include_directive();
        case TMM_DIRECTIVE_INCBIN:   return tmm_parse_incbin_directive();
        case TMM_DIRECTIVE_DEFINE:   return tmm_parse_define_directive();
        case TMM_DIRECTIVE_UNDEF:    return tmm_parse_undef_directive();
        case TMM_DIRECTIVE_IF:       return tmm_parse_if_directive();
        case TMM_DIRECTIVE_ELSE:     return tmm_parse_else_directive();
        case TMM_DIRECTIVE_ENDIF:    return tmm_parse_endif_directive();
        case TMM_DIRECTIVE_BYTE:     return tmm_parse_byte_directive();
        case TMM_DIRECTIVE_WORD:     return tmm_parse_word_directive();
        case TMM_DIRECTIVE_LONG:     return tmm_parse_long_directive();
        default:
        {
            tm_errorf("tmm: unexpected directive keyword '%s'.\n", l_token->m_name);
            return nullptr;
        } break;
    }
}

/* Static Functions - Statement Parsing ***************************************/

static tmm_syntax_t* tmm_parse_label_statement (tmm_syntax_t* l_expression)
{
    tm_assert(l_expression != nullptr);

    // Get the source token of the expression.
    const tmm_token_t* l_token = &l_expression->m_token;

    // Create the label statement node.
    tmm_syntax_statement_label_t* l_statement = 
        (tmm_syntax_statement_label_t*) tmm_create_syntax(TMM_SYNTAX_STATEMENT_LABEL, l_token);
    l_statement->m_identifier = l_expression;
    return (tmm_syntax_t*) l_statement;
}

static tmm_syntax_t* tmm_parse_instruction_statement (const tmm_keyword_t* p_keyword)
{
    // Peek the next token.
    const tmm_token_t* l_token = tmm_peek_token(0);
    
    // Create the instruction statement node.
    tmm_syntax_statement_instruction_t* l_statement = 
        (tmm_syntax_statement_instruction_t*) tmm_create_syntax(TMM_SYNTAX_STATEMENT_INSTRUCTION, l_token);
    l_statement->m_mnemonic = p_keyword->m_subtype;

    // An instruction may have a number of operands it may need to execute. The
    // number of operands is found in the provided keyword's extra parameter.
    for (int i = 0; i < p_keyword->m_param; ++i)
    {
        // Parse the operand expression.
        tmm_syntax_t* l_operand = tmm_parse_expression();
        if (l_operand == nullptr)
        {
            tm_errorf("tmm:   while parsing operand expression of instruction '%s'.\n", p_keyword->m_name);
            tmm_destroy_syntax((tmm_syntax_t*) l_statement);
            return nullptr;
        }

        tmm_push_syntax(&l_statement->m_operands, l_operand);
        
        // If this is not the instruction's last operand, then a comma is expected.
        if (i < p_keyword->m_param - 1)
        {
            if (tmm_advance_token_if_type(TMM_TOKEN_COMMA) == nullptr)
            {
                tm_errorf("tmm: expected comma ',' after operand expression of instruction '%s'.\n", p_keyword->m_name);
                tmm_destroy_syntax((tmm_syntax_t*) l_statement);
                return nullptr;
            }
        }
    }

    return (tmm_syntax_t*) l_statement;
}

tmm_syntax_t* tmm_parse_block ()
{
    tmm_syntax_block_t* l_block = (tmm_syntax_block_t*) tmm_create_syntax(TMM_SYNTAX_BLOCK, tmm_peek_token(0));
    while (tmm_has_more_tokens() == true)
    {
        const tmm_token_t* l_token = tmm_peek_token(0);
        if (l_token->m_type == TMM_TOKEN_CLOSE_BRACE)
        {
            tmm_advance_token();
            return (tmm_syntax_t*) l_block;
        }

        tmm_syntax_t* l_statement = tmm_parse_statement();
        if (l_statement == nullptr)
        {
            tm_errorf("tmm:   while parsing block statement.\n");
            tmm_destroy_syntax((tmm_syntax_t*) l_block);
            return nullptr;
        }

        tmm_push_syntax(&l_block->m_body, l_statement);
    }

    tm_errorf("tmm: expected closing brace '}' at end of block.\n");
    tmm_destroy_syntax((tmm_syntax_t*) l_block);
    return nullptr;
}

tmm_syntax_t* tmm_parse_statement ()
{
    // Peek the next token.
    const tmm_token_t* l_token = tmm_peek_token(0);

    // If the token is a period, then a directive is being parsed.
    if (l_token->m_type == TMM_TOKEN_PERIOD)
    {
        tmm_advance_token();
        return tmm_parse_directive();
    }

    // If the token is an open brace, then a block is being parsed.
    else if (l_token->m_type == TMM_TOKEN_OPEN_BRACE)
    {
        tmm_advance_token();
        return tmm_parse_block();
    }

    // Check to see if the token is a keyword.
    else if (l_token->m_type == TMM_TOKEN_KEYWORD)
    {
        tmm_advance_token();
        const tmm_keyword_t* l_keyword = tmm_lookup_keyword(l_token->m_name, TMM_KEYWORD_NONE);
        switch (l_keyword->m_type)
        {
            case TMM_KEYWORD_INSTRUCTION:   return tmm_parse_instruction_statement(l_keyword);
            default:
            {
                tm_errorf("tmm: unexpected keyword '%s' in statement.\n", l_token->m_name);
                return nullptr;
            } break;
        }
    }
    
    // Parse an expression statement.
    tmm_syntax_t* l_expression = tmm_parse_expression();

    // If the expression is not null, and the next token is a colon ':', then
    // we are parsing a label statement.
    if (l_expression != nullptr && tmm_advance_token_if_type(TMM_TOKEN_COLON) != nullptr)
    {
        return tmm_parse_label_statement(l_expression);
    }

    // Otherwise, we are parsing an expression statement.
    return l_expression;
}

/* Public Functions ***********************************************************/

void tmm_init_parser ()
{
    s_parser.m_root = (tmm_syntax_block_t*) tmm_create_syntax(TMM_SYNTAX_BLOCK, tmm_peek_token(0));
}

void tmm_shutdown_parser ()
{
    tmm_destroy_syntax((tmm_syntax_t*) s_parser.m_root);
    s_parser.m_root = nullptr;
}

bool tmm_parse_tokens (tmm_syntax_block_t* p_block)
{
    while (tmm_has_more_tokens() == true)
    {
        const tmm_token_t* l_token = tmm_peek_token(0);
        tmm_syntax_t* l_statement = tmm_parse_statement();
        if (l_statement == nullptr)
        {
            tm_errorf("tmm:   in file '%s:%zu'.\n", l_token->m_source_file, l_token->m_line);
            return false;
        }

        if (p_block != nullptr)
        {
            tmm_push_syntax(&p_block->m_body, l_statement);
        }
        else
        {
            tmm_push_syntax(&s_parser.m_root->m_body, l_statement);
        }
    }

    tmm_reset_lexer();
    return true;
}
