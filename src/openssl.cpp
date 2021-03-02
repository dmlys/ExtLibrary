#ifdef EXT_ENABLE_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/dh.h>
#include <openssl/pkcs12.h>

#include <ext/codecvt_conv/generic_conv.hpp>
#include <ext/codecvt_conv/wchar_cvt.hpp>

#include <boost/predef.h> // for BOOST_OS_WINDOWS
#include <boost/static_assert.hpp>

#include <ext/openssl.hpp>
#include <ext/time_fmt.hpp>

#include <cstring> // for std::memset

#if BOOST_OS_WINDOWS
#include <winsock2.h> // winsock2.h should be included before windows.h
#include <windows.h>

#ifdef _MSC_VER
#pragma comment(lib, "crypt32.lib")
#endif // _MSC_VER

#endif // BOOST_OS_WINDOWS


int  intrusive_ptr_add_ref(BIO * ptr)  { return ::BIO_up_ref(ptr); }
void intrusive_ptr_release(BIO * ptr)  { return ::BIO_free_all(ptr);   }

int  intrusive_ptr_add_ref(X509 * ptr) { return ::X509_up_ref(ptr); }
void intrusive_ptr_release(X509 * ptr) { return ::X509_free(ptr);   }

int  intrusive_ptr_add_ref(RSA * ptr)  { return ::RSA_up_ref(ptr);  }
void intrusive_ptr_release(RSA * ptr)  { return ::RSA_free(ptr);    }

int  intrusive_ptr_add_ref(EVP_PKEY * ptr) { return ::EVP_PKEY_up_ref(ptr); }
void intrusive_ptr_release(EVP_PKEY * ptr) { return ::EVP_PKEY_free(ptr);   }

int  intrusive_ptr_add_ref(SSL * ptr) { return ::SSL_up_ref(ptr); }
void intrusive_ptr_release(SSL * ptr) { return ::SSL_free(ptr);   }

int  intrusive_ptr_add_ref(SSL_CTX * ptr) { return ::SSL_CTX_up_ref(ptr); }
void intrusive_ptr_release(SSL_CTX * ptr) { return ::SSL_CTX_free(ptr);   }


#if OPENSSL_VERSION_NUMBER < 0x10101000L
static int ASN1_TIME_to_tm(const ::ASN1_TIME * time, std::tm * tm);
#endif


namespace ext::openssl
{
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_error::none)              == SSL_ERROR_NONE);
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_error::ssl)               == SSL_ERROR_SSL);
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_error::want_read)         == SSL_ERROR_WANT_READ);
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_error::want_write)        == SSL_ERROR_WANT_WRITE);
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_error::want_X509_lookup)  == SSL_ERROR_WANT_X509_LOOKUP);
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_error::syscall)           == SSL_ERROR_SYSCALL);
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_error::zero_return)       == SSL_ERROR_ZERO_RETURN);
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_error::want_connect)      == SSL_ERROR_WANT_CONNECT);
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_error::want_accept)       == SSL_ERROR_WANT_ACCEPT);


	/************************************************************************/
	/*                      error_category                                  */
	/************************************************************************/
	struct openssl_err_category_impl : std::error_category
	{
		const char * name() const noexcept override;
		std::string message(int code) const override;
	};

	const char * openssl_err_category_impl::name() const noexcept
	{
		return "openssl_err";
	}

	std::string openssl_err_category_impl::message(int code) const
	{
		// https://www.openssl.org/docs/man1.1.1/man3/ERR_error_string.html
		// openssl says that buffer should be at least 256 bytes long for ERR_error_string.
		constexpr std::size_t buflen = 512;
		char errbuf[buflen];
		::ERR_error_string_n(code, errbuf, buflen);
		// errbuf will be null terminated
		return errbuf;
	}

	struct openssl_ssl_category_impl : std::error_category
	{
		const char * name() const noexcept override;
		std::string message(int code) const override;
	};

	const char * openssl_ssl_category_impl::name() const noexcept
	{
		return "openssl_ssl";
	}

	std::string openssl_ssl_category_impl::message(int code) const
	{
		switch (code)
		{
			case SSL_ERROR_NONE:             return "SSL_ERROR_NONE";
			case SSL_ERROR_SSL:              return "SSL_ERROR_SSL";
			case SSL_ERROR_WANT_READ:        return "SSL_ERROR_WANT_READ";
			case SSL_ERROR_WANT_WRITE:       return "SSL_ERROR_WANT_WRITE";
			case SSL_ERROR_WANT_X509_LOOKUP: return "SSL_ERROR_WANT_X509_LOOKUP";
			case SSL_ERROR_SYSCALL:          return "SSL_ERROR_SYSCALL";
			
			case SSL_ERROR_ZERO_RETURN:      return "SSL_ERROR_ZERO_RETURN";
			case SSL_ERROR_WANT_CONNECT:     return "SSL_ERROR_WANT_CONNECT";
			case SSL_ERROR_WANT_ACCEPT:      return "SSL_ERROR_WANT_ACCEPT";
			
			default: return "SSL_ERROR_UNKNOWN";
		}
	}
	
	openssl_err_category_impl openssl_err_category_instance;
	openssl_ssl_category_impl openssl_ssl_category_instance;

	const std::error_category & openssl_err_category() noexcept
	{
		return openssl_err_category_instance;
	}

	const std::error_category & openssl_ssl_category() noexcept
	{
		return openssl_ssl_category_instance;
	}

	std::error_code openssl_geterror(int sslcode, error_retrieve rtype) noexcept
	{
		if (sslcode == SSL_ERROR_SSL || sslcode == SSL_ERROR_SYSCALL)
		{
			int err = rtype == error_retrieve::get ? ::ERR_get_error() : ::ERR_peek_error();
			if (err) return {err, openssl_err_category()};
			
#if BOOST_OS_WINDOWS
			err = ::WSAGetLastError();
#else
			err = errno;
#endif
			if (err) return {err, std::system_category()};
		}
		
		return {sslcode, openssl_ssl_category()};
	}

	void openssl_clear_errors() noexcept
	{
		::ERR_clear_error();
	}

	std::error_code last_error(error_retrieve rtype) noexcept
	{
		int err = rtype == error_retrieve::get ? ::ERR_get_error() : ::ERR_peek_error();
		return std::error_code(err, openssl_err_category());
	}

	[[noreturn]] void throw_last_error(const std::string & errmsg, error_retrieve rtype)
	{
		throw std::system_error(last_error(rtype), errmsg);
	}

	static int print_error_queue_string_cb(const char * data, std::size_t len, void * ptr)
	{
		auto * pstr = static_cast<std::string *>(ptr);
		pstr->append(data, len);
		return 1;
	}

	static int print_error_queue_streambuf_cb(const char * data, std::size_t len, void * ptr)
	{
		auto * pos = static_cast<std::streambuf *>(ptr);
		return pos->sputn(data, len) == static_cast<std::streamsize>(len);
	}

	static int print_error_queue_ostream_cb(const char * data, std::size_t len, void * ptr)
	{
		auto * pos = static_cast<std::ostream *>(ptr);
		pos->write(data, len);
		return static_cast<bool>(*pos);
	}

	void print_error_queue(std::string & str)
	{
		::ERR_print_errors_cb(print_error_queue_string_cb, &str);
	}

	void print_error_queue(std::ostream & os)
	{
		::ERR_print_errors_cb(print_error_queue_ostream_cb, &os);
	}

	void print_error_queue(std::streambuf & os)
	{
		::ERR_print_errors_cb(print_error_queue_streambuf_cb, &os);
	}

	std::string print_error_queue()
	{
		std::string str;
		print_error_queue(str);
		return str;
	}

	/************************************************************************/
	/*                  init/cleanup                                        */
	/************************************************************************/
	void crypto_init()
	{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
		ERR_load_crypto_strings();
#elif OPENSSL_VERSION_NUMBER >= 0x10100000L
		OPENSSL_init_crypto(0, nullptr);
#endif		
	}
	
	void ssl_init()
	{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
		SSL_load_error_strings();
		SSL_library_init();
#elif OPENSSL_VERSION_NUMBER >= 0x10100000L
		OPENSSL_init_ssl(0, nullptr);
#endif

	}

	void lib_cleanup()
	{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
		// https://mta.openssl.org/pipermail/openssl-users/2015-January/000326.html
		CRYPTO_cleanup_all_ex_data();
		ERR_free_strings();
		EVP_cleanup();
#endif
	}

	/************************************************************************/
	/*                  smart ptr stuff                                     */
	/************************************************************************/
	void ssl_deleter::operator()(SSL * ssl) const noexcept
	{
		::SSL_free(ssl);
	}

	void ssl_ctx_deleter::operator()(SSL_CTX * sslctx) const noexcept
	{
		::SSL_CTX_free(sslctx);
	}

	void bio_deleter::operator()(BIO * bio) const noexcept
	{
		::BIO_vfree(bio);
	}

	void x509_deleter::operator()(X509 * cert) const noexcept
	{
		::X509_free(cert);
	}

	void stackof_x509_deleter::operator()(STACK_OF(X509) * ca) const noexcept
	{
		// https://wiki.nikhef.nl/grid/How_to_handle_OpenSSL_and_not_get_hurt_and_what_does_that_library_call_really_do%3F#Proper_memory_liberation_of_a_STACK_OF_.28X509.29_.2A
		// sk_X509_pop_free should be used instead of sk_X509_free, or memory leaks will occur or certificate chains longer than 1
		::sk_X509_pop_free(ca, ::X509_free);
	}

	void rsa_deleter::operator()(RSA * rsa) const noexcept
	{
		::RSA_free(rsa);
	}

	void evp_pkey_deleter::operator()(EVP_PKEY * pkey) const noexcept
	{
		::EVP_PKEY_free(pkey);
	}

	void pkcs12_deleter::operator()(PKCS12 * pkcs12) const noexcept
	{
		::PKCS12_free(pkcs12);
	}

	/************************************************************************/
	/*                  print helpers                                       */
	/************************************************************************/
	std::string x509_name_string(const ::X509_NAME * name, int flags)
	{
		ext::openssl::bio_uptr mem_bio(::BIO_new(::BIO_s_mem()));
		// XN_FLAG_DN_REV - for XML DSIG, names subject and issuers should be in reversed order
		if (-1 == ::X509_NAME_print_ex(mem_bio.get(), name, 0, flags))
			ext::openssl::throw_last_error("::X509_NAME_print_ex failed");

		char * data;
		int len = BIO_get_mem_data(mem_bio.get(), &data);
		return std::string(data, len);
	}
	
	std::string x509_name_string(const ::X509_NAME * name)
	{
		constexpr int flags = (XN_FLAG_RFC2253 | XN_FLAG_SEP_CPLUS_SPC) & ~XN_FLAG_SEP_COMMA_PLUS & ~ASN1_STRFLGS_ESC_MSB;
		return x509_name_string(name, flags);
	}
	
	std::string x509_name_reversed_string(const ::X509_NAME * name)
	{
		// XN_FLAG_DN_REV - for XML DSIG, names subject and issuers should be in reversed order
		constexpr int flags = (XN_FLAG_RFC2253 | XN_FLAG_DN_REV | XN_FLAG_SEP_CPLUS_SPC) & ~XN_FLAG_SEP_COMMA_PLUS & ~ASN1_STRFLGS_ESC_MSB;
		return x509_name_string(name, flags);
	}
	
	std::string bignum_string(const ::BIGNUM * num)
	{
		auto * str = ::BN_bn2dec(num);
		std::string result = str;
		::OPENSSL_free(str);
		return result;
	}
	
	std::string asn1_integer_string(const ::ASN1_INTEGER * integer)
	{
		auto * bn = ::ASN1_INTEGER_to_BN(integer, nullptr);
		auto result = bignum_string(bn);
		::BN_free(bn);

		return result;
	}
	
	std::string asn1_time_string(const ::ASN1_TIME * time)
	{
		constexpr unsigned buffsize = 128;
		char buffer[buffsize];
		auto t = asn1_time_timet(time);
		
		std::tm tm;
		ext::localtime(&t, &tm);
		auto written = std::strftime(buffer, buffsize, "%c", &tm);
		return std::string(buffer, written);
	}
	
	std::string asn1_time_print(const ::ASN1_TIME * time)
	{
		ext::openssl::bio_uptr mem_bio(::BIO_new(::BIO_s_mem()));
		if (1 != ::ASN1_TIME_print(mem_bio.get(), time))
			ext::openssl::throw_last_error("::ASN1_TIME_print failed");
		
		char * data;
		int len = BIO_get_mem_data(mem_bio.get(), &data);
		return std::string(data, len);
	}
		
	std::tm asn1_time_tm(const ::ASN1_TIME * time)
	{
		std::tm tm;
		if (1 != ::ASN1_TIME_to_tm(time, &tm))
			ext::openssl::throw_last_error("::ASN1_TIME_to_tm failed");
		
		return tm;
	}
	
	time_t asn1_time_timet(const ::ASN1_TIME * time)
	{
		// TODO: handle special case 99991231235959Z
		auto tm = asn1_time_tm(time);
		return ext::mkgmtime(&tm);
	}
	
	/************************************************************************/
	/*                  utility stuff                                       */
	/************************************************************************/
	static int password_callback(char * buff, int bufsize, int rwflag, void * userdata)
	{
		auto & passwd = *static_cast<std::string_view *>(userdata);
		passwd.copy(buff, bufsize);
		return 0;
	}

	x509_iptr load_certificate(const char * data, std::size_t len, std::string_view passwd)
	{
		bio_uptr bio_uptr;

		auto * bio = ::BIO_new_mem_buf(data, static_cast<int>(len));
		bio_uptr.reset(bio);
		if (not bio) throw_last_error("ext::openssl::load_certificate: ::BIO_new_mem_buf failed");

		X509 * cert = ::PEM_read_bio_X509(bio, nullptr, password_callback, &passwd);
		if (not cert) throw_last_error("ext::openssl::load_certificate: ::PEM_read_bio_X509 failed");
		return x509_iptr(cert, ext::noaddref);
	}

	evp_pkey_iptr load_private_key(const char * data, std::size_t len, std::string_view passwd)
	{
		bio_uptr bio_uptr;

		auto * bio = BIO_new_mem_buf(data, static_cast<int>(len));
		bio_uptr.reset(bio);
		if (not bio) throw_last_error("ext::openssl::load_private_key: ::BIO_new_mem_buf failed");

		EVP_PKEY * pkey = ::PEM_read_bio_PrivateKey(bio, nullptr, password_callback, &passwd);
		if (not bio) throw_last_error("ext::openssl::load_private_key: ::PEM_read_bio_PrivateKey failed");
		return evp_pkey_iptr(pkey, ext::noaddref);
	}

	x509_iptr load_certificate_from_file(const char * path, std::string_view passwd)
	{
#if BOOST_OS_WINDOWS
		auto wpath = ext::codecvt_convert::wchar_cvt::to_wchar(path);
		std::FILE * fp = ::_wfopen(wpath.c_str(), L"r");
#else
		std::FILE * fp = std::fopen(path, "r");
#endif
		if (fp == nullptr)
		{
			std::error_code errc(errno, std::generic_category());
			throw std::system_error(errc, "ext::openssl::load_certificate_from_file: std::fopen failed");
		}

		X509 * cert = ::PEM_read_X509(fp, nullptr, password_callback, &passwd);
		std::fclose(fp);

		if (not cert) throw_last_error("ext::openssl::load_certificate_from_file: ::PEM_read_X509 failed");
		return x509_iptr(cert, ext::noaddref);
	}

	x509_iptr load_certificate_from_file(const wchar_t * wpath, std::string_view passwd)
	{
#if not BOOST_OS_WINDOWS
		auto path = ext::codecvt_convert::wchar_cvt::to_utf8(wpath);
		std::FILE * fp = std::fopen(path.c_str(), "r");
#else
		std::FILE * fp = ::_wfopen(wpath, L"r");
#endif
		if (fp == nullptr)
		{
			std::error_code errc(errno, std::generic_category());
			throw std::system_error(errc, "ext::openssl::load_certificate_from_file: std::fopen failed");
		}

		X509 * cert = ::PEM_read_X509(fp, nullptr, password_callback, &passwd);
		std::fclose(fp);

		if (not cert) throw_last_error("ext::openssl::load_certificate_from_file: ::PEM_read_X509 failed");
		return x509_iptr(cert, ext::noaddref);
	}

	x509_iptr load_certificate_from_file(std::FILE * file, std::string_view passwd)
	{
		assert(file);
		X509 * cert = ::PEM_read_X509(file, nullptr, password_callback, &passwd);

		if (not cert) throw_last_error("ext::openssl::load_certificate_from_file: ::PEM_read_X509 failed");
		return x509_iptr(cert, ext::noaddref);
	}


	evp_pkey_iptr load_private_key_from_file(const char * path, std::string_view passwd)
	{
#if BOOST_OS_WINDOWS
		auto wpath = ext::codecvt_convert::wchar_cvt::to_wchar(path);
		std::FILE * fp = ::_wfopen(wpath.c_str(), L"r");
#else
		std::FILE * fp = std::fopen(path, "r");
#endif

		if (fp == nullptr)
		{
			std::error_code errc(errno, std::generic_category());
			throw std::system_error(errc, "ext::openssl::load_private_key_from_file: std::fopen failed");
		}

		//                  traditional or PKCS#8 format
		EVP_PKEY * pkey = ::PEM_read_PrivateKey(fp, nullptr, password_callback, &passwd);
		std::fclose(fp);

		if (not pkey) throw_last_error("ext::openssl::load_private_key_from_file: ::PEM_read_PrivateKey failed");
		return evp_pkey_iptr(pkey, ext::noaddref);
	}

	evp_pkey_iptr load_private_key_from_file(const wchar_t * wpath, std::string_view passwd)
	{
#if not BOOST_OS_WINDOWS
		auto path = ext::codecvt_convert::wchar_cvt::to_utf8(wpath);
		std::FILE * fp = std::fopen(path.c_str(), "r");
#else
		std::FILE * fp = ::_wfopen(wpath, L"r");
#endif

		if (fp == nullptr)
		{
			std::error_code errc(errno, std::generic_category());
			throw std::system_error(errc, "ext::openssl::load_private_key_from_file: std::fopen failed");
		}

		//                  traditional or PKCS#8 format
		EVP_PKEY * pkey = ::PEM_read_PrivateKey(fp, nullptr, password_callback, &passwd);
		std::fclose(fp);

		if (not pkey) throw_last_error("ext::openssl::load_private_key_from_file: ::PEM_read_PrivateKey failed");
		return evp_pkey_iptr(pkey, ext::noaddref);
	}

	evp_pkey_iptr load_private_key_from_file(std::FILE * file, std::string_view passwd)
	{
		assert(file);
		//                  traditional or PKCS#8 format
		EVP_PKEY * pkey = ::PEM_read_PrivateKey(file, nullptr, password_callback, &passwd);

		if (not pkey) throw_last_error("ext::openssl::load_private_key_from_file: ::PEM_read_PrivateKey failed");
		return evp_pkey_iptr(pkey, ext::noaddref);
	}

	pkcs12_uptr load_pkcs12(const char * data, std::size_t len)
	{
		bio_uptr source_bio(::BIO_new_mem_buf(data, static_cast<int>(len)));
		if (not source_bio) throw_last_error("ext::openssl::load_pkcs12: ::BIO_new_mem_buf failed");

		pkcs12_uptr pkcs12(::d2i_PKCS12_bio(source_bio.get(), nullptr));
		if (not pkcs12) throw_last_error("ext::openssl::load_pkcs12_from_file: ::d2i_PKCS12_fp failed");
		return pkcs12;
	}

	pkcs12_uptr load_pkcs12_from_file(const char * path)
	{
#if BOOST_OS_WINDOWS
		auto wpath = ext::codecvt_convert::wchar_cvt::to_wchar(path);
		std::FILE * fp = ::_wfopen(wpath.c_str(), L"r");
#else
		std::FILE * fp = std::fopen(path, "r");
#endif

		if (fp == nullptr)
		{
			std::error_code errc(errno, std::generic_category());
			throw std::system_error(errc, "ext::openssl::load_pkcs12_from_file: std::fopen failed");
		}

		pkcs12_uptr pkcs12(::d2i_PKCS12_fp(fp, nullptr));
		std::fclose(fp);

		if (not pkcs12) throw_last_error("ext::openssl::load_pkcs12_from_file: ::d2i_PKCS12_fp failed");
		return pkcs12;
	}

	pkcs12_uptr load_pkcs12_from_file(const wchar_t * wpath)
	{
#if not BOOST_OS_WINDOWS
		auto path = ext::codecvt_convert::wchar_cvt::to_utf8(wpath);
		std::FILE * fp = std::fopen(path.c_str(), "r");
#else
		std::FILE * fp = ::_wfopen(wpath, L"r");
#endif

		if (fp == nullptr)
		{
			std::error_code errc(errno, std::generic_category());
			throw std::system_error(errc, "ext::openssl::load_pkcs12_from_file: std::fopen failed");
		}

		pkcs12_uptr pkcs12(::d2i_PKCS12_fp(fp, nullptr));
		std::fclose(fp);

		if (not pkcs12) throw_last_error("ext::openssl::load_pkcs12_from_file: ::d2i_PKCS12_fp failed");
		return pkcs12;
	}

	pkcs12_uptr load_pkcs12_from_file(std::FILE * file)
	{
		assert(file);
		pkcs12_uptr pkcs12(::d2i_PKCS12_fp(file, nullptr));

		if (not pkcs12) throw_last_error("ext::openssl::load_pkcs12_from_file: ::d2i_PKCS12_fp failed");
		return pkcs12;
	}

	void parse_pkcs12(PKCS12 * pkcs12, std::string passwd, evp_pkey_iptr & evp_pkey, x509_iptr & x509, stackof_x509_uptr & ca)
	{
		X509 * raw_cert = nullptr;
		EVP_PKEY * raw_pkey = nullptr;
		STACK_OF(X509) * raw_ca = nullptr;

		int res = ::PKCS12_parse(pkcs12, passwd.c_str(), &raw_pkey, &raw_cert, &raw_ca);
		if (res <= 0) throw_last_error("ext::openssl::parse_pkcs12: ::PKCS12_parse failed");

		evp_pkey.reset(raw_pkey, ext::noaddref);
		x509.reset(raw_cert, ext::noaddref);
		ca.reset(raw_ca);
	}

	auto parse_pkcs12(PKCS12 * pkcs12, std::string passwd) -> std::tuple<evp_pkey_iptr, x509_iptr, stackof_x509_uptr>
	{
		std::tuple<evp_pkey_iptr, x509_iptr, stackof_x509_uptr> result;
		parse_pkcs12(pkcs12, passwd, std::get<0>(result), std::get<1>(result), std::get<2>(result));
		return result;
	}


	ssl_ctx_iptr create_sslctx(X509 * cert, EVP_PKEY * pkey, stack_st_X509 * ca_chain)
	{
		auto * method = ::SSLv23_server_method();
		return create_sslctx(method, cert, pkey, ca_chain);
	}

	ssl_ctx_iptr create_sslctx(const SSL_METHOD * method, X509 * cert, EVP_PKEY * pkey, stack_st_X509 * ca_chain)
	{
		auto * ctx = ::SSL_CTX_new(method);

		ssl_ctx_iptr ssl_ctx_iptr(ctx, ext::noaddref);

		if (::SSL_CTX_use_cert_and_key(ctx, cert, pkey, ca_chain, 1) != 1)
			throw_last_error("ext::openssl::create_sslctx: ::SSL_CTX_use_cert_and_key failed");

		return ssl_ctx_iptr;
	}

	ssl_ctx_iptr create_anonymous_sslctx()
	{
		auto * method = ::SSLv23_server_method();
		return create_anonymous_sslctx(method);
	}

	ssl_ctx_iptr create_anonymous_sslctx(const SSL_METHOD * method)
	{
		auto * ctx = ::SSL_CTX_new(method);

		ssl_ctx_iptr ssl_ctx_iptr(ctx, ext::noaddref);
		if (::SSL_CTX_set_cipher_list(ctx, "aNULL:eNULL") != 1)
			throw_last_error("ext::openssl::create_anonymous_sslctx: ::SSL_CTX_set_cipher_list failed");

		::DH * dh = ::DH_get_2048_256();
		::SSL_CTX_set_tmp_dh(ctx, dh);
		::DH_free(dh);

		return ssl_ctx_iptr;
	}
}

static unsigned double_digits_to_uint(const char *& str)
{
	unsigned n = 10 * (*str++ - '0');
	return n + (*str++ - '0');
}

#if OPENSSL_VERSION_NUMBER < 0x10101000L
static int ASN1_TIME_to_tm(const ::ASN1_TIME * time, std::tm * tm)
{
	if (not time) return -1;
	const char * str = reinterpret_cast<const char*>(time->data);
	if (not str) return -1;
	
	unsigned year, month, day, hour, min, sec;
	switch(time->type) {
		// https://tools.ietf.org/html/rfc5280#section-4.1.2.5.1
		case V_ASN1_UTCTIME: // YYMMDDHHMMSSZ
			year = double_digits_to_uint(str);
			year += year < 50 ? 2000 : 1900;
			break;
		case V_ASN1_GENERALIZEDTIME: // YYYYMMDDHHMMSSZ
			year  = 100 * double_digits_to_uint(str);
			year +=       double_digits_to_uint(str);
			break;
		default: return -1; // error
	}
	
	month = double_digits_to_uint(str);
	day   = double_digits_to_uint(str);
	hour  = double_digits_to_uint(str);
	min   = double_digits_to_uint(str);
	sec   = double_digits_to_uint(str);
	if (*str != 'Z') return -1;
	
	if (year == 9999 && month == 12 && day == 31 && hour == 23 && min == 59 && sec == 59) // 99991231235959Z rfc 5280
		return -1;
	
	std::memset(tm, 0, sizeof(*tm));
	tm->tm_year = year - 1900;
	tm->tm_mon  = month;
	tm->tm_mday = day;
	tm->tm_hour = hour;
	tm->tm_min  = min;
	tm->tm_sec  = sec;
	
	return 1;
}
#endif

#endif // #ifdef EXT_ENABLE_OPENSSL
