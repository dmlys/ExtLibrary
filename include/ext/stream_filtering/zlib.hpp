#pragma once
#include <ext/stream_filtering/filter_types.hpp>

#ifdef EXT_ENABLE_CPPZLIB
#include <ext/cppzlib.hpp>

namespace ext::stream_filtering
{
	class zlib_inflate_filter : public filter
	{
	private:
		zlib::inflate_stream m_inflator;
		
	public:
		virtual auto process(const char * input, std::size_t inputsz, char * data, std::size_t outputsz, bool eos)
			-> std::tuple<std::size_t, std::size_t, bool> override;
		
		virtual void reset() override;
		virtual std::string_view name() const override { return "zlib_inflate_filter"; }
		
	public:
		zlib_inflate_filter() : zlib_inflate_filter(MAX_WBITS + 32) {}
		zlib_inflate_filter(zlib::inflate_stream inflator) : m_inflator(std::move(inflator)) {}
	};
	
	class zlib_deflate_filter : public filter
	{
	private:
		zlib::deflate_stream m_deflator;
		
	public:
		virtual auto process(const char * input, std::size_t inputsz, char * data, std::size_t outputsz, bool eos)
			-> std::tuple<std::size_t, std::size_t, bool> override;
		
		virtual void reset() override;
		virtual std::string_view name() const override { return "zlib_deflate_filter"; }
		
	public:
		zlib_deflate_filter() : zlib_deflate_filter(zlib::deflate_stream(Z_DEFAULT_COMPRESSION, MAX_WBITS + 16)) {} // gzip
		zlib_deflate_filter(zlib::deflate_stream deflator) : m_deflator(std::move(deflator)) {}
	};
}


#endif
