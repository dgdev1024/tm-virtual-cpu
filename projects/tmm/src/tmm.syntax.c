/// @file tmm.syntax.c

#include <tmm.syntax.h>

/* Static Functions - Syntax Body Management **********************************/

static void tmm_init_syntax_body (tmm_syntax_body_t* p_body)
{
    tm_assert(p_body);

    p_body->m_nodes = tm_malloc(TMM_SYNTAX_BODY_CAPACITY, tmm_syntax_t*);
    tm_expect_p(p_body->m_nodes, "tmm: failed to allocate syntax body nodes");

    p_body->m_count = 0;
    p_body->m_capacity = TMM_SYNTAX_BODY_CAPACITY;
}

static void tmm_destroy_syntax_body (tmm_syntax_body_t* p_body)
{
    if (p_body != nullptr)
    {
        for (size_t i = 0; i < p_body->m_count; ++i)
        {
            tmm_destroy_syntax(p_body->m_nodes[i]);
        }

        tm_free(p_body->m_nodes);
    }
}

static void tmm_resize_syntax_body (tmm_syntax_body_t* p_body)
{
    tm_assert(p_body);
    if (p_body->m_count + 1 >= p_body->m_capacity)
    {
        p_body->m_capacity *= 2;
        tmm_syntax_t** l_reallocated = tm_realloc(p_body->m_nodes, p_body->m_capacity, tmm_syntax_t*);
        tm_expect_p(l_reallocated, "tmm: failed to reallocate syntax body nodes");

        p_body->m_nodes = l_reallocated;
    }
}

/* Static Functions - Syntax Node Creation ************************************/

static tmm_syntax_block_t* tmm_create_syntax_block (const tmm_token_t* p_token)
{
    tm_assert(p_token);

    tmm_syntax_block_t* l_block = tm_calloc(1, tmm_syntax_block_t);
    tm_expect_p(l_block, "tmm: failed to allocate syntax block");

    l_block->m_type = TMM_SYNTAX_BLOCK;
    l_block->m_token = *p_token;
    tmm_init_syntax_body(&l_block->m_body);

    return l_block;
}

static tmm_syntax_directive_org_t* tmm_create_syntax_directive_org (const tmm_token_t* p_token)
{
    tm_assert(p_token);

    tmm_syntax_directive_org_t* l_directive = tm_calloc(1, tmm_syntax_directive_org_t);
    tm_expect_p(l_directive, "tmm: failed to allocate syntax directive");

    l_directive->m_type = TMM_SYNTAX_DIRECTIVE_ORG;
    l_directive->m_token = *p_token;
    l_directive->m_expression = nullptr;

    return l_directive;
}

static tmm_syntax_directive_include_t* tmm_create_syntax_directive_include (const tmm_token_t* p_token)
{
    tm_assert(p_token);

    tmm_syntax_directive_include_t* l_directive = tm_calloc(1, tmm_syntax_directive_include_t);
    tm_expect_p(l_directive, "tmm: failed to allocate syntax directive");

    l_directive->m_type = TMM_SYNTAX_DIRECTIVE_INCLUDE;
    l_directive->m_token = *p_token;
    l_directive->m_expression = nullptr;

    return l_directive;
}

static tmm_syntax_directive_incbin_t* tmm_create_syntax_directive_incbin (const tmm_token_t* p_token)
{
    tm_assert(p_token);

    tmm_syntax_directive_incbin_t* l_directive = tm_calloc(1, tmm_syntax_directive_incbin_t);
    tm_expect_p(l_directive, "tmm: failed to allocate syntax directive");

    l_directive->m_type = TMM_SYNTAX_DIRECTIVE_INCBIN;
    l_directive->m_token = *p_token;
    l_directive->m_expression = nullptr;
    l_directive->m_offset = nullptr;
    l_directive->m_length = nullptr;

    return l_directive;
}

static tmm_syntax_directive_define_t* tmm_create_syntax_directive_define (const tmm_token_t* p_token)
{
    tm_assert(p_token);

    tmm_syntax_directive_define_t* l_directive = tm_calloc(1, tmm_syntax_directive_define_t);
    tm_expect_p(l_directive, "tmm: failed to allocate syntax directive");

    l_directive->m_type = TMM_SYNTAX_DIRECTIVE_DEFINE;
    l_directive->m_token = *p_token;
    l_directive->m_identifier = nullptr;
    l_directive->m_statement = nullptr;

    return l_directive;
}

static tmm_syntax_directive_undef_t* tmm_create_syntax_directive_undef (const tmm_token_t* p_token)
{
    tm_assert(p_token);

    tmm_syntax_directive_undef_t* l_directive = tm_calloc(1, tmm_syntax_directive_undef_t);
    tm_expect_p(l_directive, "tmm: failed to allocate syntax directive");

    l_directive->m_type = TMM_SYNTAX_DIRECTIVE_UNDEF;
    l_directive->m_token = *p_token;
    l_directive->m_identifier = nullptr;

    return l_directive;
}

static tmm_syntax_directive_if_t* tmm_create_syntax_directive_if (const tmm_token_t* p_token)
{
    tm_assert(p_token);

    tmm_syntax_directive_if_t* l_directive = tm_calloc(1, tmm_syntax_directive_if_t);
    tm_expect_p(l_directive, "tmm: failed to allocate syntax directive");

    l_directive->m_type = TMM_SYNTAX_DIRECTIVE_IF;
    l_directive->m_token = *p_token;
    l_directive->m_expression = nullptr;

    return l_directive;
}

static tmm_syntax_directive_else_t* tmm_create_syntax_directive_else (const tmm_token_t* p_token)
{
    tm_assert(p_token);

    tmm_syntax_directive_else_t* l_directive = tm_calloc(1, tmm_syntax_directive_else_t);
    tm_expect_p(l_directive, "tmm: failed to allocate syntax directive");

    l_directive->m_type = TMM_SYNTAX_DIRECTIVE_ELSE;
    l_directive->m_token = *p_token;

    return l_directive;
}

static tmm_syntax_directive_endif_t* tmm_create_syntax_directive_endif (const tmm_token_t* p_token)
{
    tm_assert(p_token);

    tmm_syntax_directive_endif_t* l_directive = tm_calloc(1, tmm_syntax_directive_endif_t);
    tm_expect_p(l_directive, "tmm: failed to allocate syntax directive");

    l_directive->m_type = TMM_SYNTAX_DIRECTIVE_ENDIF;
    l_directive->m_token = *p_token;

    return l_directive;
}

static tmm_syntax_directive_byte_t* tmm_create_syntax_directive_byte (const tmm_token_t* p_token)
{
    tm_assert(p_token);

    tmm_syntax_directive_byte_t* l_directive = tm_calloc(1, tmm_syntax_directive_byte_t);
    tm_expect_p(l_directive, "tmm: failed to allocate syntax directive");

    l_directive->m_type = TMM_SYNTAX_DIRECTIVE_BYTE;
    l_directive->m_token = *p_token;
    tmm_init_syntax_body(&l_directive->m_body);

    return l_directive;
}

static tmm_syntax_directive_word_t* tmm_create_syntax_directive_word (const tmm_token_t* p_token)
{
    tm_assert(p_token);

    tmm_syntax_directive_word_t* l_directive = tm_calloc(1, tmm_syntax_directive_word_t);
    tm_expect_p(l_directive, "tmm: failed to allocate syntax directive");

    l_directive->m_type = TMM_SYNTAX_DIRECTIVE_WORD;
    l_directive->m_token = *p_token;
    tmm_init_syntax_body(&l_directive->m_body);

    return l_directive;
}

static tmm_syntax_directive_long_t* tmm_create_syntax_directive_long (const tmm_token_t* p_token)
{
    tm_assert(p_token);

    tmm_syntax_directive_long_t* l_directive = tm_calloc(1, tmm_syntax_directive_long_t);
    tm_expect_p(l_directive, "tmm: failed to allocate syntax directive");

    l_directive->m_type = TMM_SYNTAX_DIRECTIVE_LONG;
    l_directive->m_token = *p_token;
    tmm_init_syntax_body(&l_directive->m_body);

    return l_directive;
}

static tmm_syntax_statement_label_t* tmm_create_syntax_statement_label (const tmm_token_t* p_token)
{
    tm_assert(p_token);

    tmm_syntax_statement_label_t* l_statement = tm_calloc(1, tmm_syntax_statement_label_t);
    tm_expect_p(l_statement, "tmm: failed to allocate syntax statement");

    l_statement->m_type = TMM_SYNTAX_STATEMENT_LABEL;
    l_statement->m_token = *p_token;
    l_statement->m_identifier = nullptr;

    return l_statement;
}

static tmm_syntax_statement_instruction_t* tmm_create_syntax_statement_instruction (const tmm_token_t* p_token)
{
    tm_assert(p_token);

    tmm_syntax_statement_instruction_t* l_statement = tm_calloc(1, tmm_syntax_statement_instruction_t);
    tm_expect_p(l_statement, "tmm: failed to allocate syntax statement");

    l_statement->m_type = TMM_SYNTAX_STATEMENT_INSTRUCTION;
    l_statement->m_token = *p_token;
    tmm_init_syntax_body(&l_statement->m_operands);

    return l_statement;
}

static tmm_syntax_expression_binary_t* tmm_create_syntax_expression_binary (const tmm_token_t* p_token)
{
    tm_assert(p_token);

    tmm_syntax_expression_binary_t* l_expression = tm_calloc(1, tmm_syntax_expression_binary_t);
    tm_expect_p(l_expression, "tmm: failed to allocate syntax expression");

    l_expression->m_type = TMM_SYNTAX_EXPRESSION_BINARY;
    l_expression->m_token = *p_token;
    l_expression->m_left = nullptr;
    l_expression->m_right = nullptr;

    return l_expression;
}

static tmm_syntax_expression_unary_t* tmm_create_syntax_expression_unary (const tmm_token_t* p_token)
{
    tm_assert(p_token);

    tmm_syntax_expression_unary_t* l_expression = tm_calloc(1, tmm_syntax_expression_unary_t);
    tm_expect_p(l_expression, "tmm: failed to allocate syntax expression");

    l_expression->m_type = TMM_SYNTAX_EXPRESSION_UNARY;
    l_expression->m_token = *p_token;
    l_expression->m_operand = nullptr;

    return l_expression;
}

static tmm_syntax_expression_ternary_t* tmm_create_syntax_expression_ternary (const tmm_token_t* p_token)
{
    tm_assert(p_token);

    tmm_syntax_expression_ternary_t* l_expression = tm_calloc(1, tmm_syntax_expression_ternary_t);
    tm_expect_p(l_expression, "tmm: failed to allocate syntax expression");

    l_expression->m_type = TMM_SYNTAX_EXPRESSION_TERNARY;
    l_expression->m_token = *p_token;
    l_expression->m_condition = nullptr;
    l_expression->m_true = nullptr;
    l_expression->m_false = nullptr;

    return l_expression;
}

static tmm_syntax_expression_identifier_t* tmm_create_syntax_expression_identifier (const tmm_token_t* p_token)
{
    tm_assert(p_token);

    tmm_syntax_expression_identifier_t* l_expression = tm_calloc(1, tmm_syntax_expression_identifier_t);
    tm_expect_p(l_expression, "tmm: failed to allocate syntax expression");

    l_expression->m_type = TMM_SYNTAX_EXPRESSION_IDENTIFIER;
    l_expression->m_token = *p_token;

    return l_expression;
}

static tmm_syntax_expression_pointer_t* tmm_create_syntax_expression_pointer (const tmm_token_t* p_token)
{
    tm_assert(p_token);

    tmm_syntax_expression_pointer_t* l_expression = tm_calloc(1, tmm_syntax_expression_pointer_t);
    tm_expect_p(l_expression, "tmm: failed to allocate syntax expression");

    l_expression->m_type = TMM_SYNTAX_EXPRESSION_POINTER;
    l_expression->m_token = *p_token;

    return l_expression;
}

static tmm_syntax_expression_register_literal_t* tmm_create_syntax_expression_register_literal (const tmm_token_t* p_token)
{
    tm_assert(p_token);

    tmm_syntax_expression_register_literal_t* l_expression = tm_calloc(1, tmm_syntax_expression_register_literal_t);
    tm_expect_p(l_expression, "tmm: failed to allocate syntax expression");

    l_expression->m_type = TMM_SYNTAX_EXPRESSION_REGISTER_LITERAL;
    l_expression->m_token = *p_token;

    return l_expression;
}

static tmm_syntax_expression_condition_literal_t* tmm_create_syntax_expression_condition_literal (const tmm_token_t* p_token)
{
    tm_assert(p_token);

    tmm_syntax_expression_condition_literal_t* l_expression = tm_calloc(1, tmm_syntax_expression_condition_literal_t);
    tm_expect_p(l_expression, "tmm: failed to allocate syntax expression");

    l_expression->m_type = TMM_SYNTAX_EXPRESSION_CONDITION_LITERAL;
    l_expression->m_token = *p_token;

    return l_expression;
}

static tmm_syntax_expression_numeric_literal_t* tmm_create_syntax_expression_numeric_literal (const tmm_token_t* p_token)
{
    tm_assert(p_token);

    tmm_syntax_expression_numeric_literal_t* l_expression = tm_calloc(1, tmm_syntax_expression_numeric_literal_t);
    tm_expect_p(l_expression, "tmm: failed to allocate syntax expression");

    l_expression->m_type = TMM_SYNTAX_EXPRESSION_NUMERIC_LITERAL;
    l_expression->m_token = *p_token;

    return l_expression;
}

static tmm_syntax_expression_string_literal_t* tmm_create_syntax_expression_string_literal (const tmm_token_t* p_token)
{
    tm_assert(p_token);

    tmm_syntax_expression_string_literal_t* l_expression = tm_calloc(1, tmm_syntax_expression_string_literal_t);
    tm_expect_p(l_expression, "tmm: failed to allocate syntax expression");

    l_expression->m_type = TMM_SYNTAX_EXPRESSION_STRING_LITERAL;
    l_expression->m_token = *p_token;

    return l_expression;
}

static tmm_syntax_expression_placeholder_literal_t* tmm_create_syntax_expression_placeholder_literal (const tmm_token_t* p_token)
{
    tm_assert(p_token);

    tmm_syntax_expression_placeholder_literal_t* l_expression = tm_calloc(1, tmm_syntax_expression_placeholder_literal_t);
    tm_expect_p(l_expression, "tmm: failed to allocate syntax expression");

    l_expression->m_type = TMM_SYNTAX_EXPRESSION_PLACEHOLDER_LITERAL;
    l_expression->m_token = *p_token;

    return l_expression;
}

/* Static Functions - Syntax Node Destruction *********************************/

static void tmm_destroy_syntax_block (tmm_syntax_block_t* p_block)
{
    if (p_block != nullptr)
    {
        tmm_destroy_syntax_body(&p_block->m_body);
        tm_free(p_block);
    }
}

static void tmm_destroy_syntax_directive_org (tmm_syntax_directive_org_t* p_directive)
{
    if (p_directive != nullptr)
    {
        tmm_destroy_syntax(p_directive->m_expression);
        tm_free(p_directive);
    }
}

static void tmm_destroy_syntax_directive_include (tmm_syntax_directive_include_t* p_directive)
{
    if (p_directive != nullptr)
    {
        tmm_destroy_syntax(p_directive->m_expression);
        tm_free(p_directive);
    }
}

static void tmm_destroy_syntax_directive_incbin (tmm_syntax_directive_incbin_t* p_directive)
{
    if (p_directive != nullptr)
    {
        tmm_destroy_syntax(p_directive->m_expression);
        tmm_destroy_syntax(p_directive->m_offset);
        tmm_destroy_syntax(p_directive->m_length);
        tm_free(p_directive);
    }
}

static void tmm_destroy_syntax_directive_define (tmm_syntax_directive_define_t* p_directive)
{
    if (p_directive != nullptr)
    {
        tmm_destroy_syntax(p_directive->m_identifier);
        tmm_destroy_syntax(p_directive->m_statement);
        tm_free(p_directive);
    }
}

static void tmm_destroy_syntax_directive_undef (tmm_syntax_directive_undef_t* p_directive)
{
    if (p_directive != nullptr)
    {
        tmm_destroy_syntax(p_directive->m_identifier);
        tm_free(p_directive);
    }
}

static void tmm_destroy_syntax_directive_if (tmm_syntax_directive_if_t* p_directive)
{
    if (p_directive != nullptr)
    {
        tmm_destroy_syntax(p_directive->m_expression);
        tm_free(p_directive);
    }
}

static void tmm_destroy_syntax_directive_else (tmm_syntax_directive_else_t* p_directive)
{
    if (p_directive != nullptr)
    {
        tm_free(p_directive);
    }
}

static void tmm_destroy_syntax_directive_endif (tmm_syntax_directive_endif_t* p_directive)
{
    if (p_directive != nullptr)
    {
        tm_free(p_directive);
    }
}

static void tmm_destroy_syntax_directive_byte (tmm_syntax_directive_byte_t* p_directive)
{
    if (p_directive != nullptr)
    {
        tmm_destroy_syntax_body(&p_directive->m_body);
        tm_free(p_directive);
    }
}

static void tmm_destroy_syntax_directive_word (tmm_syntax_directive_word_t* p_directive)
{
    if (p_directive != nullptr)
    {
        tmm_destroy_syntax_body(&p_directive->m_body);
        tm_free(p_directive);
    }
}

static void tmm_destroy_syntax_directive_long (tmm_syntax_directive_long_t* p_directive)
{
    if (p_directive != nullptr)
    {
        tmm_destroy_syntax_body(&p_directive->m_body);
        tm_free(p_directive);
    }
}

static void tmm_destroy_syntax_statement_label (tmm_syntax_statement_label_t* p_statement)
{
    if (p_statement != nullptr)
    {
        tmm_destroy_syntax(p_statement->m_identifier);
        tm_free(p_statement);
    }
}

static void tmm_destroy_syntax_statement_instruction (tmm_syntax_statement_instruction_t* p_statement)
{
    if (p_statement != nullptr)
    {
        tmm_destroy_syntax_body(&p_statement->m_operands);
        tm_free(p_statement);
    }
}

static void tmm_destroy_syntax_expression_binary (tmm_syntax_expression_binary_t* p_expression)
{
    if (p_expression != nullptr)
    {
        tmm_destroy_syntax(p_expression->m_left);
        tmm_destroy_syntax(p_expression->m_right);
        tm_free(p_expression);
    }
}

static void tmm_destroy_syntax_expression_unary (tmm_syntax_expression_unary_t* p_expression)
{
    if (p_expression != nullptr)
    {
        tmm_destroy_syntax(p_expression->m_operand);
        tm_free(p_expression);
    }
}

static void tmm_destroy_syntax_expression_ternary (tmm_syntax_expression_ternary_t* p_expression)
{
    if (p_expression != nullptr)
    {
        tmm_destroy_syntax(p_expression->m_condition);
        tmm_destroy_syntax(p_expression->m_true);
        tmm_destroy_syntax(p_expression->m_false);
        tm_free(p_expression);
    }
}

static void tmm_destroy_syntax_expression_identifier (tmm_syntax_expression_identifier_t* p_expression)
{
    if (p_expression != nullptr)
    {
        tm_free(p_expression);
    }
}

static void tmm_destroy_syntax_expression_pointer (tmm_syntax_expression_pointer_t* p_expression)
{
    if (p_expression != nullptr)
    {
        tmm_destroy_syntax(p_expression->m_expression);
        tm_free(p_expression);
    }
}

static void tmm_destroy_syntax_expression_register_literal (tmm_syntax_expression_register_literal_t* p_expression)
{
    if (p_expression != nullptr)
    {
        tm_free(p_expression);
    }
}

static void tmm_destroy_syntax_expression_condition_literal (tmm_syntax_expression_condition_literal_t* p_expression)
{
    if (p_expression != nullptr)
    {
        tm_free(p_expression);
    }
}

static void tmm_destroy_syntax_expression_numeric_literal (tmm_syntax_expression_numeric_literal_t* p_expression)
{
    if (p_expression != nullptr)
    {
        tm_free(p_expression);
    }
}

static void tmm_destroy_syntax_expression_string_literal (tmm_syntax_expression_string_literal_t* p_expression)
{
    if (p_expression != nullptr)
    {
        tm_free(p_expression);
    }
}

static void tmm_destroy_syntax_expression_placeholder_literal (tmm_syntax_expression_placeholder_literal_t* p_expression)
{
    if (p_expression != nullptr)
    {
        tm_free(p_expression);
    }
}

/* Public Functions ***********************************************************/

tmm_syntax_t* tmm_create_syntax (tmm_syntax_type_t p_type, const tmm_token_t* p_token)
{
    tm_assert(p_token);

    switch (p_type)
    {
        case TMM_SYNTAX_BLOCK:                  
            return (tmm_syntax_t*) tmm_create_syntax_block(p_token);
        case TMM_SYNTAX_DIRECTIVE_ORG:          
            return (tmm_syntax_t*) tmm_create_syntax_directive_org(p_token);
        case TMM_SYNTAX_DIRECTIVE_INCLUDE:      
            return (tmm_syntax_t*) tmm_create_syntax_directive_include(p_token);
        case TMM_SYNTAX_DIRECTIVE_INCBIN:       
            return (tmm_syntax_t*) tmm_create_syntax_directive_incbin(p_token);
        case TMM_SYNTAX_DIRECTIVE_DEFINE:       
            return (tmm_syntax_t*) tmm_create_syntax_directive_define(p_token);
        case TMM_SYNTAX_DIRECTIVE_UNDEF:        
            return (tmm_syntax_t*) tmm_create_syntax_directive_undef(p_token);
        case TMM_SYNTAX_DIRECTIVE_IF:           
            return (tmm_syntax_t*) tmm_create_syntax_directive_if(p_token);
        case TMM_SYNTAX_DIRECTIVE_ELSE:         
            return (tmm_syntax_t*) tmm_create_syntax_directive_else(p_token);
        case TMM_SYNTAX_DIRECTIVE_ENDIF:        
            return (tmm_syntax_t*) tmm_create_syntax_directive_endif(p_token);
        case TMM_SYNTAX_DIRECTIVE_BYTE:         
            return (tmm_syntax_t*) tmm_create_syntax_directive_byte(p_token);
        case TMM_SYNTAX_DIRECTIVE_WORD:         
            return (tmm_syntax_t*) tmm_create_syntax_directive_word(p_token);
        case TMM_SYNTAX_DIRECTIVE_LONG:         
            return (tmm_syntax_t*) tmm_create_syntax_directive_long(p_token);
        case TMM_SYNTAX_STATEMENT_LABEL:        
            return (tmm_syntax_t*) tmm_create_syntax_statement_label(p_token);
        case TMM_SYNTAX_STATEMENT_INSTRUCTION:  
            return (tmm_syntax_t*) tmm_create_syntax_statement_instruction(p_token);
        case TMM_SYNTAX_EXPRESSION_BINARY:      
            return (tmm_syntax_t*) tmm_create_syntax_expression_binary(p_token);
        case TMM_SYNTAX_EXPRESSION_UNARY:       
            return (tmm_syntax_t*) tmm_create_syntax_expression_unary(p_token);
        case TMM_SYNTAX_EXPRESSION_TERNARY:
            return (tmm_syntax_t*) tmm_create_syntax_expression_ternary(p_token);
        case TMM_SYNTAX_EXPRESSION_IDENTIFIER:  
            return (tmm_syntax_t*) tmm_create_syntax_expression_identifier(p_token);
        case TMM_SYNTAX_EXPRESSION_POINTER:
            return (tmm_syntax_t*) tmm_create_syntax_expression_pointer(p_token);
        case TMM_SYNTAX_EXPRESSION_REGISTER_LITERAL:
            return (tmm_syntax_t*) tmm_create_syntax_expression_register_literal(p_token);
        case TMM_SYNTAX_EXPRESSION_CONDITION_LITERAL:
            return (tmm_syntax_t*) tmm_create_syntax_expression_condition_literal(p_token);
        case TMM_SYNTAX_EXPRESSION_NUMERIC_LITERAL:
            return (tmm_syntax_t*) tmm_create_syntax_expression_numeric_literal(p_token);
        case TMM_SYNTAX_EXPRESSION_STRING_LITERAL:
            return (tmm_syntax_t*) tmm_create_syntax_expression_string_literal(p_token);
        case TMM_SYNTAX_EXPRESSION_PLACEHOLDER_LITERAL:
            return (tmm_syntax_t*) tmm_create_syntax_expression_placeholder_literal(p_token);
        default:
            tm_expect(false, "tmm: attempt to create syntax node with invalid type: %d!\n", p_type);
    }
}

void tmm_destroy_syntax (tmm_syntax_t* p_syntax)
{
    if (p_syntax == nullptr)
    {
        return;
    }

#if defined(TM_DEBUG) && defined(TM_VERBOSE)
    tm_printf("tmm: destroying syntax node with type: %d\n", p_syntax->m_type);
#endif

    switch (p_syntax->m_type)
    {
        case TMM_SYNTAX_BLOCK:                  
            tmm_destroy_syntax_block((tmm_syntax_block_t*) p_syntax);
            break;
        case TMM_SYNTAX_DIRECTIVE_ORG:          
            tmm_destroy_syntax_directive_org((tmm_syntax_directive_org_t*) p_syntax);
            break;
        case TMM_SYNTAX_DIRECTIVE_INCLUDE:      
            tmm_destroy_syntax_directive_include((tmm_syntax_directive_include_t*) p_syntax);
            break;
        case TMM_SYNTAX_DIRECTIVE_INCBIN:       
            tmm_destroy_syntax_directive_incbin((tmm_syntax_directive_incbin_t*) p_syntax);
            break;
        case TMM_SYNTAX_DIRECTIVE_DEFINE:       
            tmm_destroy_syntax_directive_define((tmm_syntax_directive_define_t*) p_syntax);
            break;
        case TMM_SYNTAX_DIRECTIVE_UNDEF:        
            tmm_destroy_syntax_directive_undef((tmm_syntax_directive_undef_t*) p_syntax);
            break;
        case TMM_SYNTAX_DIRECTIVE_IF:           
            tmm_destroy_syntax_directive_if((tmm_syntax_directive_if_t*) p_syntax);
            break;
        case TMM_SYNTAX_DIRECTIVE_ELSE:         
            tmm_destroy_syntax_directive_else((tmm_syntax_directive_else_t*) p_syntax);
            break;
        case TMM_SYNTAX_DIRECTIVE_ENDIF:        
            tmm_destroy_syntax_directive_endif((tmm_syntax_directive_endif_t*) p_syntax);
            break;
        case TMM_SYNTAX_DIRECTIVE_BYTE:         
            tmm_destroy_syntax_directive_byte((tmm_syntax_directive_byte_t*) p_syntax);
            break;
        case TMM_SYNTAX_DIRECTIVE_WORD:         
            tmm_destroy_syntax_directive_word((tmm_syntax_directive_word_t*) p_syntax);
            break;
        case TMM_SYNTAX_DIRECTIVE_LONG:         
            tmm_destroy_syntax_directive_long((tmm_syntax_directive_long_t*) p_syntax);
            break;
        case TMM_SYNTAX_STATEMENT_LABEL:        
            tmm_destroy_syntax_statement_label((tmm_syntax_statement_label_t*) p_syntax);
            break;
        case TMM_SYNTAX_STATEMENT_INSTRUCTION:  
            tmm_destroy_syntax_statement_instruction((tmm_syntax_statement_instruction_t*) p_syntax);
            break;
        case TMM_SYNTAX_EXPRESSION_BINARY:
            tmm_destroy_syntax_expression_binary((tmm_syntax_expression_binary_t*) p_syntax);
            break;
        case TMM_SYNTAX_EXPRESSION_UNARY:
            tmm_destroy_syntax_expression_unary((tmm_syntax_expression_unary_t*) p_syntax);
            break;
        case TMM_SYNTAX_EXPRESSION_TERNARY:
            tmm_destroy_syntax_expression_ternary((tmm_syntax_expression_ternary_t*) p_syntax);
            break;
        case TMM_SYNTAX_EXPRESSION_IDENTIFIER:
            tmm_destroy_syntax_expression_identifier((tmm_syntax_expression_identifier_t*) p_syntax);
            break;
        case TMM_SYNTAX_EXPRESSION_POINTER:
            tmm_destroy_syntax_expression_pointer((tmm_syntax_expression_pointer_t*) p_syntax);
            break;
        case TMM_SYNTAX_EXPRESSION_REGISTER_LITERAL:
            tmm_destroy_syntax_expression_register_literal((tmm_syntax_expression_register_literal_t*) p_syntax);
            break;
        case TMM_SYNTAX_EXPRESSION_CONDITION_LITERAL:
            tmm_destroy_syntax_expression_condition_literal((tmm_syntax_expression_condition_literal_t*) p_syntax);
            break;
        case TMM_SYNTAX_EXPRESSION_NUMERIC_LITERAL:
            tmm_destroy_syntax_expression_numeric_literal((tmm_syntax_expression_numeric_literal_t*) p_syntax);
            break;
        case TMM_SYNTAX_EXPRESSION_STRING_LITERAL:
            tmm_destroy_syntax_expression_string_literal((tmm_syntax_expression_string_literal_t*) p_syntax);
            break;
        case TMM_SYNTAX_EXPRESSION_PLACEHOLDER_LITERAL:
            tmm_destroy_syntax_expression_placeholder_literal((tmm_syntax_expression_placeholder_literal_t*) p_syntax);
            break;
        default:
            tm_expect(false, "tmm: attempt to destroy syntax node with invalid type: %d!\n", p_syntax->m_type);
    }
}

void tmm_push_syntax (tmm_syntax_body_t* p_body, tmm_syntax_t* p_syntax)
{
    tm_assert(p_body);
    tm_assert(p_syntax);

    tmm_resize_syntax_body(p_body);
    p_body->m_nodes[p_body->m_count++] = p_syntax;
}
