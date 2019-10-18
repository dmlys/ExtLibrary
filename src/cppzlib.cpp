#ifdef EXT_ENABLE_CPPZLIB
#include <ext/cppzlib.hpp>
#include <ext/Errors.hpp>
#include <cstring> // for std::strlen

namespace zlib
{
	/************************************************************************/
	/*                      error_category                                  */
	/************************************************************************/
	struct zlib_category_impl : std::error_category
	{
		virtual const char * name() const noexcept override;
		virtual std::string message(int val) const override;
	};

	const char * zlib_category_impl::name() const noexcept
	{
		return "zlib";
	}

	std::string zlib_category_impl::message(int val) const
	{
		switch (val)
		{
			case Z_OK:            return "ok";
			case Z_STREAM_END:    return "zstream end";
			case Z_NEED_DICT:     return "dictionary needed";
			case Z_BUF_ERROR:     return "buffer error";

			case Z_ERRNO:         return "errno error";
			case Z_STREAM_ERROR:  return "stream error";
			case Z_DATA_ERROR:    return "data error";
			case Z_MEM_ERROR:     return "memory error";
			case Z_VERSION_ERROR: return "version error";
			default:              return "unknown error";
		}
	}

	zlib_category_impl zlib_category_instance;

	const std::error_category & zlib_category() noexcept
	{
		return zlib_category_instance;
	}

	/************************************************************************/
	/*                     others                                           */
	/************************************************************************/
	zlib_error::zlib_error(int code, const char * msg)
		: std::runtime_error("")
	{
		ec = make_zlib_error(code);
		errmsg = ext::FormatError(ec);
		size_t len;
		if (msg && (len = std::strlen(msg)) > 0)
		{
			errmsg += "; msg - ";
			errmsg += msg;
		}
	}

	void check_error(int code, zstream_handle stream)
	{
		switch (code)
		{
			case Z_OK:
			case Z_STREAM_END:
			case Z_NEED_DICT:
			case Z_BUF_ERROR:
				return;

			case Z_ERRNO:
			case Z_STREAM_ERROR:
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
			case Z_VERSION_ERROR:
			default:
				throw_zlib_error(code, stream);
		}
	}

} // namespace zlib

#endif // EXT_ENABLE_CPPZLIB
