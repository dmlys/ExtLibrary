#pragma once
#include <ext/base64.hpp>
#include <ext/base16.hpp>
#include <ext/stream_filtering/filter_types.hpp>
#include <ext/stream_filtering/transform_width_filter.hpp>

namespace ext::stream_filtering
{
	struct base64_encode_processor
	{
		char * operator()(const char * first, const char * last, char * output) const
		{
			return ext::encode_base64(first, last, output);
		}
	};
	
	struct base64_decode_processor
	{
		char * operator()(const char * first, const char * last, char * output) const
		{
			return ext::decode_base64(first, last, output);
		}
	};
	
	struct base16_encode_processor
	{
		char * operator()(const char * first, const char * last, char * output) const
		{
			return ext::encode_base16(first, last, output);
		}
	};
	
	struct base16_decode_processor
	{
		char * operator()(const char * first, const char * last, char * output) const
		{
			return ext::decode_base16(first, last, output);
		}
	};
	
	
	using base64_encode_filter = transform_width_filter<base64_encode_processor, 3, 4>;
	using base64_decode_filter = transform_width_filter<base64_decode_processor, 4, 3>;
	
	using base16_encode_filter = transform_width_filter<base16_encode_processor, 1, 2>;
	using base16_decode_filter = transform_width_filter<base16_decode_processor, 2, 1>;
}
