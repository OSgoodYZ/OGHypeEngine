# RT Transmission/Refraction 구현

## 목표
DragonAttenuation 모델의 유리 오브젝트가 투명하게 보이도록 굴절(refraction) 레이 트레이싱 구현

## 수정 파일
- `Project/sources/sample/public/core/OgRayTracingSample.cpp`

## 작업 항목

### 1. maxRecursionDepth 증가
- 현재: `2` (primary + shadow)
- 변경: `4` (primary → refraction → refraction(exit) → shadow)

### 2. Closest Hit 셰이더에 굴절 로직 추가
- `transmissionFactor > 0`일 때 굴절 레이 트레이싱
- Snell's law로 굴절 방향 계산 (`refract()` GLSL 내장함수)
- IOR (Index of Refraction) 사용 (material.ior, 기본 1.5)
- 노멀 방향으로 inside/outside 판별 (dot(ray, normal))
- Fresnel로 반사/굴절 비율 결정
- Beer-Lambert 법칙으로 attenuation 적용

### 3. 구현 흐름 (closest hit 내부)
```
if transmissionFactor > 0:
    1. inside/outside 판별 (NdotV)
    2. eta 계산 (outside: 1/ior, inside: ior/1)
    3. refract() 로 굴절 방향
    4. 전반사 체크 (total internal reflection)
    5. Fresnel 비율로 반사/굴절 혼합
    6. traceRayEXT로 굴절 레이 발사
    7. attenuation 적용 (inside일 때)
    8. 최종 색상 = mix(opaque_color, refracted_color, transmissionFactor)
else:
    기존 opaque PBR 로직
```
