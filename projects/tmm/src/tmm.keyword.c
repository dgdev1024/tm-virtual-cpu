/// @file tmm.keyword.c

#include <tmm.keyword.h>

/* Keyword Lookup Table *******************************************************/

static const tmm_keyword_t TMM_KEYWORDS[] = {

    // Language Keywords

    // Directive Keywords
    { "org",        TMM_KEYWORD_DIRECTIVE,      TMM_DIRECTIVE_ORG,      0 },
    { "include",    TMM_KEYWORD_DIRECTIVE,      TMM_DIRECTIVE_INCLUDE,  0 },
    { "incbin",     TMM_KEYWORD_DIRECTIVE,      TMM_DIRECTIVE_INCBIN,   0 },
    { "define",     TMM_KEYWORD_DIRECTIVE,      TMM_DIRECTIVE_DEFINE,   0 },
    { "undef",      TMM_KEYWORD_DIRECTIVE,      TMM_DIRECTIVE_UNDEF,    0 },
    { "if",         TMM_KEYWORD_DIRECTIVE,      TMM_DIRECTIVE_IF,       0 },
    { "else",       TMM_KEYWORD_DIRECTIVE,      TMM_DIRECTIVE_ELSE,     0 },
    { "endif",      TMM_KEYWORD_DIRECTIVE,      TMM_DIRECTIVE_ENDIF,    0 },
    { "byte",       TMM_KEYWORD_DIRECTIVE,      TMM_DIRECTIVE_BYTE,     0 },
    { "word",       TMM_KEYWORD_DIRECTIVE,      TMM_DIRECTIVE_WORD,     0 },
    { "long",       TMM_KEYWORD_DIRECTIVE,      TMM_DIRECTIVE_LONG,     0 },

    // Register Keywords
    { "a",          TMM_KEYWORD_REGISTER,       TM_REGISTER_A,          0 },
    { "aw",         TMM_KEYWORD_REGISTER,       TM_REGISTER_AW,         0 },
    { "ah",         TMM_KEYWORD_REGISTER,       TM_REGISTER_AH,         0 },
    { "al",         TMM_KEYWORD_REGISTER,       TM_REGISTER_AL,         0 },
    { "b",          TMM_KEYWORD_REGISTER,       TM_REGISTER_B,          0 },
    { "bw",         TMM_KEYWORD_REGISTER,       TM_REGISTER_BW,         0 },
    { "bh",         TMM_KEYWORD_REGISTER,       TM_REGISTER_BH,         0 },
    { "bl",         TMM_KEYWORD_REGISTER,       TM_REGISTER_BL,         0 },
    { "c",          TMM_KEYWORD_REGISTER,       TM_REGISTER_C,          0 },
    { "cw",         TMM_KEYWORD_REGISTER,       TM_REGISTER_CW,         0 },
    { "ch",         TMM_KEYWORD_REGISTER,       TM_REGISTER_CH,         0 },
    { "cl",         TMM_KEYWORD_REGISTER,       TM_REGISTER_CL,         0 },
    { "d",          TMM_KEYWORD_REGISTER,       TM_REGISTER_D,          0 },
    { "dw",         TMM_KEYWORD_REGISTER,       TM_REGISTER_DW,         0 },
    { "dh",         TMM_KEYWORD_REGISTER,       TM_REGISTER_DH,         0 },
    { "dl",         TMM_KEYWORD_REGISTER,       TM_REGISTER_DL,         0 },

    // Flag Keywords
    { "z",          TMM_KEYWORD_FLAG,           TM_FLAG_Z,              0 },
    { "n",          TMM_KEYWORD_FLAG,           TM_FLAG_N,              0 },
    { "h",          TMM_KEYWORD_FLAG,           TM_FLAG_H,              0 },
    { "c",          TMM_KEYWORD_FLAG,           TM_FLAG_C,              0 },
    { "o",          TMM_KEYWORD_FLAG,           TM_FLAG_O,              0 },
    { "u",          TMM_KEYWORD_FLAG,           TM_FLAG_U,              0 },
    { "l",          TMM_KEYWORD_FLAG,           TM_FLAG_L,              0 },
    { "s",          TMM_KEYWORD_FLAG,           TM_FLAG_S,              0 },

    // Condition Keywords
    { "nc",         TMM_KEYWORD_CONDITION,      TM_CONDITION_N,         0 },
    { "cs",         TMM_KEYWORD_CONDITION,      TM_CONDITION_CS,        0 },
    { "cc",         TMM_KEYWORD_CONDITION,      TM_CONDITION_CC,        0 },
    { "zs",         TMM_KEYWORD_CONDITION,      TM_CONDITION_ZS,        0 },
    { "zc",         TMM_KEYWORD_CONDITION,      TM_CONDITION_ZC,        0 },
    { "os",         TMM_KEYWORD_CONDITION,      TM_CONDITION_OS,        0 },
    { "us",         TMM_KEYWORD_CONDITION,      TM_CONDITION_US,        0 },

    // Instruction Keywords
    // The extra parameter is the number of operands required by each instruction.
    { "nop",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_NOP,     0 },
    { "stop",       TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_STOP,    0 },
    { "halt",       TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_HALT,    0 },
    { "sec",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_SEC,     1 },
    { "cec",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_CEC,     0 },
    { "di",         TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_DI,      0 },
    { "ei",         TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_EI,      0 },
    { "daa",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_DAA,     0 },
    { "cpl",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_CPL,     0 },
    { "cpw",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_CPW,     0 },
    { "cpb",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_CPB,     0 },
    { "scf",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_SCF,     0 },
    { "ccf",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_CCF,     0 },
    { "ld",         TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_LD,      2 },
    { "ldq",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_LDQ,     2 },
    { "ldh",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_LDH,     2 },
    { "st",         TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_ST,      2 },
    { "stq",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_STQ,     2 },
    { "sth",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_STH,     2 },
    { "mv",         TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_MV,      2 },
    { "push",       TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_PUSH,    1 },
    { "pop",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_POP,     1 },
    { "jmp",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_JMP,     2 },
    { "jpb",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_JPB,     2 },
    { "call",       TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_CALL,    2 },
    { "rst",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_RST,     1 },
    { "ret",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_RET,     1 },
    { "reti",       TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_RETI,    0 },
    { "inc",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_INC,     1 },
    { "dec",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_DEC,     1 },
    { "add",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_ADD,     2 },
    { "adc",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_ADC,     2 },
    { "sub",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_SUB,     2 },
    { "sbc",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_SBC,     2 },
    { "and",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_AND,     2 },
    { "or",         TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_OR,      2 },
    { "xor",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_XOR,     2 },
    { "cmp",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_CMP,     2 },
    { "sla",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_SLA,     1 },
    { "sra",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_SRA,     1 },
    { "srl",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_SRL,     1 },
    { "rl",         TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_RL,      1 },
    { "rlc",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_RLC,     1 },
    { "rr",         TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_RR,      1 },
    { "rrc",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_RRC,     1 },
    { "bit",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_BIT,     2 },
    { "set",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_SET,     2 },
    { "res",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_RES,     2 },
    { "swap",       TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_SWAP,    1 },
    { "jps",        TMM_KEYWORD_INSTRUCTION,    TM_INSTRUCTION_JPS,     0 },

    // End Of Table
    { "",           TMM_KEYWORD_NONE }

};

/* Public Functions ***********************************************************/

const tmm_keyword_t* tmm_lookup_keyword (const char* p_name, tmm_keyword_type_t p_type)
{
    for (size_t i = 0; ; ++i)
    {
        const tmm_keyword_t* l_keyword = &TMM_KEYWORDS[i];
        if (l_keyword->m_type == TMM_KEYWORD_NONE)
        {
            return l_keyword;
        }
        if (p_type == TMM_KEYWORD_NONE || l_keyword->m_type == p_type)
        {
            if (strncmp(p_name, l_keyword->m_name, TMM_KEYWORD_STRLEN) == 0)
            {
                return l_keyword;
            }
        }
    }
}

const char* tmm_stringify_keyword_type (tmm_keyword_type_t p_type)
{
    switch (p_type)
    {
        case TMM_KEYWORD_NONE:          return "none";
        case TMM_KEYWORD_LANGUAGE:      return "language";
        case TMM_KEYWORD_DIRECTIVE:     return "directive";
        case TMM_KEYWORD_REGISTER:      return "register";
        case TMM_KEYWORD_FLAG:          return "flag";
        case TMM_KEYWORD_CONDITION:     return "condition";
        case TMM_KEYWORD_INSTRUCTION:   return "instruction";
    }
    return "unknown";
}
