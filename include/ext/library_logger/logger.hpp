#pragma once
#include <cassert>
#include <memory>
#include <utility>
#include <string>
#include <string_view>
#include <iostream>
#include <sstream>
#include <ext/strings/aci_string.hpp>

namespace ext::library_logger
{
	/// известные уровни логирования
	constexpr unsigned Fatal = 0;
	constexpr unsigned Error = 1;
	constexpr unsigned Warn  = 2;
	constexpr unsigned Info  = 3;
	constexpr unsigned Debug = 4;
	constexpr unsigned Trace = 5;

	/// explicitly disabled, not matter what configuration states
	constexpr unsigned Disabled = -1; // MAX_UINT

	/// returns string representation of log_level
	std::string_view log_level_string(unsigned log_level);
	/// parses word as log_level, if parsing fails - returns Disabled
	unsigned parse_log_level(std::string_view word);

	/// rationale:
	/// некоторым библиотекам требуется возможность логирования.
	///   * они могут использовать стороннюю библиотеку,
	///     это решение имеет недостаток предоставление зависимости от этой библиотеки и ее методов конфигурации.
	///
	///   * использовать абстрактный интерфейс, который должен реализовать пользователь библиотеки.
	///     в самом простом случае имеющий метод
	///     log(int log_lvl, const std::string & msg, int souce_line, const char * soure_file).
	///
	///     данное решение имеет недостаток в том что, _каждая_ библиотека должна реализовать
	///     несколько вспомогательных функций/макросов для адекватного использования этого интерфейса
	///     а так же то, что данное решение не очень эффективно, поскольку, наверняка,
	///     на каждую запись будет создаваться stringstream,
	///     даже если, в конечном итоге мы всего лишь напрямую пишем в std::clog/std::ofstream
	///     или используется библиотеки предоставляющая свой ostream, например boost.log
	///
	/// library_logger::logger предоставляет интерфейс и вспомогательные макросы позволяющие
	/// эффективно и безопасно логировать как обычную строку, так и в абстрактный ostream с поддержкой log_level, source_file, source_line.
	///     реализации logger могут быть как thread-safe, так и нет
	///
	///
	/// базовый интерфейс логирования, для использования клиентами - библиотеками,
	/// реализуется пользователями библиотек.
	///
	/// клиентская часть:
	///   смотри так же макросы в ext/library_logger/logging_macros.hpp
	///   шаблон использования:
	///     if (logger->is_enabled_for(log_level)
	///         logger->log(log_level, str, __FILE__, __LINE__);
	///
	///   или через ostream:
	///     record rec = logger->open_record(log_level, __FILE__, __LINE__);
	///     if (rec)
	///     {
	///         rec.get_ostream() << ....;
	///         rec.push();
	///     }
	///
	///   record - класс записи позволяет получить поток в который можно писать.
	///            запись можно протолкнуть (push) или отбросить(discard), после чего запись становится недействительной
	///            при разрушении запись отбрасывается, если не протолкнута
	///
	///   проталкивание записи вызывается при нормальном ходе логирования
	///   отбрасывание подразумевает что в ходе формирование записи произошла ошибка, например bad_alloc
	///   реализация вольна все равно залогировать такую запись, но она может быть не полной, или даже пустой
	///   open_record может вернуть null запись - это флаг что увроень лоогирования ниже запрошенного
	///
	///   is_enabled_for(log_level) - проверяет что логгер включен для заданного уровня
	///   log(log_level, str, source_file, source_line) - логирукт строку с заданным уровнем
	///
	///   open_record - открывает запись для заданного уровня логирования.
	///   push_record/rec.push() - проталкивает запись
	///   discard_record/rec.discard() - отбрасывает запись
	///
	///
	///
	/// часть реализации:
	///   record_context - контекст записи логирования
	///                    тут хранится указатель на поток логирования,
	///                    классы наследники могут расширить данный контекст путем наследования
	///                    и хранить в нем дополнительные данные нужные им
	///   do_open_record - возвращает указатель на record_context,
	///                    контекст может создаваться каждый раз новый или повторно использоваться
	///                    гарантируется что на каждый вызов do_open_record, вернувший не nullptr
	///                    будет вызов push_record или discard_record(но не оба).
	///                    если уровень логирования ниже запрошенного логгер может вернуть nullptr
	///   do_discard_record - реализация отбрасывания записи, тут может быть освобождение ресурсов
	///                       допустимо все равно залогировать запись - на усмотрение реализации
	///   do_push_record - добавление записи в лог, а так же освобождение ресурсов
	class logger
	{
	protected:
		struct record_context
		{
			std::ostream * os;
		};

	protected:
		virtual bool do_is_enabled_for(unsigned log_level) const = 0;
		virtual void do_log(unsigned log_level, const std::string & str, const char * source_file, int source_line) = 0;

		virtual record_context * do_open_record(unsigned log_level, const char * source_file, int source_line) = 0;
		virtual void do_push_record(record_context * rctx) = 0;
		virtual void do_discard_record(record_context * rctx) noexcept = 0;

	public:
		class record;

		bool is_enabled_for(unsigned log_level) const;
		void log(unsigned log_level, const std::string & str, const char * source_file, int source_line);

		record open_record(unsigned log_level, const char * source_file = nullptr, int source_line = -1);
		void push_record(record & rec);
		void discard_record(record & rec) noexcept;

	public:
		virtual ~logger() = default;
	};

	/************************************************************************/
	/*   RAII safe record class.                                            */
	/*   представляет запись в которую можно добавлять данные логирования   */
	/************************************************************************/
	class logger::record
	{
		friend logger;
		
		record_context * rctx = nullptr;
		logger * owner = nullptr;

	public:
		/// проверяет валидность записи(null запись/активная)
		explicit operator bool() const noexcept { return rctx != nullptr; }
		std::ostream & get_ostream();
		/// аналогично logger->push_record(*this)
		void push() { owner->push_record(*this); }
		/// аналогично logger->discard_record(*this)
		void discard() noexcept { owner->discard_record(*this);  }

	private:
		record(record_context * rctx, logger * owner) : rctx(rctx), owner(owner) {}

	public:
		record() noexcept = default;
		~record() noexcept { if (owner) owner->discard_record(*this); }

		record(record && rec) noexcept : rctx(rec.rctx), owner(rec.owner)  { rec.rctx = nullptr; rec.owner = nullptr; }
		record & operator =(record && rec) noexcept                        { rctx = std::exchange(rec.rctx, nullptr); owner = std::exchange(rec.owner, nullptr); return *this; }

		//noncopyable
		record(const record & rec) = delete;
		record & operator =(const record & rec) = delete;
	};

	inline std::ostream & logger::record::get_ostream()
	{
		assert(rctx);
		return *rctx->os;
	}

	/************************************************************************/
	/*                  Logger methods                                      */
	/************************************************************************/
	inline bool logger::is_enabled_for(unsigned log_level) const
	{
		return do_is_enabled_for(log_level);
	}

	inline void logger::log(unsigned log_level, const std::string & str, const char * source_file, int source_line)
	{
		do_log(log_level, str, source_file, source_line);
	}

	inline logger::record logger::open_record(unsigned log_level, const char * source_file, int source_line)
	{
		auto ctx = do_open_record(log_level, source_file, source_line);
		return {ctx, this};
	}

	inline void logger::push_record(record & rec)
	{
		auto rctx = std::exchange(rec.rctx, nullptr);
		if (rctx) do_push_record(rctx);
	}

	inline void logger::discard_record(record & rec) noexcept
	{
		auto rctx = std::exchange(rec.rctx, nullptr);
		if (rctx) do_discard_record(rec.rctx);
	}

	/************************************************************************/
	/*                   convenience helpers                                */
	/************************************************************************/
	inline logger::record open_record(logger * lg, unsigned log_level, const char * source_file, int source_line)
	{
		return lg ? lg->open_record(log_level, source_file, source_line) : logger::record();
	}

	inline logger::record open_record(logger & lg, unsigned log_level, const char * source_file, int source_line)
	{
		return lg.open_record(log_level, source_file, source_line);
	}

	/************************************************************************/
	/*                        log_level stuff                               */
	/************************************************************************/
	inline std::string_view log_level_string(unsigned log_level)
	{
		switch (log_level)
		{
		    case Trace:    return "Trace";
		    case Debug:    return "Debug";
		    case Info:     return "Info";
		    case Warn:     return "Warn";
		    case Error:    return "Error";
		    case Fatal:    return "Fatal";

		    case Disabled: return "Disabled";
		    default:       return "Unknown";
		}
	}

	inline unsigned parse_log_level(std::string_view word)
	{
		ext::aci_string_view aci_word(word.data(), word.length());

		if      (aci_word == "fatal") return Fatal;
		else if (aci_word == "error") return Error;
		else if (aci_word == "warn" ) return Warn;
		else if (aci_word == "info" ) return Info;
		else if (aci_word == "debug") return Debug;
		else if (aci_word == "trace") return Trace;
		else                          return Disabled;
	}


	/************************************************************************/
	/*                some hepler/basic implementations                     */
	/************************************************************************/
	/// simple logger for some iostream:
	///   cout, cerr, clog or any other arbitrary ostream.
	/// Thread safety - same as argument ostream
	class stream_logger : public ext::library_logger::logger
	{
	protected:
		record_context ctx;
		unsigned lvl = Info;

	protected:
		bool do_is_enabled_for(unsigned log_level) const override { return log_level <= lvl; }
		void do_log(unsigned log_level, const std::string & str, const char * source_file, int source_line) override;

		record_context * do_open_record(unsigned log_level, const char * source_file, int source_line) override;
		void do_push_record(record_context * rctx) override { *rctx->os << std::endl; }
		void do_discard_record(record_context * rctx) noexcept override {}

	public:
		void flush() { ctx.os->flush(); }
		auto stream() const noexcept { return ctx.os; }
		auto log_level() const noexcept { return lvl; }

	public:
		stream_logger(std::ostream & os = std::clog, int lvl = Info)
		    : lvl(lvl) { ctx.os = &os; }		
		~stream_logger() noexcept;

		stream_logger(const stream_logger & lg) = delete;
		stream_logger & operator =(const stream_logger &) = delete;
	};

	inline auto stream_logger::do_open_record(unsigned log_level, const char * source_file, int source_line) -> record_context *
	{
		return log_level <= lvl ? &ctx : nullptr;
	}

	inline void stream_logger::do_log(unsigned log_level, const std::string & str, const char * source_file, int source_line)
	{
		*ctx.os << str << std::endl;
	}

	inline stream_logger::~stream_logger() noexcept
	{
		try
		{
			ctx.os->flush();
		}
		catch (...)
		{
			// nothing
		}
	}

	/// simple_logger/sequenced_simple_logger
	/// упрощение интерфейса к более простому
	/// is_enabled_for(log_level), log(log_level, logStr)
	/// например таким интерфейсом обладает log4cplus
	///
	/// наследник должен реализовать is_enabled_for/log методы
	class simple_logger : public logger
	{
	protected:
		using base_record_context = ext::library_logger::logger::record_context;
		struct record_context : base_record_context
		{
			std::ostringstream ss;
			unsigned log_level;
			unsigned source_line;
			const char * source_file;
		};

	protected:
		std::locale loc;

	protected:
		record_context * do_open_record(unsigned log_level, const char * source_file, int source_line) override;
		void do_push_record(base_record_context * rctx) override;
		void do_discard_record(base_record_context * rctx) noexcept override;

	public:
		simple_logger() = default;
		simple_logger(std::locale loc) : loc(loc) {}

		simple_logger(const simple_logger &) = delete;
		simple_logger & operator =(const simple_logger &) = delete;
	};


	inline simple_logger::record_context * simple_logger::do_open_record(unsigned log_level, const char * source_file, int source_line)
	{
		if (!do_is_enabled_for(log_level))
			return nullptr;

		auto rctx = new record_context;
		rctx->log_level = log_level;
		rctx->source_line = source_line;
		rctx->source_file = source_file;
		rctx->ss.imbue(loc);
		rctx->os = &rctx->ss;
		return rctx;
	}

	inline void simple_logger::do_push_record(base_record_context * rctx_base)
	{
		std::unique_ptr<record_context> rctx(static_cast<record_context *>(rctx_base));
		do_log(rctx->log_level, rctx->ss.str(), rctx->source_file, rctx->source_line);
	}

	inline void simple_logger::do_discard_record(base_record_context * rctx_base) noexcept
	{
		delete static_cast<record_context *>(rctx_base);
	}

	/// упорядоченная sequenced (seq) версия логгера,
	/// вызовы должны быть строго упорядоченны по парам open_record -> соответствующий ему push_record/discard_record
	/// соответственно логгер должен использоваться только из одного потока, или же гарантии вызовов должны обеспечиваться
	/// механизмами синхронизации.
	/// пользователи logger интерфейса могут предоставлять такие гарантии, тогда можно использовать данный интерфейс
	class sequenced_simple_logger : public logger
	{
	protected:
		std::ostringstream ss;
		record_context rctx;
		unsigned log_level;
		unsigned source_line;
		const char * source_file;
	
	private:
		record_context * do_open_record(unsigned log_level, const char * source_file, int source_line) override;
		void do_push_record(record_context * rctx) override;
		void do_discard_record(record_context * rctx) noexcept override;

	public:
		sequenced_simple_logger(const std::locale & loc = std::locale());
	};

	inline sequenced_simple_logger::sequenced_simple_logger(const std::locale & loc)
	{
		ss.imbue(loc);
		rctx.os = &ss;
	}

	inline sequenced_simple_logger::record_context * sequenced_simple_logger::do_open_record(unsigned log_level, const char * source_file, int source_line)
	{
		if (!do_is_enabled_for(log_level))
			return nullptr;

		this->log_level = log_level;
		this->source_file = source_file;
		this->source_line = source_line;
		
		return &rctx;
	}

	inline void sequenced_simple_logger::do_push_record(record_context * rctx)
	{
		do_log(log_level, ss.str(), source_file, source_line);
		do_discard_record(rctx);
	}

	inline void sequenced_simple_logger::do_discard_record(record_context * rctx) noexcept
	{
		ss.str(std::string());
		ss.clear();
	}
}
