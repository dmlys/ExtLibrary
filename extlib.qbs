import qbs
import qbs.Environment


Project
{
	property bool with_zlib: false

	StaticLibrary
	{
		Depends { name: "cpp" }
		Depends { name: "ProjectSettings"; required: false }

		cpp.cxxLanguageVersion : "c++17"
		cpp.cxxFlags: project.additionalCxxFlags
		cpp.driverFlags: project.additionalDriverFlags
		//cpp.defines: project.additionalDefines
		cpp.systemIncludePaths: project.additionalSystemIncludePaths
		cpp.includePaths: ["include"].concat(project.additionalIncludePaths || [])
		cpp.libraryPaths: project.additionalLibraryPaths

		cpp.defines: {
			var defines = []

			if (project.with_zlib)
				defines.push("EXT_ENABLE_CPPZLIB")

			if (project.additionalDefines)
				defines = defines.uniqueConcat(project.additionalDefines)

			return defines
		}

		Export
		{
			property bool with_zlib: project.with_zlib
			
			Depends { name: "cpp" }
			cpp.cxxLanguageVersion : "c++17"

			cpp.includePaths: ["include"]
			cpp.defines: {
				var defines = []

				if (project.with_zlib)
					defines.push("EXT_ENABLE_CPPZLIB")

				return defines;
			}
		}

		files: [
			"include/ext/**",
			"src/**",
		]
	}
}
