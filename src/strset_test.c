// vim: set colorcolumn=85
// vim: fdm=marker

#include "koh_destral_ecs.h"
#include "koh_strset.h"
#include "munit.h"
#include "koh.h"
#include "raylib.h"
#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Lines {
    char    **lines;
    int     num;
};

bool iter_set_remove(const char *key, void *udata) {
    return false;
}

bool iter_set_cmp(const char *key, void *udata) {
    struct Lines *lines = udata;

    for (int i = 0; i < lines->num; ++i) {
        if (!strcmp(lines->lines[i], key)) {
            return true;
        }
    }

    printf("iter_set: key %s not found in each itertor\n", key);
    munit_assert(false);

    return true;
}

static MunitResult test_new_add_exist_free(
    const MunitParameter params[], void* data
) {
    StrSet *set = strset_new();
    munit_assert_ptr_not_null(set);

    const char *lines[] = {
        "privet",
        "Ya ded",
        "obed",
        "privet11",
        "Ya ne ded",
    };

    const char *other_lines[] = {
        "prIvet",
        "ya ded",
        "_obed",
        "prvet11",
        "Yane ded",
        "some line",
    };
    
    int lines_num = sizeof(lines) / sizeof(lines[0]);
    int other_lines_num = sizeof(other_lines) / sizeof(other_lines[0]);

    for (int i = 0; i< lines_num; ++i) {
        strset_add(set, lines[i]);
    }

    for (int i = 0; i< lines_num; ++i) {
        munit_assert(strset_exist(set, lines[i]));
    }

    for (int i = 0; i< other_lines_num; ++i) {
        munit_assert(!strset_exist(set, other_lines[i]));
    }

    struct Lines lines_ctx = {
        .lines = (char**)lines, 
        .num = lines_num,
    };
    strset_each(set, iter_set_cmp, &lines_ctx);

    strset_each(set, iter_set_remove, &lines_ctx);
    strset_add(set, "NEWLINE");

    for (int i = 0; i< lines_num; ++i) {
        munit_assert(!strset_exist(set, lines[i]));
    }

    munit_assert(strset_exist(set, "NEWLINE"));
    strset_clear(set);
    munit_assert(!strset_exist(set, "NEWLINE"));

    strset_free(set);
    return MUNIT_OK;
}

static MunitTest test_suite_tests[] = {
  {
    (char*) "/new_add_exist_free",
    test_new_add_exist_free,
    NULL,
    NULL,
    MUNIT_TEST_OPTION_NONE,
    NULL
  },
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

static const MunitSuite test_suite = {
  (char*) "strset", test_suite_tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};

int main(int argc, char **argv) {
    return munit_suite_main(&test_suite, (void*) "µnit", argc, argv);
}
