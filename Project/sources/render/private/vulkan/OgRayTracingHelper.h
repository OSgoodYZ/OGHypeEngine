#pragma once
#ifndef _OG_RAYTRACING_HELPER_H_
#define _OG_RAYTRACING_HELPER_H_

#include "OgPrecompile.h"

OG_NAMESPACE_RENDER_BEGIN

// Utility function to align up
template<typename T>
inline T AlignUp(T value, T alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

// OgCommandEncoderHandle 확장 - 레이트레이싱 명령
class OgCommandEncoderHandle;


// Ray tracing extension names
#define VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME "VK_KHR_acceleration_structure"
#define VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME "VK_KHR_ray_tracing_pipeline"
#define VK_KHR_RAY_QUERY_EXTENSION_NAME "VK_KHR_ray_query"
#define VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME "VK_KHR_deferred_host_operations"
#define VK_KHR_SPIRV_1_4_EXTENSION_NAME "VK_KHR_spirv_1_4"
#define VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME "VK_KHR_shader_float_controls"
#define VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME "VK_KHR_buffer_device_address"

OG_NAMESPACE_RENDER_END

#endif // _OG_RAYTRACING_HELPER_H_
