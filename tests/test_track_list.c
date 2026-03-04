/*
 * test_track_list.c - 航迹列表单元测试
 */

#include "unity.h"
#include "mec_common.h"
#include <string.h>

static track_list_t *test_list = NULL;

void setUp(void);
void tearDown(void);

/* Implementation of setUp and tearDown */
void setUp(void) {
    test_list = track_list_create(10);
}

void tearDown(void) {
    if (test_list) {
        track_list_release(test_list);
        test_list = NULL;
    }
}

/* 测试：创建航迹列表 */
void test_track_list_create(void) {
    track_list_t *list = track_list_create(10);
    TEST_ASSERT_NOT_NULL_MESSAGE(list, "Failed to create track list");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, list->count, "Initial count should be 0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(10, list->capacity, "Initial capacity should be 10");
    track_list_release(list);
}

/* 测试：添加航迹 */
void test_track_list_add(void) {
    target_track_t track = {
        .id = 1,
        .type = TARGET_VEHICLE,
        .position.latitude = 39.9042,
        .position.longitude = 116.4074,
        .velocity = 60.0,
        .heading = 90.0,
        .confidence = 0.95
    };
    
    int result = track_list_add(test_list, &track);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "Add should succeed");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, test_list->count, "Count should be 1");
}

/* 测试：添加多个航迹 */
void test_track_list_add_multiple(void) {
    for (int i = 0; i < 5; i++) {
        target_track_t track = {.id = i + 1};
        track_list_add(test_list, &track);
    }
    TEST_ASSERT_EQUAL_INT(5, test_list->count);
}

/* 测试：自动扩容 */
void test_track_list_expand(void) {
    // 初始容量为10，添加15个航迹应该触发扩容
    for (int i = 0; i < 15; i++) {
        target_track_t track = {.id = i + 1};
        track_list_add(test_list, &track);
    }
    TEST_ASSERT_EQUAL_INT(15, test_list->count);
    TEST_ASSERT_TRUE(test_list->capacity >= 15);
}

/* 测试：清空列表 */
void test_track_list_clear(void) {
    for (int i = 0; i < 5; i++) {
        target_track_t track = {.id = i + 1};
        track_list_add(test_list, &track);
    }
    
    track_list_clear(test_list);
    TEST_ASSERT_EQUAL_INT(0, test_list->count);
}

/* 测试：引用计数 */
void test_track_list_reference_counting(void) {
    // 初始引用为1
    TEST_ASSERT_EQUAL_INT(1, atomic_load(&test_list->ref_count));
    
    // retain 增加引用
    track_list_retain(test_list);
    TEST_ASSERT_EQUAL_INT(2, atomic_load(&test_list->ref_count));
    
    // release 减少引用
    track_list_release(test_list);
    TEST_ASSERT_EQUAL_INT(1, atomic_load(&test_list->ref_count));
}

/* 测试：空列表添加空指针 */
void test_track_list_add_null(void) {
    int result = track_list_add(NULL, NULL);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

/* 测试：空列表 */
void test_track_list_add_to_null_list(void) {
    track_list_release(test_list);
    test_list = NULL;
    
    target_track_t track = {.id = 1};
    int result = track_list_add(test_list, &track);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_track_list_create);
    RUN_TEST(test_track_list_add);
    RUN_TEST(test_track_list_add_multiple);
    RUN_TEST(test_track_list_expand);
    RUN_TEST(test_track_list_clear);
    RUN_TEST(test_track_list_reference_counting);
    RUN_TEST(test_track_list_add_null);
    RUN_TEST(test_track_list_add_to_null_list);
    
    return UNITY_END();
}
