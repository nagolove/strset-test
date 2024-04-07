// vim: set colorcolumn=85
// vim: fdm=marker

#include "koh_rand.h"
#include "koh_strset.h"
#include "munit.h"
#include <unistd.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Lines {
    char    **lines;
    int     num;
};

StrSetAction iter_set_remove(const char *key, void *udata) {
    return SSA_remove;
}

StrSetAction iter_set_cmp(const char *key, void *udata) {
    struct Lines *lines = udata;

    for (int i = 0; i < lines->num; ++i) {
        if (!strcmp(lines->lines[i], key)) {
            return SSA_next;
        }
    }

    printf("iter_set: key %s not found in each itertor\n", key);
    munit_assert(false);

    return SSA_next;
}

static MunitResult test_difference_internal(
    const MunitParameter params[], void* data, 
    char **lines1, size_t lines1_num,
    char **lines2, size_t lines2_num,
    char **should_be, size_t should_be_num
) {
    StrSet *set1 = strset_new(NULL);
    munit_assert_ptr_not_null(set1);

    StrSet *set2 = strset_new(NULL);
    munit_assert_ptr_not_null(set2);

    for (int i = 0; i< lines1_num; ++i) {
        strset_add(set1, lines1[i]);
    }

    for (int i = 0; i< lines2_num; ++i) {
        strset_add(set2, lines2[i]);
    }

    StrSet *difference = strset_difference(set1, set2);
    munit_assert(strset_compare_strs(
        difference, should_be, should_be_num) == true
    );

    strset_free(set1);
    strset_free(set2);
    strset_free(difference);

    return MUNIT_OK;
}

static MunitResult test_compare(
    const MunitParameter params[], void* data
) {
    StrSet *set1 = strset_new(NULL);
    munit_assert_ptr_not_null(set1);

    StrSet *set2 = strset_new(NULL);
    munit_assert_ptr_not_null(set2);

    StrSet *set3 = strset_new(NULL);
    munit_assert_ptr_not_null(set3);

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
        strset_add(set1, lines[i]);
        strset_add(set2, lines[i]);
    }

    for (int i = 0; i< other_lines_num; ++i) {
        strset_add(set3, other_lines[i]);
    }

    munit_assert(strset_compare(set1, set2));
    munit_assert(!strset_compare(set1, set3));
    munit_assert(!strset_compare(set2, set3));

    strset_free(set1);
    strset_free(set2);
    strset_free(set3);
    return MUNIT_OK;
}

static MunitResult test_massive_add_get(
    const MunitParameter params[], void* data
) {
    StrSet *set = strset_new(NULL);
    munit_assert_ptr_not_null(set);

    xorshift32_state rnd1 = xorshift32_init();
    usleep(200);
    xorshift32_state rnd2 = xorshift32_init();
    const int iters = 1000000 / 2;

    char **lines = calloc(iters, sizeof(lines[0]));
    for (int i = 0; i< iters; ++i) {
        char buf[64] = {};
        sprintf(buf, "%u%u", xorshift32_rand(&rnd1), xorshift32_rand(&rnd2));
        lines[i] = strdup(buf);
        strset_add(set, lines[i]);
    }

    for (int i = 0; i < iters; ++i) {
        munit_assert(strset_exist(set, lines[i]));
    }

    for (int j = 0; j < iters; j++) {
        if (lines[j])
            free(lines[j]);
    }
    free(lines);
    strset_clear(set);

    strset_free(set);
    return MUNIT_OK;
}

static MunitResult test_difference(
    const MunitParameter params[], void* data 
) {

    {
        char *lines1[] = { "1", "2", "3", "4", };
        char *lines2[] = { "1", "2", "3", };
        char *should_be[] = { "4", };
        
        size_t lines1_num = sizeof(lines1) / sizeof(lines1[0]), 
               lines2_num = sizeof(lines2) / sizeof(lines2[0]),
               should_be_num = sizeof(should_be) / sizeof(should_be[0]);

        test_difference_internal(
            params, data, 
            lines1, lines1_num, 
            lines2, lines2_num, 
            should_be, should_be_num
        );
    }

    {
        char *lines1[] = { "1", "3", "2", "1", };
        char *lines2[] = { "1", "2", "3", };
        char *should_be[] = { };
        
        size_t lines1_num = sizeof(lines1) / sizeof(lines1[0]), 
               lines2_num = sizeof(lines2) / sizeof(lines2[0]),
               should_be_num = sizeof(should_be) / sizeof(should_be[0]);

        test_difference_internal(
            params, data, 
            lines1, lines1_num, 
            lines2, lines2_num, 
            should_be, should_be_num
        );
    }

    {
        char *lines1[] = { "1", };
        char *lines2[] = { "1", "2", "3", };
        char *should_be[] = { };
        
        size_t lines1_num = sizeof(lines1) / sizeof(lines1[0]), 
               lines2_num = sizeof(lines2) / sizeof(lines2[0]),
               should_be_num = sizeof(should_be) / sizeof(should_be[0]);

        test_difference_internal(
            params, data, 
            lines1, lines1_num, 
            lines2, lines2_num, 
            should_be, should_be_num
        );
    }

    return MUNIT_OK;
}

static MunitResult test_new_add_exist_free(
    const MunitParameter params[], void* data
) {
    StrSet *set = strset_new(NULL);
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
  {
    (char*) "/massive_add_get",
    test_massive_add_get,
    NULL,
    NULL,
    MUNIT_TEST_OPTION_NONE,
    NULL
  },
  {
    (char*) "/compare",
    test_compare,
    NULL,
    NULL,
    MUNIT_TEST_OPTION_NONE,
    NULL
  },
  {
    (char*) "/difference",
    test_difference,
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
    koh_hashers_init();
    return munit_suite_main(&test_suite, (void*) "µnit", argc, argv);
}
