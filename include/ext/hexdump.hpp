#pragma once
#include <ciso646>
#include <cstddef>
#include <stdexcept>

#include <string>
#include <iterator>
#include <vector>
#include <algorithm>

#include <ext/itoa.hpp>
#include <ext/range.hpp>
#include <ext/type_traits.hpp>
#include <ext/iostreams/utility.hpp>
#include <ext/base16.hpp>


/// those functions write hexdump of data into given destination(container, iterator, sink)
/// HP - hex pair
/// hexdump format is: <addr> <HP1> <H2> ... <HP16>  <ASCII data>\n

namespace ext
{
	namespace hexdump
	{
		static_assert(CHAR_BIT == 8, "byte is not octet");
		extern const char ascii_tranlation_array[256];
		constexpr unsigned rowsize = 16;

		/// returns text width that is used for printing mem offset addr in hexdump, minimum width is 2
		std::size_t addr_width(std::size_t count) noexcept;
		/// returns buffer size needed for hexdump for data of count size
		std::size_t buffer_estimation(std::size_t count) noexcept;		
		std::size_t buffer_estimation(std::size_t addr_width, std::size_t count) noexcept;


		template <class RandomAccessIterator, class OutputIterator>
		OutputIterator write_hexdump_impl(std::size_t & nrow, std::size_t addr_width, RandomAccessIterator first, RandomAccessIterator last, OutputIterator out)
		{
			constexpr unsigned rowsize = 16;
			auto count = last - first;
			auto left  = count % rowsize;
			char itoa_buffer[2 + std::numeric_limits<std::size_t>::digits / 4];

			auto offset_width = addr_width + 1;
			auto offset_last  = std::end(itoa_buffer);
			auto offset_first = std::end(itoa_buffer) - offset_width;

			last = last - left;
			auto row = first;

			for (; row < last; row += rowsize, nrow += rowsize)
			{
				std::fill(offset_first,
					ext::unsafe_itoa(nrow, offset_first, offset_width, 16),
					'0');

				out = std::copy(offset_first, offset_last - 1, out);
				*out = ' ', ++out;
				*out = ' ', ++out;

				auto cfirst = row;
				auto clast  = row + rowsize;
				for (auto cur = cfirst; cur < clast; ++cur)
				{
					out = ext::base16::encode_char(out, *cur);
					*out = ' ', ++out;
				}

				*out = ' ', ++out;
				*out = ' ', ++out;

				for (auto cur = cfirst; cur < clast; ++cur)
				{
					unsigned char ch = *cur;
					ch = ascii_tranlation_array[ch];
					*out++ = ch;
				}

				*out = '\n', ++out;
			}

			if (not left) return out;

			std::fill(offset_first,
				ext::unsafe_itoa(nrow, offset_first, offset_width, 16),
				'0');

			out = std::copy(offset_first, offset_last - 1, out);
			*out = ' ', ++out;
			*out = ' ', ++out;


			first = last;
			last  = last + left;

			for (auto cur = first; cur < last; ++cur)
			{
				out = ext::base16::encode_char(out, *cur);
				*out = ' ', ++out;
			}

			for (unsigned u = left; u < rowsize; ++u)
			{
				*out = ' ', ++out;
				*out = ' ', ++out;
				*out = ' ', ++out;
			}

			*out = ' ', ++out;
			*out = ' ', ++out;

			for (auto cur = first; cur < last; ++cur)
			{
				unsigned char ch = *cur;
				ch = ascii_tranlation_array[ch];
				*out++ = ch;
			}

			*out = '\n', ++out;
			nrow += rowsize;

			return out;
		}

	} // namespace hexdump

	template <class RandomAccessIterator, class OutputIterator>
	std::enable_if_t<ext::is_iterator_v<OutputIterator>, OutputIterator>
	write_hexdump(RandomAccessIterator first, RandomAccessIterator last, OutputIterator out)
	{
		std::size_t nrow = 0;
		return hexdump::write_hexdump_impl(nrow, first, last, out);
	}

	template <class RandomAccessIterator, class OutputContainer>
	std::enable_if_t<ext::is_container_v<OutputContainer>>
	write_hexdump(RandomAccessIterator first, RandomAccessIterator last, OutputContainer & cont)
	{
		// we are appending
		auto count = last - first;
		auto sizeest = hexdump::buffer_estimation(count);
		auto old_size = cont.size();
		cont.resize(old_size + sizeest);

		auto out_beg = boost::begin(cont) + old_size;
		//auto out_end = boost::end(cont);

		std::size_t nrow = 0;
		std::size_t addr_width = hexdump::addr_width(count);
		auto out_last = hexdump::write_hexdump_impl(nrow, addr_width, first, last, out_beg);
		cont.erase(out_last, cont.end());
	}

	template <class InputRange, class OutputContainer>
	std::enable_if_t<ext::is_container_v<OutputContainer>>
	write_hexdump(const InputRange & input, OutputContainer & cont)
	{
		auto view = ext::str_view(input);
		auto first = ext::data(view);
		auto last  = first + boost::size(view);
		return write_hexdump(first, last, cont);
	}

	template <class OutputContainer = std::string, class InputRange>
	OutputContainer write_hexdump(const InputRange & input)
	{
		OutputContainer cont;
		write_hexdump(input, cont);
		return cont;
	}

	template <class RandomAccessIterator, class Sink>
	std::enable_if_t<ext::iostreams::is_device_v<Sink>, Sink &>
	write_hexdump(RandomAccessIterator first, RandomAccessIterator last, Sink & sink)
	{
		std::size_t nrow = 0;
		std::size_t addr_width = hexdump::addr_width(last - first);
		const std::size_t step_size = 256;
		const std::size_t buffer_size = hexdump::buffer_estimation(addr_width, step_size);

		std::vector<char> buffer(buffer_size);
		auto out = buffer.data();

		while (first < last)
		{
			auto step_last = first + std::min<std::ptrdiff_t>(step_size, last - first);
			auto out_last = hexdump::write_hexdump_impl(nrow, addr_width, first, step_last, out);
			ext::iostreams::write_all(sink, out, out_last - out);
			first = step_last;
		}

		return sink;
	}

	template <class InputRange, class Sink>
	std::enable_if_t<ext::iostreams::is_device_v<Sink>, Sink &>
	write_hexdump(const InputRange & input, Sink & sink)
	{
		auto view = ext::str_view(input);
		auto first = ext::data(view);
		auto last  = first + boost::size(view);
		return write_hexdump(first, last, sink);
	}

} // namespace ext::net
