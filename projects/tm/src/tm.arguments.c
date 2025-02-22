/// @file tm.arguments.c

#include <tm.arguments.h>

/* Static Members *************************************************************/

static int      s_argc = 0;
static char**   s_argv = nullptr;

/* Public Functions ***********************************************************/

void tm_capture_arguments (int p_argc, char** p_argv)
{
    s_argc = p_argc;
    s_argv = p_argv;
}

void tm_release_arguments ()
{
    s_argc = 0;
    s_argv = nullptr;
}

bool tm_has_argument (const char* p_longform, const char p_shortform)
{
    tm_expect(p_longform != nullptr, "tm: longform argument cannot be null!");
    tm_expect(p_longform[0] != '\0', "tm: longform argument cannot be empty!");
    tm_expect(p_shortform != '\0', "tm: shortform argument cannot be a null byte!");
    tm_expect(s_argv != nullptr, "tm: program arguments not captured!");

    for (int i = 1; i < s_argc; ++i)
    {
        if (s_argv[i] != nullptr && s_argv[i][0] == '-')
        {
            if (s_argv[i][1] == '-' && s_argv[i][2] != '\0')
            {
                if (strcmp(s_argv[i] + 2, p_longform) == 0)
                {
                    return true;
                }
            }
            else if (s_argv[i][1] != '\0')
            {
                if (strchr(s_argv[i] + 1, p_shortform) != nullptr)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

const char* tm_get_argument_value (const char* p_longform, const char p_shortform)
{
    tm_expect(p_longform != nullptr, "tm: longform argument cannot be null!");
    tm_expect(p_longform[0] != '\0', "tm: longform argument cannot be empty!");
    tm_expect(p_shortform != '\0', "tm: shortform argument cannot be a null byte!");
    tm_expect(s_argv != nullptr, "tm: program arguments not captured!");

    for (int i = 1; i < s_argc; ++i)
    {
        if (s_argv[i] != nullptr && s_argv[i][0] == '-')
        {
            if (s_argv[i][1] == '-' && s_argv[i][2] != '\0')
            {
                if (strcmp(s_argv[i] + 2, p_longform) == 0)
                {
                    if (i + 1 < s_argc && s_argv[i + 1] != nullptr && s_argv[i + 1][0] != '-')
                    {
                        return s_argv[i + 1];
                    }
                }
            }
            else if (s_argv[i][1] != '\0')
            {
                if (strchr(s_argv[i] + 1, p_shortform) != nullptr)
                {
                    if (i + 1 < s_argc && s_argv[i + 1] != nullptr && s_argv[i + 1][0] != '-')
                    {
                        return s_argv[i + 1];
                    }
                }
            }
        }
    }

    return nullptr;
}

const char* tm_get_argument_value_at (const char* p_longform, const char p_shortform, size_t p_index)
{
    tm_expect(p_longform != nullptr, "tm: longform argument cannot be null!");
    tm_expect(p_longform[0] != '\0', "tm: longform argument cannot be empty!");
    tm_expect(p_shortform != '\0', "tm: shortform argument cannot be a null byte!");
    tm_expect(s_argv != nullptr, "tm: program arguments not captured!");

    size_t count = 0;
    for (int i = 1; i < s_argc; ++i)
    {
        if (s_argv[i] != nullptr && s_argv[i][0] == '-')
        {
            if (s_argv[i][1] == '-' && s_argv[i][2] != '\0')
            {
                if (strcmp(s_argv[i] + 2, p_longform) == 0)
                {
                    if (i + 1 < s_argc && s_argv[i + 1] != nullptr && s_argv[i + 1][0] != '-')
                    {
                        if (count == p_index)
                        {
                            return s_argv[i + 1];
                        }
                        ++count;
                    }
                }
            }
            else if (s_argv[i][1] != '\0')
            {
                if (strchr(s_argv[i] + 1, p_shortform) != nullptr)
                {
                    if (i + 1 < s_argc && s_argv[i + 1] != nullptr && s_argv[i + 1][0] != '-')
                    {
                        if (count == p_index)
                        {
                            return s_argv[i + 1];
                        }
                        ++count;
                    }
                }
            }
        }
    }

    return nullptr;
}
