// vim: set colorcolumn=85
// vim: fdm=marker

#include "koh_rand.h"
#include "koh_strset.h"
#include "uthash.h"
#include "munit.h"
#include <unistd.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool verbose = true;

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

static MunitResult test_compare_internal(
    const MunitParameter params[], void* data,
    struct StrSetSetup *setup
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

struct Line {
    char            line[512];
    UT_hash_handle  hh;
};

struct EachCtx {
    struct Line *lines;
};

StrSetAction iter_each(const char *key, void *udata) {
    struct EachCtx *ctx = udata;
    struct Line *found = NULL;
    HASH_FIND_STR(ctx->lines, key, found);

    //printf("iter_each: key '%s'\n", key);
    
    if (!found)
        printf("iter_each: key '%s'\n", key);
       
    //munit_assert_not_null(found);
    return SSA_next;
}

static MunitResult test_stress(
    const MunitParameter params[], void* data
) {
    StrSet *set = strset_new(NULL);
    const size_t lines_num = 30, max_line_len = 10, min_line_len = 1;
    size_t word_len;
    char **lines = calloc(lines_num, sizeof(lines[0]));

    for (int i = 0; i < lines_num; i++) {
        lines[i] = calloc(max_line_len + 1, sizeof(lines[0][0]));
        word_len = rand() % (max_line_len - min_line_len) + min_line_len;
        for (int j = 0; j < word_len; j++) {
            lines[i][j] = rand() % 26 + 'a';
        }
        printf("test_stress: '%s'\n", lines[i]);
    }

    for (int i = 0; i < lines_num; i++) {
        strset_add(set, lines[i]);
    }

    // Удалить строки которые короче threshold
    int threshold = 3, removed_num = 0;
    for (int i = 0; i < lines_num; i++) {
        if (lines[i] && strlen(lines[i]) < threshold) {
            strset_remove(set, lines[i]);
            free(lines[i]);
            lines[i] = NULL;
            removed_num++;
        }
    }
    printf("test_stress: removed_num %d\n", removed_num);

    printf("\n");
    FILE *T = fopen("1.txt", "w");
    strset_print_f(set, T, "%s\n");
    fclose(T);
    printf("\n");

    T = fopen("2.txt", "w");
    for (int i = 0; i < lines_num; ++i) {
        if (lines[i]) {
            printf("%s\n", lines[i]);
            fprintf(T, "%s\n", lines[i]);
        }
    }
    fclose(T);

    for (int i = 0; i < lines_num; ++i) {
        if (lines[i]) {
            bool exists = strset_exist(set, lines[i]);
            if (!exists) {
                printf("test_stress: '%s'\n", lines[i]);
            }
            munit_assert(exists);
        }
    }

    strset_free(set);
    for (int i = 0; i< lines_num; ++i) {
        if (lines[i])
            free(lines[i]);
    }
    free(lines);
    return MUNIT_OK;
}

static MunitResult test_add_remove_internal2(
    const MunitParameter params[], void* data, struct StrSetSetup *setup
) {
    if (verbose) {
        const char *hasher_name = NULL;
        for (int i = 0; koh_hashers[i].f; i++)
            if (setup->hasher == koh_hashers[i].f) {
                hasher_name = koh_hashers[i].fname;
                break;
            }

        printf(
            "test_add_remove_internal: hasher '%s', capacity %zu\n",
            hasher_name, setup->capacity
        );
    }
    StrSet *set = strset_new(setup);

    strset_add(set, "1");
    strset_add(set, "2");

    munit_assert(strset_exist(set, "1"));
    munit_assert(strset_exist(set, "2"));

    strset_remove(set, "1");

    munit_assert(strset_exist(set, "1") == false);
    munit_assert(strset_exist(set, "2"));

    strset_remove(set, "2");

    munit_assert(strset_exist(set, "1") == false);
    munit_assert(strset_exist(set, "2") == false);

    strset_add(set, "1");

    munit_assert(strset_exist(set, "1"));
    munit_assert(strset_exist(set, "2") == false);

    strset_add(set, "2");

    munit_assert(strset_exist(set, "1"));
    munit_assert(strset_exist(set, "2"));

    strset_free(set);
    return MUNIT_OK;
}

static MunitResult test_add_remove_internal4(
    const MunitParameter params[], void* data, struct StrSetSetup *setup
) {
    if (verbose) {
        const char *hasher_name = NULL;
        for (int i = 0; koh_hashers[i].f; i++)
            if (setup->hasher == koh_hashers[i].f) {
                hasher_name = koh_hashers[i].fname;
                break;
            }

        printf(
            "test_add_remove_internal: hasher '%s', capacity %zu\n",
            hasher_name, setup->capacity
        );
    }
    StrSet *set = strset_new(setup);

    strset_add(set, "1");
    strset_add(set, "2");
    strset_add(set, "3");
    strset_add(set, "4");

    munit_assert(strset_exist(set, "1"));
    munit_assert(strset_exist(set, "2"));
    munit_assert(strset_exist(set, "3"));
    munit_assert(strset_exist(set, "4"));
    munit_assert(strset_exist(set, "5") == false);

    strset_remove(set, "4");

    munit_assert(strset_exist(set, "1"));
    munit_assert(strset_exist(set, "2"));
    munit_assert(strset_exist(set, "3"));
    munit_assert(strset_exist(set, "4") == false);
    munit_assert(strset_exist(set, "5") == false);

    strset_remove(set, "3");

    munit_assert(strset_exist(set, "1"));
    munit_assert(strset_exist(set, "2"));
    munit_assert(strset_exist(set, "3") == false);
    munit_assert(strset_exist(set, "4") == false);
    munit_assert(strset_exist(set, "5") == false);

    strset_remove(set, "1");

    munit_assert(strset_exist(set, "1") == false);
    munit_assert(strset_exist(set, "2"));
    munit_assert(strset_exist(set, "3") == false);
    munit_assert(strset_exist(set, "4") == false);
    munit_assert(strset_exist(set, "5") == false);

    strset_remove(set, "2");

    munit_assert(strset_exist(set, "1") == false);
    munit_assert(strset_exist(set, "2") == false);
    munit_assert(strset_exist(set, "3") == false);
    munit_assert(strset_exist(set, "4") == false);
    munit_assert(strset_exist(set, "5") == false);

    strset_add(set, "1");

    munit_assert(strset_exist(set, "1"));
    munit_assert(strset_exist(set, "2") == false);
    munit_assert(strset_exist(set, "3") == false);
    munit_assert(strset_exist(set, "4") == false);
    munit_assert(strset_exist(set, "5") == false);

    strset_add(set, "2");

    munit_assert(strset_exist(set, "1"));
    munit_assert(strset_exist(set, "2"));
    munit_assert(strset_exist(set, "3") == false);
    munit_assert(strset_exist(set, "4") == false);
    munit_assert(strset_exist(set, "5") == false);

    strset_free(set);
    return MUNIT_OK;
}

static MunitResult test_add_remove2(
    const MunitParameter params[], void* data
) {
    //size_t capacities[] = { 0, 1, 2, 11 };
    size_t capacities[] = { 0, 1, 11, 2 };
    //size_t capacities[] = { 0, 1, 11 };
    size_t capacities_num = sizeof(capacities) / sizeof(capacities[0]);

    for (int j = 0; j < capacities_num; j++) {
        for (int i = 0; koh_hashers[i].f; i++) {
            test_add_remove_internal2(params, data, &(struct StrSetSetup) {
                .capacity = capacities[j],
                .hasher = koh_hashers[i].f,
            });
        }
    }

    return MUNIT_OK;
}

static MunitResult test_add_remove4(
    const MunitParameter params[], void* data
) {
    //size_t capacities[] = { 0, 1, 2, 11 };
    size_t capacities[] = { 0, 1, 11, 2 };
    size_t capacities_num = sizeof(capacities) / sizeof(capacities[0]);

    for (int j = 0; j < capacities_num; j++) {
        for (int i = 0; koh_hashers[i].f; i++) {
            test_add_remove_internal4(params, data, &(struct StrSetSetup) {
                .capacity = capacities[j],
                .hasher = koh_hashers[i].f,
            });
        }
    }

    return MUNIT_OK;
}

static MunitResult test_compare_with_uthash(
    const MunitParameter params[], void* data
) {
    FILE *file_data = fopen("./strset_data1.txt", "r");
    assert(file_data);

    char line[512] = {};
    size_t max_line_len = sizeof(line);
    if (verbose) {
        printf("test_compare_with_uthash: max_line_len %zu\n", max_line_len);
    }

    StrSet *set = strset_new(NULL);
    struct Line *lines = NULL;

    while (fgets(line, max_line_len, file_data)) {
        size_t line_len = strlen(line);
        if (line[line_len - 1] == '\n') {
            line[line_len - 1] = 0;
        }
        //printf("line '%s'\n", line);
        strset_add(set, line);
    }

    fseek(file_data, 0, SEEK_SET);
    while (fgets(line, max_line_len, file_data)) {
        size_t line_len = strlen(line);
        if (line[line_len - 1] == '\n') {
            line[line_len - 1] = 0;
        }

        struct Line *found = NULL;
        HASH_FIND_STR(lines, line, found);
        if (!found) {
            struct Line *new = calloc(1, sizeof(*new));
            assert(new);
            strncpy(new->line, line, max_line_len);
            //strcpy(new->line, line);
            HASH_ADD_STR(lines, line, new);
        } else {
            //printf("found\n");
        }
    }

    fclose(file_data);

    struct Line *item, *tmp;

    if (verbose) {
        printf("lines count %u\n", HASH_COUNT(lines));
        printf("strset_count %zu\n", strset_count(set));
    }

    StrSet *set_hh = strset_new(NULL);

    FILE *T;
    T = fopen("T_hh.txt", "w");
    HASH_ITER(hh, lines, item, tmp) {
        strset_add(set_hh, item->line);
        fprintf(T, "%s", item->line);
        munit_assert(strset_exist(set, item->line));
    }
    fclose(T);

    printf(
        "test_compare_with_uthash: are set_hh & set equal? %s, %s\n",
        strset_compare(set_hh, set) ? "true" : "false",
        strset_compare(set, set_hh) ? "true" : "false"
    );

    HASH_ITER(hh, lines, item, tmp) {
        HASH_DEL(lines, item);
        free(item);
    }

    T = fopen("T_set.txt", "w");
    strset_print(set, T);
    fclose(T);

    /*
    struct EachCtx ctx = {
        .lines = lines,
    };
    strset_each(set, iter_each, &ctx);
    */
    
    strset_free(set_hh);
    strset_free(set);
    return MUNIT_OK;
}

/*
static MunitResult test_iter(
    const MunitParameter params[], void* data
) {
    struct StrSet *set = strset_new(NULL);

    const char *lines[] = {
        "0", "1", "2", "3", NULL,
    };

    for (int i = 0; lines[i]; i++) {
        strset_add(set, lines[i]);
    }

    for (struct StrSetIter i = strset_iter_new(set); strset_iter_valid(&i); 
        strset_iter_next(&i)
    ) {
        for (int j = 0; lines[j]; j++) {
            if (!strcmp(lines[j], strset_iter_get(&i))) {
                goto _next;
            }
        }
        munit_assert(false);
_next:
        continue;
    }

    strset_free(set);

    return MUNIT_OK;
}
*/

static MunitResult test_compare(
    const MunitParameter params[], void* data
) {

    int i = 0;
    while (koh_hashers[i].f) {
        if (verbose) {
            printf(
                "test_compare_internal: using '%s' function\n",
                koh_hashers[i].fname
            );
        }
        test_compare_internal(params, data, &(struct StrSetSetup) {
            .capacity = 11,
            .hasher = koh_hashers[i].f,
        });
        i++;
    }

    return MUNIT_OK;
}

static MunitResult test_massive_add_get_internal(
    const MunitParameter params[], void* data,
    struct StrSetSetup *setup
) {
    StrSet *set = strset_new(setup);
    munit_assert_ptr_not_null(set);

    xorshift32_state rnd1 = xorshift32_init();
    usleep(200);
    xorshift32_state rnd2 = xorshift32_init();
    const int iters = 100000 / 2;

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

static MunitResult test_massive_add_get(
    const MunitParameter params[], void* data
) {

    int i = 0;
    while (koh_hashers[i].f) {
        if (verbose) {
            printf(
                "test_massive_add_get: using '%s' function\n",
                koh_hashers[i].fname
            );
        }
        test_massive_add_get_internal(params, data, &(struct StrSetSetup) {
            .capacity = 11,
            .hasher = koh_hashers[i].f,
        });
        i++;
    }

    return MUNIT_OK;
}

static MunitResult test_difference_internal_setup(
    const MunitParameter params[], void* data,
    struct StrSetSetup *setup
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

static MunitResult test_difference(
    const MunitParameter params[], void* data 
) {
    int i = 0;
    while (koh_hashers[i].f) {
        if (verbose) {
            printf(
                "test_difference: using '%s' function\n",
                koh_hashers[i].fname
            );
        }
        test_difference_internal_setup(params, data, &(struct StrSetSetup) {
            .capacity = 11,
            .hasher = koh_hashers[i].f,
        });
        i++;
    }
    return MUNIT_OK;
}

static MunitResult test_new_add_exist_free_internal(
    const MunitParameter params[], void* data,
    struct StrSetSetup *setup
) {

    StrSet *set = strset_new(setup);
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

static MunitResult test_new_add_exist_free(
    const MunitParameter params[], void* data
) {
    test_new_add_exist_free_internal(params, data, NULL);

    int i = 0;
    while (koh_hashers[i].f) {
        if (verbose) {
            printf(
                "test_new_add_exist_free: using '%s' function\n",
                koh_hashers[i].fname
            );
        }
        test_new_add_exist_free_internal(params, data, &(struct StrSetSetup) {
            .capacity = 11,
            .hasher = koh_hashers[i].f,
        });
        i++;
    }

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

  {
    (char*) "/compare_with_uthash",
    test_compare_with_uthash,
    NULL,
    NULL,
    MUNIT_TEST_OPTION_NONE,
    NULL
  },

  /*
    {
      (char*) "/add_remove4",
      test_add_remove4,
      NULL,
      NULL,
      MUNIT_TEST_OPTION_NONE,
      NULL
    },
*/

    {
      (char*) "/add_remove2",
      test_add_remove2,
      NULL,
      NULL,
      MUNIT_TEST_OPTION_NONE,
      NULL
    },

/*
  {
    (char*) "/stress",
    test_stress,
    NULL,
    NULL,
    MUNIT_TEST_OPTION_NONE,
    NULL
  },
*/

  /*
  {
    (char*) "/iter",
    test_iter,
    NULL,
    NULL,
    MUNIT_TEST_OPTION_NONE,
    NULL
  },
  */

  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

static const MunitSuite test_suite = {
  (char*) "strset", test_suite_tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};

int main(int argc, char **argv) {
    koh_hashers_init();
    strset_verbose = false;
    return munit_suite_main(&test_suite, (void*) "µnit", argc, argv);
}
