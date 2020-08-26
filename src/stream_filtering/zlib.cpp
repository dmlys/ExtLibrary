#include <ext/stream_filtering/zlib.hpp>
#ifdef EXT_ENABLE_CPPZLIB

namespace ext::stream_filtering
{
	auto zlib_inflate_filter::process(const char * input, std::size_t inputsz, char * output, std::size_t outputsz, bool eos)
		-> std::tuple<std::size_t, std::size_t, bool>
	{
		m_inflator.set_in(input, inputsz);
		m_inflator.set_out(output, outputsz);
		
		int res = ::inflate(m_inflator, Z_NO_FLUSH);
		auto * next_out = reinterpret_cast<      char *>(m_inflator.next_out());
		auto * next_in  = reinterpret_cast<const char *>(m_inflator.next_in());
		std::size_t consumed = next_in - input;
		std::size_t written  = next_out - output;
		switch (res)
		{
			case Z_OK:
				return std::make_tuple(consumed, written, /*eof*/ false);
				
			case Z_STREAM_END:
				return std::make_tuple(consumed, written, /*eof*/ true);
	
			case Z_NEED_DICT:
			case Z_BUF_ERROR:
			case Z_ERRNO:
			case Z_STREAM_ERROR:
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
			case Z_VERSION_ERROR:
			default:
				zlib::throw_zlib_error(res, m_inflator);
		}
	}
	
	
	auto zlib_deflate_filter::process(const char * input, std::size_t inputsz, char * output, std::size_t outputsz, bool eos)
		-> std::tuple<std::size_t, std::size_t, bool>
	{
		m_deflator.set_in(input, inputsz);
		m_deflator.set_out(output, outputsz);
		
		int res = ::deflate(m_deflator, eos ? Z_FINISH : Z_NO_FLUSH);
		auto * next_out = reinterpret_cast<      char *>(m_deflator.next_out());
		auto * next_in  = reinterpret_cast<const char *>(m_deflator.next_in());
		std::size_t consumed = next_in - input;
		std::size_t written  = next_out - output;
		switch (res)
		{
			case Z_OK:
				return std::make_tuple(consumed, written, /*eof*/ false);
				
			case Z_STREAM_END:
				return std::make_tuple(consumed, written, /*eof*/ true);
				
			case Z_NEED_DICT:
			case Z_BUF_ERROR:
			case Z_ERRNO:
			case Z_STREAM_ERROR:
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
			case Z_VERSION_ERROR:
			default:
				zlib::throw_zlib_error(res, m_deflator);
		}
	}
}

#endif
