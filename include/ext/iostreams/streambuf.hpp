#pragma once
#include <streambuf>

namespace ext
{
	/// фикс std::streambuf
	/// стандартный std::streambuf имеет буфер для put area и get area,
	/// но эти буферы не доступны публично(они protected), что заставляет копировать/писать в еще один вспомогательный буфер.
	/// Я не вижу причины почему бы не дать частичный доступ к этим областям данных.
	///
	/// Данный класс прокидывает методы eback, ... как public
	/// после чего можно или наследоваться от этого класса,
	/// или делать же:
	/// std::streambuf & sb;
	/// static_cast<ext::streambuf &>(sb)
	/// но нужно быть осторожным, поскольку теоретически некоторые streambuf не буфферизированны
	class streambuf : public std::streambuf
	{
		typedef std::streambuf base_type;

	public:
		using base_type::eback;
		using base_type::gptr;
		using base_type::egptr;
		using base_type::gbump;

		using base_type::pbase;
		using base_type::pptr;
		using base_type::epptr;
		using base_type::pbump;

		using base_type::setg;
		using base_type::setp;
	};
}
