#pragma once
// License: MIT  http://opensource.org/licenses/MIT
// Author: dustpg   mailto:dustpg@gmail.com

/// <summary>
/// 错误码
/// </summary>
typedef enum sfc_error_code {
    SFC_ERROR_OK = 0,
    SFC_ERROR_FAILED,
    SFC_ERROR_MAPPER_NOT_FOUND,
    SFC_ERROR_FILE_NOT_FOUND,
    SFC_ERROR_ILLEGAL_FILE,
    SFC_ERROR_OUT_OF_MEMORY,
    SFC_ERROR_VERSION_NOT_MATCHED,
    SFC_ERROR_BAD_ENDAIN,
    SFC_ERROR_UNSUPPORTED,
} sfc_ecode;