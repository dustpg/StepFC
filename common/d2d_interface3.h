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

typedef unsigned char sfc_byte_t;

typedef struct {
    float           color[4];
    float           freq;
    sfc_byte_t      key_on;
    sfc_byte_t      mask;
    sfc_byte_t      volume;
    union {
        sfc_byte_t  sq_duty;
        sfc_byte_t  noi_mode;
    }               ud;
    union {
        // VRC7
        struct {
            sfc_byte_t  instrument;

            sfc_byte_t  mod_state;
            sfc_byte_t  car_state;

            sfc_byte_t  mod_am;
            sfc_byte_t  mod_fm;
            sfc_byte_t  car_am;
            sfc_byte_t  car_fm;
        }           vrc7;
        // FME7
        struct {
            sfc_byte_t  noi;
            sfc_byte_t  env;
            sfc_byte_t  tone;
        }           fme7;
        // N163
        struct {
            unsigned    wavtbl_len;
            unsigned    wavtbl_off;
        }           n163;
        float       unused_float[2];
    }               ex;
} sfc_visualizers_t;


SFC_EXTERN_C void d2d_set_visualizers(
    const sfc_visualizers_t*, unsigned len,
    const float* n163_wave_table,
    const float* fds1_wave_table,
    const float* vrc7_wave_table, unsigned vrc7_wtlen,
    const float* chbuf, unsigned sample_per_frame
) SFC_NOEXCEPT;


enum {
    SFC_VIS_PCM_NOISE = 0,
    SFC_VIS_PCM_DMC,
    SFC_VIS_PCM_PCM,
    SFC_VIS_PCM_COUNT,
};

SFC_EXTERN_C void d2d_set_time_lines()  SFC_NOEXCEPT;


SFC_EXTERN_C void d2d_n163_wavtbl_changed(unsigned)  SFC_NOEXCEPT;
SFC_EXTERN_C void d2d_fds1_wavtbl_changed()  SFC_NOEXCEPT;
SFC_EXTERN_C void d2d_vrc7_wavtbl_changed(unsigned)  SFC_NOEXCEPT;



