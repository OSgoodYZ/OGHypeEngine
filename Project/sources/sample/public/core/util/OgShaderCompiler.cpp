#include "OgShaderCompiler.h"
#include "slang.h"
#include "slang-com-ptr.h"
#include <iostream>
#include <cstring>

#include "render/OgRenderContext.h"

using namespace Og::Render;

OG_NAMESPACE_SAMPLE_BEGIN

// COM 스마트 포인터 사용
template<typename T>
using ComPtr = Slang::ComPtr<T>;

std::string OgShaderCompiler::s_lastError;

// Helper function to convert shader type to Slang stage
static SlangStage ConvertShaderTypeToStage(OgShaderType shaderType)
{
	switch (shaderType)
	{
	case OgShaderType::VERTEX:                  return SLANG_STAGE_VERTEX;
	case OgShaderType::FRAGMENT:                return SLANG_STAGE_FRAGMENT;
	case OgShaderType::COMPUTE:                 return SLANG_STAGE_COMPUTE;
	case OgShaderType::GEOMETRY:                return SLANG_STAGE_GEOMETRY;
	case OgShaderType::TESSELLATION_CONTROL:    return SLANG_STAGE_HULL;
	case OgShaderType::TESSELLATION_EVALUATION: return SLANG_STAGE_DOMAIN;
	case OgShaderType::RAYGEN:                  return SLANG_STAGE_RAY_GENERATION;
	case OgShaderType::ANY_HIT:                 return SLANG_STAGE_ANY_HIT;
	case OgShaderType::CLOSEST_HIT:             return SLANG_STAGE_CLOSEST_HIT;
	case OgShaderType::MISS:                    return SLANG_STAGE_MISS;
	case OgShaderType::INTERSECTION:            return SLANG_STAGE_INTERSECTION;
	case OgShaderType::CALLABLE:                return SLANG_STAGE_CALLABLE;
	default:                                    return SLANG_STAGE_NONE;
	}
}

bool OgShaderCompiler::CompileGLSLtoSPIRV(
	const char* shaderCode,
	Render::OgShaderType shaderType,
	std::vector<uint32_t>& spirvOut)
{
	// Slang 글로벌 세션 생성
	SlangGlobalSessionDesc globalSessionDesc = {};
	globalSessionDesc.structureSize = sizeof(SlangGlobalSessionDesc);
	globalSessionDesc.enableGLSL = true;  // GLSL 지원 활성화

	ComPtr<slang::IGlobalSession> globalSession;
	if (SLANG_FAILED(slang::createGlobalSession(&globalSessionDesc, globalSession.writeRef())))
	{
		s_lastError = "Failed to create Slang global session";
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}

	// 타겟 설정
	slang::TargetDesc targetDesc = {};
	targetDesc.structureSize = sizeof(slang::TargetDesc);
	targetDesc.format = SLANG_SPIRV;
	targetDesc.profile = globalSession->findProfile("spirv_1_6+vulkan_1_4");



	std::vector<slang::CompilerOptionEntry>   options;
	options.push_back({ slang::CompilerOptionName::EmitSpirvDirectly, {slang::CompilerOptionValueKind::Int, 1} });
	options.push_back({ slang::CompilerOptionName::VulkanUseEntryPointName, {slang::CompilerOptionValueKind::Int, 1} });
	options.push_back({ slang::CompilerOptionName::DebugInformation, {slang::CompilerOptionValueKind::Int, 1} });
	options.push_back({ slang::CompilerOptionName::DebugInformation, {slang::CompilerOptionValueKind::Int, 0} });



	// 세션 생성
	slang::SessionDesc sessionDesc = {};
	sessionDesc.structureSize = sizeof(slang::SessionDesc);
	sessionDesc.targets = &targetDesc;
	sessionDesc.targetCount = 1;
	sessionDesc.allowGLSLSyntax = true;
	sessionDesc.compilerOptionEntries = options.data();
	sessionDesc.compilerOptionEntryCount = uint32_t(options.size());

	ComPtr<slang::ISession> session;
	if (SLANG_FAILED(globalSession->createSession(sessionDesc, session.writeRef())))
	{
		s_lastError = "Failed to create session";
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}

	// GLSL 소스코드를 모듈로 로드
	ComPtr<slang::IBlob> diagnostics;
	ComPtr<slang::IModule> module;
	module = session->loadModuleFromSourceString(
		"glsl_shader",
		"shader.glsl",
		shaderCode,
		diagnostics.writeRef()
	);

	logAndAppendDiagnostics(diagnostics);

	if (!module)
	{
		s_lastError = "Failed to load GLSL module";
		if (diagnostics)
		{
			s_lastError += ": ";
			s_lastError += (const char*)diagnostics->getBufferPointer();
		}
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}

	// 셰이더 스테이지 변환
	SlangStage stage = ConvertShaderTypeToStage(shaderType);
	if (stage == SLANG_STAGE_NONE)
	{
		s_lastError = "Unsupported shader type";
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}

	// 진입점 찾기
	ComPtr<slang::IEntryPoint> entryPoint;
	if (SLANG_FAILED(module->findAndCheckEntryPoint(
		"main",
		stage,
		entryPoint.writeRef(),
		diagnostics.writeRef())))
	{
		s_lastError = "Failed to find entry point 'main'";
		if (diagnostics)
		{
			s_lastError += ": ";
			s_lastError += (const char*)diagnostics->getBufferPointer();
		}
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}

	// 컴포넌트 타입 생성
	slang::IComponentType* components[] = { module, entryPoint };
	ComPtr<slang::IComponentType> compositeProgram;
	if (SLANG_FAILED(session->createCompositeComponentType(
		components,
		2,
		compositeProgram.writeRef(),
		diagnostics.writeRef())))
	{
		s_lastError = "Failed to create composite component type";
		if (diagnostics)
		{
			s_lastError += ": ";
			s_lastError += (const char*)diagnostics->getBufferPointer();
		}
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}

	if (!compositeProgram)
	{
		return false;
	}

	// 프로그램 링크
	ComPtr<slang::IComponentType> linkedProgram;
	if (SLANG_FAILED(compositeProgram->link(
		linkedProgram.writeRef(),
		diagnostics.writeRef())))
	{
		s_lastError = "Failed to link program";
		if (diagnostics)
		{
			s_lastError += ": ";
			s_lastError += (const char*)diagnostics->getBufferPointer();
		}
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}


	// SPIR-V 코드 생성
	ComPtr<slang::IBlob> spirvCode;

	//linkedProgram->getTargetCode(
	//	0, // 타겟 인덱스 (0은 첫 번째 타겟)
	//	nullptr,
	//	nullptr
	//);

	if (SLANG_FAILED(linkedProgram->getTargetCode(
		0, spirvCode.writeRef(),
		diagnostics.writeRef())))
	{
		s_lastError = "Failed to generate SPIR-V code";
		if (diagnostics)
		{
			s_lastError += ": ";
			s_lastError += (const char*)diagnostics->getBufferPointer();
		}
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}

	size_t codeSize = spirvCode->getBufferSize();
	size_t wordCount = codeSize / sizeof(uint32_t);
	const uint32_t* spirvWords = reinterpret_cast<const uint32_t*>(spirvCode->getBufferPointer());
	spirvOut.resize(wordCount);
	std::memcpy(spirvOut.data(), spirvWords, codeSize);

	return true;
}

Render::OgShaderHandle* OgShaderCompiler::CreateShaderFromGLSL(
	Render::OgRenderContext* renderContext,
	const char* shaderCode,
	Render::OgShaderType shaderType,
	const char* shaderName)
{
	std::vector<uint32_t> spirvCode;

	if (!CompileGLSLtoSPIRV(shaderCode, shaderType, spirvCode))
	{
		LOGE(OG_ID, "Failed to compile GLSL shader: %s", s_lastError.c_str());
		return nullptr;
	}

	OgShaderHandle* shader = renderContext->CreateShader(
		shaderType,
		reinterpret_cast<const char*>(spirvCode.data()),
		spirvCode.size() * sizeof(uint32_t),
		"main");

	if (shader && shaderName)
	{
		shader->name = shaderName;
	}

	return shader;
}

Render::OgProgramHandle* OgShaderCompiler::CreateProgramFromGLSL(
	Render::OgRenderContext* renderContext,
	const char* vertexShaderCode,
	const char* fragmentShaderCode,
	const char* programName)
{
	// 버텍스 셰이더 컴파일
	OgShaderHandle* vertexShader = CreateShaderFromGLSL(
		renderContext,
		vertexShaderCode,
		OgShaderType::VERTEX,
		programName ? (std::string(programName) + "_VS").c_str() : nullptr
	);

	if (!vertexShader)
	{
		return nullptr;
	}
	vertexShader->Retain();

	// 프래그먼트 셰이더 컴파일
	OgShaderHandle* fragmentShader = CreateShaderFromGLSL(
		renderContext,
		fragmentShaderCode,
		OgShaderType::FRAGMENT,
		programName ? (std::string(programName) + "_FS").c_str() : nullptr
	);

	if (!fragmentShader)
	{
		renderContext->DestroyShader(vertexShader);
		return nullptr;
	}
	fragmentShader->Retain();

	// 프로그램 생성
	OgShaderHandle* shaders[2] = { vertexShader, fragmentShader };
	OgProgramHandle* program = renderContext->CreateProgram(shaders, 2);

	if (program && programName)
	{
		program->name = programName;
	}

	return program;
}

bool OgShaderCompiler::CompileSlangToSPIRV(
	const char* shaderCode,
	Render::OgShaderType shaderType,
	std::vector<uint32_t>& spirvOut)
{
	// Slang 글로벌 세션 생성
	SlangGlobalSessionDesc globalSessionDesc = {};
	globalSessionDesc.structureSize = sizeof(SlangGlobalSessionDesc);

	ComPtr<slang::IGlobalSession> globalSession;
	if (SLANG_FAILED(slang::createGlobalSession(&globalSessionDesc, globalSession.writeRef())))
	{
		s_lastError = "Failed to create Slang global session";
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}

	// 타겟 설정
	slang::TargetDesc targetDesc = {};
	targetDesc.structureSize = sizeof(slang::TargetDesc);
	targetDesc.format = SLANG_SPIRV;
	targetDesc.profile = globalSession->findProfile("spirv_1_5");

	// Ray tracing 셰이더의 경우 추가 설정
	if (shaderType >= OgShaderType::RAYGEN && shaderType <= OgShaderType::CALLABLE)
	{
		// Ray tracing extensions 활성화를 위한 프로파일
		targetDesc.profile = globalSession->findProfile("spirv_1_5+spv1.4");
	}

	// 세션 생성
	slang::SessionDesc sessionDesc = {};
	sessionDesc.structureSize = sizeof(slang::SessionDesc);
	sessionDesc.targets = &targetDesc;
	sessionDesc.targetCount = 1;

	ComPtr<slang::ISession> session;
	if (SLANG_FAILED(globalSession->createSession(sessionDesc, session.writeRef())))
	{
		s_lastError = "Failed to create session";
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}

	// Slang 소스코드를 모듈로 로드
	ComPtr<slang::IBlob> diagnostics;
	ComPtr<slang::IModule> module;
	module = session->loadModuleFromSourceString(
		"slang_shader",
		"shader.slang",
		shaderCode,
		diagnostics.writeRef()
	);

	if (!module)
	{
		s_lastError = "Failed to load Slang module";
		if (diagnostics)
		{
			s_lastError += ": ";
			s_lastError += (const char*)diagnostics->getBufferPointer();
		}
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}

	// 셰이더 스테이지 변환
	SlangStage stage = ConvertShaderTypeToStage(shaderType);
	if (stage == SLANG_STAGE_NONE)
	{
		s_lastError = "Unsupported shader type";
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}

	// 진입점 찾기
	ComPtr<slang::IEntryPoint> entryPoint;
	if (SLANG_FAILED(module->findAndCheckEntryPoint(
		"main",
		stage,
		entryPoint.writeRef(),
		diagnostics.writeRef())))
	{
		s_lastError = "Failed to find entry point 'main'";
		if (diagnostics)
		{
			s_lastError += ": ";
			s_lastError += (const char*)diagnostics->getBufferPointer();
		}
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}

	// 컴포넌트 타입 생성
	slang::IComponentType* components[] = { module, entryPoint };
	ComPtr<slang::IComponentType> compositeProgram;
	if (SLANG_FAILED(session->createCompositeComponentType(
		components,
		2,
		compositeProgram.writeRef(),
		diagnostics.writeRef())))
	{
		s_lastError = "Failed to create composite component type";
		if (diagnostics)
		{
			s_lastError += ": ";
			s_lastError += (const char*)diagnostics->getBufferPointer();
		}
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}

	// 프로그램 링크
	ComPtr<slang::IComponentType> linkedProgram;
	if (SLANG_FAILED(compositeProgram->link(
		linkedProgram.writeRef(),
		diagnostics.writeRef())))
	{
		s_lastError = "Failed to link program";
		if (diagnostics)
		{
			s_lastError += ": ";
			s_lastError += (const char*)diagnostics->getBufferPointer();
		}
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}

	// SPIR-V 코드 생성
	ComPtr<slang::IBlob> spirvCode;
	if (SLANG_FAILED(linkedProgram->getEntryPointCode(
		0, 0,
		spirvCode.writeRef(),
		diagnostics.writeRef())))
	{
		s_lastError = "Failed to generate SPIR-V code";
		if (diagnostics)
		{
			s_lastError += ": ";
			s_lastError += (const char*)diagnostics->getBufferPointer();
		}
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}

	// SPIR-V 코드를 출력 벡터에 복사
	size_t codeSize = spirvCode->getBufferSize();
	size_t wordCount = codeSize / sizeof(uint32_t);
	const uint32_t* spirvWords = reinterpret_cast<const uint32_t*>(spirvCode->getBufferPointer());
	spirvOut.resize(wordCount);
	std::memcpy(spirvOut.data(), spirvWords, codeSize);

	return true;
}

Render::OgShaderHandle* OgShaderCompiler::CreateShaderFromSlang(
	Render::OgRenderContext* renderContext,
	const char* shaderCode,
	Render::OgShaderType shaderType,
	const char* shaderName)
{
	std::vector<uint32_t> spirvCode;

	if (!CompileSlangToSPIRV(shaderCode, shaderType, spirvCode))
	{
		LOGE(OG_ID, "Failed to compile Slang shader: %s", s_lastError.c_str());
		return nullptr;
	}

	OgShaderHandle* shader = renderContext->CreateShader(
		shaderType,
		reinterpret_cast<const char*>(spirvCode.data()),
		spirvCode.size() * sizeof(uint32_t),
		"main");

	if (shader && shaderName)
	{
		shader->name = shaderName;
	}

	return shader;
}

void OgShaderCompiler::logAndAppendDiagnostics(slang::IBlob* diagnostics)
{
	s_lastError.clear();
	if (diagnostics)
	{
		const char* message = reinterpret_cast<const char*>(diagnostics->getBufferPointer());

		// Append onto m_lastDiagnosticMessage, separated by a newline:
		if (s_lastError.empty())
		{
			s_lastError += '\n';
		}
		s_lastError += message;
		LOGE(OG_ID, "Failed to compile Slang shader: %s", s_lastError.c_str());
	}
}

OG_NAMESPACE_SAMPLE_END
