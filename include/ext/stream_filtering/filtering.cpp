#include <ext/stream_filtering/filtering.hpp>
#include <ext/errors.hpp>
#include <boost/core/demangle.hpp>
#include <fmt/core.h>

namespace ext::stream_filtering
{
	void read_stream(std::istream & is, data_context & dctx)
	{
		auto first = dctx.data_ptr + dctx.consumed;
		auto last  = dctx.data_ptr + dctx.written;
		auto * stopped = std::move(first, last, dctx.data_ptr);
		dctx.written = stopped - dctx.data_ptr;
		dctx.consumed = 0;
		
		is.read(stopped, dctx.capacity - dctx.written);
		// https://en.cppreference.com/w/cpp/io/basic_istream/read
		// Extracts characters from stream.
		// Behaves as UnformattedInputFunction.
		// After constructing and checking the sentry object, extracts characters and stores them into successive locations of the character array
		// whose first element is pointed to by s. Characters are extracted and stored until any of the following conditions occurs:
		//     count characters were extracted and stored
		//     end of file condition occurs on the input sequence (in which case, setstate(failbit|eofbit) is called).
		//     The number of successfully extracted characters can be queried using gcount().
		auto state = is.rdstate();
		if (state == 0 or state == (std::ios::eofbit | std::ios::failbit))
		{
			dctx.written += is.gcount();
			dctx.finished = is.eof();
		}
		else
		{
			int err = errno;
			const auto & sb = *is.rdbuf();
			auto classname = boost::core::demangle(typeid(sb).name());
			auto errdescr = fmt::format("ext::stream_filtering:read_data: istream read error: rdstate = {:b}, streambuf class = {}, errno = {}", is.rdstate(), classname, ext::format_errno(err));
			throw std::runtime_error(std::move(errdescr));
		}
	}
	
	void read_stream(std::streambuf & sb, data_context & dctx)
	{
		auto first = dctx.data_ptr + dctx.consumed;
		auto last  = dctx.data_ptr + dctx.written;
		auto * stopped = std::move(first, last, dctx.data_ptr);
		dctx.written = stopped - dctx.data_ptr;
		dctx.consumed = 0;
		
		auto toread = dctx.capacity - dctx.written;
		auto read = sb.sgetn(stopped, toread);
		dctx.written += read;
		dctx.finished = read < toread;
	}
	
	void write_stream(std::ostream & os, data_context & dctx)
	{
		assert(dctx.consumed == 0);
		os.write(dctx.data_ptr, dctx.written);
		dctx.written = 0;
		
		// https://en.cppreference.com/w/cpp/io/basic_ostream/write
		// Behaves as an UnformattedOutputFunction.
		// After constructing and checking the sentry object, outputs the characters from successive locations in the character array
		// whose first element is pointed to by s. Characters are inserted into the output sequence until one of the following occurs:
		//    exactly count characters are inserted
		//    inserting into the output sequence fails (in which case setstate(badbit) is called)
		if (not os.good())
		{
			int err = errno;
			const auto & sb = *os.rdbuf();
			auto classname = boost::core::demangle(typeid(sb).name());
			auto errdescr = fmt::format("ext::stream_filtering::write_data: ostream write error: rdstate = {:b}, streambuf class = {}, errno = {}", os.rdstate(), classname, ext::format_errno(err));
			throw std::runtime_error(std::move(errdescr));
		}
	}
	
	void write_stream(std::streambuf & sb, data_context & dctx)
	{
		assert(dctx.consumed == 0);
		auto written = sb.sputn(dctx.data_ptr, dctx.written);
		dctx.written -= written;
		
		if (dctx.written != 0)
		{
			int err = errno;
			auto classname = boost::core::demangle(typeid(sb).name());
			auto errdescr = fmt::format("ext::stream_filtering::write_data: streambuf write error: streambuf class = {:b}, errno = {}", classname, ext::format_errno(err));
			throw std::runtime_error(std::move(errdescr));
		}
	}
	
}
