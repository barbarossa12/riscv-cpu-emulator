#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "unity.h"
#include "../src/log.h"

/* log_message writes to stderr, so each test redirects fd 2 to a temporary file, runs the
 * code that logs, then restores stderr and reads back what was written. */

static char captured[1024];
static int saved_stderr;
static FILE *capture_file;

static void capture_start(void) {
    fflush(stderr);
    saved_stderr = dup(STDERR_FILENO);
    capture_file = tmpfile();
    TEST_ASSERT_NOT_NULL(capture_file);
    dup2(fileno(capture_file), STDERR_FILENO);
}

static void capture_stop(void) {
    fflush(stderr);
    dup2(saved_stderr, STDERR_FILENO);
    close(saved_stderr);
    rewind(capture_file);
    size_t n = fread(captured, 1, sizeof captured - 1, capture_file);
    captured[n] = '\0';
    fclose(capture_file);
}

static int count_char(const char *s, char c) {
    int n = 0;
    for (; *s; s++) {
        if (*s == c) n++;
    }
    return n;
}

void setUp(void) {
    log_set_level(LOG_LEVEL_INFO);
    captured[0] = '\0';
}
void tearDown(void) {
    log_set_level(LOG_LEVEL_INFO);
}

void test_log_message_ends_with_one_newline(void) {
    capture_start();
    LOG_INFO("hello %d", 5);
    capture_stop();

    size_t len = strlen(captured);
    TEST_ASSERT_TRUE(len > 0);
    TEST_ASSERT_EQUAL_CHAR('\n', captured[len - 1]);
    TEST_ASSERT_EQUAL_INT(1, count_char(captured, '\n'));
}

void test_log_message_line_format(void) {
    capture_start();
    LOG_INFO("hello %d", 5);
    capture_stop();

    /* [LEVEL] file:line: message\n */
    TEST_ASSERT_EQUAL_INT(0, strncmp(captured, "[INFO] ", 7));
    TEST_ASSERT_NOT_NULL(strstr(captured, "test_log.c:"));
    TEST_ASSERT_NOT_NULL(strstr(captured, ": hello 5\n"));
}

void test_log_two_messages_are_on_separate_lines(void) {
    capture_start();
    LOG_INFO("first");
    LOG_INFO("second");
    capture_stop();

    TEST_ASSERT_EQUAL_INT(2, count_char(captured, '\n'));
    TEST_ASSERT_NOT_NULL(strstr(captured, ": first\n[INFO] "));
    char *second = strstr(captured, ": second\n");
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_EQUAL_CHAR('\0', second[strlen(": second\n")]); // nothing after the last newline
}

void test_log_level_names(void) {
    log_set_level(LOG_LEVEL_DEBUG);
    capture_start();
    LOG_ERROR("e");
    LOG_WARN("w");
    LOG_INFO("i");
    LOG_DEBUG("d");
    capture_stop();

    TEST_ASSERT_EQUAL_INT(4, count_char(captured, '\n'));
    TEST_ASSERT_NOT_NULL(strstr(captured, "[ERROR] "));
    TEST_ASSERT_NOT_NULL(strstr(captured, "[WARN] "));
    TEST_ASSERT_NOT_NULL(strstr(captured, "[INFO] "));
    TEST_ASSERT_NOT_NULL(strstr(captured, "[DEBUG] "));
}

void test_log_suppressed_message_prints_nothing_not_even_a_newline(void) {
    /* the default level is INFO, so DEBUG is dropped */
    capture_start();
    LOG_DEBUG("hidden");
    capture_stop();

    TEST_ASSERT_EQUAL_STRING("", captured);
}

void test_log_set_level_filters(void) {
    const struct { LogLevel level; int lines; } v[] = {
        {LOG_LEVEL_ERROR, 1},
        {LOG_LEVEL_WARN,  2},
        {LOG_LEVEL_INFO,  3},
        {LOG_LEVEL_DEBUG, 4},
    };
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        log_set_level(v[i].level);
        capture_start();
        LOG_ERROR("e");
        LOG_WARN("w");
        LOG_INFO("i");
        LOG_DEBUG("d");
        capture_stop();
        TEST_ASSERT_EQUAL_INT(v[i].lines, count_char(captured, '\n'));
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_log_message_ends_with_one_newline);
    RUN_TEST(test_log_message_line_format);
    RUN_TEST(test_log_two_messages_are_on_separate_lines);
    RUN_TEST(test_log_level_names);
    RUN_TEST(test_log_suppressed_message_prints_nothing_not_even_a_newline);
    RUN_TEST(test_log_set_level_filters);
    return UNITY_END();
}
