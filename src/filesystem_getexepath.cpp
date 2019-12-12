#include <ext/filesystem_utils.hpp>
#include <boost/predef.h>

/************************************************************************/
/*                   getexepath implementation                          */
/************************************************************************/
namespace ext
{
	static void getexepath(int argc, char *argv[], std::filesystem::path & path, std::error_code & ec);

	std::filesystem::path getexepath(int argc, char *argv[], std::error_code & ec)
	{
		std::filesystem::path path;
		getexepath(argc, argv, path, ec);
		return path;
	}

	std::filesystem::path getexepath(int argc, char *argv[])
	{
		std::error_code ec;
		std::filesystem::path path;
		getexepath(argc, argv, path, ec);
		if (ec) throw std::system_error(ec, "getexepath failed");

		return path;
	}

	/*
	namespace
	{
		static std::filesystem::path getexepath_via_command_line(int argc, char *argv[])
		{
			if (argc == 0)
				return "";

			std::filesystem::path zero = argv[0];
			// путь абсолютный, возвращаем как есть
			if (zero.is_absolute())
				return zero;

			if (zero.has_parent_path())
				return absolute(zero);

			return "";
		}

		static std::filesystem::path getexepath_via_environment(int argc, char *argv[])
		{
			if (argc == 0)
				return "";

			std::filesystem::path zero = argv[0];

			//ищем через path
			std::string path = getenv("PATH");
			if (path.empty())
				return "";

			for (auto beg = path.begin(), end = std::find(beg, path.end(), ';');
				 beg != end;
				 beg = end, end = std::find(beg, end, ';'))
			{
				std::filesystem::path inpath(beg, end);
				inpath /= zero;
				if (std::filesystem::exists(inpath))
					return inpath;
			}

			return "";
		}
	}
	*/
}

/*
http://stackoverflow.com/questions/1023306/finding-current-executables-path-without-proc-self-exe

Some OS-specific interfaces:

Mac OS X: _NSGetExecutablePath() (man 3 dyld)
Linux: readlink /proc/self/exe
Solaris: getexecname()
FreeBSD: sysctl CTL_KERN KERN_PROC KERN_PROC_PATHNAME -1
FreeBSD if it has procfs: readlink /proc/curproc/file (FreeBSD doesn't have procfs by default)
NetBSD: readlink /proc/curproc/exe
DragonFly BSD: readlink /proc/curproc/file
Windows: GetModuleFileName() with hModule = NULL

The portable (but less reliable) method is to use argv[0].
Although it could be set to anything by the calling program,
by convention it is set to either a path name of the executable or a name that was found using $PATH.

Some shells, including bash and ksh,
set the environment variable "_" to the full path of the executable before it is executed.
In that case you can use getenv("_") to get it.
However this is unreliable because not all shells do this,
and it could be set to anything or be left over from a parent process
which did not change it before executing your program.
*/


#if BOOST_OS_WINDOWS || BOOST_OS_CYGWIN
#include <windows.h>

namespace ext
{
	void getexepath(int argc, char *argv[], std::filesystem::path & path, std::error_code & ec)
	{
		std::vector<wchar_t> buffer(MAX_PATH);

		DWORD sz = ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
		while (buffer.size() == sz) // if buffer size is not enough, increase by 2
		{
			buffer.resize(buffer.size() * 2);
			sz = ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
		}

		if (!sz)
			ec.assign(::GetLastError(), std::system_category());
		else
			path.assign(buffer.data(), buffer.data() + sz);

	}
}

#elif BOOST_OS_LINUX
#include <unistd.h>
#include <limits.h>
#include <linux/limits.h>

namespace ext
{
	void getexepath(int argc, char *argv[], std::filesystem::path & path, std::error_code & ec)
	{
		char buffer[PATH_MAX];
		
		ssize_t len = ::readlink("/proc/self/exe", buffer, sizeof(buffer));
		if (len == -1 || len == sizeof(buffer))
			ec.assign(errno, std::system_category());
		else
			path.assign(buffer, buffer + len);
	}
}

#elif BOOST_OS_MACOS
#include <mach-o/dyld.h>

namespace ext
{
	void getexepath(int argc, char *argv[], std::filesystem::path & path, std::error_code & ec)
	{
		char buffer[PATH_MAX];
		uint32_t len = sizeof(buffer);

		if (_NSGetExecutablePath(buffer, &len) != 0)
		{
			ec.assign(errno, std::system_category());
		}
		else
		{
			// resolve symlinks, ., .. if possible
			auto * canonicalPath = ::realpath(buffer, NULL);
			if (canonicalPath != NULL)
			{
				strncpy(buffer, canonicalPath, len);
				free(canonicalPath);
				path.assign(buffer, buffer + len);
			}
		}
	}
}

#elif BOOST_OS_SOLARIS
#include <stdlib.h>
#include <limits.h>

namespace ext
{
	void getexepath(int argc, char *argv[], std::filesystem::path & path, std::error_code & ec)
	{
		char buffer[PATH_MAX];
		if (::realpath(getexecname(), buffer) == NULL)
			ec.assign(errno, std::generic_category());
		else
			path.assign(buffer);
	}
}

#elif BOOST_OS_BSD_FREE
#include <sys/types.h>
#include <sys/sysctl.h>

namespace ext
{
	void getexepath(int argc, char *argv[], std::filesystem::path & path, std::error_code & ec)
	{
		int mib[4];
		mib[0] = CTL_KERN;
		mib[1] = KERN_PROC;
		mib[2] = KERN_PROC_PATHNAME;
		mib[3] = -1;

		char buffer[2048];
		size_t len = sizeof(buffer);

		if (sysctl(mib, 4, buffer, &len, NULL, 0) != 0)
			ec.assign(errno, std::generic_category());
		else
			path.assign(buffer, buffer + len);
	}
}

#elif BOOST_OS_HPUX
#include <sys/pstat.h>

namespace ext
{
	void getexepath(int argc, char *argv[], std::filesystem::path & path, std::error_code & ec)
	{
		struct pst_status pst;
		std::memset(&pst, 0, sizeof(pst));

		int res, pid = ::getpid();
		// Acquire proc info, it contains file id of text file(executable)
		res = ::pstat_getproc(&pst, sizeof(pst), 0, pid);
		if (res < 0)
		{
			ec.assign(errno, std::generic_category());
			return;
		}

		pst_fid * fid_text = &pst.pst_fid_text;
		char buffer[PATH_MAX];

		// Now get pathname. According to man pstat_getpathname returns name from system cache,
		// and it actually can be missing this information.
		// In this case function will return 0, errno will be unchanged
		res = ::pstat_getpathname(buffer, PATH_MAX, fid_text);
		if (res < 0)
		{
			ec.assign(errno, std::generic_category());
			return;
		}

		path.assign(buffer, buffer + res);
	}
}

#else

#error(Not implmented)

#endif
