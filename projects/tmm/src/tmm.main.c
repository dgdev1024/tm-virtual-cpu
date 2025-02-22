#include <tm.arguments.h>
#include <tmm.lexer.h>
#include <tmm.parser.h>

static void tmm_atexit ()
{
    tmm_shutdown_parser();
    tmm_shutdown_lexer();
    tm_release_arguments ();
}

static int tmm_print_help (bool p_error)
{
    FILE* l_output = p_error ? stderr : stdout;

    if (p_error == false)
    {
        fprintf(l_output, "tmm - TM CPU Assembler\n");
        fprintf(l_output, "By: Dennis Griffin\n\n");
    }

    fprintf(l_output, "Usage: tmm [options]\n");
    fprintf(l_output, "Options:\n");
    fprintf(l_output, "  -i, --input-file <filename>  Specify the input file to process.\n");
    fprintf(l_output, "  -l, --lex-only               Only perform lexical analysis.\n");
    fprintf(l_output, "  -h, --help                   Display this help message.\n");
    return p_error ? EXIT_FAILURE : EXIT_SUCCESS;
}

int main (int p_argc, char** p_argv)
{
    atexit(tmm_atexit);
    tm_capture_arguments(p_argc, p_argv);

    const char* l_input_file    = tm_get_argument_value("input-file", 'i');
    bool        l_lex_only      = tm_has_argument("lex-only", 'l');
    bool        l_help          = tm_has_argument("help", 'h');

    if (l_help)
    {
        return tmm_print_help(false);
    }

    if (l_input_file == nullptr)
    {
        tm_errorf("tmm: no input file specified.\n");
        return tmm_print_help(true);
    }

    tmm_init_lexer();
    if (!tmm_lex_file(l_input_file))
    {
        tm_errorf("tmm: failed to lex input file '%s'.\n", l_input_file);
        return EXIT_FAILURE;
    }

    if (l_lex_only)
    {
        tmm_print_tokens();
        return 0;
    }

    tmm_init_parser();
    if (!tmm_parse_tokens(nullptr))
    {
        tm_errorf("tmm: failed to parse input file '%s'.\n", l_input_file);
        return EXIT_FAILURE;
    }

    return 0;
}
