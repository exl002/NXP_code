#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "target_config.h"

#include "nxp_log.h"

#include "SEGGER_RTT.h"
#include "SEGGER_RTT_Conf.h"

#define BUFFER_INDEX_RTT    (0)
#define INFO_PREFIX         "[INFO]: "
#define INFO_PREFIX_LEN     (8)
#define DEBUG_PREFIX        "[DEBUG]: "
#define DEBUG_PREFIX_LEN    (9)
#define WARNING_PREFIX      "[WARNING]: "
#define WARNING_PREFIX_LEN  (11)
#define ERROR_PREFIX        "[ERROR]: "
#define ERROR_PREFIX_LEN    (9)

#define NXP_LOG_RTT_MAX_STR_LENGTH 256

#ifdef NXP_PRINT_ON
#define NXP_LOG_RTT_ENABLED (1)
#endif

static nxp_log_color_mapping_t m_nxp_log_color_map[] =
{
    {NXP_LOG_BLACK,   RTT_CTRL_TEXT_BLACK},
    {NXP_LOG_RED,     RTT_CTRL_TEXT_RED},
    {NXP_LOG_GREEN,   RTT_CTRL_TEXT_GREEN},
    {NXP_LOG_YELLOW,  RTT_CTRL_TEXT_YELLOW},
    {NXP_LOG_BLUE,    RTT_CTRL_TEXT_BLUE},
    {NXP_LOG_MAGENTA, RTT_CTRL_TEXT_MAGENTA},
    {NXP_LOG_CYAN,    RTT_CTRL_TEXT_CYAN},
    {NXP_LOG_WHITE,   RTT_CTRL_TEXT_WHITE},
};

void NXP_LOG_INFO(const char * sFormat, ...)
{
#ifdef NXP_PRINT_ON
    char info[NXP_LOG_RTT_MAX_STR_LENGTH];
    va_list ParamList;
    va_start(ParamList, sFormat);
    sprintf(info, "%s %s", RTT_CTRL_TEXT_BRIGHT_WHITE, INFO_PREFIX);
    vsnprintf(info + INFO_PREFIX_LEN + sizeof(RTT_CTRL_BG_BRIGHT_RED), sizeof(info) - strlen(info), sFormat, ParamList);

    SEGGER_RTT_vprintf(BUFFER_INDEX_RTT, info, &ParamList);
    va_end(ParamList);
#else
    
#endif
}

void NXP_LOG_WARNING(const char * sFormat, ...)
{
#if (NXP_LOG_RTT_ENABLED == 1)
    char warning[NXP_LOG_RTT_MAX_STR_LENGTH];
    va_list ParamList;
    va_start(ParamList, sFormat);
    sprintf(warning, "%s %s", RTT_CTRL_TEXT_BRIGHT_YELLOW, WARNING_PREFIX);
    vsnprintf(warning + WARNING_PREFIX_LEN + sizeof(RTT_CTRL_TEXT_BRIGHT_GREEN), sizeof(warning) - strlen(warning), sFormat, ParamList);

    SEGGER_RTT_vprintf(BUFFER_INDEX_RTT, warning, &ParamList);
    va_end(ParamList);
#else

#endif
}

void NXP_LOG_DEBUG(const char * sFormat, ...)
{
#if (NXP_LOG_RTT_ENABLED == 1)
    va_list ParamList;
    va_start(ParamList, sFormat);
    SEGGER_RTT_vprintf(BUFFER_INDEX_RTT, sFormat, &ParamList);
    va_end(ParamList);
#else

#endif
}

void NXP_LOG_ERROR(const char * sFormat, ...)
{
#if (NXP_LOG_RTT_ENABLED == 1)
    char error[NXP_LOG_RTT_MAX_STR_LENGTH];
    va_list ParamList;
    va_start(ParamList, sFormat);
    sprintf(error, "%s %s", RTT_CTRL_TEXT_BRIGHT_RED, ERROR_PREFIX);
    vsnprintf(error + ERROR_PREFIX_LEN + sizeof(RTT_CTRL_TEXT_BRIGHT_RED), sizeof(error) - strlen(error), sFormat, ParamList);

    SEGGER_RTT_vprintf(BUFFER_INDEX_RTT, error, &ParamList);
    va_end(ParamList);
#else

#endif
}

void NXP_LOG_INFO_COLOR(nxp_log_color_t color, const char * sFormat, ...)
{
#if (NXP_LOG_RTT_ENABLED == 1)
    char info[NXP_LOG_RTT_MAX_STR_LENGTH];
    va_list ParamList;
    va_start(ParamList, sFormat);
    sprintf(info, "%s %s", m_nxp_log_color_map[color].color_code_string, INFO_PREFIX);
    vsnprintf(info + INFO_PREFIX_LEN + strlen(m_nxp_log_color_map[color].color_code_string) + 1, sizeof(info) - strlen(info), sFormat, ParamList);

    SEGGER_RTT_vprintf(BUFFER_INDEX_RTT, info, &ParamList);
    va_end(ParamList);
#else

#endif
}
