#pragma once
#ifndef _OG_SHADER_COMPILER_H__
#define _OG_SHADER_COMPILER_H__

#include "OgPrecompile.h"
#include "render/OgRenderDefinitions.h"
#include <vector>
#include <string>

OG_NAMESPACE_SAMPLE_BEGIN

/**
 * @brief GLSL 셰이더를 SPIR-V로 컴파일하는 유틸리티 클래스
 */
class OG_API OgShaderCompiler
{
public:
	/**
	 * @brief GLSL 셰이더 코드를 SPIR-V로 컴파일
	 * @param shaderCode GLSL 셰이더 소스 코드
	 * @param shaderType 셰이더 타입 (Vertex, Fragment 등)
	 * @param spirvOut 컴파일된 SPIR-V 코드가 저장될 벡터
	 * @return 컴파일 성공 여부
	 */
	static bool CompileGLSLtoSPIRV(
		const char* shaderCode,
		Render::OgShaderType shaderType,
		std::vector<uint32_t>& spirvOut);
	
	/**
	 * @brief GLSL 셰이더 코드로부터 OgShaderHandle 생성
	 * @param renderContext 렌더 컨텍스트
	 * @param shaderCode GLSL 셰이더 소스 코드
	 * @param shaderType 셰이더 타입
	 * @param shaderName 셰이더 이름 (디버깅용)
	 * @return 생성된 셰이더 핸들 (실패시 nullptr)
	 */
	static Render::OgShaderHandle* CreateShaderFromGLSL(
		Render::OgRenderContext* renderContext,
		const char* shaderCode,
		Render::OgShaderType shaderType,
		const char* shaderName = nullptr);
	
	/**
	 * @brief 버텍스/프래그먼트 셰이더 쌍으로부터 프로그램 생성
	 * @param renderContext 렌더 컨텍스트
	 * @param vertexShaderCode 버텍스 셰이더 GLSL 코드
	 * @param fragmentShaderCode 프래그먼트 셰이더 GLSL 코드
	 * @param programName 프로그램 이름 (디버깅용)
	 * @return 생성된 프로그램 핸들 (실패시 nullptr)
	 */
	static Render::OgProgramHandle* CreateProgramFromGLSL(
		Render::OgRenderContext* renderContext,
		const char* vertexShaderCode,
		const char* fragmentShaderCode,
		const char* programName = nullptr);
	
	/**
	 * @brief Slang 셰이더 코드를 SPIR-V로 컴파일
	 * @param shaderCode Slang 셰이더 소스 코드
	 * @param shaderType 셰이더 타입 (Vertex, Fragment, RayGen 등)
	 * @param spirvOut 컴파일된 SPIR-V 코드가 저장될 벡터
	 * @return 컴파일 성공 여부
	 */
	static bool CompileSlangToSPIRV(
		const char* shaderCode,
		Render::OgShaderType shaderType,
		std::vector<uint32_t>& spirvOut);
	
	/**
	 * @brief Slang 셰이더 코드로부터 OgShaderHandle 생성
	 * @param renderContext 렌더 컨텍스트
	 * @param shaderCode Slang 셰이더 소스 코드
	 * @param shaderType 셰이더 타입
	 * @param shaderName 셰이더 이름 (디버깅용)
	 * @return 생성된 셰이더 핸들 (실패시 nullptr)
	 */
	static Render::OgShaderHandle* CreateShaderFromSlang(
		Render::OgRenderContext* renderContext,
		const char* shaderCode,
		Render::OgShaderType shaderType,
		const char* shaderName = nullptr);
	
	/**
	 * @brief 마지막 컴파일 에러 메시지 반환
	 * @return 에러 메시지 문자열
	 */
	static const std::string& GetLastError() { return s_lastError; }
	
private:
	static std::string s_lastError;
};

OG_NAMESPACE_SAMPLE_END

#endif // _OG_SHADER_COMPILER_H__
