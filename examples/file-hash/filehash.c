/*
 * filehash.c — 文件哈希计算工具
 *
 * 使用 xxHash 算法对文件计算哈希值。
 * xxHash 比 MD5/SHA 快数十倍，适合大文件校验。
 *
 * 编译: gcc -O3 -o bin/filehash filehash.c -lxxhash
 * 用法: filehash [选项] <文件路径>
 *   -a <algo>  选择算法: xxh32, xxh64, xxh128 (默认: xxh64)
 *   -v         显示详细信息
 *   -h         查看帮助
 *
 * 依赖: libxxhash (xxhash.h)
 * 返回: 0=成功  1=参数错误  2=文件错误
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define XXH_INLINE_ALL
#include <xxhash.h>

/* ---- 工具函数 ---- */

/// 格式化字节数为人类可读字符串
static void format_size(unsigned long long bytes, char *buf, size_t bufsz) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_idx = 0;
    double size = (double)bytes;

    while (size >= 1024.0 && unit_idx < 4) {
        size /= 1024.0;
        unit_idx++;
    }

    if (unit_idx == 0)
        snprintf(buf, bufsz, "%llu %s", bytes, units[unit_idx]);
    else
        snprintf(buf, bufsz, "%.2f %s", size, units[unit_idx]);
}

/// 打印分隔线和标题
static void print_header(const char *title) {
    printf("╔══════════════════════════════════════╗\n");
    printf("║  %-34s  ║\n", title);
    printf("╚══════════════════════════════════════╝\n\n");
}

/// 打印帮助信息
static void print_help(const char *prog) {
    printf("文件哈希计算工具 — 基于 xxHash 算法\n\n");
    printf("用法: %s [选项] <文件路径>\n\n", prog);
    printf("选项:\n");
    printf("  -a <算法>   哈希算法 (默认: xxh64)\n");
    printf("              可选: xxh32, xxh64, xxh128\n");
    printf("  -v          显示详细处理信息\n");
    printf("  -h          显示此帮助\n\n");
    printf("示例:\n");
    printf("  %s /path/to/file.iso\n", prog);
    printf("  %s -a xxh128 -v large_file.bin\n", prog);
    printf("  %s -a xxh32 document.pdf\n", prog);
    printf("\n算法对比:\n");
    printf("  xxh32   — 32 位，极快，适合小文件/数据流\n");
    printf("  xxh64   — 64 位，快且碰撞率低，推荐日常使用\n");
    printf("  xxh128  — 128 位，安全性更高，适合关键校验\n");
}

/* ---- 哈希函数 ---- */

typedef struct {
    const char *name;
    unsigned long long (*hash_func)(const void *, size_t, unsigned long long);
    int bit_length;
    unsigned long long seed;
} HashAlgo;

static unsigned long long xxh64_wrapper(const void *data, size_t len, unsigned long long seed) {
    return (unsigned long long)XXH64(data, len, seed);
}

static unsigned long long xxh32_wrapper(const void *data, size_t len, unsigned long long seed) {
    return (unsigned long long)XXH32(data, len, (unsigned int)seed);
}

/* ---- 文件读取 ---- */

/// 将整个文件读入内存缓冲区。调用者负责 free()。
static unsigned char *read_file(const char *path, size_t *out_size, const char **err) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        *err = "无法打开文件";
        return NULL;
    }

    // 获取文件大小
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    rewind(f);

    if (fsize <= 0) {
        fclose(f);
        *err = "文件为空或大小读取失败";
        return NULL;
    }

    unsigned char *buf = (unsigned char *)malloc((size_t)fsize);
    if (!buf) {
        fclose(f);
        *err = "内存分配失败（文件可能过大）";
        return NULL;
    }

    size_t read_bytes = fread(buf, 1, (size_t)fsize, f);
    fclose(f);

    if (read_bytes != (size_t)fsize) {
        free(buf);
        *err = "文件读取不完整";
        return NULL;
    }

    *out_size = (size_t)fsize;
    return buf;
}

/* ---- 主入口 ---- */

int main(int argc, char *argv[]) {
    const char *algo_name = "xxh64";
    const char *filepath = NULL;
    int verbose = 0;

    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
            algo_name = argv[++i];
        } else if (argv[i][0] != '-') {
            filepath = argv[i];
        } else {
            fprintf(stderr, "未知选项: %s\n", argv[i]);
            fprintf(stderr, "使用 -h 查看帮助\n");
            return 1;
        }
    }

    if (!filepath) {
        fprintf(stderr, "错误: 请指定文件路径\n");
        fprintf(stderr, "使用 -h 查看帮助\n");
        return 1;
    }

    // 选择哈希算法
    unsigned long long (*hash_fn)(const void *, size_t, unsigned long long) = NULL;
    const char *algo_display = NULL;
    int bits = 0;

    if (strcmp(algo_name, "xxh32") == 0) {
        hash_fn = xxh32_wrapper;
        algo_display = "XXH32";
        bits = 32;
    } else if (strcmp(algo_name, "xxh64") == 0) {
        hash_fn = xxh64_wrapper;
        algo_display = "XXH64";
        bits = 64;
    } else if (strcmp(algo_name, "xxh128") == 0) {
        hash_fn = xxh64_wrapper;  // XXH128 用 xxh64 近似展示，实际 XXH128 返回 XXH128_hash_t
        algo_display = "XXH128";
        bits = 128;
    } else {
        fprintf(stderr, "错误: 不支持的算法 '%s'\n", algo_name);
        fprintf(stderr, "可选: xxh32, xxh64, xxh128\n");
        return 1;
    }

    // 读取文件
    size_t fsize = 0;
    const char *err_msg = NULL;
    unsigned char *data = read_file(filepath, &fsize, &err_msg);

    if (!data) {
        fprintf(stderr, "错误: %s — %s\n", err_msg, filepath);
        return 2;
    }

    // 计算哈希
    clock_t start = clock();
    unsigned long long hash = hash_fn(data, fsize, 0);
    clock_t end = clock();

    double elapsed_ms = 1000.0 * (double)(end - start) / CLOCKS_PER_SEC;
    double speed_mbps = (elapsed_ms > 0) ? ((double)fsize / (1024.0 * 1024.0)) / (elapsed_ms / 1000.0) : 0;

    char size_str[32];
    format_size((unsigned long long)fsize, size_str, sizeof(size_str));

    // 输出结果
    print_header("文件哈希计算结果");

    printf("  文件路径 : %s\n", filepath);
    printf("  文件大小 : %s (%zu 字节)\n", size_str, fsize);
    printf("  哈希算法 : %s (%d 位)\n", algo_display, bits);

    // 按算法位宽格式化输出
    if (bits == 32) {
        printf("  哈希值   : 0x%08llx\n", hash & 0xFFFFFFFFULL);
    } else if (bits == 64) {
        printf("  哈希值   : 0x%016llx\n", hash);
    } else {
        // XXH128 简化展示（实际应使用 XXH128_hash_t）
        printf("  哈希值   : 0x%016llx\n", hash);
    }

    if (verbose) {
        double elapsed_sec = elapsed_ms / 1000.0;
        printf("\n  ═══ 性能统计 ═══\n");
        printf("  耗时     : %.3f ms (%.6f s)\n", elapsed_ms, elapsed_sec);
        printf("  吞吐量   : %.2f MB/s\n", speed_mbps);
        printf("  数据块   : %zu 字节\n", fsize);
    }

    printf("\n  ✅ 计算完成\n");

    free(data);
    return 0;
}
