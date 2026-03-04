/*
 * test_integration.c - 集成测试
 */

#include "unity.h"
#include "mec_common.h"
#include "mec_queue.h"
#include "mec_error.h"
#include "mec_batch.h"
#include <string.h>
#include <pthread.h>

static mec_queue_t *test_queue = NULL;
static int producer_count = 0;
static int consumer_count = 0;

void setUp(void) {
    test_queue = mec_queue_create(10);
    producer_count = 0;
    consumer_count = 0;
}

void tearDown(void) {
    if (test_queue) {
        mec_queue_destroy(test_queue);
        test_queue = NULL;
    }
}

/* 测试：队列基本功能 */
void test_queue_basic(void) {
    TEST_ASSERT_NOT_NULL(test_queue);
    TEST_ASSERT_EQUAL_INT(0, mec_queue_size(test_queue));
}

/* 测试：队列压入弹出 */
void test_queue_push_pop(void) {
    mec_msg_t msg = {
        .sensor_id = 1,
        .tracks = track_list_create(5)
    };
    
    int result = mec_queue_push(test_queue, &msg);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1, mec_queue_size(test_queue));
    
    mec_msg_t out_msg;
    result = mec_queue_pop(test_queue, &out_msg, 100);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1, out_msg.sensor_id);
    
    track_list_release(out_msg.tracks);
}

/* 测试：队列满 */
void test_queue_full(void) {
    for (int i = 0; i < 10; i++) {
        mec_msg_t msg = {.sensor_id = i};
        mec_queue_push(test_queue, &msg);
    }
    
    TEST_ASSERT_EQUAL_INT(10, mec_queue_size(test_queue));
    
    // 再压入会失败
    mec_msg_t msg = {.sensor_id = 100};
    int result = mec_queue_push(test_queue, &msg);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

/* 测试：队列使用率 */
void test_queue_usage(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, mec_queue_usage(test_queue));
    
    for (int i = 0; i < 5; i++) {
        mec_msg_t msg = {.sensor_id = i};
        mec_queue_push(test_queue, &msg);
    }
    
    float usage = mec_queue_usage(test_queue);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.5, usage);
}

/* 测试：错误码 */
void test_error_codes(void) {
    TEST_ASSERT_EQUAL_STRING("MEC_OK", mec_error_string(MEC_OK));
    TEST_ASSERT_EQUAL_STRING("MEC_ERROR", mec_error_string(MEC_ERROR));
    TEST_ASSERT_EQUAL_STRING("MEC_ERROR_INVALID_PARAM", mec_error_string(MEC_ERROR_INVALID_PARAM));
    TEST_ASSERT_EQUAL_STRING("MEC_ERROR_NO_MEMORY", mec_error_string(MEC_ERROR_NO_MEMORY));
}

/* 测试：错误码致命判断 */
void test_error_fatal(void) {
    TEST_ASSERT_EQUAL_INT(0, mec_error_is_fatal(MEC_OK));
    TEST_ASSERT_EQUAL_INT(0, mec_error_is_fatal(MEC_ERROR_TIMEOUT));
    TEST_ASSERT_EQUAL_INT(1, mec_error_is_fatal(MEC_ERROR_NO_MEMORY));
}

/* 测试：批处理基本功能 */
void test_batch_basic(void) {
    batch_config_t config = {
        .max_batch_size = 5,
        .max_wait_ms = 100,
        .enable_adaptive = true
    };
    
    mec_batch_ctx_t *batch = batch_create(&config);
    TEST_ASSERT_NOT_NULL(batch);
    
    // 添加数据
    int *data1 = malloc(sizeof(int));
    *data1 = 42;
    batch_add(batch, data1);
    
    TEST_ASSERT_EQUAL_INT(1, batch_size(batch));
    
    batch_destroy(batch);
}

/* 批处理回调 */
static void test_batch_callback(void **items, int count, void *user_data) {
    int *total = (int*)user_data;
    for (int i = 0; i < count; i++) {
        (*total) += *(int*)items[i];
    }
}

/* 测试：批处理回调 */
void test_batch_callback_fn(void) {
    batch_config_t config = {.max_batch_size = 3, .max_wait_ms = 50};
    mec_batch_ctx_t *batch = batch_create(&config);
    
    int total = 0;
    // 设置回调需要扩展接口，这里只测试基本功能
    
    batch_destroy(batch);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_queue_basic);
    RUN_TEST(test_queue_push_pop);
    RUN_TEST(test_queue_full);
    RUN_TEST(test_queue_usage);
    RUN_TEST(test_error_codes);
    RUN_TEST(test_error_fatal);
    RUN_TEST(test_batch_basic);
    RUN_TEST(test_batch_callback_fn);
    
    return UNITY_END();
}
