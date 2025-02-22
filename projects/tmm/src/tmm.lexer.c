/// @file tmm.lexer.c

#include <tmm.lexer.h>

/* Lexer Context Structure ****************************************************/

static struct
{
    tmm_token_t*    m_tokens;
    size_t          m_token_size;
    size_t          m_token_capacity;
    size_t          m_token_pointer;

    char**          m_include_files;
    size_t          m_include_size;
    size_t          m_include_capacity;

    const char*     m_current_file;
    size_t          m_current_line;
} s_lexer = {
    .m_tokens           = nullptr,
    .m_token_size       = 0,
    .m_token_capacity   = 0,
    .m_token_pointer    = 0,
    .m_include_files    = nullptr,
    .m_include_size     = 0,
    .m_include_capacity = 0,
    .m_current_file     = nullptr,
    .m_current_line     = 0
};

/* Static Functions ***********************************************************/

static void tmm_resize_tokens ()
{
    if (s_lexer.m_token_size + 1 >= s_lexer.m_token_capacity)
    {
        s_lexer.m_token_capacity *= 2;
        tmm_token_t* l_reallocated = tm_realloc(s_lexer.m_tokens, s_lexer.m_token_capacity, tmm_token_t);
        tm_expect_p(l_reallocated, "tmm: failed to reallocate memory for lexer tokens");

        s_lexer.m_tokens = l_reallocated;
    }
}

static void tmm_resize_includes ()
{
    if (s_lexer.m_include_size + 1 >= s_lexer.m_include_capacity)
    {
        s_lexer.m_include_capacity *= 2;
        char** l_reallocated = tm_realloc(s_lexer.m_include_files, s_lexer.m_include_capacity, char*);
        tm_expect_p(l_reallocated, "tmm: failed to reallocate memory for include filenames");

        s_lexer.m_include_files = l_reallocated;
    }
}

static bool tmm_add_include_file (const char* p_relative_filename, const char** p_absolute_filename)
{
    tm_expect(p_relative_filename != nullptr, "tmm: relative filename is null!\n");
    tm_expect(p_absolute_filename != nullptr, "tmm: absolute filename is null!\n");

    char* l_absolute_filename = realpath(p_relative_filename, nullptr);
    if (l_absolute_filename == nullptr)
    {
        if (errno == ENOENT)
        {
            tm_errorf("tmm: include file '%s' not found.\n", p_relative_filename);
            return false;
        }

        tm_perrorf("tmm: failed to resolve include file '%s'", p_relative_filename);
        return false;
    }

    for (size_t i = 0; i < s_lexer.m_include_size; ++i)
    {
        if (strncmp(s_lexer.m_include_files[i], l_absolute_filename, PATH_MAX) == 0)
        {
            tm_free(l_absolute_filename)
            *p_absolute_filename = nullptr;
            return true;
        }
    }

    tmm_resize_includes();
    s_lexer.m_include_files[s_lexer.m_include_size++] = l_absolute_filename;
    *p_absolute_filename = l_absolute_filename;
    return true;
}

static bool tmm_insert_token (tmm_token_type_t p_type, const char* p_value)
{
    tmm_resize_tokens();

    tmm_token_t* l_token = &s_lexer.m_tokens[s_lexer.m_token_size++];
    l_token->m_type = p_type;
    l_token->m_line = s_lexer.m_current_line;
    l_token->m_source_file = s_lexer.m_current_file;
    
    if (p_value != nullptr && p_value[0] != '\0')
    {
        strncpy(l_token->m_name, p_value, TMM_TOKEN_STRLEN);
    }
    else
    {
        memset(l_token->m_name, 0, TMM_TOKEN_STRLEN);
    }

    return true;
}

static bool tmm_collect_identifier (file_t* p_file, int* p_character)
{
    char l_lowercase[TMM_TOKEN_STRLEN] = { 0 };
    char l_buffer[TMM_TOKEN_STRLEN] = { 0 };
    size_t l_index = 0;

    do
    {
        if (l_index >= TMM_TOKEN_STRLEN)
        {
            tm_errorf("tmm: identifier token is too long.\n");
            return false;
        }

        l_lowercase[l_index] = (char) tolower(*p_character);
        l_buffer[l_index++] = (char) *p_character;
        *p_character = fgetc(p_file);
    } while (isalnum(*p_character) || *p_character == '_');

    ungetc(*p_character, p_file);

    const tmm_keyword_t* l_keyword = tmm_lookup_keyword(l_lowercase, TMM_KEYWORD_NONE);
    if (l_keyword->m_type != TMM_KEYWORD_NONE)
    {
        return tmm_insert_token(TMM_TOKEN_KEYWORD, l_lowercase);
    }

    return tmm_insert_token(TMM_TOKEN_IDENTIFIER, l_buffer);
}

static bool tmm_collect_string (file_t* p_file, int* p_character)
{
    // Skip the opening double quote.
    *p_character = fgetc(p_file);

    char l_buffer[TMM_TOKEN_STRLEN] = { 0 };
    size_t l_index = 0;

    while (*p_character != '"')
    {
        if (l_index >= TMM_TOKEN_STRLEN)
        {
            tm_errorf("tmm: string token is too long.\n");
            return false;
        }

        l_buffer[l_index++] = (char) *p_character;
        *p_character = fgetc(p_file);

        if (*p_character == EOF)
        {
            tm_errorf("tmm: unexpected end of file in string token.\n");
            return false;
        }
    }

    return tmm_insert_token(TMM_TOKEN_STRING, l_buffer);
}

static bool tmm_collect_character (file_t* p_file, int* p_character)
{
    // Skip the opening single quote.
    *p_character = fgetc(p_file);

    // Collect one character. Account for possible escape sequences.
    char l_buffer[3] = { 0 };   // 3 bytes (1 for possible escape, 1 for character, 1 for null terminator).
    size_t l_index = 0;

    while (*p_character != '\'')
    {
        if (l_index >= 2)
        {
            tm_errorf("tmm: character token is too long.\n");
            return false;
        }

        if (*p_character == '\\')
        {
            l_buffer[l_index++] = (char) *p_character;
            *p_character = fgetc(p_file);

            if (*p_character == EOF)
            {
                tm_errorf("tmm: unexpected end of file in escaped character token.\n");
                return false;
            }
        }

        l_buffer[l_index++] = (char) *p_character;
        *p_character = fgetc(p_file);

        if (*p_character == EOF)
        {
            tm_errorf("tmm: unexpected end of file in character token.\n");
            return false;
        }
    }

    return tmm_insert_token(TMM_TOKEN_CHARACTER, l_buffer);
}

static bool tmm_collect_binary (file_t* p_file, int* p_character)
{
    char l_buffer[TMM_TOKEN_STRLEN] = { 0 };
    size_t l_index = 0;

    do
    {
        if (l_index >= TMM_TOKEN_STRLEN)
        {
            tm_errorf("tmm: binary token is too long.\n");
            return false;
        }

        l_buffer[l_index++] = (char) *p_character;
        *p_character = fgetc(p_file);
    } while (*p_character == '0' || *p_character == '1');

    ungetc(*p_character, p_file);
    return tmm_insert_token(TMM_TOKEN_BINARY, l_buffer);
}

static bool tmm_collect_octal (file_t* p_file, int* p_character)
{
    char l_buffer[TMM_TOKEN_STRLEN] = { 0 };
    size_t l_index = 0;

    do
    {
        if (l_index >= TMM_TOKEN_STRLEN)
        {
            tm_errorf("tmm: octal token is too long.\n");
            return false;
        }

        l_buffer[l_index++] = (char) *p_character;
        *p_character = fgetc(p_file);
    } while (isdigit(*p_character) && *p_character < '8');

    ungetc(*p_character, p_file);
    return tmm_insert_token(TMM_TOKEN_OCTAL, l_buffer);
}

static bool tmm_collect_hexadecimal (file_t* p_file, int* p_character)
{
    char l_buffer[TMM_TOKEN_STRLEN] = { 0 };
    size_t l_index = 0;

    do
    {
        if (l_index >= TMM_TOKEN_STRLEN)
        {
            tm_errorf("tmm: hexadecimal token is too long.\n");
            return false;
        }

        l_buffer[l_index++] = (char) *p_character;
        *p_character = fgetc(p_file);
    } while (isxdigit(*p_character));

    ungetc(*p_character, p_file);
    return tmm_insert_token(TMM_TOKEN_HEXADECIMAL, l_buffer);
}

static bool tmm_collect_number (file_t* p_file, int* p_character)
{
    if (*p_character == '0')
    {
        int l_peek = fgetc(p_file);
        if (l_peek == 'b' || l_peek == 'B')
        {
            return tmm_collect_binary(p_file, p_character);
        }
        else if (l_peek == 'x' || l_peek == 'X')
        {
            return tmm_collect_hexadecimal(p_file, p_character);
        }
        else if (l_peek == 'o' || l_peek == 'O')
        {
            return tmm_collect_octal(p_file, p_character);
        }
    }

    char l_buffer[TMM_TOKEN_STRLEN] = { 0 };
    size_t l_index = 0;
    bool l_is_float = false;

    do
    {
        if (*p_character == '.')
        {
            if (l_is_float == true) { break; }
            l_is_float = true;
        }

        if (l_index >= TMM_TOKEN_STRLEN)
        {
            tm_errorf("tmm: number token is too long.\n");
            return false;
        }

        l_buffer[l_index++] = (char) *p_character;
        *p_character = fgetc(p_file);
    } while (isdigit(*p_character) || *p_character == '.');

    ungetc(*p_character, p_file);
    return tmm_insert_token(TMM_TOKEN_NUMBER, l_buffer);
}

static bool tmm_collect_symbol (file_t* p_file, int* p_character)
{
    int l_peek1 = 0, l_peek2 = 0;

    switch (*p_character)
    {
        case '+':
            l_peek1 = fgetc(p_file);
            if (l_peek1 == '=')
            {
                return tmm_insert_token(TMM_TOKEN_ADD_ASSIGN, nullptr);
            }
            ungetc(l_peek1, p_file);
            return tmm_insert_token(TMM_TOKEN_ADD, nullptr);
        case '-':
            l_peek1 = fgetc(p_file);
            if (l_peek1 == '=')
            {
                return tmm_insert_token(TMM_TOKEN_SUB_ASSIGN, nullptr);
            }
            ungetc(l_peek1, p_file);
            return tmm_insert_token(TMM_TOKEN_SUBTRACT, nullptr);
        case '*':
            l_peek1 = fgetc(p_file);
            if (l_peek1 == '=')
            {
                return tmm_insert_token(TMM_TOKEN_MUL_ASSIGN, nullptr);
            }
            else if (l_peek1 == '*')
            {
                l_peek2 = fgetc(p_file);
                if (l_peek2 == '=')
                {
                    return tmm_insert_token(TMM_TOKEN_EXP_ASSIGN, nullptr);
                }
                ungetc(l_peek2, p_file);
                return tmm_insert_token(TMM_TOKEN_EXPONENT, nullptr);
            }
            ungetc(l_peek1, p_file);
            return tmm_insert_token(TMM_TOKEN_MULTIPLY, nullptr);
        case '/':
            l_peek1 = fgetc(p_file);
            if (l_peek1 == '=')
            {
                return tmm_insert_token(TMM_TOKEN_DIV_ASSIGN, nullptr);
            }
            ungetc(l_peek1, p_file);
            return tmm_insert_token(TMM_TOKEN_DIVIDE, nullptr);
        case '%':
            l_peek1 = fgetc(p_file);
            if (l_peek1 == '=')
            {
                return tmm_insert_token(TMM_TOKEN_MOD_ASSIGN, nullptr);
            }
            ungetc(l_peek1, p_file);
            return tmm_insert_token(TMM_TOKEN_MODULO, nullptr);
        case '&':
            l_peek1 = fgetc(p_file);
            if (l_peek1 == '=')
            {
                return tmm_insert_token(TMM_TOKEN_AND_ASSIGN, nullptr);
            }
            else if (l_peek1 == '&')
            {
                return tmm_insert_token(TMM_TOKEN_LOGICAL_AND, nullptr);
            }
            ungetc(l_peek1, p_file);
            return tmm_insert_token(TMM_TOKEN_BITWISE_AND, nullptr);
        case '|':
            l_peek1 = fgetc(p_file);
            if (l_peek1 == '=')
            {
                return tmm_insert_token(TMM_TOKEN_OR_ASSIGN, nullptr);
            }
            else if (l_peek1 == '|')
            {
                return tmm_insert_token(TMM_TOKEN_LOGICAL_OR, nullptr);
            }
            ungetc(l_peek1, p_file);
            return tmm_insert_token(TMM_TOKEN_BITWISE_OR, nullptr);
        case '^':
            l_peek1 = fgetc(p_file);
            if (l_peek1 == '=')
            {
                return tmm_insert_token(TMM_TOKEN_XOR_ASSIGN, nullptr);
            }
            ungetc(l_peek1, p_file);
            return tmm_insert_token(TMM_TOKEN_BITWISE_XOR, nullptr);
        case '~':
            return tmm_insert_token(TMM_TOKEN_BITWISE_NOT, nullptr);
        case '<':
            l_peek1 = fgetc(p_file);
            if (l_peek1 == '=')
            {
                return tmm_insert_token(TMM_TOKEN_LESS_EQUAL, nullptr);
            }
            else if (l_peek1 == '<')
            {
                l_peek2 = fgetc(p_file);
                if (l_peek2 == '=')
                {
                    return tmm_insert_token(TMM_TOKEN_LSHIFT_ASSIGN, nullptr);
                }
                ungetc(l_peek2, p_file);
                return tmm_insert_token(TMM_TOKEN_BITWISE_LSHIFT, nullptr);
            }
            ungetc(l_peek1, p_file);
            return tmm_insert_token(TMM_TOKEN_LESS, nullptr);
        case '>':
            l_peek1 = fgetc(p_file);
            if (l_peek1 == '=')
            {
                return tmm_insert_token(TMM_TOKEN_GREATER_EQUAL, nullptr);
            }
            else if (l_peek1 == '>')
            {
                l_peek2 = fgetc(p_file);
                if (l_peek2 == '=')
                {
                    return tmm_insert_token(TMM_TOKEN_RSHIFT_ASSIGN, nullptr);
                }
                ungetc(l_peek2, p_file);
                return tmm_insert_token(TMM_TOKEN_BITWISE_RSHIFT, nullptr);
            }
            ungetc(l_peek1, p_file);
            return tmm_insert_token(TMM_TOKEN_GREATER, nullptr);
        case '=':
            l_peek1 = fgetc(p_file);
            if (l_peek1 == '=')
            {
                return tmm_insert_token(TMM_TOKEN_EQUAL, nullptr);
            }
            else if (l_peek1 == '>')
            {
                return tmm_insert_token(TMM_TOKEN_ARROW, nullptr);
            }
            ungetc(l_peek1, p_file);
            return tmm_insert_token(TMM_TOKEN_ASSIGN, nullptr);
        case '!':
            l_peek1 = fgetc(p_file);
            if (l_peek1 == '=')
            {
                return tmm_insert_token(TMM_TOKEN_NOT_EQUAL, nullptr);
            }
            ungetc(l_peek1, p_file);
            return tmm_insert_token(TMM_TOKEN_LOGICAL_NOT, nullptr);
        case ',':
            return tmm_insert_token(TMM_TOKEN_COMMA, nullptr);
        case ';':
            return tmm_insert_token(TMM_TOKEN_SEMICOLON, nullptr);
        case ':':
            return tmm_insert_token(TMM_TOKEN_COLON, nullptr);
        case '.':
            return tmm_insert_token(TMM_TOKEN_PERIOD, nullptr);
        case '?':
            return tmm_insert_token(TMM_TOKEN_QUESTION, nullptr);
        case '(':
            return tmm_insert_token(TMM_TOKEN_OPEN_PAREN, nullptr);
        case ')':
            return tmm_insert_token(TMM_TOKEN_CLOSE_PAREN, nullptr);
        case '[':
            return tmm_insert_token(TMM_TOKEN_OPEN_BRACKET, nullptr);
        case ']':
            return tmm_insert_token(TMM_TOKEN_CLOSE_BRACKET, nullptr);
        case '{':
            return tmm_insert_token(TMM_TOKEN_OPEN_BRACE, nullptr);
        case '}':
            return tmm_insert_token(TMM_TOKEN_CLOSE_BRACE, nullptr);
        default:
            tm_errorf("tmm: unexpected symbol '%c' at line %zu in file '%s'.\n", *p_character, s_lexer.m_current_line, s_lexer.m_current_file);
            return false;
    }
}

static bool tmm_collect_tokens (file_t* p_file)
{
    tm_expect(p_file != nullptr, "tmm: file is null!\n");
    
    int     l_character = 0;
    bool    l_line_comment = false;
    bool    l_block_comment = false;
    bool    l_good = false;

    while (true)
    {

        // Get the next character from the stream.
        l_character = fgetc(p_file);

        // Check for end of file.
        if (l_character == EOF)
        {
            return tmm_insert_token(TMM_TOKEN_EOF, nullptr);
        }

        // Check for new line. Disable comment mode if found.
        if (l_character == '\n')
        {
            s_lexer.m_current_line++;
            l_line_comment = false;
            tmm_insert_token(TMM_TOKEN_EOL, nullptr);
            continue;
        }

        // If we are in a comment block, check for the end of the comment.
        if (l_block_comment == true)
        {
            if (l_character == '*')
            {
                l_character = fgetc(p_file);
                if (l_character == '/')
                {
                    l_block_comment = false;
                }
            }
            continue;
        }

        // If we are in a line comment, ignore the rest of the line.
        if (l_line_comment == true)
        {
            continue;
        }

        // Check for whitespace.
        if (isspace(l_character))
        {
            continue;
        }

        // Check for line or block comment.
        if (l_character == '/')
        {
            l_character = fgetc(p_file);
            if (l_character == '/')
            {
                l_line_comment = true;
                continue;
            }
            else if (l_character == '*')
            {
                l_block_comment = true;
                continue;
            }

            ungetc(l_character, p_file);
        }

        // Check for an identifier or keyword.
        if (isalpha(l_character) || l_character == '_')
        {
            l_good = tmm_collect_identifier(p_file, &l_character);
        }

        // Check for a string.
        else if (l_character == '"')
        {
            l_good = tmm_collect_string(p_file, &l_character);
        }

        // Check for a character.
        else if (l_character == '\'')
        {
            l_good = tmm_collect_character(p_file, &l_character);
        }

        // Check for a number.
        else if (isdigit(l_character))
        {
            l_good = tmm_collect_number(p_file, &l_character);
        }
        
        // Check for a symbol.
        else
        {
            l_good = tmm_collect_symbol(p_file, &l_character);
        }

        if (!l_good)
        {
            return false;
        }
    }
}

/* Public Functions ***********************************************************/

void tmm_init_lexer ()
{
    s_lexer.m_tokens = tm_malloc(TMM_LEXER_DEFAULT_CAPACITY, tmm_token_t);
    tm_expect_p(s_lexer.m_tokens, "tmm: failed to allocate memory for lexer tokens");

    s_lexer.m_token_capacity = TMM_LEXER_DEFAULT_CAPACITY;
    s_lexer.m_token_size = 0;
    s_lexer.m_token_pointer = 0;

    s_lexer.m_include_files = tm_malloc(TMM_LEXER_DEFAULT_CAPACITY, char*);
    tm_expect_p(s_lexer.m_include_files, "tmm: failed to allocate memory for include files");

    s_lexer.m_include_capacity = TMM_LEXER_DEFAULT_CAPACITY;
    s_lexer.m_include_size = 0;
}

void tmm_shutdown_lexer ()
{
    for (size_t i = 0; i < s_lexer.m_include_size; ++i)
    {
        tm_free(s_lexer.m_include_files[i]);
    }

    tm_free(s_lexer.m_include_files);
    tm_free(s_lexer.m_tokens);
}

bool tmm_lex_file (const char* p_filename)
{
    tm_expect(p_filename != nullptr, "tmm: filename is null!\n");
    
    if (p_filename[0] == '\0')
    {
        tm_errorf("tmm: lex filename is empty.\n");
        return false;
    }

    const char* l_absolute_filename = nullptr;
    if (!tmm_add_include_file(p_filename, &l_absolute_filename))
    {
        return false;
    }
    else if (l_absolute_filename == nullptr)
    {
        return true;
    }

    file_t* l_file = fopen(p_filename, "r");
    if (l_file == nullptr)
    {
        tm_perrorf("tmm: failed to open file '%s'", p_filename);
        return false;
    }

    s_lexer.m_current_file = l_absolute_filename;
    s_lexer.m_current_line = 1;

    bool l_success = tmm_collect_tokens(l_file);
    if (!l_success)
    {
        tm_errorf("tmm:   while lexing file '%s'.\n", p_filename);
    }

    fclose(l_file);
    return l_success;
}

void tmm_print_tokens ()
{
    for (size_t i = 0; i < s_lexer.m_token_size; ++i)
    {
        tmm_token_t* l_token = &s_lexer.m_tokens[i];
        tm_printf("\t%zu: '%s'", i + 1, tmm_stringify_token_type(l_token->m_type));
        if (l_token->m_name[0] != '\0')
        {
            tm_printf(" = '%s'", l_token->m_name);
        }
        tm_printf("\n");
    }
}