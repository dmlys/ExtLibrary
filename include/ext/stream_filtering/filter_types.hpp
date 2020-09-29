#pragma once
#include <cstddef>
#include <tuple>
#include <optional>
#include <vector>
#include <functional>
#include <string_view>

#if not EXT_STREAM_FILTERING_DEFAULT_BUFFER_SIZE
#define EXT_STREAM_FILTERING_DEFAULT_BUFFER_SIZE 1024
#endif

#if not EXT_STREAM_FILTERING_MINIMUM_BUFFER_SIZE
#define EXT_STREAM_FILTERING_MINIMUM_BUFFER_SIZE 1024
#endif

#if not EXT_STREAM_FILTERING_MAXIMUM_BUFFER_SIZE
#define EXT_STREAM_FILTERING_MAXIMUM_BUFFER_SIZE 1024 * 10
#endif

/// See description in ext/stream_filtering/filtering.hpp

namespace ext::stream_filtering
{
	//                                       consumed,  written,    finished - no more data will be produced
	using filter_result_type = std::tuple<std::size_t, std::size_t, bool>;
	using filter_signature = filter_result_type(const char * input, std::size_t inputsz, char * output, std::size_t outputsz, bool eos);
	
	/// basic filter interface
	class filter
	{
	public:
		/// does filtering from input to output, eos - end of stream, input one),
		/// returns how much was consumed from input, written into output, is filter finished(eos, eof, name it)
		virtual auto process(const char * input, std::size_t inputsz, char * output, std::size_t outputsz, bool eos)
			-> std::tuple<std::size_t, std::size_t, bool> = 0;
		//                  consumed,   written,   finished
		
		/// resets this filter state into default constructed state, ready to start over again
		virtual void reset() = 0;
		
		/// Filter name, used only for logging and diagnostics purposes.
		/// Some filter type name, like zlib_defalte_filter, optionally you can include some object instance naming
		virtual std::string_view name() const = 0;
		
		// for future implementation
		//virtual std::optional<std::size_t>  input_buffer_size() const { return std::nullopt; }
		//virtual std::optional<std::size_t> output_buffer_size() const { return std::nullopt; }
	};
	
	/// filter processing parameters, currently buffer sizes.
	/// Minimum and maximum buffer size do limit buffer size.
	struct processing_parameters
	{
		unsigned default_buffer_size = 0;  // 0 - implementation default
		
		// Through currently buffer size can be set only via default_buffer_size,
		// in future filter may be able to return wanted buffer sizes, those will limit them
		unsigned minimum_buffer_size = 0;  // 0 - implementation default
		unsigned maximum_buffer_size = 0;  // 0 - implementation default
	};
	
	constexpr unsigned impldef_default_buffer_size = EXT_STREAM_FILTERING_DEFAULT_BUFFER_SIZE; // 1024
	constexpr unsigned impldef_minimum_buffer_size = EXT_STREAM_FILTERING_MINIMUM_BUFFER_SIZE; // 1024
	constexpr unsigned impldef_maximum_buffer_size = EXT_STREAM_FILTERING_MAXIMUM_BUFFER_SIZE; // 1024 * 10
	
	/// holds buffer state(does not owns)
	struct data_context
	{
		char * data_ptr = nullptr; // buffer pointer
		unsigned capacity = 0;     // how much this buffer can hold
		unsigned consumed = 0;     // how much was already processed
		unsigned written  = 0;     // how much was written into this buffer, basicly buffer size
		bool finished = false;     // is this buffer finished - data stream reached natural end or filter got eos/finish on it's own
	};
	
	/// holds filtering state: filters, buffers state, buffers
	template <class FilterVector, class DataContextVector, class BufferVector>
	struct streaming_context
	{
		processing_parameters params;
		DataContextVector data_contexts;
		FilterVector filters;
		BufferVector buffers; // should it be there?
	};
	
	
	/// traits for calling filter, so this lib be able to work not only with this filter interface.
	template <class Filter, class = std::void_t<>>
	struct filter_traits;
	
	template <class Filter>
	struct filter_traits<Filter *>
	{
		inline static auto call(filter * filter, const char * input, std::size_t inputsz, char * output, std::size_t outputsz, bool eos)
		{
			return filter_traits<Filter>::call(*filter, input, inputsz, output, outputsz, eos);
		}
	};
	
	template <class Filter>
	struct filter_traits<Filter, std::void_t<std::enable_if_t<std::is_base_of_v<filter, Filter>>>>
	{
		inline static auto call(filter & filter, const char * input, std::size_t inputsz, char * output, std::size_t outputsz, bool eos)
			-> std::tuple<std::size_t, std::size_t, bool>
		{
			return filter.process(input, inputsz, output, outputsz, eos);
		}
	};
	
	template <>
	struct filter_traits<std::function<filter_signature>>
	{
		inline static auto call(const std::function<filter_signature> & filter, const char * input, std::size_t inputsz, char * output, std::size_t outputsz, bool eos)
			-> std::tuple<std::size_t, std::size_t, bool>
		{
			return filter(input, inputsz, output, outputsz, eos);
		}
	};
	
	// TODO: specialization should be somehow more precize
	template <template <class> class SmartPointer, class Filter>
	struct filter_traits<SmartPointer<Filter>>
	{
		inline static auto call(SmartPointer<Filter> & filter_ptr, const char * input, std::size_t inputsz, char * output, std::size_t outputsz, bool eos)
			-> std::tuple<std::size_t, std::size_t, bool>
		{
			return filter_traits<Filter>::call(*filter_ptr, input, inputsz, output, outputsz, eos);
		}
	};
	
	
	constexpr void preprocess_processing_parameters(processing_parameters & par)
	{
		if (par.default_buffer_size == 0) par.default_buffer_size = impldef_default_buffer_size;
		if (par.maximum_buffer_size == 0) par.maximum_buffer_size = impldef_maximum_buffer_size;
	}
	
}
