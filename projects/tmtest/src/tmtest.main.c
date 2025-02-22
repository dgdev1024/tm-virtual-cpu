#include <tm.arguments.h>

void tmtest_atexit ()
{
    tm_release_arguments();
}

void tmtest_test_has_argument() {
    char* argv[] = {"program", "--option", "-o", "value"};
    int argc = 4;

    tm_capture_arguments(argc, argv);
    tm_assert(tm_has_argument("option", 'o') == true);
    tm_assert(tm_has_argument("missing", 'm') == false);
    tm_assert(tm_has_argument("option", 'm') == true);
    tm_assert(tm_has_argument("missing", 'o') == true);
    tm_release_arguments();
}

void tmtest_test_get_argument_value() {
    char* argv[] = {"program", "--option", "value", "-o", "value2"};
    int argc = 5;

    tm_capture_arguments(argc, argv);
    tm_assert(strcmp(tm_get_argument_value("option", 'o'), "value") == 0);
    tm_assert(strcmp(tm_get_argument_value("option", 'x'), "value") == 0);
    tm_assert(strcmp(tm_get_argument_value("missing", 'o'), "value2") == 0);
    tm_assert(tm_get_argument_value("missing", 'm') == NULL);
    tm_release_arguments();
}

void tmtest_test_get_argument_value_at() {
    char* argv[] = {"program", "--option", "value1", "--option", "value2", "-o", "value3"};
    int argc = 7;

    tm_capture_arguments(argc, argv);
    tm_assert(strcmp(tm_get_argument_value_at("option", 'o', 0), "value1") == 0);
    tm_assert(strcmp(tm_get_argument_value_at("option", 'o', 1), "value2") == 0);
    tm_assert(strcmp(tm_get_argument_value_at("option", 'o', 2), "value3") == 0);
    tm_assert(tm_get_argument_value_at("option", 'o', 3) == NULL);
    tm_release_arguments();
}

int main() {
    tmtest_test_has_argument();
    tmtest_test_get_argument_value();
    tmtest_test_get_argument_value_at();

    printf("All tests passed!\n");
    return 0;
}
