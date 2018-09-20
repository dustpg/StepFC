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


SFC_EXTERN_C int xa2_init(uint32_t sample_persec) SFC_NOEXCEPT;
SFC_EXTERN_C void xa2_clean() SFC_NOEXCEPT;

SFC_EXTERN_C void xa2_submit_buffer(const float*, uint32_t len) SFC_NOEXCEPT;
SFC_EXTERN_C void xa2_flush_buffer() SFC_NOEXCEPT;
SFC_EXTERN_C unsigned xa2_buffer_left() SFC_NOEXCEPT;
