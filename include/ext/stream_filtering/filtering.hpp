#pragma once
#include <cassert>
#include <istream>
#include <ostream>
#include <algorithm>

#include <boost/scope_exit.hpp>
#include <boost/mp11.hpp>

#include <ext/type_traits.hpp>
#include <ext/utility.hpp>
#include <ext/stream_filtering/filter_types.hpp>

/// stream_filtering - simple library for writting and using byte stream filters and filter chains.
/// 
/// Filter is some class that transforms some binary stream: base64 encode/decode, zlib deflate/inflate, etc.
///   Basically filter has some process method with signature:
///   std::tuple<std::size_t, std::size_t, bool>(const char * input, std::size_t inputsz, char * output, std::size_t outputsz, bool finish)
///   
///   This method transforms data from input buffer with size inputsz into output buffer with size outputsz,
///   returns how much was consumed, written and was filter finished
/// 
///   Filter must do some work: consume something or produce something or return that's it's finished.
///   If needed some data should be stored temporary internally in filter,
///   through with adequate buffer sizes there are should not be need for that.
/// 
/// 
/// Library provides functions for processing filter chains:
/// providing, managing and balancing buffers and their state, tracking end of stream/filter.
/// 
/// User should implement some filter(or take already implemented one),
/// build filter chain(basically std::vector of filters)
/// and then just pass it to this library with some data stream source and destination.
/// 
/// Currently stream source/destination can be some memory container - std::vector or alike,
/// and std::ostream/std::istream, std::streambuf
/// 
/// Adding more supported stream classes should not be a problem,
/// through currently library does provide no mechanism at all.
/// TODO: define and provide some traits classes for that.
/// 
/// some internal documentation:
///  For each filter there is input and output buffer, one filter output buffer is next filter input buffer;
///  + first buffer for input byte stream and last buffer for output byte stream.
/// 
///  data stream natural end(eof, eos or finished) is passed at some time to filter and filter returns as well(after all data has been written)
///  through some filter can produce eos/finish on it's own, like zlib filter(there is some trailing data after gzip footer).
///  
///  data_context - holds and tracks buffer state, it does not own this buffer, nor does knows what this buffer is(buffer container class or alike),
///  basicly it's a pointer with some counters/indexes.
///  
///  First and last buffer are either input, output memory containers(when working with memory)
///  or std::vector<char> provided by this library and are read from input stream/written to output stream.
///  All intermediatory buffers are always provided by this library.
///  Buffer size is configurable via processing_parameters
/// 

namespace ext::stream_filtering
{
	/// Filters data stream from stream is to stream os via given filter chain, basic exception guarantee
	template <class FilterVector, class InputStream, class OutputStream>
	void filter_stream(FilterVector & filters, InputStream & is, OutputStream & os);
	/// Filters data stream from stream is to stream os via given filter chain, basic exception guarantee
	template <class FilterVector, class InputStream, class OutputStream>
	void filter_stream(processing_parameters params, FilterVector & filters, InputStream & is, OutputStream & os);
	
	/// Filters data stream from memory buffer input to memory container os via given filter chain, basic exception guarantee
	template <class FilterVector, class InputBuffer, class OutputBuffer>
	void filter_memory(FilterVector & filters, const InputBuffer & input, OutputBuffer & output);
	/// Filters data stream from memory buffer input to memory container os via given filter chain, basic exception guarantee
	template <class FilterVector, class InputBuffer, class OutputBuffer>
	void filter_memory(processing_parameters params, FilterVector & filters, const InputBuffer & input, OutputBuffer & output);


	/// \internal
	/// calculates threshold for determine if buffer is full
	/// basically buffer is full if it's size is 80% of capacity
	constexpr inline unsigned fullbuffer_threshold(unsigned capacity) noexcept
	{
		return capacity - capacity / 5; // full threshold is 80% of capacity
		
		//return capacity / 5 + (capacity % 5 ? 1 : 0); // full threshold is 20% of capacity
		
		// full threshold is 80% of capacity, division rounded up
		//unsigned one_fifth = capacity / 5 + (capacity % 5 ? 1 : 0);
		//return std::max(1u, capacity - one_fifth);
	}

	/// \internal
	/// calculates threshold for determine if buffer is full
	/// basically buffer is full if it's size is 80% of capacity
	constexpr inline unsigned fullbuffer_threshold(unsigned capacity, const processing_parameters & par) noexcept
	{
		//return capacity / par.full_threshold_part + (capacity % par.full_threshold_part ? 1 : 0);
		return fullbuffer_threshold(capacity);
	}
	
	/// \internal
	/// Finds last full(size >= fullbuffer_threshold or finished) data_context.
	/// Returns it's index, if there is no any - returns 0.
	template <class ... Args>
	unsigned find_ready_data(streaming_context<Args...> & ctx) noexcept;
	
	/// \internal
	/// Sort of main function of this library. Performs filtering step:
	/// * finds data_context ready for processing
	/// * calls appropriate filter and optionally filters following filters.
	/// * updates data_context state.
	/// Any filter will be called no more than 1 time, but not necessary every filter will be called.
	/// At least 1 filter will be called and some work will be done.
	/// This method should be called on loop until last data_context becomes finished,
	/// note that first data context is expected to become finished at some point.
	/// 
	/// Basic exception guarantee
	template <class ... Args>
	void filter_step(streaming_context<Args...> & ctx);
	
	/// \internal reads data from stream is, throws if stream error occurs
	void read_stream(std::istream & is, data_context & dctx);
	void read_stream(std::streambuf & sb, data_context & dctx);
	/// \internal writes data to stream os, throws if stream error occurs
	void write_stream(std::ostream & os, data_context & dctx);
	void write_stream(std::streambuf & sb, data_context & dctx);
	
	/// \internal
	/// copies input stream into output stream via temporary buffer
	template <class BufferType = std::vector<char>, class InputSteam, class OutputStream>
	void copy_stream(InputSteam & is, OutputStream & os, unsigned buffer_size = impldef_default_buffer_size);
	
	
	template <class ... Args>
	unsigned find_ready_data(streaming_context<Args...> & ctx) noexcept
	{
		assert(not ctx.filters.empty());
		
		unsigned index = ctx.filters.size() - 1;
		for (; index != 0; --index)
		{
			auto & context = ctx.data_contexts[index];
			// if context/filter is finished or has unconsumed data more then 20% of capacity
			auto unconsumed = context.written - context.consumed;
			auto threshold = stream_filtering::fullbuffer_threshold(context.capacity, ctx.params);
			if (context.finished or unconsumed >= threshold)
				return index;
		}
		
		return 0;
	}
	
	template <class ... Args>
	void filter_step(streaming_context<Args...> & ctx)
	{
		unsigned idx = find_ready_data(ctx);
		for (; idx < ctx.filters.size(); ++idx)
		{
			auto & filter = ctx.filters[idx];
			auto & source = ctx.data_contexts[idx];
			auto & dest   = ctx.data_contexts[idx + 1];
			
			using filter_type = ext::remove_cvref_t<decltype (filter)>;
			using filter_traits = ext::stream_filtering::filter_traits<filter_type>;
			
			assert((source.data_ptr and source.capacity) or source.finished);
			assert(dest.data_ptr and dest.capacity);
			
			auto * source_ptr  = source.data_ptr + source.consumed;
			auto   source_size = source.written  - source.consumed;
			auto   source_threshold = stream_filtering::fullbuffer_threshold(source.capacity, ctx.params);
			bool   source_empty = idx != 0 and not source.finished and source_size < source_threshold;
			
			if (source_empty) break;
			
			{
				auto first = dest.data_ptr + dest.consumed;
				auto last  = dest.data_ptr + dest.written;
				auto * stopped = std::move(first, last, dest.data_ptr);
				dest.written = stopped - dest.data_ptr;
				dest.consumed = 0;
			}
			
			auto * dest_ptr  = dest.data_ptr + dest.written;
			auto   dest_size = dest.capacity - dest.written;
			
			unsigned consumed, written, finished;
			std::tie(consumed, written, finished) = filter_traits::call(filter, source_ptr, source_size, dest_ptr, dest_size, source.finished);
			if (consumed == 0 and written == 0 and not finished)
				throw std::runtime_error("ext::stream_filtering::filter_step: filter produced nothing nor consumed anything");
			
			source.consumed += consumed;
			dest.written += written;
			dest.finished = finished;
			
			// filter produced nothing, can't continue - this step is over
			if (not written) break;
		}
	}
	
	template <class FilterVector, class InputBuffer, class OutputBuffer>
	void filter_memory(processing_parameters params, FilterVector & filters, const InputBuffer & input, OutputBuffer & output)
	{
		preprocess_processing_paramers(params);
		
		static_assert (boost::mp11::mp_is_list<FilterVector>::value);
		using data_context_vector = boost::mp11::mp_assign<FilterVector, boost::mp11::mp_list<data_context>>;
		using buffer_vector = boost::mp11::mp_assign<FilterVector, boost::mp11::mp_list<InputBuffer>>;
		using streaming_context = ext::stream_filtering::streaming_context<FilterVector, data_context_vector, buffer_vector>;
		
		if (filters.empty())
		{
			output.assign(input.begin(), input.end());
			return;
		}
		
		streaming_context ctx;
		ctx.params = std::move(params);
		ctx.filters = std::move(filters);
		ctx.data_contexts.resize(ctx.filters.size() + 1);
		ctx.buffers.resize(ctx.filters.size() - 1);
		
		const std::size_t buffer_size = std::clamp(ctx.params.default_buffer_size, ctx.params.minimum_buffer_size, ctx.params.maximum_buffer_size);
		
		for (auto & buffer : ctx.buffers)
			buffer.resize(buffer_size);
		
		output.resize(std::max(output.capacity(), buffer_size));
		
		ctx.data_contexts[0].data_ptr = ext::unconst(input.data());
		ctx.data_contexts[0].written = input.size();
		ctx.data_contexts[0].capacity = input.size();
		ctx.data_contexts[0].finished = true;
		
		auto & result_buffer = output;
		auto & result_dctx = ctx.data_contexts.back();
		result_dctx.data_ptr = output.data();
		result_dctx.capacity = output.size();
		
		for (unsigned i = 0; i < ctx.buffers.size(); ++i)
		{
			auto & dctx = ctx.data_contexts[i + 1];
			auto & buffer = ctx.buffers[i];
			dctx.data_ptr = buffer.data();
			dctx.capacity = buffer.size();
			//dctx.finished = false;
		}
		
		for (;;)
		{
			filter_step(ctx);
			if (result_dctx.finished) break;
			
			auto space_avail = result_dctx.capacity - result_dctx.written;
			auto space_threshold = stream_filtering::fullbuffer_threshold(result_dctx.capacity, ctx.params);
			if (space_avail <= space_threshold)
			{
				auto newsize = result_buffer.size();
				newsize += newsize / 2 + newsize % 2;
				result_buffer.resize(newsize);
				
				result_dctx.data_ptr = result_buffer.data();
				result_dctx.capacity = result_buffer.size();
			}
		}
		
		result_buffer.resize(result_dctx.written);
	}
	
	
	template <class BufferType, class InputStream, class OutputStream>
	void copy_stream(InputStream & is, OutputStream & os, unsigned buffer_size)
	{
		BufferType buffer(buffer_size);
		data_context data_ctx;
		data_ctx.data_ptr = buffer.data();
		data_ctx.capacity = buffer.size();
		
		do
		{
			read_stream(is, data_ctx);
			write_stream(os, data_ctx);
			
		} while (not data_ctx.finished);
	}
	
	template <class FilterVector, class InputStream, class OutputStream>
	void filter_stream(processing_parameters params, FilterVector & filters, InputStream & is, OutputStream & os)
	{
		preprocess_processing_paramers(params);
		
		static_assert (boost::mp11::mp_is_list<FilterVector>::value);
		using data_context_vector = boost::mp11::mp_assign<FilterVector, boost::mp11::mp_list<data_context>>;
		using buffer_type = boost::mp11::mp_assign<FilterVector, boost::mp11::mp_list<char>>;
		using buffer_vector = boost::mp11::mp_assign<FilterVector, boost::mp11::mp_list<buffer_type>>;
		using streaming_context = ext::stream_filtering::streaming_context<FilterVector, data_context_vector, buffer_vector>;
		
		if (filters.empty())
		{
			const auto buffer_size = std::clamp(params.default_buffer_size, params.minimum_buffer_size, params.maximum_buffer_size);
			return copy_stream<buffer_type>(is, os, buffer_size);
		}
		
		streaming_context ctx;
		ctx.params = std::move(params);
		ctx.filters = std::move(filters);
		ctx.data_contexts.resize(ctx.filters.size() + 1);
		ctx.buffers.resize(ctx.filters.size() + 1);
		
		const auto buffer_size = std::clamp(ctx.params.default_buffer_size, ctx.params.minimum_buffer_size, ctx.params.maximum_buffer_size);
		
		for (auto & buffer : ctx.buffers)
			buffer.resize(buffer_size);
		
		for (unsigned i = 0; i < ctx.buffers.size(); ++i)
		{
			auto & dctx = ctx.data_contexts[i];
			auto & buffer = ctx.buffers[i];
			dctx.data_ptr = buffer.data();
			dctx.capacity = buffer.size();
			//dctx.finished = false;
		}
		
		auto & first_ctx = ctx.data_contexts.front();
		auto & last_ctx = ctx.data_contexts.back();
		
		do
		{
			auto unconsumed = first_ctx.written - first_ctx.consumed;
			auto threshold  = stream_filtering::fullbuffer_threshold(first_ctx.capacity, ctx.params);
			if (not first_ctx.finished and unconsumed <= threshold)
				read_stream(is, first_ctx);
			
			filter_step(ctx);
			
			unconsumed = last_ctx.written - last_ctx.consumed;
			threshold  = stream_filtering::fullbuffer_threshold(last_ctx.capacity, ctx.params);
			if (last_ctx.finished or unconsumed >= threshold)
				write_stream(os, last_ctx);
			
		} while (not last_ctx.finished);
		
	}
	
	
	template <class FilterVector, class InputStream, class OutputStream>
	inline void filter_stream(FilterVector & filters, InputStream & is, OutputStream & os)
	{
		processing_parameters params;
		return filter_stream(std::move(params), filters, is, os);
	}
	
	template <class FilterVector, class InputBuffer, class OutputBuffer>
	inline void filter_memory(FilterVector & filters, const InputBuffer & input, OutputBuffer & output)
	{
		processing_parameters params;
		return filter_memory(std::move(params), filters, input, output);
	}
}
