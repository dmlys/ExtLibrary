#pragma once
#include <ext/library_logger/logger.hpp>
#include <log4cplus/logger.h>

namespace log4cplus
{
	inline LogLevel log_level(unsigned log_level)
	{
		switch (log_level)
		{
		    default:
		    case ext::library_logger::Trace: return TRACE_LOG_LEVEL;
		    case ext::library_logger::Debug: return DEBUG_LOG_LEVEL;
		    case ext::library_logger::Info:  return INFO_LOG_LEVEL;
		    case ext::library_logger::Warn:  return WARN_LOG_LEVEL;
		    case ext::library_logger::Error: return ERROR_LOG_LEVEL;
		    case ext::library_logger::Fatal: return FATAL_LOG_LEVEL;
		}
	}
}

namespace ext::library_logger
{
	/// log4cplus implementation of ext::library_logger::logger
	class log4cplus_logger : public simple_logger
	{
		log4cplus::Logger * m_logger = nullptr;

	protected:
		virtual bool do_is_enabled_for(unsigned log_level) const override;
		virtual void do_log(unsigned log_level, const std::string & log_str, const char * source_file, int source_line) override;

	public:
		log4cplus_logger(log4cplus::Logger & logger)
		    : m_logger(&logger) {}
	};

	inline bool log4cplus_logger::do_is_enabled_for(unsigned log_level) const
	{
		return log_level != Disabled and m_logger->isEnabledFor(log4cplus::log_level(log_level));
	}

	inline void log4cplus_logger::do_log(unsigned log_level, const std::string & log_str, const char * source_file, int source_line)
	{
		m_logger->log(log4cplus::log_level(log_level), log_str, source_file, source_line);
	}


	/// log4cplus implementation of ext::library_logger::logger for simple sequenced cases
	class sequenced_log4cplus_logger : public sequenced_simple_logger
	{
		log4cplus::Logger * m_logger = nullptr;

	protected:
		virtual bool do_is_enabled_for(unsigned log_level) const override;
		virtual void do_log(unsigned log_level, const std::string & log_str, const char * source_file, int source_line) override;

	public:
		sequenced_log4cplus_logger(log4cplus::Logger & logger)
		    : m_logger(&logger) {}
	};

	inline bool sequenced_log4cplus_logger::do_is_enabled_for(unsigned log_level) const
	{
		return log_level != Disabled and m_logger->isEnabledFor(log4cplus::log_level(log_level));
	}

	inline void sequenced_log4cplus_logger::do_log(unsigned log_level, const std::string & log_str, const char * source_file, int source_line)
	{
		m_logger->log(log4cplus::log_level(log_level), log_str, source_file, source_line);
	}
}
