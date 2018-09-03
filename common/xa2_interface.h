#pragma once

#ifndef SFC_NOEXCEPT
#ifdef __cplusplus
#define SFC_EXTERN_C extern "C" 
#define SFC_NOEXCEPT noexcept
#else
#define SFC_EXTERN_C extern
#define SFC_NOEXCEPT
#endif
#endif


SFC_EXTERN_C int xa2_init() SFC_NOEXCEPT;
SFC_EXTERN_C void xa2_clean() SFC_NOEXCEPT;

SFC_EXTERN_C void xa2_play_square1(float frequency, uint16_t duty, uint16_t volume) SFC_NOEXCEPT;
SFC_EXTERN_C void xa2_play_square2(float frequency, uint16_t duty, uint16_t volume) SFC_NOEXCEPT;
SFC_EXTERN_C void xa2_play_triangle(float frequency) SFC_NOEXCEPT;
SFC_EXTERN_C void xa2_play_noise(uint16_t data, uint16_t volume) SFC_NOEXCEPT;

