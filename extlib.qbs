import qbs
import qbs.Environment

Project
{
	property bool with_zlib: false
	property bool with_openssl: false

	StaticLibrary
	{
		Depends { name: "cpp" }
		Depends { name: "dmlys.qbs-common"; required: false }
		Depends { name: "ProjectSettings"; required: false }

		cpp.cxxLanguageVersion : "c++17"
		cpp.includePaths: ["include"]

		cpp.defines: {
			var defines = []

			if (project.with_zlib)
				defines.push("EXT_ENABLE_CPPZLIB")

			if (project.with_openssl)
				defines.push("EXT_ENABLE_OPENSSL")

			return defines
		}

		Export
		{
			property bool with_zlib: project.with_zlib
			property bool with_openssl: project.with_openssl
			
			Depends { name: "cpp" }
			cpp.cxxLanguageVersion : "c++17"

			cpp.includePaths: ["include"]
			cpp.defines: {
				var defines = []

				if (project.with_zlib)
					defines.push("EXT_ENABLE_CPPZLIB")
				
				if (project.with_openssl)
					defines.push("EXT_ENABLE_OPENSSL")

				return defines;
			}
		}

		files: [
			"include/ext/**",
			"src/**",
		]
	}
}
