#ifndef __NXP_LOG_H__
#define __NXP_LOG_H__

#include <stdarg.h>
#include <stdlib.h>
#include "fsl_common.h"

#include "SEGGER_RTT.h"

typedef enum
{
    NXP_LOG_BLACK = 0,
    NXP_LOG_RED,
    NXP_LOG_GREEN,
    NXP_LOG_YELLOW,
    NXP_LOG_BLUE,
    NXP_LOG_MAGENTA,
    NXP_LOG_CYAN,
    NXP_LOG_WHITE,
} nxp_log_color_t;

typedef struct
{
    nxp_log_color_t   color_code;
    const char       *color_code_string;
} nxp_log_color_mapping_t;


void NXP_LOG_INFO       (const char * sFormat, ...);
void NXP_LOG_DEBUG      (const char * sFormat, ...);
void NXP_LOG_WARNING    (const char * sFormat, ...);
void NXP_LOG_ERROR      (const char * sFormat, ...);
void NXP_LOG_INFO_COLOR (nxp_log_color_t color, const char * sFormat, ...);

#endif
