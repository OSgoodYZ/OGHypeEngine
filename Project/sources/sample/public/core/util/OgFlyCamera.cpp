#include "OgFlyCamera.h"
#include "system/OgInput.h"
#include <algorithm>

OG_NAMESPACE_SAMPLE_BEGIN

OgFlyCamera::OgFlyCamera()
	: _position(0.0f, 0.0f, 3.0f)
	, _target(0.0f, 0.0f, 0.0f)
	, _worldUp(0.0f, 1.0f, 0.0f)
	, _yaw(-90.0f)
	, _pitch(0.0f)
	, _orbitDistance(3.0f)
	, _fov(45.0f)
	, _aspectRatio(16.0f / 9.0f)
	, _nearPlane(0.1f)
	, _farPlane(1000.0f)
	, _moveSpeed(0.01f)
	, _fastMoveSpeed(0.02f)
	, _rotateSpeed(0.1f)
	, _zoomSpeed(2.0f)
	, _panSpeed(0.01f)
	, _mode(Mode::FLY)
	, _rightMousePressed(false)
	, _leftMousePressed(false)
	, _middleMousePressed(false)
	, _altPressed(false)
	, _shiftPressed(false)
	, _keyW(false), _keyA(false), _keyS(false), _keyD(false), _keyQ(false), _keyE(false)
	, _lastMouseX(0.0)
	, _lastMouseY(0.0)
	, _firstMouse(true)
	, _moveVelocity(0.0f)
{
	updateVectors();
}

void OgFlyCamera::SetPosition(const glm::vec3& position)
{
	_position = position;
	_orbitDistance = glm::length(_position - _target);
}

void OgFlyCamera::SetTarget(const glm::vec3& target)
{
	_target = target;
	
	// 타겟을 바라보도록 yaw/pitch 계산
	glm::vec3 direction = glm::normalize(_target - _position);
	
	// asin의 입력값이 [-1, 1] 범위를 벗어나지 않도록 clamp
	float y_clamped = glm::clamp(direction.y, -1.0f, 1.0f);
	_pitch = glm::degrees(asin(y_clamped));
	
	// atan2로 yaw 계산
	_yaw = glm::degrees(atan2(direction.z, direction.x));
	
	_orbitDistance = glm::length(_position - _target);
	updateVectors();
}

void OgFlyCamera::SetUpVector(const glm::vec3& up)
{
	_worldUp = glm::normalize(up);
	updateVectors();
}

void OgFlyCamera::SetFovDegrees(float fov)
{
	_fov = fov;
}

void OgFlyCamera::SetAspectRatio(float aspect)
{
	_aspectRatio = aspect;
}

void OgFlyCamera::SetNearFar(float nearPlane, float farPlane)
{
	_nearPlane = nearPlane;
	_farPlane = farPlane;
}

void OgFlyCamera::OnMouseButton(int button, int action, int mods)
{
	if (button == OG_MOUSE_BUTTON_RIGHT)
	{
		_rightMousePressed = (action == OG_PRESS);
		if (_rightMousePressed)
		{
			_mode = Mode::FLY;
			_firstMouse = true;
		}
	}
	else if (button == OG_MOUSE_BUTTON_LEFT)
	{
		_leftMousePressed = (action == OG_PRESS);
		if (_leftMousePressed && _altPressed)
		{
			_mode = Mode::ORBIT;
			_firstMouse = true;
		}
	}
	else if (button == OG_MOUSE_BUTTON_MIDDLE)
	{
		_middleMousePressed = (action == OG_PRESS);
		if (_middleMousePressed)
		{
			_mode = Mode::PAN;
			_firstMouse = true;
		}
	}

	_altPressed = (mods & OG_MOD_ALT) != 0;
	_shiftPressed = (mods & OG_MOD_SHIFT) != 0;
}

void OgFlyCamera::OnMouseMove(double x, double y)
{
	if (_firstMouse)
	{
		_lastMouseX = x;
		_lastMouseY = y;
		_firstMouse = false;
		return;
	}

	double xoffset = x - _lastMouseX;
	double yoffset = _lastMouseY - y; // Y축 반전
	_lastMouseX = x;
	_lastMouseY = y;

	if (_mode == Mode::FLY && _rightMousePressed)
	{
		// FLY 모드: 마우스로 카메라 회전
		xoffset *= _rotateSpeed;
		yoffset *= _rotateSpeed;

		_yaw += static_cast<float>(xoffset);
		_pitch += static_cast<float>(yoffset);

		// Pitch 제한
		_pitch = std::clamp(_pitch, -89.0f, 89.0f);

		updateVectors();
	}
	else if (_mode == Mode::ORBIT && _leftMousePressed && _altPressed)
	{
		// ORBIT 모드: 타겟을 중심으로 회전
		xoffset *= _rotateSpeed;
		yoffset *= _rotateSpeed;

		_yaw += static_cast<float>(xoffset);
		_pitch += static_cast<float>(yoffset);
		_pitch = std::clamp(_pitch, -89.0f, 89.0f);

		updateOrbitPosition();
	}
	else if (_mode == Mode::PAN && _middleMousePressed)
	{
		// PAN 모드: 화면 평면에서 이동
		float panX = static_cast<float>(xoffset) * _panSpeed * _orbitDistance;
		float panY = static_cast<float>(yoffset) * _panSpeed * _orbitDistance;

		_position += _right * panX + _up * panY;
		_target += _right * panX + _up * panY;
	}
}

void OgFlyCamera::OnMouseScroll(double xoffset, double yoffset)
{
	// 마우스 휠로 줌 인/아웃
	if (_mode == Mode::ORBIT || (_leftMousePressed && _altPressed))
	{
		// 오빗 모드에서는 타겟까지의 거리 조절
		_orbitDistance -= static_cast<float>(yoffset) * _zoomSpeed * 0.1f;
		_orbitDistance = std::max(0.1f, _orbitDistance);
		updateOrbitPosition();
	}
	else
	{
		// 일반 모드에서는 FOV 조절 또는 전진/후진
		_position += _forward * static_cast<float>(yoffset) * _zoomSpeed;
	}
}

void OgFlyCamera::OnKeyPress(int key, int action, int mods)
{
	bool pressed = (action == OG_PRESS || action == OG_REPEAT);

	switch (key)
	{
	case OG_KEY_W:
		_keyW = pressed;
		break;
	case OG_KEY_A:
		_keyA = pressed;
		break;
	case OG_KEY_S:
		_keyS = pressed;
		break;
	case OG_KEY_D:
		_keyD = pressed;
		break;
	case OG_KEY_Q:
		_keyQ = pressed;
		break;
	case OG_KEY_E:
		_keyE = pressed;
		break;
	case OG_KEY_LEFT_SHIFT:
	case OG_KEY_RIGHT_SHIFT:
		_shiftPressed = pressed;
		break;
	case OG_KEY_LEFT_ALT:
	case OG_KEY_RIGHT_ALT:
		_altPressed = pressed;
		if (_leftMousePressed && _altPressed)
		{
			_mode = Mode::ORBIT;
			_firstMouse = true;
		}
		break;
	}
}

void OgFlyCamera::Update(float deltaTime)
{
	if (_mode == Mode::FLY)
	{
		handleFlyMode(deltaTime);
	}
	else if (_mode == Mode::ORBIT)
	{
		handleOrbitMode(deltaTime);
	}
	else if (_mode == Mode::PAN)
	{
		handlePanMode(deltaTime);
	}

	// 속도 감속 (부드러운 정지)
	_moveVelocity *= 0.9f;
}

void OgFlyCamera::handleFlyMode(float deltaTime)
{
	if (!_rightMousePressed)
		return;

	float velocity = _shiftPressed ? _fastMoveSpeed : _moveSpeed;
	velocity *= deltaTime;

	// WASD 이동
	if (_keyW)
		_moveVelocity += _forward * velocity;
	if (_keyS)
		_moveVelocity -= _forward * velocity;
	if (_keyA)
		_moveVelocity -= _right * velocity;
	if (_keyD)
		_moveVelocity += _right * velocity;
	if (_keyQ)
		_moveVelocity -= _worldUp * velocity;
	if (_keyE)
		_moveVelocity += _worldUp * velocity;

	_position += _moveVelocity;
}

void OgFlyCamera::handleOrbitMode(float deltaTime)
{
	// 오빗 모드에서는 WASD로 타겟 이동 가능
	if (_leftMousePressed && _altPressed)
	{
		float velocity = _shiftPressed ? _fastMoveSpeed : _moveSpeed;
		velocity *= deltaTime * 0.5f; // 오빗 모드에서는 느리게

		glm::vec3 targetMove(0.0f);
		if (_keyW)
			targetMove += _forward * velocity;
		if (_keyS)
			targetMove -= _forward * velocity;
		if (_keyA)
			targetMove -= _right * velocity;
		if (_keyD)
			targetMove += _right * velocity;

		_target += targetMove;
		_position += targetMove;
	}
}

void OgFlyCamera::handlePanMode(float deltaTime)
{
	// PAN 모드는 마우스 이동으로만 처리
}

void OgFlyCamera::updateVectors()
{
	// 전방 벡터 계산
	glm::vec3 front;
	front.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
	front.y = sin(glm::radians(_pitch));
	front.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
	_forward = glm::normalize(front);

	// 오른쪽 벡터와 위 벡터 계산
	_right = glm::normalize(glm::cross(_forward, _worldUp));
	_up = glm::normalize(glm::cross(_right, _forward));
}

void OgFlyCamera::updateOrbitPosition()
{
	// 구면 좌표계를 사용하여 타겟 주위를 회전
	_position.x = _target.x + _orbitDistance * cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
	_position.y = _target.y + _orbitDistance * sin(glm::radians(_pitch));
	_position.z = _target.z + _orbitDistance * sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));

	// 벡터 업데이트
	updateVectors();
}

glm::mat4 OgFlyCamera::GetViewMatrix() const
{
	return glm::lookAt(_position, _position + _forward, _up);
}

glm::mat4 OgFlyCamera::GetProjectionMatrix() const
{
	return glm::perspective(glm::radians(_fov), _aspectRatio, _nearPlane, _farPlane);
}

glm::mat4 OgFlyCamera::GetViewProjectionMatrix() const
{
	return GetProjectionMatrix() * GetViewMatrix();
}

OG_NAMESPACE_SAMPLE_END
