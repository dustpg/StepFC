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

SFC_EXTERN_C void main_cpp(
    const char* shader_bin_file_name, 
    const char* shader_res_file_name
) SFC_NOEXCEPT;

SFC_EXTERN_C void main_render(void* rgba) SFC_NOEXCEPT;
SFC_EXTERN_C void user_input(int index, unsigned char data) SFC_NOEXCEPT;
SFC_EXTERN_C void qsave() SFC_NOEXCEPT;
SFC_EXTERN_C void qload() SFC_NOEXCEPT;

SFC_EXTERN_C void d2d_submit_wave(const float*, unsigned len) SFC_NOEXCEPT;

#ifdef SFC_NO_INPUT
SFC_EXTERN_C void user_input(int index, unsigned char data) SFC_NOEXCEPT {

}
#endif

#ifdef SFC_NO_SL
SFC_EXTERN_C void qsave() SFC_NOEXCEPT {
}
SFC_EXTERN_C void qload() SFC_NOEXCEPT {
}
#endif

