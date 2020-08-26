#pragma once
#include <cstddef>
#include <algorithm>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/pipeline.hpp>
#include <ext/iostreams/utility.hpp>

namespace ext {
namespace iostreams
{
	const std::size_t DefaultTransformWidthEncoderBufferSize = 4096;

	template <class TransformWidthIterator>
	struct iterator_encoder
	{
		template <class Iterator, class OutputIterator>
		inline OutputIterator operator()(Iterator first, Iterator last, OutputIterator out) const
		{
			return std::copy(TransformWidthIterator(first), TransformWidthIterator(last), out);
		}
	};

	/// boost::iostreams::output_filter. Encodes input binary stream into binary stream via EncodeFunctor
	/// This class manages stream processing and it's corner cases, such as trailing input, padding ending(optionally) and so on
	/// 
	/// EncodeFunctor must:
	///   * do conversion X * InputGroupSize bytes into Y * OutputGroupSize bytes
	///   * EncodeFunctor only needs to Encode given buffer into destination buffer pointed by iterator,
	///     destination buffer will always be large enough for passed input buffer
	///   * most time filter will call functor with input data length divisible by InputGroupSize,
	///     but if input data has trailing group at the end - filter will call functor with it,
	///     so it must be able to handle that data
	///   * next expressions must be valid:
	///     EncodeFunctor func;
	///       out = func(first, last, out);
	/// 
	/// @Param EncodeFunctor stateless EncodeFunctor
	/// @Param PaddingCharacter padding character, pass 0 if padding is not needed
	/// @Param InputGroupSize input not encoded group size, must be > 0
	/// @Param OutputGroupSize output encoded group size, must be > 0
	/// @Param BufferSize internal buffer size used for encoding, must be >= OutputGroupSize
	template <
		class EncodeFunctor,
		char PaddingCharacter,
		unsigned InputGroupSize,
		unsigned OutputGroupSize,
		std::size_t BufferSize = DefaultTransformWidthEncoderBufferSize
	>
	class transform_width_encoder
	{
	public:
		static_assert(InputGroupSize,  "InputGroupSize must be greater than 0");
		static_assert(OutputGroupSize, "OutputGroupSize must be greater than 0");
		static_assert(BufferSize >= OutputGroupSize, "BufferSize must be >= OutputGroupSize");

		typedef char char_type;

		struct category :
			boost::iostreams::multichar_output_filter_tag,
			boost::iostreams::optimally_buffered_tag,
			boost::iostreams::closable_tag
		{};

		typedef EncodeFunctor encoder_type;

	private:
		encoder_type m_encoder;

		// helper buffer for storing trailing groups
		// m_storedCount in [0..InputGroupSize]
		unsigned m_storedCount = 0;
		char m_storebuf[InputGroupSize];

	private:
		inline char * encode(const char * first, const char * last, char * out)
		{
			return m_encoder(first, last, out);
		}

		template <class Sink>
		static void flush_buffer(Sink & sink, const char * first, const char * last)
		{
			// we can't return proper written count to caller.
			// If we wrote not full group to sink, than how much data we wrote from original request?
			// instead we better do blocking write
			write_all(sink, first, last - first);
		}

		/// manages trailing groups
		/// adjusts buffer, data, size, taking necessary data to fill trailing group
		/// and printing into buffer.
		/// also takes trailing from data if any
		void process_trailing(char * & output, const char * & input, std::size_t & size)
		{
			// if we had something try complete it
			if (m_storedCount)
			{
				unsigned consumed = InputGroupSize - m_storedCount;
				consumed = consumed < size ? consumed : static_cast<unsigned>(size);
				std::copy_n(input, consumed, m_storebuf + m_storedCount);
				input += consumed;
				size -= consumed;
				m_storedCount += consumed;

				// if completed - print into buffer
				if (m_storedCount == InputGroupSize)
				{
					output = encode(m_storebuf, m_storebuf + InputGroupSize, output);
					m_storedCount = 0;
					// successfully processed trailing data - process new one at the end of current
					goto new_trailing_data;
				}
			}
			else new_trailing_data:
			{
				// add trailing data
				m_storedCount = size % InputGroupSize;
				size -= m_storedCount;
				std::copy_n(input + size, m_storedCount, m_storebuf);
			}
		}

	public:
		template <class Sink>
		std::streamsize write(Sink & sink, const char * data, std::streamsize n)
		{
			// on every call we should:
			//  * if we have not full group from previous call - try to fill it
			//    and if successful - write it to buffer.
			//    adjust data, by taken data
			//   
			//  * store not full group at the end of input batch into m_storebuf(it tried at previous step)
			//  * encode full groups into buffer, write into sink, repeat until all is written
			//  
			//  - on flush - fill group with padding and write to sink
			
			std::size_t count = boost::numeric_cast<std::size_t>(n);
			char buffer[BufferSize];
			auto output = buffer;
			auto input = data;
			// will adjust output, input, count,
			process_trailing(output, input, count);
			
			BOOST_ASSERT(count % InputGroupSize == 0);
			while (count)
			{
				std::size_t towrite = BufferSize - (output - buffer);
				std::size_t toread = towrite / OutputGroupSize * InputGroupSize;
				toread = std::min(toread, count);

				output = encode(input, input + toread, output);
				flush_buffer(sink, buffer, output);

				output = buffer;
				input += toread;
				count -= toread;
			}

			// if something left - flush it
			// this can happen if we have something trailing, and roundedCount == 0
			if (buffer != output)
				flush_buffer(sink, buffer, output);

			return n;
		}

		/// if there is trailing group - pads if required
		/// writes it into sink and returns number of written characters
		template <class Sink>
		std::size_t complete(Sink & sink)
		{
			// flush trailing
			if (m_storedCount == 0)
				return 0;
			
			char buffer[OutputGroupSize];
			auto last = encode(m_storebuf, m_storebuf + m_storedCount, buffer);
			m_storedCount = 0;

			auto end = last;
			// do padding if needed
			if (PaddingCharacter)
			{
				end = buffer + OutputGroupSize;
				std::fill(last, end, PaddingCharacter);
			}

			flush_buffer(sink, buffer, end);
			return end - buffer;
		}

		template <class Sink>
		inline void close(Sink & sink)
		{
			complete(sink);
		}

		std::size_t optimal_buffer_size() const
		{
			return BufferSize / OutputGroupSize * InputGroupSize;
		}
	};






	// boost::iostreams pipable intergration
	template <
		class EncodeFunctor,
		char PaddingCharacter,
		unsigned InputGroupSize,
		unsigned OutputGroupSize,
		std::size_t BufferSize,
		typename Component
	>
	::boost::iostreams::pipeline<
		::boost::iostreams::detail::pipeline_segment<
			transform_width_encoder<EncodeFunctor, PaddingCharacter, InputGroupSize, OutputGroupSize, BufferSize>
		>,
		Component
	>
	operator |(const transform_width_encoder<EncodeFunctor, PaddingCharacter, InputGroupSize, OutputGroupSize, BufferSize> & filter,
	           const Component & c)
	{
		typedef ::boost::iostreams::detail::pipeline_segment <
			transform_width_encoder<EncodeFunctor, PaddingCharacter, InputGroupSize, OutputGroupSize, BufferSize>
		> segment;

		return ::boost::iostreams::pipeline<segment, Component>(segment(filter), c);
	}
}}
