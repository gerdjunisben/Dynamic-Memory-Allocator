#ifndef SFMM_TEST_H
#define SFMM_TEST_H

#include <criterion/criterion.h>
#include "sfmm.h"
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#define LOOP_LIMIT                10000

void _assert_free_list_is_empty(void);
void _assert_block_is_valid(sf_block * hp);
void _assert_heap_is_valid(void);
void _assert_block_info(sf_block * hp, int alloc, size_t b_size);
void _assert_nonnull_payload_pointer(void * pp);
void _assert_null_payload_pointer(void * pp);
void _assert_free_block_count(size_t size, int count);
void _assert_quick_list_block_count(size_t size, int count);
void _assert_errno_eq(int n);

#endif
