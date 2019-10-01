#pragma once
#include <ext/library_logger/logger.hpp>

/// можно выключать некоторые макросы логирования
#if defined(EXTLL_DISABLE_ALL) && !defined(EXTLL_DISABLE_FATAL)
#define EXTLL_DISABLE_FATAL
#endif

#if defined(EXTLL_DISABLE_FATAL) && !defined(EXTLL_DISABLE_ERROR)
#define EXTLL_DISABLE_ERROR
#endif
#if defined(EXTLL_DISABLE_ERROR) && !defined(EXTLL_DISABLE_WARN)
#define EXTLL_DISABLE_WARN
#endif
#if defined(EXTLL_DISABLE_WARN) && !defined(EXTLL_DISABLE_INFO)
#define EXTLL_DISABLE_INFO
#endif
#if defined(EXTLL_DISABLE_INFO) && !defined(EXTLL_DISABLE_DEBUG)
#define EXTLL_DISABLE_DEBUG
#endif
#if defined(EXTLL_DISABLE_DEBUG) && !defined(EXTLL_DISABLE_TRACE)
#define EXTLL_DISABLE_TRACE
#endif

namespace ext {
namespace library_logger
{
	inline logger * get_logger(logger * lg) { return lg; }
	inline logger * get_logger(logger & lg) { return &lg; }
	inline logger * get_logger(logger && lg) = delete;

	//inline const logger * get_logger(const logger * lg) { return lg; }
	//inline const logger * get_logger(const logger & lg) { return &lg; }
	//inline const logger * get_logger(const logger && lg) = delete;

	// smart pointer overload
	template <class Pointer> inline auto get_logger(Pointer & ptr) { return ptr.get(); }
}}

#if defined(_MSC_VER)
#define EXTLL_SUPPRESS_DOWHILE_WARNING()  \
    __pragma (warning (push))             \
    __pragma (warning (disable:4127))

#define EXTLL_RESTORE_DOWHILE_WARNING()   \
    __pragma (warning (pop))

#else
#define EXTLL_SUPPRESS_DOWHILE_WARNING() /* empty */
#define EXTLL_RESTORE_DOWHILE_WARNING()  /* empty */

#endif

#ifndef EXTLL_DISABLE_ALL
#define EXTLL_LOG(lg, log_level, expr)                                                  \
EXTLL_SUPPRESS_DOWHILE_WARNING()                                                        \
do {                                                                                    \
    ext::library_logger::logger * ll = ext::library_logger::get_logger(lg);             \
    if (ll)                                                                             \
    {                                                                                   \
        auto rec = ll->open_record(log_level, __FILE__, __LINE__);                      \
        if (rec)                                                                        \
        {                                                                               \
            rec.get_ostream() << expr;                                                  \
            rec.push();                                                                 \
        }                                                                               \
    }                                                                                   \
} while(0)                                                                              \
EXTLL_RESTORE_DOWHILE_WARNING()

#define EXTLL_LOG_STR(lg, log_level, str)                                               \
EXTLL_SUPPRESS_DOWHILE_WARNING()                                                        \
do {                                                                                    \
    ext::library_logger::logger * ll = ext::library_logger::get_logger(lg);             \
    if (ll and ll->is_enabled_for(log_level))                                           \
    {                                                                                   \
        ll->log(log_level, str, __FILE__, __LINE__);                                    \
    }                                                                                   \
} while(0)                                                                              \
EXTLL_RESTORE_DOWHILE_WARNING()

#else

#define EXTLL_LOG(lg, log_level, expr) ((void)0)
#define EXTLL_LOG_STR(lg, log_level, expr) ((void)0)

#endif


#ifndef EXTLL_DISABLE_FATAL
#define EXTLL_FATAL(lg, expr) EXTLL_LOG(lg, ext::library_logger::Fatal, expr)
#define EXTLL_FATAL_STR(lg, str) EXTLL_LOG_STR(lg, ext::library_logger::Fatal, str)
#else
#define EXTLL_FATAL(lg, expr) ((void)0)
#define EXTLL_FATAL_STR(lg, str) ((void)0)
#endif

#ifndef EXTLL_DISABLE_ERROR
#define EXTLL_ERROR(lg, expr) EXTLL_LOG(lg, ext::library_logger::Error, expr)
#define EXTLL_ERROR_STR(lg, str) EXTLL_LOG_STR(lg, ext::library_logger::Error, str)
#else
#define EXTLL_ERROR(lg, expr) ((void)0)
#define EXTLL_ERROR_STR(lg, str) ((void)0)
#endif

#ifndef EXTLL_DISABLE_WARN
#define EXTLL_WARN(lg, expr) EXTLL_LOG(lg, ext::library_logger::Warn, expr)
#define EXTLL_WARN_STR(lg, str) EXTLL_LOG_STR(lg, ext::library_logger::Warn, str)
#else
#define EXTLL_WARN(lg, expr) ((void)0)
#define EXTLL_WARN_STR(lg, str) ((void)0)
#endif

#ifndef EXTLL_DISABLE_INFO
#define EXTLL_INFO(lg, expr) EXTLL_LOG(lg, ext::library_logger::Info, expr)
#define EXTLL_INFO_STR(lg, str) EXTLL_LOG_STR(lg, ext::library_logger::Info, str)
#else
#define EXTLL_INFO(lg, expr) ((void)0)
#define EXTLL_INFO_STR(lg, str) ((void)0)
#endif

#ifndef EXTLL_DISABLE_DEBUG
#define EXTLL_DEBUG(lg, expr) EXTLL_LOG(lg, ext::library_logger::Debug, expr)
#define EXTLL_DEBUG_STR(lg, str) EXTLL_LOG_STR(lg, ext::library_logger::Debug, str)
#else
#define EXTLL_DEBUG(lg, expr) ((void)0)
#define EXTLL_DEBUG_STR(lg, str) ((void)0)
#endif

#ifndef EXTLL_DISABLE_TRACE
#define EXTLL_TRACE(lg, expr) EXTLL_LOG(lg, ext::library_logger::Trace, expr)
#define EXTLL_TRACE_STR(lg, str) EXTLL_LOG_STR(lg, ext::library_logger::Trace, str)
#else
#define EXTLL_TRACE(lg, expr) ((void)0)
#define EXTLL_TRACE_STR(lg, str) ((void)0)
#endif



/// addition convenience macros for direct libfmt usage, NOTE this file does not include anything from fmt
/// EXTLL_INFO_FMT(lg, "Hello {}", "World") same as EXTLL_INFO(lg, fmt::format("Hello {}", "Worlds"))

#define EXTLL_LOG_FMT(lg, log_level, ...)   EXTLL_LOG_STR(lg, log_level, fmt::format(__VA_ARGS__))
#define EXTLL_FATAL_FMT(lg, ...)            EXTLL_FATAL_STR(lg, fmt::format(__VA_ARGS__))
#define EXTLL_ERROR_FMT(lg, ...)            EXTLL_ERROR_STR(lg, fmt::format(__VA_ARGS__))
#define EXTLL_WARN_FMT(lg, ...)             EXTLL_WARN_STR(lg, fmt::format(__VA_ARGS__))
#define EXTLL_INFO_FMT(lg, ...)             EXTLL_INFO_STR(lg, fmt::format(__VA_ARGS__))
#define EXTLL_DEBUG_FMT(lg, ...)            EXTLL_DEBUG_STR(lg, fmt::format(__VA_ARGS__))
#define EXTLL_TRACE_FMT(lg, ...)            EXTLL_TRACE_STR(lg, fmt::format(__VA_ARGS__))
