#pragma once

#ifdef __cplusplus
#define SFC_EXTERN_C extern "C" 
#define SFC_NOEXCEPT noexcept
#else
#define SFC_EXTERN_C extern
#define SFC_NOEXCEPT
#endif

SFC_EXTERN_C void main_cpp() SFC_NOEXCEPT;
SFC_EXTERN_C void main_render(void* bgrx) SFC_NOEXCEPT;
SFC_EXTERN_C void user_input(int index, unsigned char data) SFC_NOEXCEPT;

#ifdef SFC_NO_INPUT
SFC_EXTERN_C void user_input(int index, unsigned char data) SFC_NOEXCEPT {

}
#endif
