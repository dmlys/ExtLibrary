#pragma once
#include <ciso646>
#include <string>
#include <algorithm>
#include <ext/range.hpp>
#include <ext/config.hpp>
#include <ext/type_traits.hpp>
#include <ext/iostreams/utility.hpp>


namespace ext
{
	namespace base64
	{
		namespace encoding_tables
		{
			static_assert(CHAR_BIT == 8, "byte is not octet");

			/// table used for hex encoding: 0 - '0' ... 10 - 'A' ... 15 - 'F'
			extern const char base64_encoding_array[64];
			extern const char base64_decoding_array[256];
			constexpr char padding = '=';
		}

		constexpr unsigned OutputGroupSize = 4;
		constexpr unsigned InputGroupSize = 3;

		/// base exception for all base64
		class base64_exception : public std::runtime_error
		{
		public:
			base64_exception(const char * msg) : std::runtime_error(msg) {}
		};

		/// input has non valid hex char
		class non_base64_char : public base64_exception
		{
		public:
			non_base64_char() : base64_exception("ext::base64::decode: bad char in base64 group") {}
		};

		/// input has not enough input
		class not_enough_input : public base64_exception
		{
		public:
			not_enough_input() : base64_exception("ext::base64::decode: bad base64 group") {}
		};

		inline char decode_char(char ch)
		{
			ch = encoding_tables::base64_decoding_array[static_cast<unsigned char>(ch)];
			if (ch >= 0) return ch;

			throw non_base64_char();
		}

		constexpr std::size_t encode_estimation(std::size_t size)
		{
			return (size + InputGroupSize - 1) / InputGroupSize * OutputGroupSize;
		}

		constexpr std::size_t decode_estimation(std::size_t size)
		{
			return (size + OutputGroupSize - 1) / OutputGroupSize * InputGroupSize;
		}

		template <class Iterator>
		Iterator rskip_padding(Iterator first, Iterator last)
		{
			// skip = symbols
			while (last != first)
			{
				if (*--last == encoding_tables::padding)
					continue;

				++last; break;
			}

			return last;
		}
	}

	template <class RandomAccessIterator, class OutputIterator>
	std::enable_if_t<ext::is_iterator_v<OutputIterator>, OutputIterator>
	encode_base64(RandomAccessIterator first, RandomAccessIterator last, OutputIterator out)
	{
		using namespace base64;
		using namespace encoding_tables;
		constexpr unsigned mask = 0x3Fu;
		std::size_t groups = (last - first) / InputGroupSize;
		std::uint32_t val;

		auto blast = first + InputGroupSize * groups;
		for (; first != blast; first += InputGroupSize)
		{
			// /    << 16      /      << 8     /               /
			// +---------------+---------------+---------------+
			// |7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|
			// +---------------+---------------+---------------+
			// /    >> 18  /   >> 12   /    >> 6   /           /

			val = static_cast<std::uint32_t>(static_cast<unsigned char>(first[0])) << 16u
				| static_cast<std::uint32_t>(static_cast<unsigned char>(first[1])) << 8u
			    | static_cast<std::uint32_t>(static_cast<unsigned char>(first[2]));

			*out++ = base64_encoding_array[ static_cast<unsigned char>(val >> 18u       ) ];
			*out++ = base64_encoding_array[ static_cast<unsigned char>(val >> 12u & mask) ];
			*out++ = base64_encoding_array[ static_cast<unsigned char>(val >> 6u  & mask) ];
			*out++ = base64_encoding_array[ static_cast<unsigned char>(val        & mask) ];
		}

		if (first != last)
		{
			switch (last - first)
			{
				case 1:
					// /    lowest     /  pseudo byte  /
					// +---------------+---------------+
					// |7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|
					// +---------------+---------------+
					// /    >> 2   /<<4        /

					val = static_cast<unsigned char>(first[0]);
					*out++ = base64_encoding_array[ static_cast<unsigned char>(val >> 2u)        ];
					*out++ = base64_encoding_array[ static_cast<unsigned char>(val << 4u & mask) ];
					*out++ = padding;
					*out++ = padding;
					break;

				case 2:
					// /     << 8      /    lowest     /     pseudo    /
					// +---------------+---------------+---------------+
					// |7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|
					// +---------------+---------------+---------------+
					// /    >> 10  /    >> 4   /    << 2   /           /
					val = static_cast<std::uint32_t>(static_cast<unsigned char>(first[0])) << 8
						| static_cast<std::uint32_t>(static_cast<unsigned char>(first[1]));

					*out++ = base64_encoding_array[ static_cast<unsigned char>(val >> 10u       ) ];
					*out++ = base64_encoding_array[ static_cast<unsigned char>(val >> 4u  & mask) ];
					*out++ = base64_encoding_array[ static_cast<unsigned char>(val << 2u  & mask) ];
					*out++ = padding;
					break;

				default: EXT_UNREACHABLE();
			}
		}

		return out;
	}

	/// encodes text from range into container out, the last group is padded with '='
	template <class RandomAccessIterator, class OutputContainer>
	std::enable_if_t<ext::is_container_v<OutputContainer>/*, OutputContainer &*/>
	encode_base64(RandomAccessIterator first, RandomAccessIterator last, OutputContainer & out)
	{
		auto out_size = base64::encode_estimation(last - first);
		auto old_size = out.size();
		out.resize(old_size + out_size);

		auto out_beg = boost::begin(out) + old_size;
		auto out_end = boost::end(out);
		out_end = encode_base64(first, last, out_beg);
	}

	/// encodes text from range into container out, the last group is padded with '='
	template <class InputRange, class OutputContainer>
	inline std::enable_if_t<ext::is_container_v<OutputContainer>/*, OutputContainer &*/>
	encode_base64(const InputRange & input, OutputContainer & out)
	{
		auto inplit = ext::str_view(input);
		return encode_base64(boost::begin(inplit), boost::end(inplit), out);
	}

	/// encodes text from range into container out, the last group is padded with '='
	template <class OutputContainer = std::string, class InputRange>
	OutputContainer encode_base64(const InputRange & input)
	{
		OutputContainer out;
		ext::encode_base64(input, out);
		return out;
	}


	template <class RandomAccessIterator, class Sink>
	std::enable_if_t<ext::iostreams::is_device_v<Sink>, Sink &>
	encode_base64(RandomAccessIterator first, RandomAccessIterator last, Sink & sink)
	{
		// encoding produces more than input
		using namespace base64;
		constexpr std::size_t buffer_size = 256;
		constexpr std::size_t step_size = base64::decode_estimation(buffer_size);
		char buffer[buffer_size];

		while (first < last)
		{
			auto step_last = first + std::min<std::ptrdiff_t>(step_size, last - first);
			auto buf_end = ext::encode_base64(first, step_last, buffer);
			ext::iostreams::write_all(sink, buffer, buf_end - buffer);
			first = step_last;
		}

		return sink;
	}

	template <class InputRange, class Sink>
	inline std::enable_if_t<ext::iostreams::is_device_v<Sink>, Sink &>
	encode_base64(const InputRange & input, Sink & out)
	{
		auto inplit = ext::str_view(input);
		return ext::encode_base64(boost::begin(inplit), boost::end(inplit), out);
	}



	template <class RandomAccessIterator, class OutputIterator>
	std::enable_if_t<ext::is_iterator_v<OutputIterator>, OutputIterator>
	decode_base64(RandomAccessIterator first, RandomAccessIterator last, OutputIterator out)
	{
		using namespace base64;
		using namespace encoding_tables;
		constexpr unsigned mask = 0xFFu;

		last = rskip_padding(first, last);
		std::size_t groups = (last - first) / OutputGroupSize;
		auto blast = first + groups * OutputGroupSize;
		std::uint32_t val;

		for (; first != blast; first += OutputGroupSize)
		{
			// /    << 18  /   << 12   /    << 6   /           /
			// +---------------+---------------+---------------+
			// |7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|
			// +---------------+---------------+---------------+
			// /    >> 16      /      >> 8     /               /

			val = static_cast<std::uint32_t>(static_cast<unsigned char>(decode_char(first[0]))) << 18u
				| static_cast<std::uint32_t>(static_cast<unsigned char>(decode_char(first[1]))) << 12u
			    | static_cast<std::uint32_t>(static_cast<unsigned char>(decode_char(first[2]))) << 6u
				| static_cast<std::uint32_t>(static_cast<unsigned char>(decode_char(first[3])));

			*out++ = static_cast<unsigned char>(val >> 16u & mask);
			*out++ = static_cast<unsigned char>(val >> 8u  & mask);
			*out++ = static_cast<unsigned char>(val        & mask);
		}

		if (first != last)
		{
			switch (last - first)
			{
				case 1:
					// theoretically this is normal, and we should process it, but in practice -
					// there is no way base64 encoder would produce not a full quadruplet with 1 character
					throw not_enough_input();

				case 2:
					//          /   << 6   /           /
					// +---------------+---------------+
					// |7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|
					// +---------------+---------------+
					//          /     >> 4     /

					val = static_cast<std::uint32_t>(static_cast<unsigned char>(decode_char(first[0]))) << 6u
						| static_cast<std::uint32_t>(static_cast<unsigned char>(decode_char(first[1])));

					*out++ = static_cast<unsigned char>(val >> 4u & mask);
					break;

				case 3:
					//             /   << 12   /    << 6   /           /
					// +---------------+---------------+---------------+
					// |7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|
					// +---------------+---------------+---------------+
					//             /    >> 10      /      >> 2     /

					val = static_cast<std::uint32_t>(static_cast<unsigned char>(decode_char(first[0]))) << 12u
						| static_cast<std::uint32_t>(static_cast<unsigned char>(decode_char(first[1]))) << 6u
						| static_cast<std::uint32_t>(static_cast<unsigned char>(decode_char(first[2])));

					*out++ = static_cast<unsigned char>(val >> 10u & mask);
					*out++ = static_cast<unsigned char>(val >>  2u & mask);

					break;

				default: EXT_UNREACHABLE();
			}
		}

		return out;
	}

	template <class RandomAccessIterator, class OutputContainer>
	std::enable_if_t<ext::is_container_v<OutputContainer>/*, OutputContainer &*/>
	decode_base64(RandomAccessIterator first, RandomAccessIterator last, OutputContainer & out)
	{
		// we are appending
		auto out_size = base64::decode_estimation(last - first);
		auto old_size = out.size();
		out.resize(old_size + out_size);

		auto out_beg  = boost::begin(out) + old_size;
		auto out_last = boost::end(out);
		out_last = decode_base64(first, last, out_beg);
		out.resize(out_last - out_beg);
	}

	template <class InputRange, class OutputContainer>
	inline std::enable_if_t<ext::is_container_v<OutputContainer>/*, OutputContainer &*/>
	decode_base64(const InputRange & input, OutputContainer & out)
	{
		auto inplit = ext::str_view(input);
		return decode_base64(boost::begin(inplit), boost::end(inplit), out);
	}

	template <class OutputContainer = std::string, class InputRange>
	OutputContainer decode_base64(const InputRange & input)
	{
		OutputContainer out;
		ext::decode_base64(input, out);
		return out;
	}

	template <class RandomAccessIterator, class Sink>
	std::enable_if_t<ext::iostreams::is_device_v<Sink>, Sink &>
	decode_base64(RandomAccessIterator first, RandomAccessIterator last, Sink & sink)
	{
		// decoding never produces more than input
		using namespace base64;
		constexpr std::size_t buffer_size = 256;
		constexpr std::size_t step_size = buffer_size;
		char buffer[buffer_size];

		while (first < last)
		{
			auto step_last = first + std::min<std::ptrdiff_t>(step_size, last - first);
			auto buf_end = ext::decode_base64(first, step_last, buffer);
			ext::iostreams::write_all(sink, buffer, buf_end - buffer);
			first = step_last;
		}

		return sink;
	}

	template <class InputRange, class Sink>
	inline std::enable_if_t<ext::iostreams::is_device_v<Sink>, Sink &>
	decode_base64(const InputRange & input, Sink & out)
	{
		auto inplit = ext::str_view(input);
		return ext::decode_base64(boost::begin(inplit), boost::end(inplit), out);
	}
}
