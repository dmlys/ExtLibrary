#pragma once
#ifdef EXT_ENABLE_CPPZLIB
#include <zlib.h>
#include <cassert>
#include <memory>
#include <utility>
#include <stdexcept>
#include <system_error>

#include <boost/config.hpp> // for BOOST_NORETURN

/// very simple z_stream wrapper
/// provides:
///	 * RAII
///	 * some conventional methods/constructors
///  * return value checking - throw exception
namespace zlib
{
	/************************************************************************/
	/*                       Common                                         */
	/************************************************************************/
	typedef ::z_stream   zstream_type;
	typedef ::z_stream * zstream_handle;

	/// error category for zlib
	const std::error_category & zlib_category() noexcept;
	inline std::error_code make_zlib_error(int code) noexcept { return {code, zlib_category()}; }

	/// checks that code error is in:
	///  Z_OK, Z_STREAM_END, Z_NEED_DICT, Z_BUF_ERROR
	/// if not - throws zlib_error
	void check_error(int code, zstream_handle stream);

	/// zlib excpetion, std::system_error compatible,
	/// but inherited from runtime_error
	/// formats message zlib:<num>, <descr>; msg - <msg>
	/// example:
	/// zlib: 0, ok
	/// zlib: -2, stream error; msg - <msg>
	class zlib_error : public std::runtime_error
	{
		std::string errmsg;
		std::error_code ec;

	public:
		const std::error_code & code() const noexcept { return ec; }
		const char * what() const noexcept override { return errmsg.c_str(); }
		
		zlib_error(int code, const char * msg);
	};

	BOOST_NORETURN inline void throw_zlib_error(int code, const char * msg) { throw zlib_error(code, msg); }
	BOOST_NORETURN inline void throw_zlib_error(int code, zstream_handle handle) { throw_zlib_error(code, handle->msg); }

	/************************************************************************/
	/*                   Buffer setters                                     */
	/************************************************************************/
	inline void set_in(zstream_handle stream, const char * in_first, const char * in_last)
	{
		//stream->next_in = reinterpret_cast<const unsigned char *>(in_first);
		stream->next_in = (unsigned char *)in_first;
		stream->avail_in = uInt(in_last - in_first);
	}

	inline void set_in(zstream_handle stream, const char * in_first, std::size_t in_size)
	{
		//stream->next_in = reinterpret_cast<const unsigned char *>(in_first);
		stream->next_in = (unsigned char *)in_first;
		stream->avail_in = uInt(in_size);
	}

	inline void set_out(zstream_handle stream, char * out_first, char * out_last)
	{
		stream->next_out = reinterpret_cast<unsigned char *>(out_first);
		stream->avail_out = uInt(out_last - out_first);
	}

	inline void set_out(zstream_handle stream, char * out_first, std::size_t out_size)
	{
		stream->next_out = reinterpret_cast<unsigned char *>(out_first);
		stream->avail_out = uInt(out_size);
	}

	inline void set_buffers(zstream_handle stream,
	                        const char * in_first, const char * in_last,
	                        char * out_first, char * out_last)
	{
		set_in(stream, in_first, in_last);
		set_out(stream, out_first, out_last);
	}

	inline void set_buffers(zstream_handle stream,
	                        const char * in_first, std::size_t in_size,
	                        char * out_first, std::size_t out_size)
	{
		set_in(stream, in_first, in_size);
		set_out(stream, out_first, out_size);
	}

	/************************************************************************/
	/*                        Common                                        */
	/************************************************************************/
	/// common handle, lifetime, access to members
	/// @Closer  closes z_stream, must call deflateEnd/inflateEnd and return result.
	///          zstream itself will call delete
	template <class Closer>
	class zstream
	{
	protected:
		typedef Closer closer_type;
		struct deleter_type : closer_type
		{
			void operator()(zstream_handle handle)
			{
				closer_type::operator()(handle);
				delete handle;
			}
		};

		std::unique_ptr<zstream_type, deleter_type> handle;

	protected:
		zstream() = default;
		~zstream() = default;
		zstream(zstream_handle handle) noexcept : handle(handle) {}

		zstream(const zstream &) = delete;
		zstream & operator =(const zstream & ) = delete;

		zstream(zstream &&) = default;
		zstream & operator =(zstream &&) = default;
		
	public:
		friend void swap(zstream & s1, zstream & s2) noexcept { std::swap(s1.handle, s2.handle); }
		
	public:
		operator zstream_handle() noexcept { return native(); }
		zstream_handle native() const noexcept { return handle.get(); }
		zstream_handle release() noexcept { return handle.release(); }
		void assign(zstream_handle handle) noexcept { zstream::handle = handle; }
		
		int close() noexcept
		{ return handle ? handle.get_deleter().closer_type::operator()(handle.get()) : Z_OK; }
		// same as close
		int end() { return close(); }

		void set_in(const char * first, const char * last)  { zlib::set_in(native(), first, last); }
		void set_in(const char * first, std::size_t size)   { zlib::set_in(native(), first, size); }
		void set_out(char * first, const char * last) { zlib::set_out(native(), first, last); }
		void set_out(char * first, std::size_t size)  { zlib::set_out(native(), first, size); }

		void set_buffers(const char * in_first, std::size_t in_size, char * out_first, std::size_t out_size)
		{ zlib::set_buffers(native(), in_first, in_size, out_first, out_size); }

		void set_buffers(const char * in_first, const char * in_last, char * out_first, char * out_last)
		{ zlib::set_buffers(native(), in_first, in_last, out_first, out_last); }
	
		// access helpers, use those, or use native()
		z_const Bytef * & next_in()  { return handle->next_in; }
		uInt  & avail_in()           { return handle->avail_in; }
		uLong & total_in()           { return handle->total_in; }
		
		z_const Bytef * next_in() const { return handle->next_in; }
		uInt avail_in() const           { return handle->avail_in; }
		uLong total_in() const          { return handle->total_in; }

		z_const Bytef * & next_out()  { return handle->next_out; }
		uInt  & avail_out()           { return handle->avail_out; }
		uLong & total_out()           { return handle->total_out; }

		z_const Bytef * next_out() const { return handle->next_out; }
		uInt avail_out() const           { return handle->avail_out; }
		uLong total_out() const          { return handle->total_out; }

		int data_type() const { return handle->data_type; }
		uLong adler() const   { return handle->adler; }
	};


	/************************************************************************/
	/*                      Inflate                                         */
	/************************************************************************/
	struct inflate_deleter
	{
		int operator()(zstream_handle handle) const noexcept
		{
			return ::inflateEnd(handle);
		}
	};

	/// inflate zstream, decompresses input data stream
	/// remember windowBits configures deflate formats
	///    8 ..  MAX_WBITS       - zlib format
	///   -8 .. -MAX_WBITS       - raw deflate format
	///    8 ..  MAX_WBITS + 16  - gzip format
	///    8 ..  MAX_WBITS + 32  - auto detect both gzip and zlib
	/// prefer using MAX_WBITS
	/// windowBits can be set by constructor, init, reset methods
	class inflate_stream : public zstream<inflate_deleter>
	{
		typedef zstream<inflate_deleter> base_type;
		
	public: // forward constructors
		inflate_stream(zstream_handle handle) noexcept : base_type(handle) {}
		inflate_stream(inflate_stream && op) noexcept : base_type(std::move(op)) {}

		inflate_stream & operator =(inflate_stream && op) noexcept
		{
			handle = std::exchange(handle, nullptr);
			return *this;
		}
		
		// z_stream allows copying, but it is expensive, disable it, and make it explicit via method copy
		inflate_stream(const inflate_stream & op) = delete;
		inflate_stream & operator = (const inflate_stream & op) = delete;

		int copy(inflate_stream & dest) const
		{
			dest.close();
			int res = inflateCopy(dest.native(), native());
			check_error(res, native());
			return res;
		}

	public: // init constructors
		inflate_stream() : base_type(new zstream_type)
		{ init(nullptr, 0); }

		inflate_stream(const char * input, std::size_t size) : base_type(new zstream_type)
		{ init(input, size); }

		inflate_stream(int windowBits) : base_type(new zstream_type)
		{ init(windowBits); }

		inflate_stream(int windowBits, const char * input, std::size_t size) : base_type(new zstream_type)
		{ init(windowBits, input, size); }

	public: // inits
		int init()               { return init(nullptr, 0); }
		int init(int windowBits) { return init(windowBits, nullptr, 0); }

		int init(int windowBits, const char * input, std::size_t size)
		{
			handle->zalloc = Z_NULL;
			handle->zfree = Z_NULL;
			handle->opaque = Z_NULL;
			set_in(input, size);

			int res = inflateInit2(native(), windowBits);
			check_error(res, native());
			return res;
		}

		int init(const char * input, std::size_t size)
		{
			handle->zalloc = Z_NULL;
			handle->zfree = Z_NULL;
			handle->opaque = Z_NULL;
			set_in(input, size);

			int res = inflateInit(native());
			check_error(res, native());
			return res;
		}

	public: // methods
		int (inflate)(int flush)
		{
			int res = ::inflate(native(), flush);
			check_error(res, native());
			return res;
		}

		void reset()
		{
			int res = ::inflateReset(native());
			check_error(res, native());
		}

		void reset(int windowBits)
		{
			int res = ::inflateReset2(native(), windowBits);
			check_error(res, native());
		}
	};


	/************************************************************************/
	/*                     Deflate                                          */
	/************************************************************************/
	struct deflate_deleter
	{
		int operator()(zstream_handle handle) const noexcept
		{
			return ::deflateEnd(handle);
		}
	};

	/// deflate zstream, compresses input data stream
	/// remember windowBits configures deflate formats
	///    8 ..  MAX_WBITS      - zlib format
	///   -8 .. -MAX_WBITS      - raw deflate format
	///    8 ..  MAX_WBITS + 16 - gzip format
	class deflate_stream : public zstream<deflate_deleter>
	{
		typedef zstream<deflate_deleter> base_type;
		
	public: // forward constructors
		deflate_stream(zstream_handle handle) noexcept : base_type(handle) {}
		deflate_stream(deflate_stream && op) noexcept : base_type(std::move(op)) {}

		deflate_stream & operator =(deflate_stream && op) noexcept
		{
			handle = std::exchange(handle, nullptr);
			return *this;
		}

		// z_stream allows copying, but it is expensive, disable it, and make it explicit via method copy
		deflate_stream(const deflate_stream & op) = delete;
		deflate_stream & operator = (const deflate_stream & op) = delete;

		int copy(deflate_stream & dest) const
		{
			dest.close();
			int res = deflateCopy(dest.native(), native());
			check_error(res, native());
			return res;
		}

	public: // init constructors
		deflate_stream(int compressionLevel = Z_DEFAULT_COMPRESSION) : base_type(new zstream_type)
		{ init(compressionLevel); }

		deflate_stream(int compressionLevel, int windowBits, int memoryLevel = 8, int strategy = Z_DEFAULT_STRATEGY)
			: base_type(new zstream_type)
		{ init(compressionLevel, windowBits, memoryLevel, strategy); }

	public: // inits
		int init(int compressionLevel = Z_DEFAULT_COMPRESSION)
		{
			handle->zalloc = Z_NULL;
			handle->zfree = Z_NULL;
			handle->opaque = Z_NULL;

			int res = deflateInit(native(), compressionLevel);
			check_error(res, native());
			return res;
		}

		/// method is Z_DEFLATED
		/// windowBits can be:
		///   8..MAX_WBITS  - zlib format
		///   -8..-MAX_WBITS - raw deflate format
		///   -8..-MAX_WBITS + 16 - gzip format
		/// memLevel 1..9, default is 8
		int init(int compressionLevel, int windowBits, int memoryLevel = 8, int strategy = Z_DEFAULT_STRATEGY)
		{
			handle->zalloc = Z_NULL;
			handle->zfree = Z_NULL;
			handle->opaque = Z_NULL;

			int res = deflateInit2(native(), compressionLevel, Z_DEFLATED, windowBits, memoryLevel, strategy);
			check_error(res, native());
			return res;
		}

	public: // methods
		int (deflate)(int flush)
		{
			int res = ::deflate(native(), flush);
			check_error(res, native());
			return res;
		}

		uLong bound(uLong source_len) noexcept
		{
			return ::deflateBound(native(), source_len);
		}

		template <class Arithmetic>
		uLong bound(Arithmetic source_len) noexcept
		{
			return bound(static_cast<uLong>(source_len));
		}

		void reset()
		{
			int res = ::deflateReset(native());
			check_error(res, native());
		}
	};
} // namespace zlib

#endif // EXT_ENABLE_CPPZLIB
