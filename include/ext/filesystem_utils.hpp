#pragma once
#ifndef FILESYSTEM_UTILS_H_
#define FILESYSTEM_UTILS_H_

#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <regex>
#include <filesystem>
#include <boost/predef.h>

#if BOOST_OS_WINDOWS
#include <ext/codecvt_conv/generic_conv.hpp>
#include <ext/codecvt_conv/wchar_cvt.hpp>
#endif

#include <ext/range.hpp>
#include <ext/errors.hpp>
#include <ext/is_string.hpp>

namespace ext
{
	namespace filesystem_utils_detail
	{
#if BOOST_OS_WINDOWS
		inline auto * to_utf8(const char    * str) { return str; }
		inline auto   to_utf8(const wchar_t * str) { return ext::codecvt_convert::wchar_cvt::to_utf8(str); }
#else
		inline auto * to_utf8(const char * str)    { return str; }
#endif
	}
	
	/// данный класс-предикат предназначен для фильтрации коллекций
	/// boost.filesystem.path / boost.filesystem.directory_entry по заданному регекс выражению
	struct fsmask
	{
	private:
		std::regex rx;

	public:
		using value_type = std::filesystem::path::value_type;

		template <class String>
		static constexpr auto char_string = ext::is_string_v<String> and std::is_same_v<ext::range_value_t<String>, std::filesystem::path::value_type>;

		template <class String>
		static constexpr auto other_string = ext::is_string_v<String> and std::is_same_v<ext::range_value_t<String>, std::filesystem::path::value_type>;

	public:
		fsmask(std::regex mask) : rx(std::move(mask)) {};
		fsmask(const std::string & mask) : rx(mask) {};

		bool operator ()(const std::filesystem::path & item)             const { return operator()(item.string().c_str()); }
		bool operator ()(const std::filesystem::directory_entry & item)  const { return operator()(item.path()); }
		bool operator ()(const char * str)                               const { return regex_match(str, rx); }

		template <class String>
		auto operator ()(const String & str) const -> std::enable_if_t<char_string<String>>
		{ return regex_match(str.begin(), str.end(), rx); }

		template <class String>
		auto operator ()(const String & str) const -> std::enable_if_t<other_string<String>>
		{ return operator()(std::filesystem::path(str)); }
	};

	/// возвращает набор файлов по указанному пути и маске
	/// например "E:/work/*.txt"
	/// маске должна быть только в конце, т.е. E:/wo*k/*.txt - некорректно
	std::vector<std::filesystem::path> files_by_mask(const std::filesystem::path & mask);

	///проверяет содержит ли строка символы wildCard
	bool is_wild_card(const std::filesystem::path & path);

	/// считывает файл по заданному пути file в контейнер content
	/// в поток reps выдается информация о процессе загрузки файла(в том числе и в случае успеха).
	/// по умолчанию грузит файл в текстовом режиме
	template <class Container>
	bool read_file(const std::filesystem::path::value_type * path, Container & content, std::ostream & reps,
	               std::ios_base::openmode mode = std::ios_base::in /*| std::ios_base::text*/)
	{
		static_assert(ext::is_container_of_v<Container, char>, "not a char container");
		auto u8_path = filesystem_utils_detail::to_utf8(path);

		std::ifstream ifs(path, mode);
		if (!ifs.is_open())
		{
			//      "Failed to open {1}, {2}
			reps << "Failed to open " << u8_path << ", " << ext::format_errno(errno) << std::endl;
			return false;
		}

		ifs.unsetf(std::ios::skipws);
		ifs.seekg(0, std::ios::end);
		std::streamsize fileSize = ifs.tellg();
		ifs.seekg(0);

		Container content_;
		try
		{
			std::make_unsigned<std::streamsize>::type ufs = fileSize; // to silence warning on x64
			if (ufs > content_.max_size())
				throw std::bad_alloc();
			content_.resize(static_cast<std::size_t>(fileSize), 0);
		}
		catch (std::bad_alloc &)
		{
			// "Failed to read {1}, file to big, size is {2,num}
			reps << "Failed to read " << u8_path << ", file to big, size is " << fileSize << std::endl;
			return false;
		}

		ifs.read(ext::data(content_), fileSize);
		if (ifs.bad())
		{
			// "Failed to read {1}, {2}
			reps << "Failed to read " << u8_path << ", " << ext::format_errno(errno) << std::endl;
			return false;
		}

		content_.resize(static_cast<std::size_t>(ifs.gcount()));

		using std::swap;
		swap(content_, content);
		return true;
	}

	template <class Container>
	bool write_file(const std::filesystem::path::value_type * path, const Container & content, std::ostream & reps,
	                std::ios_base::openmode mode = std::ios_base::out /*| std::ios_base::text*/)
	{
		auto u8_path = filesystem_utils_detail::to_utf8(path);
		
		std::ofstream ofs(path, mode);
		if (!ofs.is_open())
		{
			//      "Failed to open {1}, {2}
			reps << "Failed to open " << u8_path << ", " << ext::format_errno(errno) << std::endl;
			return false;
		}
		
		ofs.write(ext::data(content), content.size());
		if (ofs.bad())
		{
			// "Failed to read {1}, {2}
			reps << "Failed to write " << u8_path << ", " << ext::format_errno(errno) << std::endl;
			return false;
		}
		
		return true;
	}

#if BOOST_OS_WINDOWS
	template <class Container>
	bool read_file(const char * path, Container & content, std::ostream & reps,
	               std::ios_base::openmode mode = std::ios_base::in /*| std::ios_base::text*/)
	{
		std::wstring wstr = ext::codecvt_convert::wchar_cvt::to_wchar(path);
		return read_file(wstr.c_str(), content, reps, mode);
	}

	template <class Container>
	bool write_file(const char * path, const Container & content, std::ostream & resp,
	                std::ios_base::openmode mode = std::ios_base::out /*| std::ios_base::text*/)
	{
		std::wstring wstr = ext::codecvt_convert::wchar_cvt::to_wchar(path);
		return write_file(wstr.c_str(), content, resp, mode);
	}
#endif

	template <class Path, class Container>
	bool read_file(const Path & path, Container & content, std::ostream & reps,
	               std::ios_base::openmode mode = std::ios_base::in /*| std::ios_base::text*/)
	{
		return read_file(path.c_str(), content, reps, mode);
	}

	template <class Path, class Container>
	bool write_file(const Path & path, const Container & content, std::ostream & reps,
	                std::ios_base::openmode mode = std::ios_base::out /*| std::ios_base::text*/)
	{
		return write_file(path.c_str(), content, reps, mode);
	}

	struct file_read_error : std::runtime_error
	{
		file_read_error(const std::string & msg) : std::runtime_error(msg) {}
	};

	struct file_write_error : std::runtime_error
	{
		file_write_error(const std::string & msg) : std::runtime_error(msg) {}
	};

	template <class Path, class Container>
	void read_file(const Path & path, Container & content,
	               std::ios_base::openmode mode = std::ios_base::in /*| std::ios_base::text*/)
	{
		std::stringstream reps;
		if (not read_file(path, content, reps, mode))
			throw file_read_error(reps.str());
	}

	template <class Path, class Container>
	void write_file(const Path & path, const Container & content,
	               std::ios_base::openmode mode = std::ios_base::out /*| std::ios_base::text*/)
	{
		std::stringstream reps;
		if (not write_file(path, content, reps, mode))
			throw file_write_error(reps.str());
	}


	/// получает полный путь к exe файлу, в рамках которого исполняется код
	std::filesystem::path getexepath(int argc, char *argv[], std::error_code & ec);
	std::filesystem::path getexepath(int argc, char *argv[]);
}

#endif
