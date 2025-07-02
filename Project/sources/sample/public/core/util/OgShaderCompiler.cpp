#include "OgShaderCompiler.h"
#include "slang.h"
#include <iostream>

#include "render/OgRenderContext.h"

using namespace Og::Render;

OG_NAMESPACE_SAMPLE_BEGIN

std::string OgShaderCompiler::s_lastError;

bool OgShaderCompiler::CompileGLSLtoSPIRV(
	const char* shaderCode,
	Render::OgShaderType shaderType,
	std::vector<uint32_t>& spirvOut)
{
	// Slang 세션 생성
	SlangGlobalSessionDesc globalSessionDesc = {};
	globalSessionDesc.enableGLSL = true;  // GLSL 지원 활성화

	slang::IGlobalSession* slangSession = nullptr;
	if (SLANG_FAILED(slang::createGlobalSession(&globalSessionDesc, &slangSession)))
	{
		s_lastError = "Failed to create Slang session";
		LOGD(OG_ID,"%s",s_lastError.c_str());
		return false;
	}

	// 세션 생성
	slang::SessionDesc sessionDesc = {};
	slang::ISession* session = nullptr;
	if (SLANG_FAILED(slangSession->createSession(sessionDesc, &session)))
	{
		s_lastError = "Failed to create session";
		slangSession->release();
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}

	// 컴파일 요청 생성
	SlangCompileRequest* slangRequest = nullptr;
	if (SLANG_FAILED(session->createCompileRequest(&slangRequest)))
	{
		s_lastError = "Failed to create compile request";
		session->release();
		slangSession->release();
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}

	// 셰이더 타입에 따른 프로파일 설정
	SlangStage slangStage = SLANG_STAGE_NONE;

	switch (shaderType)
	{
	case OgShaderType::VERTEX:
		slangStage = SLANG_STAGE_VERTEX;
		break;
	case OgShaderType::FRAGMENT:
		slangStage = SLANG_STAGE_FRAGMENT;
		break;
	case OgShaderType::COMPUTE:
		slangStage = SLANG_STAGE_COMPUTE;
		break;
	case OgShaderType::GEOMETRY:
		slangStage = SLANG_STAGE_GEOMETRY;
		break;
	case OgShaderType::TESSELLATION_CONTROL:
		slangStage = SLANG_STAGE_HULL;
		break;
	case OgShaderType::TESSELLATION_EVALUATION:
		slangStage = SLANG_STAGE_DOMAIN;
		break;
	case OgShaderType::RAYGEN:
		slangStage = SLANG_STAGE_RAY_GENERATION;
		break;
	case OgShaderType::ANY_HIT:
		slangStage = SLANG_STAGE_ANY_HIT;
		break;
	case OgShaderType::CLOSEST_HIT:
		slangStage = SLANG_STAGE_CLOSEST_HIT;
		break;
	case OgShaderType::MISS:
		slangStage = SLANG_STAGE_MISS;
		break;
	case OgShaderType::INTERSECTION:
		slangStage = SLANG_STAGE_INTERSECTION;
		break;
	case OgShaderType::CALLABLE:
		slangStage = SLANG_STAGE_CALLABLE;
		break;
	default:
		s_lastError = "Unsupported shader type";
		LOGD(OG_ID, "%s", s_lastError.c_str());
		spDestroyCompileRequest(slangRequest);
		session->release();
		slangSession->release();
		return false;
	}

	// SPIR-V 타겟 추가
	spAddCodeGenTarget(slangRequest, SLANG_SPIRV);

	// GLSL 소스코드 추가
	int translationUnitIndex = spAddTranslationUnit(slangRequest, SLANG_SOURCE_LANGUAGE_GLSL, nullptr);
	spAddTranslationUnitSourceString(
		slangRequest,
		translationUnitIndex,
		"shader.glsl",
		shaderCode
	);

	// 진입점 추가
	spAddEntryPoint(
		slangRequest,
		translationUnitIndex,
		"main",
		slangStage
	);

	// 컴파일 실행
	int compileResult = spCompile(slangRequest);
	if (compileResult != 0)
	{
		// 컴파일 오류 출력
		const char* diagnostics = spGetDiagnosticOutput(slangRequest);
		s_lastError = "Shader compilation failed: ";
		s_lastError += diagnostics;
		spDestroyCompileRequest(slangRequest);
		session->release();
		slangSession->release();
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}

	// SPIR-V 코드 추출
	size_t codeSize = 0;
	void const* spirvCode = spGetEntryPointCode(slangRequest, 0, &codeSize);
	if (!spirvCode || codeSize == 0)
	{
		s_lastError = "Failed to get compiled SPIR-V code";
		spDestroyCompileRequest(slangRequest);
		session->release();
		slangSession->release();
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}

	// 결과를 출력 버퍼에 복사
	size_t wordCount = codeSize / sizeof(uint32_t);
	const uint32_t* spirvWords = reinterpret_cast<const uint32_t*>(spirvCode);
	spirvOut.resize(wordCount);
	memcpy(spirvOut.data(), spirvWords, codeSize);

	// 리소스 정리
	spDestroyCompileRequest(slangRequest);
	session->release();
	slangSession->release();

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
	// Slang 세션 생성
	SlangGlobalSessionDesc globalSessionDesc = {};
	
	slang::IGlobalSession* slangSession = nullptr;
	if (SLANG_FAILED(slang::createGlobalSession(&globalSessionDesc, &slangSession)))
	{
		s_lastError = "Failed to create Slang session";
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}

	// 타겟 설정
	slang::TargetDesc targetDesc = {};
	targetDesc.format = SLANG_SPIRV;
	targetDesc.profile = slangSession->findProfile("spirv_1_5");
	
	// 세션 생성
	slang::SessionDesc sessionDesc = {};
	sessionDesc.structureSize = sizeof(slang::SessionDesc);
	sessionDesc.targets = &targetDesc;
	sessionDesc.targetCount = 1;
	
	slang::ISession* session = nullptr;
	if (SLANG_FAILED(slangSession->createSession(sessionDesc, &session)))
	{
		s_lastError = "Failed to create session";
		slangSession->release();
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}

	// 컴파일 요청 생성
	SlangCompileRequest* slangRequest = nullptr;
	if (SLANG_FAILED(session->createCompileRequest(&slangRequest)))
	{
		s_lastError = "Failed to create compile request";
		session->release();
		slangSession->release();
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}

	// 셰이더 타입에 따른 스테이지 설정
	SlangStage slangStage = SLANG_STAGE_NONE;
	const char* profileName = nullptr;

	switch (shaderType)
	{
	case OgShaderType::VERTEX:
		slangStage = SLANG_STAGE_VERTEX;
		profileName = "spirv_1_5";
		break;
	case OgShaderType::FRAGMENT:
		slangStage = SLANG_STAGE_FRAGMENT;
		profileName = "spirv_1_5";
		break;
	case OgShaderType::COMPUTE:
		slangStage = SLANG_STAGE_COMPUTE;
		profileName = "spirv_1_5";
		break;
	case OgShaderType::GEOMETRY:
		slangStage = SLANG_STAGE_GEOMETRY;
		profileName = "spirv_1_5";
		break;
	case OgShaderType::TESSELLATION_CONTROL:
		slangStage = SLANG_STAGE_HULL;
		profileName = "spirv_1_5";
		break;
	case OgShaderType::TESSELLATION_EVALUATION:
		slangStage = SLANG_STAGE_DOMAIN;
		profileName = "spirv_1_5";
		break;
	case OgShaderType::RAYGEN:
		slangStage = SLANG_STAGE_RAY_GENERATION;
		profileName = "spirv_1_5";
		break;
	case OgShaderType::ANY_HIT:
		slangStage = SLANG_STAGE_ANY_HIT;
		profileName = "spirv_1_5";
		break;
	case OgShaderType::CLOSEST_HIT:
		slangStage = SLANG_STAGE_CLOSEST_HIT;
		profileName = "spirv_1_5";
		break;
	case OgShaderType::MISS:
		slangStage = SLANG_STAGE_MISS;
		profileName = "spirv_1_5";
		break;
	case OgShaderType::INTERSECTION:
		slangStage = SLANG_STAGE_INTERSECTION;
		profileName = "spirv_1_5";
		break;
	case OgShaderType::CALLABLE:
		slangStage = SLANG_STAGE_CALLABLE;
		profileName = "spirv_1_5";
		break;
	default:
		s_lastError = "Unsupported shader type";
		LOGD(OG_ID, "%s", s_lastError.c_str());
		spDestroyCompileRequest(slangRequest);
		session->release();
		slangSession->release();
		return false;
	}

	// SPIR-V 타겟 추가
	spAddCodeGenTarget(slangRequest, SLANG_SPIRV);
	spSetTargetProfile(slangRequest, 0, slangSession->findProfile(profileName));
	
	// Ray Tracing 셰이더의 경우 특별한 설정이 필요할 수 있음
	// Slang은 import slang.rt를 통해 자동으로 Ray Tracing을 인식할 수 있음

	// Slang 소스코드 추가
	int translationUnitIndex = spAddTranslationUnit(slangRequest, SLANG_SOURCE_LANGUAGE_SLANG, nullptr);
	spAddTranslationUnitSourceString(
		slangRequest,
		translationUnitIndex,
		"shader.slang",
		shaderCode
	);

	// 진입점 추가
	spAddEntryPoint(
		slangRequest,
		translationUnitIndex,
		"main",
		slangStage
	);

	// 컴파일 실행
	int compileResult = spCompile(slangRequest);
	if (compileResult != 0)
	{
		// 컴파일 오류 출력
		const char* diagnostics = spGetDiagnosticOutput(slangRequest);
		s_lastError = "Shader compilation failed: ";
		s_lastError += diagnostics;
		spDestroyCompileRequest(slangRequest);
		session->release();
		slangSession->release();
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}

	// SPIR-V 코드 추출
	size_t codeSize = 0;
	void const* spirvCode = spGetEntryPointCode(slangRequest, 0, &codeSize);
	if (!spirvCode || codeSize == 0)
	{
		s_lastError = "Failed to get compiled SPIR-V code";
		spDestroyCompileRequest(slangRequest);
		session->release();
		slangSession->release();
		LOGD(OG_ID, "%s", s_lastError.c_str());
		return false;
	}

	// 결과를 출력 버퍼에 복사
	size_t wordCount = codeSize / sizeof(uint32_t);
	const uint32_t* spirvWords = reinterpret_cast<const uint32_t*>(spirvCode);
	spirvOut.resize(wordCount);
	memcpy(spirvOut.data(), spirvWords, codeSize);

	// 리소스 정리
	spDestroyCompileRequest(slangRequest);
	session->release();
	slangSession->release();

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

OG_NAMESPACE_SAMPLE_END
