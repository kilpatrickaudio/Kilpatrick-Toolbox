/*
 * Portable Logging Interface
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2021: Andrew Kilpatrick
 *
 * Please see the license file included with this repo for license details.
 *
 */
#ifndef PLOG_H
#define PLOG_H

#include "../plugin.hpp"

#ifdef PLATFORM_VCV
#warning PLATFORM_VCV defined - using VCV logging interface
#define PDEBUG(format, ...) rack::logger::log(rack::logger::DEBUG_LEVEL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define PINFO(format, ...) rack::logger::log(rack::logger::INFO_LEVEL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define PWARN(format, ...) rack::logger::log(rack::logger::WARN_LEVEL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define PFATAL(format, ...) rack::logger::log(rack::logger::FATAL_LEVEL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#elif defined(PLATFORM_STM32)
#define PDEBUG(format, ...) log_debug((char *)format, ##__VA_ARGS__)
#define PINFO(format, ...) log_info(format, ##__VA_ARGS__)
#define PWARN(format, ...) log_warn(format, ##__VA_ARGS__)
#define PFATAL(format, ...) log_error(format, ##__VA_ARGS__)
#else
#error log type not defined - must be: PLATFORM_VCV or PLATFORM_STM32
#endif

#endif
