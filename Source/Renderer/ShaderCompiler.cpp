#include "Renderer/ShaderCompiler.h"
#include "Core/Log.h"
#include <algorithm>
#include <dxcapi.h>
#include <filesystem>
#include <fstream>

#define SHADER_MODEL_VS L"vs_6_0"
#define SHADER_MODEL_PS L"ps_6_0"
#define SHADER_MODEL_CS L"cs_6_0"

namespace {

	inline std::wstring ToWString(const char* Chars) {
		std::string CharsStr{ Chars };
		std::wstring CharStrW{ CharsStr.begin(), CharsStr.end() };
		return CharStrW;
	}

	inline std::wstring StringToWString(const std::string& Str) {
		return std::wstring{Str.begin(), Str.end()};
	}

	static DxcCreateInstanceProc GetDXCCreateInstanceProc() {
		static DxcCreateInstanceProc s_Proc{ nullptr };
		if(!s_Proc) {
#ifdef _WIN32
			HMODULE dxcModule = ::LoadLibrary("dxcompiler.dll");
			if (dxcModule == nullptr) {
				LOG_ERROR("Failed to load dxcompiler.dll");
				return nullptr;
			}
			s_Proc = (DxcCreateInstanceProc)GetProcAddress(dxcModule, "DxcCreateInstance");
#elif defined(__linux__)
			s_Proc = DxcCreateInstance;
#endif
		}
		return s_Proc;
	};

	static DxcCreateInstanceProc GetDXILCreateInstanceProc() {
		// load DXIL CreateInstance func
		static DxcCreateInstanceProc s_Proc{ nullptr };
		if(!s_Proc) {
#ifdef _WIN32
			HMODULE dxilModule = ::LoadLibrary("dxil.dll");
			if (dxilModule == nullptr) {
				LOG_WARNING("Failed to load dxil.dll");
				return nullptr;
			}
			s_Proc = (DxcCreateInstanceProc)GetProcAddress(dxilModule, "DxcCreateInstance");
#elif defined(__linux__)
			s_Proc = DxcCreateInstance;
#endif
		}
		return s_Proc;
	}

	template<typename T>
	struct TDXDeleter { void operator()(T* ptr) { ptr->Release(); } };

	template<typename T>
	using TDXCPtr = TUniquePtr<T, TDXDeleter<T>>;

	class IncludeHandler: public IDxcIncludeHandler {
	public:
		IncludeHandler(IDxcUtils* utils) : m_Utils(utils) {}
		~IncludeHandler() = default;
		HRESULT STDMETHODCALLTYPE LoadSource(
			_In_z_ LPCWSTR pFilename,                                 // Candidate filename.
			_COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource  // Resultant source object for included file, nullptr if not found.
		) override {
			std::wstring fullPath {SHADER_PATH_W};
			fullPath.append(pFilename);
			uint32_t codePage = DXC_CP_ACP;
			TDXCPtr<IDxcBlobEncoding> code;
			HRESULT r = m_Utils->LoadFile(fullPath.c_str(), &codePage, code.Address());
			if(SUCCEEDED(r)) {
				m_Utils->CreateBlobFromBlob(code.Get(), 0, code->GetBufferSize(), ppIncludeSource);
			}
			else {
				*ppIncludeSource = nullptr;
			}
			return r;
		}
		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override { return E_NOINTERFACE; }
		ULONG STDMETHODCALLTYPE AddRef(void) override { return 1; }
		ULONG STDMETHODCALLTYPE Release(void) override { return 1; }
	private:
		IDxcUtils* m_Utils;
	};

	class DXCompiler {
	public:
		virtual ~DXCompiler() = default;
		DXCompiler() {
			DxcCreateInstanceProc dxcCreateInstance = GetDXCCreateInstanceProc();
			HRESULT r;
			// Initialize DXC library
			r = dxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(Library.Address()));
			if (FAILED(r)) {
				LOG_WARNING("[DXCompiler] Could not init DXC Library");
				return;
			}
			// Initialize DXC compiler
			r = dxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(Compiler.Address()));
			if (FAILED(r)) {
				LOG_WARNING("[DXCompiler] Could not init DXC Compiler");
				return;
			}

			// Initialize DXC utility
			r = dxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(Utils.Address()));
			if (FAILED(r)) {
				LOG_WARNING("[DXCompiler] Could not init DXC Utiliy");
				return;
			}
		}
		void Compile(const char* code, uint32 byteSize, const char* EntryName, const char* ShaderModule, const std::vector<ShaderCompiler::Macro>& Defines) {
			DxcBuffer Buffer;
			Buffer.Encoding = CP_UTF8;
			Buffer.Ptr = code;
			Buffer.Size = byteSize;
			CompileInternal(Buffer, EntryName, ShaderModule, Defines);
		}
		
		void Compile(const std::filesystem::path& FullPath, const char* EntryName, const char* ShaderModule, const std::vector<ShaderCompiler::Macro>& Defines) {
			// Load the HLSL text shader from disk
			uint32_t CodePage = DXC_CP_ACP;
			TDXCPtr<IDxcBlobEncoding> SrcCode;
			HRESULT r = Utils->LoadFile(FullPath.wstring().c_str(), &CodePage, SrcCode.Address());
			if (FAILED(r)) {
				LOG_ERROR("[HLSLCompiler] Could not load shader file: %s", FullPath.string().c_str());
				return;
			}

			DxcBuffer Buffer;
			Buffer.Encoding = DXC_CP_ACP;
			Buffer.Ptr = SrcCode->GetBufferPointer();
			Buffer.Size = SrcCode->GetBufferSize();
			CompileInternal(Buffer, EntryName, ShaderModule, Defines);
		}

		bool GetResultCode(std::vector<int8>& OutCode) {
			OutCode.clear();
			if(!CompiledBlob) {
				return false;
			}
			OutCode.resize(CompiledBlob->GetBufferSize());
			memcpy(OutCode.data(), CompiledBlob->GetBufferPointer(), CompiledBlob->GetBufferSize());
			return true;
		}

		bool SaveFile(const std::string& FullPath) {
			if(!CompiledBlob) {
				return false;
			}

			std::ofstream WriteFile;
			WriteFile.open(FullPath, std::ios::out | std::ios::binary);
			if (WriteFile.is_open()) {
				WriteFile.write((const char*)CompiledBlob->GetBufferPointer(), (uint32)CompiledBlob->GetBufferSize());
				WriteFile.close();
				return true;
			}
			return false;
		}

	protected:
		struct DxilMinimalHeader {
			UINT32 FourCC;
			UINT32 HashDigest[4];
		};
		TDXCPtr<IDxcLibrary> Library;
		TDXCPtr<IDxcCompiler3> Compiler;
		TDXCPtr<IDxcUtils> Utils;
		TDXCPtr<IDxcValidator> Validator;
		TDXCPtr<IDxcResult> Result;
		TDXCPtr<IDxcBlob> CompiledBlob;

		virtual void FillPreArgs(std::vector<LPCWSTR>& OutPreArgs) {}

		virtual bool CompileInternal(const DxcBuffer& codeBuffer, const char* EntryName, const char* ShaderModule, const std::vector<ShaderCompiler::Macro>& Defines) {
			// parse macros
			std::vector<std::wstring> StringCache; StringCache.reserve(Defines.size() * 2);
			std::vector<DxcDefine> DxcDefines; DxcDefines.reserve(Defines.size());
			for (const auto& Define : Defines) {
				if (!Define.Name.empty()) {
					auto& dxDefine = DxcDefines.emplace_back();
					StringCache.push_back(StringToWString(Define.Name));
					dxDefine.Name = StringCache.back().c_str();
					if (Define.Value.empty()) {
						dxDefine.Value = nullptr;
					}
					else {
						StringCache.push_back(StringToWString(Define.Value));
						dxDefine.Value = StringCache.back().c_str();
					}
				}
			}

			CompiledBlob.Reset();
			HRESULT r;
			// build args
			const std::wstring EntryNameW = ToWString(EntryName);
			const std::wstring ShaderModuleW = ToWString(ShaderModule);
			TDXCPtr<IDxcCompilerArgs> CompilerArgs;
			std::vector<LPCWSTR> PreArgs;
			FillPreArgs(PreArgs);
			Utils->BuildArguments(nullptr, EntryNameW.c_str(), ShaderModuleW.c_str(), PreArgs.data(), (uint32)PreArgs.size(), nullptr, 0, CompilerArgs.Address());

			// include handler
			TUniquePtr<IncludeHandler> includeHandler(new IncludeHandler(Utils.Get())); // IncludeHandler has no destruction logic in Release, so use default deleter.

			r = Compiler->Compile(&codeBuffer, CompilerArgs->GetArguments(), CompilerArgs->GetCount(), includeHandler.Get(), IID_PPV_ARGS(Result.Address()));
			if (SUCCEEDED(r)) {
				Result->GetStatus(&r);
			}

			if (SUCCEEDED(r)) {
				r = Result->GetResult(CompiledBlob.Address());
			}

			// Output error if compilation failed
			if (FAILED(r) && (Result)) {
				IDxcBlobEncoding* errorBlob;
				r = Result->GetErrorBuffer(&errorBlob);
				if (SUCCEEDED(r) && errorBlob) {
					LOG_ERROR("[Shader Compile Error] %s", (const char*)errorBlob->GetBufferPointer());
				}
				return false;
			}
			return true;
		}
	};

	class DXCompilerSPV : public DXCompiler{
	protected:
		virtual void FillPreArgs(std::vector<LPCWSTR>& OutPreArgs) override {
			OutPreArgs.push_back(L"-spirv");
		}
	};

	class DXCompilerSigned: public DXCompiler {
	protected:
		virtual bool CompileInternal(const DxcBuffer& codeBuffer, const char* EntryName, const char* ShaderModule, const std::vector<ShaderCompiler::Macro>& Defines) override {
			if(DXCompiler::CompileInternal(codeBuffer, EntryName, ShaderModule, Defines)) {
				return SignCode(CompiledBlob.Get());
			}
			return false;
		}

		bool IsCodeSigned(IDxcBlob* Blob) {
			if (Blob->GetBufferSize() < sizeof(DxilMinimalHeader)) {
				return false;
			}
			DxilMinimalHeader* header = (DxilMinimalHeader*)Blob->GetBufferPointer();
			bool has_digest = false;
			has_digest |= header->HashDigest[0] != 0x0;
			has_digest |= header->HashDigest[1] != 0x0;
			has_digest |= header->HashDigest[2] != 0x0;
			has_digest |= header->HashDigest[3] != 0x0;
			return has_digest;
		}

		bool SignCode(IDxcBlob* Blob) {
			// =============== sign code for d3d12 ===============
			if (!Validator) {

				DxcCreateInstanceProc dxilCreateInstance = GetDXILCreateInstanceProc();
				HRESULT Result = dxilCreateInstance(CLSID_DxcValidator, IID_PPV_ARGS(Validator.Address()));
				if (FAILED(Result)) {
					LOG_WARNING("Failed to create validator!");
					return false;
				}
			}
			if (IsCodeSigned(Blob)) {
				return true;
			}
			TDXCPtr<IDxcBlobEncoding> containerBlob;
			Library->CreateBlobWithEncodingFromPinned(Blob->GetBufferPointer(), Blob->GetBufferSize(), 0 /* binary, no code page */, containerBlob.Address());

			// Query validation version info
			{
				TDXCPtr<IDxcVersionInfo> versionInfo;
				if (FAILED(Validator->QueryInterface(IID_PPV_ARGS(versionInfo.Address())))) {
					LOG_WARNING("Failed to query version info interface");
					return false;
				}

				UINT32 major = 0;
				UINT32 minor = 0;
				versionInfo->GetVersion(&major, &minor);
				LOG_DEBUG("Validator version: %u.%u", major, minor);
			}

			TDXCPtr<IDxcOperationResult> result;
			if (FAILED(Validator->Validate(containerBlob.Get(), DxcValidatorFlags_InPlaceEdit /* avoid extra copy owned by dxil.dll */, result.Address()))) {
				LOG_WARNING("Failed to validate dxil container");
				return false;
			}

			HRESULT validateStatus;
			if (FAILED(result->GetStatus(&validateStatus))) {
				LOG_WARNING("Failed to get dxil validate status");
				return false;
			}

			if (FAILED(validateStatus)) {
				LOG_WARNING("The dxil container failed validation");
				std::string errorString;
				TDXCPtr<IDxcBlobEncoding> printBlob, printBlobUtf8;
				result->GetErrorBuffer(printBlob.Address());

				Library->GetBlobAsUtf8(printBlob.Get(), printBlobUtf8.Address());
				if (printBlobUtf8) {
					errorString = (const char*)printBlobUtf8->GetBufferPointer();
				}
				LOG_WARNING("Error: %s", errorString.c_str());
				return false;
			}

			if (!IsCodeSigned(Blob)) {
				LOG_WARNING("Signing failed!");
				return false;
			}
			return true;

		}
	};
}

namespace ShaderCompiler {
	bool ReadCompiledShaderFile(const char* CompiledFullPath, const char* EntryName, const char* ShaderModule, std::vector<char>& OutBytes)	{
		if(!std::filesystem::exists(CompiledFullPath)) {
			return false;
		}

		// Read file to bytes
		std::ifstream ShaderFile;
		ShaderFile.open(CompiledFullPath, std::ios::ate | std::ios::binary);
		if (ShaderFile.is_open()) {
			const size_t FileSize = (size_t)ShaderFile.tellg();
			OutBytes.resize(FileSize);
			ShaderFile.seekg(0);
			ShaderFile.read(OutBytes.data(), (uint32)FileSize);
			ShaderFile.close();
		}
		else {
			LOG_WARNING("Failed to load compiled shader: %s", CompiledFullPath);
		}
		return false;
	}

	void CompileShaderToSPV(const char* ShaderFullPath, const char* EntryName, const char* ShderModule, const std::vector<Macro>& Defines, std::vector<char>& OutBytes) {
		DXCompilerSPV Compiler{};
		Compiler.Compile(ShaderFullPath, EntryName, ShderModule, Defines);
		Compiler.GetResultCode(OutBytes);
	}

	void CompileShaderSigned(const char* ShaderFullPath, const char* EntryName, const char* ShaderModule, const std::vector<Macro>& Defines, std::vector<char>& OutBytes) {
		DXCompilerSigned Compiler{};
		Compiler.Compile(ShaderFullPath, EntryName, ShaderModule, Defines);
		Compiler.GetResultCode(OutBytes);
	}

	bool SaveCompiledShaderFile(const char* CompiledFullPath, const std::vector<char>& Bytes) {
		if (Bytes.empty()) {
			return false;
		}

		std::ofstream WriteFile;
		WriteFile.open(CompiledFullPath, std::ios::out | std::ios::binary);
		if (WriteFile.is_open()) {
			WriteFile.write(Bytes.data(), Bytes.size());
			WriteFile.close();
			return true;
		}
		return false;
	}
}
