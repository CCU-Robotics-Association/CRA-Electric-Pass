#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * THEME Timer / Scheduler
 *
 * 约束/语义（按用户确认）：
 * - 时间单位：对外接口全部使用 us（微秒）
 * - 精度：毫秒级即可（内部用 POSIX timer + CLOCK_MONOTONIC）
 * - 回调执行：允许并发执行；由用户保证回调执行时间不会超过 interval
 * - destroy/cancel：立即返回；保证之后不再触发即可（不等待正在运行的回调结束）
 * - one-shot / 有限次数：触发完成后自动取消并回收内部资源
 * - safe no-op：对已结束/已销毁的句柄，cancel/destroy 需要安全 no-op（返回 0）
 *
 * 单实例约束（S）：
 * - 进程内只允许 init 一个 app_timer_t 实例（为了在 SIGEV_THREAD trampoline 中安全定位 tm）
 */

typedef uint64_t app_timer_handle_t;
typedef void (*app_timer_cb)(void *userdata,bool is_last);

#ifndef APP_TIMER_MAX
#define APP_TIMER_MAX 1024
#endif

typedef struct {
    uint32_t gen;          // generation（用于 handle 校验；同时低 16 bit 用于 SIGEV_THREAD 的 sival_int）
    bool active;
    timer_t t;

    app_timer_cb cb;
    void *userdata;

    uint64_t interval_us;
    int64_t remaining;     // -1: infinite; >0: remaining fires
} app_timer_slot_t;

typedef struct app_timer {
    pthread_mutex_t mtx;
    bool mtx_inited;
    bool inited;

    app_timer_slot_t slots[APP_TIMER_MAX + 1]; // id: 1..APP_TIMER_MAX
    uint16_t free_ids[APP_TIMER_MAX];
    uint16_t free_top; // 栈顶索引：当前可用数量（push: free_ids[free_top++]=id; pop: id=free_ids[--free_top]）
} app_timer_t;

int app_timer_init(app_timer_t *tm);
int app_timer_destroy(app_timer_t *tm);

/**
 * 创建一个定时器
 *
 * @param out            返回句柄（handle = (gen<<32)|id）
 * @param start_delay_us 首次触发延迟(us)。若为 0，则默认使用 interval_us（若 interval_us==0 则内部使用 1us 以确保启动）
 * @param interval_us    周期(us)。无限/有限循环需要 >0；one-shot 可为 0（仅按 start_delay_us 触发一次）
 * @param fire_count     -1: 无限；>=1: 触发次数（1 为单次；>1 为有限次数）
 * @param cb             回调
 * @param userdata       回调参数
 */
int app_timer_create(app_timer_handle_t *out,
                      uint64_t start_delay_us,
                      uint64_t interval_us,
                      int64_t fire_count,
                      app_timer_cb cb,
                      void *userdata);

int app_timer_cancel(app_timer_handle_t handle);

#ifdef __cplusplus
}
#endif