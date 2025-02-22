/// @file   tmm.keyword.h
/// @brief  contains reserved keywords used by the tmm assembly language.

#pragma once
#include <tm.common.h>

/* Constants ******************************************************************/

#define TMM_KEYWORD_STRLEN 16

/* Keyword Type Enumeration ***************************************************/

typedef enum tmm_keyword_type
{
    TMM_KEYWORD_NONE,           ///< Not a keyword.
    TMM_KEYWORD_LANGUAGE,       ///< Language keyword (`let`, `const`, etc.)
    TMM_KEYWORD_DIRECTIVE,      ///< Directive keyword (`.org`, `.include`, etc.)
    TMM_KEYWORD_REGISTER,       ///< Register keyword (`a`, `b`, `c`, etc.)
    TMM_KEYWORD_FLAG,           ///< Flag keyword (`z`, `n`, `h`, etc.)
    TMM_KEYWORD_CONDITION,      ///< Condition keyword (`n`, `cs`, `cc`, etc.)
    TMM_KEYWORD_INSTRUCTION,    ///< Instruction keyword (`nop`, `stop`, etc.)
} tmm_keyword_type_t;

/* Language Keyword Type Enumeration ******************************************/

// enum tmm_language_type
// {
    
// };

/* Directive Keyword Type Enumeration *****************************************/

enum tmm_directive_type
{
    TMM_DIRECTIVE_ORG,      ///< Set the origin address.
    TMM_DIRECTIVE_INCLUDE,  ///< Include a file.
    TMM_DIRECTIVE_INCBIN,   ///< Include a binary file.
    TMM_DIRECTIVE_BYTE,     ///< Define a byte.
    TMM_DIRECTIVE_WORD,     ///< Define a word.
    TMM_DIRECTIVE_LONG,     ///< Define a long.
    TMM_DIRECTIVE_DEFINE,   ///< Define a macro.
    TMM_DIRECTIVE_UNDEF,    ///< Undefine a macro.
    TMM_DIRECTIVE_IF,       ///< Conditional directive.
    TMM_DIRECTIVE_ELSE,     ///< Conditional directive.
    TMM_DIRECTIVE_ENDIF,    ///< Conditional directive.
};

/* Keyword Structure **********************************************************/

typedef struct tmm_keyword
{
    char                m_name[TMM_KEYWORD_STRLEN]; ///< Keyword name.
    tmm_keyword_type_t  m_type;                     ///< Keyword type.
    enum_t              m_subtype;                  ///< Keyword subtype.
    int32_t             m_param;                    ///< Keyword extra parameter.
} tmm_keyword_t;

/* Public Functions ***********************************************************/

const tmm_keyword_t* tmm_lookup_keyword (const char* p_name, tmm_keyword_type_t p_type);
const char* tmm_stringify_keyword_type (tmm_keyword_type_t p_type);
