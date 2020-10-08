#pragma once
#include <cstddef>
#include <cassert>
#include <algorithm>
#include <array>
#include <ext/stream_filtering/filter_types.hpp>

namespace ext::stream_filtering
{
	template <
		class ProcessorFunctor,
		unsigned InputGroupSize,
		unsigned OutputGroupSize
	>
	class transform_width_filter : public filter
	{
	public:
		using processing_functor = ProcessorFunctor;
		
		static_assert(InputGroupSize,  "InputGroupSize must be greater than 0");
		static_assert(OutputGroupSize, "OutputGroupSize must be greater than 0");
		
	protected:
		processing_functor m_functor;
		std::array<char, InputGroupSize + OutputGroupSize> m_buffer;
		
		unsigned m_inputBufferSize = 0;
		unsigned m_outputBufferSize = 0;
		unsigned m_outputBufferConsumed = 0;
		
	protected:
		char * process_some(const char * first, const char * last, char * output) { return m_functor(first, last, output); }
		
	public:
		virtual auto process(const char * input, std::size_t inputsz, char * output, std::size_t outputsz, bool finish)
			-> std::tuple<std::size_t, std::size_t, bool> override;
		
		virtual void reset() override;
		virtual std::string_view name() const override { return "transform_width_filter"; }
		
	public:
		constexpr transform_width_filter() = default;
		transform_width_filter(processing_functor functor) : m_functor(std::move(functor)) {}
		
		transform_width_filter(const transform_width_filter &) = default;
		transform_width_filter & operator =(const transform_width_filter &) = default;
		
	};
	
	
	template <class ProcessorFunctor, unsigned InputGroupSize, unsigned OutputGroupSize>
	auto transform_width_filter<ProcessorFunctor, InputGroupSize, OutputGroupSize>::process(const char * input, std::size_t inputsz, char * output, std::size_t outputsz, bool eos)
		-> std::tuple<std::size_t, std::size_t, bool>
	{
		auto * input_buffer = m_buffer.data();
		auto * output_buffer = m_buffer.data() + InputGroupSize;
		
		std::size_t read, written, toread, towrite;
		read = written = 0;
		
		// if we have some trailing output - write it
		if (m_outputBufferSize != 0) write_trailing_output:
		{
			assert(m_inputBufferSize == 0);
			auto n = std::min<std::size_t>(m_outputBufferSize, outputsz);
			auto * stopped = std::copy_n(output_buffer + m_outputBufferConsumed, n, output);
			written = stopped - output, output += written, outputsz -= written, m_outputBufferConsumed += written, m_outputBufferSize -= n;
		}
		// if we have some trailing input - try complete it
		else if (m_inputBufferSize != 0)
		{
			auto n = std::min<std::size_t>(InputGroupSize - m_inputBufferSize, inputsz);
			std::copy_n(input, n, input_buffer + m_inputBufferSize);
			read += n, input += n, inputsz -= n, m_inputBufferSize += n;
			
			// if completed - print into buffer
			if (m_inputBufferSize == InputGroupSize or eos)
			{
				auto * stopped = process_some(input_buffer, input_buffer + m_inputBufferSize, output_buffer);
				m_outputBufferSize = stopped - output_buffer;
				m_outputBufferConsumed = 0;
				m_inputBufferSize = 0;
				// and write it into output
				goto write_trailing_output;
			}
			
			assert(inputsz == 0);
			return std::make_tuple(read, written, false);
		}
		
		// now calculate available full groups
		auto ngroups = std::min(inputsz / InputGroupSize, outputsz / OutputGroupSize);
		toread  = ngroups * InputGroupSize;
		towrite = ngroups * OutputGroupSize;
		
		// we are finishing and have enough space for whole output - just process all at once
		if (eos and towrite + OutputGroupSize <= outputsz)
			toread = inputsz;
		
		if (ngroups)
		{
			auto * stopped = process_some(input, input + toread, output);
			read += toread, written += stopped - output;
			
			assert(m_inputBufferSize == 0 and m_outputBufferSize == 0);
			return std::make_tuple(read, written, eos and read >= inputsz);
			// we can read more than inputsz(read >= inputsz) if we had some trailing input in internal buffer
		}
		
		// If we do not wrote anything, and there are not even 1 full input group to consume -
		// we must consume something, to not stall.
		if (not written)
		{
			// Consume what is given for further processing
			auto n = std::min<std::size_t>(inputsz, InputGroupSize);
			std::copy_n(input, n, input_buffer);
			read += n, m_inputBufferSize += n;
		}
		
		assert((read or written) or (inputsz == 0 or outputsz == 0));
		// we are finished if and only if there is no input left and we do not have accumulated input in any form
		eos &= inputsz == 0 and m_inputBufferSize == 0 and m_outputBufferSize == 0;
		return std::make_tuple(read, written, eos);
	}

	template <class ProcessorFunctor, unsigned InputGroupSize, unsigned OutputGroupSize>
	void transform_width_filter<ProcessorFunctor, InputGroupSize, OutputGroupSize>::reset()
	{
		m_inputBufferSize = 0;
		m_outputBufferSize = 0;
		m_outputBufferConsumed = 0;
		
		std::fill_n(m_buffer.data(), m_buffer.size(), 0);
	}
}
