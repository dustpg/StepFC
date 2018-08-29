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

SFC_EXTERN_C void xa2_play() SFC_NOEXCEPT;

