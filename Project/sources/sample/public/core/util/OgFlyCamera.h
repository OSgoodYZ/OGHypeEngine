#pragma once
#ifndef _OG_FLY_CAMERA_H__
#define _OG_FLY_CAMERA_H__

#include "OgPrecompile.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

OG_NAMESPACE_SAMPLE_BEGIN

/**
 * @brief Unity 에디터 스타일의 Fly Camera 구현
 * 
 * 컨트롤:
 * - 우클릭 + WASD: 이동
 * - 우클릭 + 마우스: 회전
 * - 좌클릭 + Alt: 오빗 회전
 * - 마우스 휠: 줌 인/아웃
 * - Shift: 빠른 이동
 */
class OG_API OgFlyCamera
{
public:
	enum class Mode
	{
		FLY,       // 우클릭 + WASD 이동
		ORBIT,     // Alt + 좌클릭 오빗
		PAN,       // 마우스 가운데 버튼 패닝
	};

	OgFlyCamera();
	~OgFlyCamera() = default;

	// 카메라 초기화
	void SetPosition(const glm::vec3& position);
	void SetTarget(const glm::vec3& target);
	void SetUpVector(const glm::vec3& up);
	void SetFovDegrees(float fov);
	void SetAspectRatio(float aspect);
	void SetNearFar(float nearPlane, float farPlane);

	// 입력 처리
	void OnMouseButton(int button, int action, int mods);
	void OnMouseMove(double x, double y);
	void OnMouseScroll(double xoffset, double yoffset);
	void OnKeyPress(int key, int action, int mods);

	// 업데이트
	void Update(float deltaTime);

	// 행렬 가져오기
	glm::mat4 GetViewMatrix() const;
	glm::mat4 GetProjectionMatrix() const;
	glm::mat4 GetViewProjectionMatrix() const;

	// 속성 접근자
	glm::vec3 GetPosition() const { return _position; }
	glm::vec3 GetForward() const { return _forward; }
	glm::vec3 GetRight() const { return _right; }
	glm::vec3 GetUp() const { return _up; }
	float GetFov() const { return _fov; }

	// 이동 속도 설정
	void SetMoveSpeed(float speed) { _moveSpeed = speed; }
	void SetRotateSpeed(float speed) { _rotateSpeed = speed; }
	void SetZoomSpeed(float speed) { _zoomSpeed = speed; }

private:
	// 내부 메서드
	void updateVectors();
	void updateOrbitPosition();
	void handleFlyMode(float deltaTime);
	void handleOrbitMode(float deltaTime);
	void handlePanMode(float deltaTime);

private:
	// 카메라 상태
	glm::vec3 _position;
	glm::vec3 _target;        // 오빗 모드용 타겟
	glm::vec3 _forward;
	glm::vec3 _right;
	glm::vec3 _up;
	glm::vec3 _worldUp;

	// 회전 각도 (도)
	float _yaw;
	float _pitch;
	float _orbitDistance;     // 오빗 모드에서 타겟까지의 거리

	// 프로젝션 설정
	float _fov;               // 도 단위
	float _aspectRatio;
	float _nearPlane;
	float _farPlane;

	// 이동 속도
	float _moveSpeed;
	float _fastMoveSpeed;     // Shift 누를 때
	float _rotateSpeed;
	float _zoomSpeed;
	float _panSpeed;

	// 입력 상태
	Mode _mode;
	bool _rightMousePressed;
	bool _leftMousePressed;
	bool _middleMousePressed;
	bool _altPressed;
	bool _shiftPressed;
	
	// WASD 키 상태
	bool _keyW, _keyA, _keyS, _keyD, _keyQ, _keyE;
	
	// 마우스 위치
	double _lastMouseX;
	double _lastMouseY;
	bool _firstMouse;

	// 이동 벡터 (부드러운 이동을 위해)
	glm::vec3 _moveVelocity;
};

OG_NAMESPACE_SAMPLE_END

#endif // _OG_FLY_CAMERA_H__
