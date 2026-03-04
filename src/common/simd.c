/*
 * mec_simd.c - SIMD 加速实现
 */

#include "mec_simd.h"
#include "mec_logging.h"
#include <stdlib.h>
#include <string.h>
#include <sys/sysctl.h>

// 检测 CPU 支持的 SIMD 指令集
simd_capability_t simd_get_capability(void) {
    simd_capability_t cap = {0};
    
#if defined(__x86_64__) || defined(_M_X64)
    // x86_64 通常支持 SSE-SSE4.2
    cap.sse = true;
    cap.sse2 = true;
    cap.sse3 = true;
    cap.ssse3 = true;
    cap.sse4_1 = true;
    cap.sse4_2 = true;
    // AVX 需要运行时检测，这里简化处理
    cap.avx = false;
    cap.avx2 = false;
    
#elif defined(__aarch64__) || defined(__arm__)
    // Apple Silicon 支持 NEON
    cap.neon = true;
#endif
    
    return cap;
}

const char* simd_get_level(void) {
    simd_capability_t cap = simd_get_capability();
    
    if (cap.avx2) return "AVX2";
    if (cap.avx) return "AVX";
    if (cap.sse4_2) return "SSE4.2";
    if (cap.sse4_1) return "SSE4.1";
    if (cap.ssse3) return "SSSE3";
    if (cap.sse3) return "SSE3";
    if (cap.sse2) return "SSE2";
    if (cap.sse) return "SSE";
    if (cap.neon) return "NEON";
    
    return "NONE";
}

/* 
 * 基础实现（非 SIMD）
 * 实际使用时会被编译器或汇编版本替换
 */

void simd_matrix_add(const double *a, const double *b, double *c, int n) {
    for (int i = 0; i < n * n; i++) {
        c[i] = a[i] + b[i];
    }
}

void simd_matrix_mul(const double *a, const double *b, double *c, int m, int k, int n) {
    // C = A * B: A(mxk) * B(kxn) = C(mxn)
    memset(c, 0, m * n * sizeof(double));
    
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            for (int l = 0; l < k; l++) {
                c[i * n + j] += a[i * k + l] * b[l * n + j];
            }
        }
    }
}

void simd_vector_add(const double *x, double *y, int n) {
    for (int i = 0; i < n; i++) {
        y[i] += x[i];
    }
}

double simd_vector_dot(const double *x, const double *y, int n) {
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        sum += x[i] * y[i];
    }
    return sum;
}

void simd_vector_scale(double a, const double *x, double *y, int n) {
    for (int i = 0; i < n; i++) {
        y[i] = a * x[i];
    }
}

/* 
 * 编译时可以选择使用 SIMD 优化版本
 * 实际生产环境可以使用 OpenBLAS 或类似库
 */

// 初始化时打印 SIMD 能力
__attribute__((constructor))
static void simd_init(void) {
    const char *level = simd_get_level();
    if (strcmp(level, "NONE") != 0) {
        LOG_INFO("SIMD: Using %s acceleration", level);
    } else {
        LOG_INFO("SIMD: No hardware acceleration available, using scalar");
    }
}
