#pragma once
#include <string>
#include <vector>
#include "Core/TUniquePtr.h"
#include "Core/Defines.h"

namespace ShaderCompiler {
	struct Macro {
		std::string Name;
		std::string Value;
	};
	bool ReadCompiledShaderFile(const char* CompiledFullPath, const char* EntryName, const char* ShaderModule, std::vector<char>& OutBytes);
	void CompileShaderToSPV(const char* ShaderFullPath, const char* EntryName, const char* ShderModule, const std::vector<Macro>& Defines, std::vector<char>& OutBytes);
	void CompileShaderSigned(const char* ShaderFullPath, const char* EntryName, const char* ShaderModule, const std::vector<Macro>& Defines, std::vector<char>& OutBytes);
	bool SaveCompiledShaderFile(const char* CompiledFullPath, const std::vector<char>& Bytes);
}
