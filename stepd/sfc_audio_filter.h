#pragma once
// License: MIT  http://opensource.org/licenses/MIT
// Author: dustpg   mailto:dustpg@gmail.com


// 一阶RC低通滤波器
typedef struct {
    float   k1;
    float   k2;
    float   py;
    float   rc;
} sfc_1storder_rc_lopass_filter_t;

// 一阶RC高通滤波器
typedef struct {
    float   k1;
    float   py;
    float   px;
    float   rc;
} sfc_1storder_rc_hipass_filter_t;

// 初始化
static inline void sfc_make_rclp(
    sfc_1storder_rc_lopass_filter_t* filter,
    float sample_frequency,
    float cutoff_frequency) {
    const float PI = 3.1415926535897932384626433832795;
    const float RC = 1.f / 2.f / PI / cutoff_frequency;
    const float k1 = 1.f / (1.f + RC * sample_frequency);
    const float k2 = 1.f - k1;

    filter->k1 = k1;
    filter->k2 = k2;
    filter->py = 0.f;
    filter->rc = RC;
}

// 初始化
static inline void sfc_make_rchp(
    sfc_1storder_rc_hipass_filter_t* filter,
    float sample_frequency,
    float cutoff_frequency) {
    const float PI = 3.1415926535897932384626433832795;
    const float RC = 1.f / 2.f / PI / cutoff_frequency;
    float k = RC / (RC + 1.f / sample_frequency);

    filter->k1 = k;
    filter->py = 0.f;
    filter->px = 0.f;
    filter->rc = RC;
}


// 进行一阶RC低通滤波
static inline float sfc_filter_rclp(sfc_1storder_rc_lopass_filter_t* filter, float x) {
    return filter->py = x * filter->k1 + filter->py * filter->k2;
}

// 进行一阶RC高通滤波
static inline float sfc_filter_rchp(sfc_1storder_rc_hipass_filter_t* filter, float x) {
    const float y = (x - filter->px + filter->py) * filter->k1;
    filter->px = x;
    filter->py = y;
    return y;
}