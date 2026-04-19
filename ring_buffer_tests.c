/*
 * ring_buffer_tests.c
 * Basic unit tests for ring_buffer.c using assert-style checks.
 * Chris Schibi 2026
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ring_buffer.h"

static int tests_run = 0;
static int tests_failed = 0;

static void expect_int_eq(const char *name, int got, int want) {
    tests_run++;
    if (got != want) {
        tests_failed++;
        printf("[FAIL] %s: got %d, want %d\n", name, got, want);
    }
}

static void expect_size_eq(const char *name, size_t got, size_t want) {
    tests_run++;
    if (got != want) {
        tests_failed++;
        printf("[FAIL] %s: got %zu, want %zu\n", name, got, want);
    }
}

static void expect_double_eq(const char *name, double got, double want) {
    tests_run++;
    double diff = got - want;
    if (diff < 0.0) diff = -diff;
    if (diff > 0.000001) {
        tests_failed++;
        printf("[FAIL] %s: got %.6f, want %.6f\n", name, got, want);
    }
}

static void expect_str_eq(const char *name, const char *got, const char *want) {
    tests_run++;
    if (strcmp(got, want) != 0) {
        tests_failed++;
        printf("[FAIL] %s: got '%s', want '%s'\n", name, got, want);
    }
}

static void test_init_ok(void) {
    RingBuffer rb;
    expect_int_eq("init_ok", ring_buffer_init(&rb, 4), 0);
    expect_size_eq("size_zero", ring_buffer_size(&rb), 0);
    expect_size_eq("capacity", ring_buffer_capacity(&rb), 4);
    expect_int_eq("is_empty", ring_buffer_is_empty(&rb), 1);
    expect_int_eq("is_full", ring_buffer_is_full(&rb), 0);
    ring_buffer_free(&rb);
}

static void test_init_zero(void) {
    RingBuffer rb;
    expect_int_eq("init_zero", ring_buffer_init(&rb, 0), -1);
}

static void test_write_read_single(void) {
    RingBuffer rb;
    ring_buffer_init(&rb, 3);
    expect_int_eq("write_one", ring_buffer_write(&rb, 'A'), 0);
    expect_size_eq("size_one", ring_buffer_size(&rb), 1);
    expect_int_eq("peek_one", ring_buffer_peek(&rb), 'A');
    expect_int_eq("read_one", ring_buffer_read(&rb), 'A');
    expect_size_eq("size_back_zero", ring_buffer_size(&rb), 0);
    ring_buffer_free(&rb);
}

static void test_read_empty(void) {
    RingBuffer rb;
    ring_buffer_init(&rb, 2);
    expect_int_eq("read_empty", ring_buffer_read(&rb), -1);
    expect_int_eq("peek_empty", ring_buffer_peek(&rb), -1);
    ring_buffer_free(&rb);
}

static void test_fill_capacity(void) {
    RingBuffer rb;
    ring_buffer_init(&rb, 4);
    expect_double_eq("usage_empty", ring_buffer_usage_percent(&rb), 0.0);
    ring_buffer_write(&rb, 'a');
    ring_buffer_write(&rb, 'b');
    expect_double_eq("usage_half", ring_buffer_usage_percent(&rb), 50.0);
    ring_buffer_write(&rb, 'c');
    ring_buffer_write(&rb, 'd');
    expect_int_eq("is_full_true", ring_buffer_is_full(&rb), 1);
    expect_size_eq("size_full", ring_buffer_size(&rb), 4);
    expect_size_eq("avail_zero", ring_buffer_available_space(&rb), 0);
    expect_double_eq("usage_100", ring_buffer_usage_percent(&rb), 100.0);
    ring_buffer_free(&rb);
}

static void test_overwrite_on_full(void) {
    RingBuffer rb;
    ring_buffer_init(&rb, 3);
    ring_buffer_write(&rb, 'a');
    ring_buffer_write(&rb, 'b');
    ring_buffer_write(&rb, 'c');
    ring_buffer_write(&rb, 'd');
    expect_size_eq("size_stays_full", ring_buffer_size(&rb), 3);
    expect_int_eq("read_after_overwrite_1", ring_buffer_read(&rb), 'b');
    expect_int_eq("read_after_overwrite_2", ring_buffer_read(&rb), 'c');
    expect_int_eq("read_after_overwrite_3", ring_buffer_read(&rb), 'd');
    ring_buffer_free(&rb);
}

static void test_wraparound_order(void) {
    RingBuffer rb;
    ring_buffer_init(&rb, 3);
    ring_buffer_write(&rb, 'a');
    ring_buffer_write(&rb, 'b');
    ring_buffer_write(&rb, 'c');
    expect_int_eq("wrap_read_1", ring_buffer_read(&rb), 'a');
    expect_int_eq("wrap_read_2", ring_buffer_read(&rb), 'b');
    ring_buffer_write(&rb, 'd');
    ring_buffer_write(&rb, 'e');
    expect_int_eq("wrap_read_3", ring_buffer_read(&rb), 'c');
    expect_int_eq("wrap_read_4", ring_buffer_read(&rb), 'd');
    expect_int_eq("wrap_read_5", ring_buffer_read(&rb), 'e');
    ring_buffer_free(&rb);
}

static void test_bulk_write_read(void) {
    RingBuffer rb;
    ring_buffer_init(&rb, 10);
    const char *msg = "hello";
    expect_int_eq("bulk_write", ring_buffer_bulk_write(&rb, msg, strlen(msg)), 0);
    expect_size_eq("bulk_size", ring_buffer_size(&rb), 5);
    char out[8] = {0};
    int n = ring_buffer_bulk_read(&rb, out, 5);
    out[n] = '\0';
    expect_int_eq("bulk_read_count", n, 5);
    expect_str_eq("bulk_read_str", out, "hello");
    ring_buffer_free(&rb);
}

static void test_bulk_write_over_capacity(void) {
    RingBuffer rb;
    ring_buffer_init(&rb, 4);
    const char *msg = "abcdef";
    ring_buffer_bulk_write(&rb, msg, strlen(msg));
    char out[8] = {0};
    int n = ring_buffer_bulk_read(&rb, out, 4);
    out[n] = '\0';
    expect_int_eq("bulk_over_count", n, 4);
    expect_str_eq("bulk_over_str", out, "cdef");
    ring_buffer_free(&rb);
}

static void test_bulk_read_partial(void) {
    RingBuffer rb;
    ring_buffer_init(&rb, 5);
    ring_buffer_bulk_write(&rb, "abcd", 4);
    char out[8] = {0};
    int n = ring_buffer_bulk_read(&rb, out, 2);
    out[n] = '\0';
    expect_int_eq("bulk_partial_count", n, 2);
    expect_str_eq("bulk_partial_str", out, "ab");
    expect_size_eq("bulk_partial_remaining", ring_buffer_size(&rb), 2);
    ring_buffer_free(&rb);
}

static void test_clear(void) {
    RingBuffer rb;
    ring_buffer_init(&rb, 3);
    ring_buffer_bulk_write(&rb, "ab", 2);
    ring_buffer_clear(&rb);
    expect_size_eq("clear_size", ring_buffer_size(&rb), 0);
    expect_int_eq("clear_empty", ring_buffer_is_empty(&rb), 1);
    expect_int_eq("clear_read", ring_buffer_read(&rb), -1);
    ring_buffer_free(&rb);
}

static void test_resize_larger(void) {
    RingBuffer rb;
    ring_buffer_init(&rb, 3);
    ring_buffer_bulk_write(&rb, "abc", 3);
    expect_int_eq("resize_larger", ring_buffer_resize(&rb, 6), 0);
    expect_size_eq("resize_larger_size", ring_buffer_size(&rb), 3);
    expect_size_eq("resize_larger_cap", ring_buffer_capacity(&rb), 6);
    char out[8] = {0};
    int n = ring_buffer_bulk_read(&rb, out, 3);
    out[n] = '\0';
    expect_str_eq("resize_larger_str", out, "abc");
    ring_buffer_free(&rb);
}

static void test_resize_smaller_no_trunc(void) {
    RingBuffer rb;
    ring_buffer_init(&rb, 5);
    ring_buffer_bulk_write(&rb, "abc", 3);
    expect_int_eq("resize_smaller_ok", ring_buffer_resize(&rb, 3), 0);
    expect_size_eq("resize_smaller_size", ring_buffer_size(&rb), 3);
    char out[8] = {0};
    int n = ring_buffer_bulk_read(&rb, out, 3);
    out[n] = '\0';
    expect_str_eq("resize_smaller_str", out, "abc");
    ring_buffer_free(&rb);
}

static void test_resize_smaller_trunc(void) {
    RingBuffer rb;
    ring_buffer_init(&rb, 5);
    ring_buffer_bulk_write(&rb, "abcde", 5);
    expect_int_eq("resize_trunc", ring_buffer_resize(&rb, 3), 0);
    expect_size_eq("resize_trunc_size", ring_buffer_size(&rb), 3);
    char out[8] = {0};
    int n = ring_buffer_bulk_read(&rb, out, 3);
    out[n] = '\0';
    expect_str_eq("resize_trunc_str", out, "cde");
    ring_buffer_free(&rb);
}

static void test_resize_with_wraparound(void) {
    RingBuffer rb;
    ring_buffer_init(&rb, 5);
    ring_buffer_bulk_write(&rb, "abcde", 5);
    ring_buffer_read(&rb); // 'a' — tail=1
    ring_buffer_read(&rb); // 'b' — tail=2
    ring_buffer_read(&rb); // 'c' — tail=3
    ring_buffer_write(&rb, 'f'); // head wraps: written at index 0
    ring_buffer_write(&rb, 'g'); // written at index 1
    // internal layout: ['f','g','_','d','e'], tail=3, head=2, count=4
    expect_int_eq("resize_wrap_ok", ring_buffer_resize(&rb, 4), 0);
    expect_size_eq("resize_wrap_size", ring_buffer_size(&rb), 4);
    char out[8] = {0};
    int n = ring_buffer_bulk_read(&rb, out, 4);
    out[n] = '\0';
    expect_str_eq("resize_wrap_str", out, "defg");
    ring_buffer_free(&rb);
}

static void test_resize_zero(void) {
    RingBuffer rb;
    ring_buffer_init(&rb, 3);
    expect_int_eq("resize_zero", ring_buffer_resize(&rb, 0), -1);
    ring_buffer_free(&rb);
}

static void test_null_safety(void) {
    expect_int_eq("null_empty", ring_buffer_is_empty(NULL), 1);
    expect_int_eq("null_full", ring_buffer_is_full(NULL), 0);
    expect_size_eq("null_size", ring_buffer_size(NULL), 0);
    expect_size_eq("null_capacity", ring_buffer_capacity(NULL), 0);
    expect_size_eq("null_available", ring_buffer_available_space(NULL), 0);
    expect_int_eq("null_read", ring_buffer_read(NULL), -1);
    expect_int_eq("null_peek", ring_buffer_peek(NULL), -1);
    expect_int_eq("null_write", ring_buffer_write(NULL, 'x'), -1);
    expect_int_eq("null_bulk_write", ring_buffer_bulk_write(NULL, "x", 1), -1);
    char out[2] = {0};
    expect_int_eq("null_bulk_read", ring_buffer_bulk_read(NULL, out, 1), -1);
}

int main(void) {
    test_init_ok();
    test_init_zero();
    test_write_read_single();
    test_read_empty();
    test_fill_capacity();
    test_overwrite_on_full();
    test_wraparound_order();
    test_bulk_write_read();
    test_bulk_write_over_capacity();
    test_bulk_read_partial();
    test_clear();
    test_resize_larger();
    test_resize_smaller_no_trunc();
    test_resize_smaller_trunc();
    test_resize_with_wraparound();
    test_resize_zero();
    test_null_safety();

    if (tests_failed == 0) {
        printf("[PASS] %d tests\n", tests_run);
        return 0;
    }

    printf("[FAIL] %d of %d tests failed\n", tests_failed, tests_run);
    return 1;
}
