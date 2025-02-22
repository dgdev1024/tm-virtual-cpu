#include <tm.arguments.h>

static void tml_atexit ()
{
    tm_release_arguments ();
}

int main (int p_argc, char** p_argv)
{
    atexit(tml_atexit);
    tm_capture_arguments(p_argc, p_argv);

    return 0;
}
