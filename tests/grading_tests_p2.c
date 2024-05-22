#define TEST_TIMEOUT 15
#include "__grading_helpers.h"
#include "debug.h"

/*
 * Check LIFO discipline on free list
 */
Test(sf_memsuite_grading, malloc_free_lifo, .timeout=TEST_TIMEOUT)
{
    size_t sz = 200;
    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info((sf_block *)((char *)x - 16), 1, 208);

    void * u = sf_malloc(sz);
    _assert_nonnull_payload_pointer(u);
    _assert_block_info((sf_block *)((char *)u - 16), 1, 208);

    void * y = sf_malloc(sz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info((sf_block *)((char *)y - 16), 1, 208);

    void * v = sf_malloc(sz);
    _assert_nonnull_payload_pointer(v);
    _assert_block_info((sf_block *)((char *)v - 16), 1, 208);

    void * z = sf_malloc(sz);
    _assert_nonnull_payload_pointer(z);
    _assert_block_info((sf_block *)((char *)z - 16), 1, 208);

    void * w = sf_malloc(sz);
    _assert_nonnull_payload_pointer(w);
    _assert_block_info((sf_block *)((char *)w - 16), 1, 208);

    sf_free(x);
    sf_free(y);
    sf_free(z);

    void * z1 = sf_malloc(sz);
    _assert_nonnull_payload_pointer(z1);
    _assert_block_info((sf_block *)((char *)z1 - 16), 1, 208);

    void * y1 = sf_malloc(sz);
    _assert_nonnull_payload_pointer(y1);
    _assert_block_info((sf_block *)((char *)y1 - 16), 1, 208);

    void * x1 = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x1);
    _assert_block_info((sf_block *)((char *)x1 - 16), 1, 208);

    cr_assert(x == x1 && y == y1 && z == z1,
      "malloc/free does not follow LIFO discipline");

    _assert_free_block_count(6896, 1);
    _assert_quick_list_block_count(0, 0);

    _assert_errno_eq(0);
}

/*
 * Realloc tests.
 */
Test(sf_memsuite_grading, realloc_larger, .timeout=TEST_TIMEOUT)
{
    size_t sz = 200;
    size_t nsz = 1024;

    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info((sf_block *)((char *)x - 16), 1, 208);

    void * y = sf_realloc(x, nsz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info((sf_block *)((char *)y - 16), 1, 1040);

    _assert_free_block_count(208, 1);

    _assert_quick_list_block_count(0, 0);

    _assert_free_block_count(6896, 1);

    _assert_errno_eq(0);
}

Test(sf_memsuite_grading, realloc_smaller, .timeout=TEST_TIMEOUT)
{
    size_t sz = 1024;
    size_t nsz = 200;

    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info((sf_block *)((char *)x - 16), 1, 1040);

    void * y = sf_realloc(x, nsz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info((sf_block *)((char *)y - 16), 1, 208);

    cr_assert_eq(x, y, "realloc to smaller size did not return same payload pointer");

    _assert_free_block_count(7936, 1);
    _assert_quick_list_block_count(0, 0);
    _assert_errno_eq(0);
}

Test(sf_memsuite_grading, realloc_same, .timeout=TEST_TIMEOUT)
{
    size_t sz = 1024;
    size_t nsz = 1024;

    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info((sf_block *)((char *)x - 16), 1, 1040);


    void * y = sf_realloc(x, nsz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info((sf_block *)((char *)y - 16), 1, 1040);

    cr_assert_eq(x, y, "realloc to same size did not return same payload pointer");

    _assert_free_block_count(7104, 1);

    _assert_errno_eq(0);
}

Test(sf_memsuite_grading, realloc_splinter, .timeout=TEST_TIMEOUT)
{
    size_t sz = 1024;
    size_t nsz = 1020;

    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info((sf_block *)((char *)x - 16), 1, 1040);

    void * y = sf_realloc(x, nsz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info((sf_block *)((char *)y - 16), 1, 1040);

    cr_assert_eq(x, y, "realloc to smaller size did not return same payload pointer");

    _assert_free_block_count(7104, 1);
    _assert_errno_eq(0);
}

Test(sf_memsuite_grading, realloc_size_0, .timeout=TEST_TIMEOUT)
{
    size_t sz = 1024;
    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info((sf_block *)((char *)x - 16), 1, 1040);


    void * y = sf_malloc(sz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info((sf_block *)((char *)y - 16), 1, 1040);


    void * z = sf_realloc(x, 0);
    _assert_null_payload_pointer(z);
    _assert_block_info((sf_block *)((char *)x - 16), 0, 1040);


    // after realloc x to (2) z, x is now a free block
    _assert_free_block_count(1040, 1);

    // the size of the remaining free block
    _assert_free_block_count(6064, 1);

    _assert_errno_eq(0);
}

/*
 * Illegal pointer tests.
 */
Test(sf_memsuite_grading, free_null, .signal = SIGABRT, .timeout = TEST_TIMEOUT)
{
    size_t sz = 1;
    (void) sf_malloc(sz);
    sf_free(NULL);
    cr_assert_fail("SIGABRT should have been received");
}

//This test tests: Freeing a memory that was free-ed already
Test(sf_memsuite_grading, free_unallocated, .signal = SIGABRT, .timeout = TEST_TIMEOUT)
{
    size_t sz = 1;
    void *x = sf_malloc(sz);
    sf_free(x);
    sf_free(x);
    cr_assert_fail("SIGABRT should have been received");
}

Test(sf_memsuite_grading, free_block_too_small, .signal = SIGABRT, .timeout = TEST_TIMEOUT)
{
    size_t sz = 1;
    void * x = sf_malloc(sz);

    ((sf_block *)((char *)x - 16))->header = 0x0UL ^ MAGIC;

    sf_free(x);
    cr_assert_fail("SIGABRT should have been received");
}

#if 0
// This test is broken in two ways:
// (1) When footer optimization is in use, the previous block is actually allocated,
//     so it will not have a valid footer that can be checked.
// (2) When quick lists are in use, the freed block ends up in a quick list, so it
//     is still technically allocated, resulting in the same situation as (1).
Test(sf_memsuite_grading, free_prev_alloc, .signal = SIGABRT, .timeout = TEST_TIMEOUT)
{
    size_t sz = 1;
    void * w = sf_malloc(sz);
    void * x = sf_malloc(sz);
    (sf_block *)((char *)x - 16)->header ^= MAGIC;
    (sf_block *)((char *)x - 16)->header &= ~PREV_BLOCK_ALLOCATED;
    (sf_block *)((char *)x - 16)->header ^= MAGIC;
    sf_free(x);
    sf_free(w);
    cr_assert_fail("SIGABRT should have been received");
}
#endif
/*
static void *SF_MALLOC(size_t size);
static void *SF_REALLOC(void *ptr, size_t size);
static void SF_FREE(void *ptr);

#define NTRACKED 1000
#define ORDER_RANGE 13

static void run_stress_test(int final_free) {
    errno = 0;

    int order_range = ORDER_RANGE;
    int nullcount = 0;

    void * tracked[NTRACKED];

    for (int i = 0; i < NTRACKED; i++)
    {
        size_t req_sz;
        int order = 0;
        do {
            order = (rand() % order_range);
            size_t extra = (rand() % (1 << order));
            req_sz = (1 << order) + extra;
            //fprintf(stderr, "req_sz: %ld", req_sz);
            if((tracked[i] = SF_MALLOC(req_sz)) != NULL || order == 0)
                break;
            //fprintf(stderr, "...failed\n");
                order--;
            } while(tracked[i] == NULL && order >= 0);
        //fprintf(stderr, "\n");

            // tracked[i] can still be NULL
            if (tracked[i] == NULL)
            {
                nullcount++;
                // It seems like there is not enough space in the heap.
                // Try to halve the size of each existing allocated block in the heap,
                // so that next mallocs possibly get free blocks.
                for (int j = 0; j < i; j++)
                {
                    if (tracked[j] == NULL)
                    {
                        continue;
                    }
                    sf_block * bp = (sf_block *)((char *)tracked[j] - 16);
                    req_sz = ((bp->header ^ MAGIC) & ~0xf) >> 1;
                    tracked[j] = SF_REALLOC(tracked[j], req_sz);
                }
            }
            errno = 0;
    }

    for (int i = 0; i < NTRACKED; i++)
    {
        if (final_free && tracked[i] != NULL)
        {
            SF_FREE(tracked[i]);
        }
    }
}


// random block assigments. Tried to give equal opportunity for each possible order to appear.
// But if the heap gets populated too quickly, try to make some space by realloc(half) existing
// allocated blocks.
Test(sf_memsuite_grading, stress_test, .timeout = TEST_TIMEOUT)
{
    run_stress_test(1);
    _assert_heap_is_valid();
}*/

// Statistics tests.

static size_t max_aggregate_payload = 0;
static size_t current_aggregate_payload = 0;
static size_t total_allocated_block_size = 0;

static void tally_alloc(void *p, size_t size) {
    if(!p)
       return;
    sf_block *bp = (sf_block *)((char *)p - 16);
    debug("tally_alloc: %p (%lu/%lu)", bp, size, ((bp->header ^ MAGIC) & ~0xf));
    if(bp) {
        current_aggregate_payload += (((bp->header ^ MAGIC) & ~0xf) - 8);
        total_allocated_block_size += ((bp->header ^ MAGIC) & ~0xf);
        if(current_aggregate_payload > total_allocated_block_size) {
            fprintf(stderr,
                "Aggregate payload (%lu) > total allocated block size (%lu) (not possible!)\n",
                current_aggregate_payload, total_allocated_block_size);
            abort();
        }
        if(current_aggregate_payload > max_aggregate_payload)
            max_aggregate_payload = current_aggregate_payload;
    }
}

static void tally_free(void *p) {
    sf_block *bp = (sf_block *)((char *)p - 16); 
    debug("tally_free: %p (%lu/%lu)", bp, ((bp->header ^ MAGIC) & ~0xf) - 8, (bp->header ^ MAGIC) & ~0xf);
    current_aggregate_payload -= (((bp->header ^ MAGIC) & ~0xf) - 8);
    total_allocated_block_size -= ((bp->header ^ MAGIC) & ~0xf);
    if(current_aggregate_payload > total_allocated_block_size) {
        fprintf(stderr,
            "Aggregate payload (%lu) > total allocated block size (%lu) (not possible!)\n",
            current_aggregate_payload, total_allocated_block_size);
        abort();
    }
}

static void *SF_MALLOC(size_t size) {
    void *p = sf_malloc(size);
    tally_alloc(p, size);
    return p;
}

static void *SF_REALLOC(void *ptr, size_t size) {
    tally_free(ptr);
    void *p = sf_realloc(ptr, size);
    tally_alloc(p, size);
    return p;
}

static void SF_FREE(void *ptr) {
    tally_free(ptr);
    sf_free(ptr);
}

static double ref_sf_fragmentation() {
    double ret;
    if (total_allocated_block_size == 0)
        return 0;
    ret = 1.0 - (double)current_aggregate_payload / total_allocated_block_size;
    debug("current_aggregate_payload: %ld, total_allocated_block_size: %ld, internal fragmentation: %f%%",
      current_aggregate_payload, total_allocated_block_size, 100*ret);
    return ret;
}

static double ref_sf_utilization_v1() {
    double ret;
    if (sf_mem_end() == sf_mem_start())
        return 0;
    ret = (double)max_aggregate_payload / (sf_mem_end() - sf_mem_start());
    debug("max_aggregate_payload: %ld, heap size = %ld, peak utilization: %f%%",
       max_aggregate_payload, sf_mem_end() - sf_mem_start(), 100*ret);
    return ret;
}

static double ref_sf_utilization_v2() {
    double ret;
    if (sf_mem_end() == sf_mem_start())
        return 0;
    ret = (double)max_aggregate_payload / (sf_mem_end() - sf_mem_start() - 48);
    debug("max_aggregate_payload: %ld, heap size = %ld, peak utilization: %f%%",
       max_aggregate_payload, sf_mem_end() - sf_mem_start() - 48, 100*ret);
    return ret;
}

Test(sf_memsuite_stats, peak_utilization, .timeout = TEST_TIMEOUT)
{
    double actual;
    double expected_v1;
    double expected_v2;

    void * w = SF_MALLOC(10);
    void * x = SF_MALLOC(300);
    void * y = SF_MALLOC(500);
    SF_FREE(x);

    actual = sf_utilization();
    expected_v1 = ref_sf_utilization_v1();
    expected_v2 = ref_sf_utilization_v2();

    cr_assert(
        fabs(actual - expected_v1) <= 0.0001 || fabs(actual - expected_v2) <= 0.0001,
        "peak utilization_1 did not match (exp=%f or exp=%f, found=%f)",
        expected_v1, expected_v2, actual
    );

    x = SF_MALLOC(400);
    void * z = SF_MALLOC(1024);
    SF_FREE(w);
    SF_FREE(x);
    x = SF_MALLOC(2048);

    actual = sf_utilization();
    expected_v1 = ref_sf_utilization_v1();
    expected_v2 = ref_sf_utilization_v2();

    cr_assert(
        fabs(actual - expected_v1) <= 0.0001 || fabs(actual - expected_v2) <= 0.0001,
        "peak utilization_2 did not match (exp=%f or exp=%f, found=%f)",
        expected_v1, expected_v2, actual
    );

    SF_FREE(x);
    x = SF_MALLOC(7400);
    SF_FREE(y);
    SF_FREE(z);

    actual = sf_utilization();
    expected_v1 = ref_sf_utilization_v1();
    expected_v2 = ref_sf_utilization_v2();

    cr_assert(
        fabs(actual - expected_v1) <= 0.0001 || fabs(actual - expected_v2) <= 0.0001,
        "peak utilization_3 did not match (exp=%f or exp=%f, found=%f)",
        expected_v1, expected_v2, actual
    );
}

/*
Test(sf_memsuite_stats, stress_test, .timeout = TEST_TIMEOUT)
{
    double actual;
    double expected_v1;
    double expected_v2;

    run_stress_test(0);

    actual = sf_utilization();
    expected_v1 = ref_sf_utilization_v1();
    expected_v2 = ref_sf_utilization_v2();

    cr_assert(
        fabs(actual - expected_v1) <= 0.0001 || fabs(actual - expected_v2) <= 0.0001,
        "peak utilization did not match (exp=%f or exp=%f, found=%f)",
        expected_v1, expected_v2, actual
    );

#if 0
    actual = sf_fragmentation();
    expected = ref_sf_fragmentation();
    cr_assert_float_eq(actual, expected, 0.0001, "internal fragmentation did not match (exp=%f, found=%f)", expected, actual);
#endif

    _assert_heap_is_valid();

}
*/