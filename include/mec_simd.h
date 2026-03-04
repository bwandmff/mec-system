/*
 * mec_simd.h - SIMD 加速接口
 * 
 * 提供 SIMD 优化的矩阵运算接口
 */

#ifndef MEC_SIMD_H
#define MEC_SIMD_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 检测 CPU 支持的 SIMD 指令集
 */
typedef struct {
    bool sse;    // SSE support
    bool sse2;   // SSE2 support
    bool sse3;   // SSE3 support
    bool ssse3;  // SSSE3 support
    bool sse4_1;  // SSE4.1 support
    bool sse4_2;  // SSE4.2 support
    bool avx;     // AVX support
    bool avx2;    // AVX2 support
    bool neon;    // ARM NEON support
} simd_capability_t;

/**
 * @brief 获取 CPU SIMD 能力
 */
simd_capability_t simd_get_capability(void);

/**
 * @brief 获取最优的 SIMD 级别
 */
const char* simd_get_level(void);

/* ==================== 矩阵运算 ==================== */

/**
 * @brief SIMD 加速的矩阵加法: C = A + B
 * 
 * @param a 输入矩阵 A
 * @param b 输入矩阵 B
 * @param c 输出矩阵 C
 * @param n 矩阵维度
 */
void simd_matrix_add(const double *a, const double *b, double *c, int n);

/**
 * @brief SIMD 加速的矩阵乘法: C = A * B
 * 
 * @param a 输入矩阵 A (m x k)
 * @param b 输入矩阵 B (k x n)
 * @param c 输出矩阵 C (m x n)
 * @param m 矩阵 A 行数
 * @param k 矩阵 A 列数 / B 行数
 * @param n 矩阵 B 列数
 */
void simd_matrix_mul(const double *a, const double *b, double *c, int m, int k, int n);

/**
 * @brief SIMD 加速的向量加法: y = y + x
 * 
 * @param x 输入向量
 * @param y 输出向量
 * @param n 向量长度
 */
void simd_vector_add(const double *x, double *y, int n);

/**
 * @brief SIMD 加速的向量点积: sum = x · y
 * 
 * @param x 输入向量 x
 * @param y 输入向量 y
 * @param n 向量长度
 * @return 点积结果
 */
double simd_vector_dot(const double *x, const double *y, int n);

/**
 * @brief SIMD 加速的向量缩放: y = a * x
 * 
 * @param a 标量系数
 * @param x 输入向量
 * @param y 输出向量
 * @param n 向量长度
 */
void simd_vector_scale(double a, const double *x, double *y, int n);

#endif // MEC_SIMD_H
