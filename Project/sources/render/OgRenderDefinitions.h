#pragma warning(disable:4251)
#pragma once
#ifndef _OG_RENDER_DEFINITIONS_H__
#define _OG_RENDER_DEFINITIONS_H__

#include "OgPrecompile.h"

#include <iostream> 
#include <vector>
#include <queue>
#include <list>
#include "glm/glm.hpp"


using namespace std;

/*#include "OgRenderContext.h" */namespace Og { namespace Render { class OgRenderContext; } }

OG_NAMESPACE_RENDER_BEGIN

enum class OgShaderType : uint8
{
	VERTEX = 0x00000001,
	TESSELLATION_CONTROL = 0x00000002,
	TESSELLATION_EVALUATION = 0x00000004,
	GEOMETRY = 0x00000008,
	FRAGMENT = 0x00000010,
	COMPUTE = 0x00000020,
};

enum class OgShaderValueType : uint8
{
	Void,
	Bool,
	Uint1,
	Int1,
	Half1,
	Float1,

	Vec2,
	Vec3,
	Vec4, //!< 4 floats vector.

	Bvec2,
	Bvec3,
	Bvec4,

	Ivec2,
	Ivec3,
	Ivec4,

	Uvec2,
	Uvec3,
	Uvec4,

	Mat2,
	Mat2x3,
	Mat2x4,

	Mat3x2,
	Mat3,
	Mat3x4,

	Mat4x2,
	Mat4x3,
	Mat4,

	Struct
};


// https://docs.unity3d.com/ScriptReference/RenderTextureFormat.html

enum class OgRenderTextureFormat : uint8
{
	NONE,

	DEFAULT_DEPTH,
	DEFAULT_DEPTH_STENCIL,
	STENCIL8,
	DEPTH16,
	DEPTH24,
	DEPTH32,
	DEPTH16_STENCIL8,
	DEPTH24_STENCIL8,
	DEPTH32_STENCIL8,

	DEFAULT_COLOR,
	R5G6B5,
	R8G8B8,
	R8G8B8A8_UNORM,
	R8G8B8A8_SNORM,
	B8G8R8A8,

	R8G8_UNORM,
	R8G8_SNORM,

	R32G32,
	R32G32B32,
	R32G32B32A32
};

// TODO : to add more population format.

// actual : http://vulkan-spec-chunked.ahcox.com/ch31s03.html
enum class OgPixelFormat : uint8
{
	NONE = 0,
	R4G4B4A4_UNORM_PACK16 = 2,
	R5G6B5_UNROM_PACK16 = 4,
	R8_UNORM = 9,
	R8_SNORM = 10,
	//R8_USCALED = 11,
	//R8_SSCALED = 12,
	R8_UINT = 13,
	R8_SINT = 14,
	R8_SRGB = 15,
	R8G8_UNORM = 16,
	R8G8_SNORM = 17,
	//R8G8_USCALED = 18,
	//R8G8_SSCALED = 19,
	R8G8_UINT = 20,
	R8G8_SINT = 21,
	R8G8_SRGB = 22,
	R8G8B8_UNORM = 23,
	R8G8B8_SNORM = 24,
	//R8G8B8_USCALED = 25,
	//R8G8B8_SSCALED = 26,
	R8G8B8_UINT = 27,
	R8G8B8_SINT = 28,
	R8G8B8_SRGB = 29,
	B8G8R8_UNORM = 30,
	B8G8R8_SNORM = 31,
	//B8G8R8_USCALED = 32,
	//B8G8R8_SSCALED = 33,
	B8G8R8_UINT = 34,
	B8G8R8_SINT = 35,
	B8G8R8_SRGB = 36,
	R8G8B8A8_UNORM = 37,
	R8G8B8A8_SNORM = 38,
	//R8G8B8A8_USCALED = 39,
	//R8G8B8A8_SSCALED = 40,
	R8G8B8A8_UINT = 41,
	R8G8B8A8_SINT = 42,
	R8G8B8A8_SRGB = 43,
	B8G8R8A8_UNORM = 44,
	B8G8R8A8_SNORM = 45,
	//B8G8R8A8_USCALED = 46,
	//B8G8R8A8_SSCALED = 47,
	B8G8R8A8_UINT = 48,
	B8G8R8A8_SINT = 49,
	B8G8R8A8_SRGB = 50,
	A8B8G8R8_UNORM_PACK32 = 51,
	A8B8G8R8_SNORM_PACK32 = 52,
	//A8B8G8R8_USCALED_PACK32 = 53,
	//A8B8G8R8_SSCALED_PACK32 = 54,
	A8B8G8R8_UINT_PACK32 = 55,
	A8B8G8R8_SINT_PACK32 = 56,
	A8B8G8R8_SRGB_PACK32 = 57,
	A2R10G10B10_UNORM_PACK32 = 58,
	A2R10G10B10_SNORM_PACK32 = 59,
	//A2R10G10B10_USCALED_PACK32 = 60,
	//A2R10G10B10_SSCALED_PACK32 = 61,
	A2R10G10B10_UINT_PACK32 = 62,
	A2R10G10B10_SINT_PACK32 = 63,
	A2B10G10R10_UNORM_PACK32 = 64,
	A2B10G10R10_SNORM_PACK32 = 65,
	//A2B10G10R10_USCALED_PACK32 = 66,
	//A2B10G10R10_SSCALED_PACK32 = 67,
	A2B10G10R10_UINT_PACK32 = 68,
	A2B10G10R10_SINT_PACK32 = 69,
	R16_UNORM = 70,
	R16_SNORM = 71,
	//R16_USCALED = 72,
	//R16_SSCALED = 73,
	R16_UINT = 74,
	R16_SINT = 75,
	R16_SFLOAT = 76,
	R16G16_UNORM = 77,
	R16G16_SNORM = 78,
	//R16G16_USCALED = 79,
	//R16G16_SSCALED = 80,
	R16G16_UINT = 81,
	R16G16_SINT = 82,
	R16G16_SFLOAT = 83,
	R16G16B16_UNORM = 84,
	R16G16B16_SNORM = 85,
	//R16G16B16_USCALED = 86,
	//R16G16B16_SSCALED = 87,
	R16G16B16_UINT = 88,
	R16G16B16_SINT = 89,
	R16G16B16_SFLOAT = 90,
	R16G16B16A16_UNORM = 91,
	R16G16B16A16_SNORM = 92,
	//R16G16B16A16_USCALED = 93,
	//R16G16B16A16_SSCALED = 94,
	R16G16B16A16_UINT = 95,
	R16G16B16A16_SINT = 96,
	R16G16B16A16_SFLOAT = 97,
	R32_UINT = 98,
	R32_SINT = 99,
	R32_SFLOAT = 100,
	R32G32_UINT = 101,
	R32G32_SINT = 102,
	R32G32_SFLOAT = 103,
	R32G32B32_UINT = 104,
	R32G32B32_SINT = 105,
	R32G32B32_SFLOAT = 106,
	R32G32B32A32_UINT = 107,
	R32G32B32A32_SINT = 108,
	R32G32B32A32_SFLOAT = 109,
	R64_UINT = 110,
	R64_SINT = 111,
	R64_SFLOAT = 112,
	R64G64_UINT = 113,
	R64G64_SINT = 114,
	R64G64_SFLOAT = 115,
	R64G64B64_UINT = 116,
	R64G64B64_SINT = 117,
	R64G64B64_SFLOAT = 118,
	R64G64B64A64_UINT = 119,
	R64G64B64A64_SINT = 120,
	R64G64B64A64_SFLOAT = 121,
	B10G11R11_UFLOAT_PACK32 = 122,
	E5B9G9R9_UFLOAT_PACK32 = 123,
	D16_UNORM = 124,
	X8_D24_UNORM_PACK32 = 125,
	D32_SFLOAT = 126,
	S8_UINT = 127,
	D16_UNORM_S8_UINT = 128,
	D24_UNORM_S8_UINT = 129,
	D32_SFLOAT_S8_UINT = 130,
	BC1_RGB_UNORM_BLOCK = 131,
	BC1_RGB_SRGB_BLOCK = 132,
	BC1_RGBA_UNORM_BLOCK = 133,
	BC1_RGBA_SRGB_BLOCK = 134,
	BC2_UNORM_BLOCK = 135,
	BC2_SRGB_BLOCK = 136,
	BC3_UNORM_BLOCK = 137,
	BC3_SRGB_BLOCK = 138,
	BC4_UNORM_BLOCK = 139,
	BC4_SNORM_BLOCK = 140,
	BC5_UNORM_BLOCK = 141,
	BC5_SNORM_BLOCK = 142,
	BC6H_UFLOAT_BLOCK = 143,
	BC6H_SFLOAT_BLOCK = 144,
	BC7_UNORM_BLOCK = 145,
	BC7_SRGB_BLOCK = 146,
	ETC2_R8G8B8_UNORM_BLOCK = 147,
	ETC2_R8G8B8_SRGB_BLOCK = 148,
	ETC2_R8G8B8A1_UNORM_BLOCK = 149,
	ETC2_R8G8B8A1_SRGB_BLOCK = 150,
	ETC2_R8G8B8A8_UNORM_BLOCK = 151,
	ETC2_R8G8B8A8_SRGB_BLOCK = 152,
	EAC_R11_UNORM_BLOCK = 153,
	EAC_R11_SNORM_BLOCK = 154,
	EAC_R11G11_UNORM_BLOCK = 155,
	EAC_R11G11_SNORM_BLOCK = 156,
	ASTC_4x4_UNORM_BLOCK = 157,
	ASTC_4x4_SRGB_BLOCK = 158,
	ASTC_5x4_UNORM_BLOCK = 159,
	ASTC_5x4_SRGB_BLOCK = 160,
	ASTC_5x5_UNORM_BLOCK = 161,
	ASTC_5x5_SRGB_BLOCK = 162,
	ASTC_6x5_UNORM_BLOCK = 163,
	ASTC_6x5_SRGB_BLOCK = 164,
	ASTC_6x6_UNORM_BLOCK = 165,
	ASTC_6x6_SRGB_BLOCK = 166,
	ASTC_8x5_UNORM_BLOCK = 167,
	ASTC_8x5_SRGB_BLOCK = 168,
	ASTC_8x6_UNORM_BLOCK = 169,
	ASTC_8x6_SRGB_BLOCK = 170,
	ASTC_8x8_UNORM_BLOCK = 171,
	ASTC_8x8_SRGB_BLOCK = 172,
	ASTC_10x5_UNORM_BLOCK = 173,
	ASTC_10x5_SRGB_BLOCK = 174,
	ASTC_10x6_UNORM_BLOCK = 175,
	ASTC_10x6_SRGB_BLOCK = 176,
	ASTC_10x8_UNORM_BLOCK = 177,
	ASTC_10x8_SRGB_BLOCK = 178,
	ASTC_10x10_UNORM_BLOCK = 179,
	ASTC_10x10_SRGB_BLOCK = 180,
	ASTC_12x10_UNORM_BLOCK = 181,
	ASTC_12x10_SRGB_BLOCK = 182,
	ASTC_12x12_UNORM_BLOCK = 183,
	ASTC_12x12_SRGB_BLOCK = 184,
};

// https://docs.unity3d.com/ScriptReference/TextureFormat.html
enum class OgTextureFormat : uint8
{
	ALPHA8,
	ARGB4444,
	RGB24,
	RGBA32,
	ARGB32,
	RGB565,
	R16,
	DXT1,
	DXT5,
	RGBA4444,
	BGRA32,
	RHALF,
	RGHALF,
	RFLOAT,
	RGFLOAT,
	RGBAFLOAT,
	YUY2,
	//RGB9E5FLOAT
	BC4,
	BC5,
	BC6H,
	BC7,
	DXT1_CRUNCHED,
	DXT5_CRUNCHED,
	PVRTC_RGB2,
	PVRTC_RGBA2,
	PVRTC_RGB4,
	PVRTC_RGBA4,
	ETC_RGB4,
	EAC_R,
	EAC_R_SINGED,
	EAC_RG,
	EAC_RG_SINGED,
	ETC2_RGB,
	ETC2_RGBA1,
	ETC2_RGBA8,
	ASTC_RGB_4X4,
	ASTC_RGB_5X5,
	ASTC_RGB_6X6,
	ASTC_RGB_8X8,
	ASTC_RGB_10X10,
	ASTC_RGB_12X12,
	RG16,
	R8,
	ETC_RGB4_CRUNCHED,
	ETC2_RGBA8_CRUNCHED

};

enum class OgVertexFormat : uint8
{
	Invalid = 0,

	BYTE2 = 1,
	BYTE3 = 2,
	BYTE4 = 3,

	SBYTE2 = 4,
	SBYTE3 = 5,
	SBYTE4 = 6,

	BYTE2_NORM = 7,
	BYTE3_NORM = 8,
	BYTE4_NORM = 9,

	SBYTE2_NORM = 10,
	SBYTE3_NORM = 11,
	SBYTE4_NORM = 12,

	USHORT2 = 13,
	USHORT3 = 14,
	USHORT4 = 15,

	SHORT2 = 16,
	SHORT3 = 17,
	SHORT4 = 18,

	USHORT2_NORM = 19,
	USHORT3_NORM = 20,
	USHORT4_NORM = 21,

	SHORT2_NORM = 22,
	SHORT3_NORM = 23,
	SHORT4_NORM = 24,

	HALF2 = 25,
	HALF3 = 26,
	HALF4 = 27,

	FLOAT1 = 28,
	FLOAT2 = 29,
	FLOAT3 = 30,
	FLOAT4 = 31,

	INT1 = 32,
	INT2 = 33,
	INT3 = 34,
	INT4 = 35,

	UINT1 = 36,
	UINT2 = 37,
	UINT3 = 38,
	UINT4 = 39,
};

enum class OgIndexType : uint8
{
	UInt16,
	UInt32
};

// If your application will render all pixels of the attachment for a given frame, use the default load action MTLLoadActionDontCare.
// The MTLLoadActionDontCare action allows the GPU to avoid loading the existing contents of the texture, ensuring the best performance.
// Otherwise, you can use the MTLLoadActionClear action to clear the previous contents of the attachment, or the MTLLoadActionLoad action to preserve them.
// The MTLLoadActionClear action also avoids loading the existing texture contents, but it incurs the cost of filling the destination with a solid color.
enum class OgRenderBufferLoadAction : uint8
{
	LOAD,
	CLEAR,
	DONT_CARE
};

// https://developer.apple.com/documentation/metal/mtlrenderpassattachmentdescriptor/1437956-storeaction ???
// https://docs.unity3d.com/2018.3/Documentation/ScriptReference/Rendering.RenderBufferStoreAction.html

// For color attachments, the MTLStoreActionStore action is the default store action,
// because applications almost always preserve the final color values in the attachment at the end of rendering pass.
// For depth and stencil attachments, MTLStoreActionDontCare is the default store action,
// because those attachments typically do not need to be preserved after the rendering pass is complete.
enum class OgRenderBufferStoreAction : uint8
{
	STORE,
	DONT_CARE
	//RESOOgE // TODO: resoOge is not used in engine code
};

// Graphic Pipeline Command.
enum class OgCommandType : uint8
{
	END,
	BEGIN,
	SET_VIEWPORT,
	SET_SCISSOR,
	BEGIN_RENDERPASS,
	END_RENDERPASS,
	BIND_PIPELINE,
	BIND_RESOURCESET,
	BIND_VERTEX_BUFFERS,
	BIND_INDEX_BUFFER,
	DRAW_INDEXED,
	DRAW_ARRAYS,
	BEGIN_DEBUG_MARKER,
	END_DEBUG_MARKER,
};

inline const char* Og_get_command_type_string(OgCommandType e)
{
	switch (e)
	{
	case OgCommandType::END:						return "END";
	case OgCommandType::BEGIN:						return "BEGIN";
	case OgCommandType::BIND_PIPELINE:              return "BIND_PIPELINE";
	case OgCommandType::SET_VIEWPORT:               return "SET_VIEWPORT";
	case OgCommandType::SET_SCISSOR:                return "SET_SCISSOR";
	case OgCommandType::BEGIN_RENDERPASS:           return "BEGIN_RENDERPASS";
	case OgCommandType::END_RENDERPASS:             return "END_RENDERPASS";
	case OgCommandType::BIND_VERTEX_BUFFERS:        return "BIND_VERTEX_BUFFERS";
	case OgCommandType::BIND_INDEX_BUFFER:          return "BIND_INDEX_BUFFER";
	case OgCommandType::DRAW_INDEXED:               return "DRAW_INDEXED";
	case OgCommandType::DRAW_ARRAYS:                return "DRAW_ARRAYS";
	case OgCommandType::BIND_RESOURCESET:			return "BIND_RESOURCESET";
	}

	return "Invaild Enum";
}

//Og_DECLARE_ENUM(OgRenderPlatform, GLES2, GLES3, VULKAN, METAL, DX11, DX12)

enum class OgRenderPlatform : uint8
{
	NONE,
	GLES2,
	GLES3,
	VULKAN,
	METAL,
	DX11,
};

enum class OgPipelineType : uint8
{
	GRAPHICS_PIPELINE,
	COMPUTE_PIPELINE
};

// https://www.khronos.org/opengl/wiki/Sampler_(GLSL)

enum class OgSamplerType : uint8
{
	TEX_1D = 0,
	TEX_2D = 1,
	TEX_3D = 2,
	TEX_CUBE = 3,
};

enum class OgSampleCountFlag : uint8
{
	COUNT_1 = 0x00000001,
	COUNT_2 = 0x00000002,
	COUNT_4 = 0x00000004,
	COUNT_8 = 0x00000008,
	COUNT_16 = 0x00000010,
	COUNT_32 = 0x00000020,
	COUNT_64 = 0x00000040
};

// Image Buffer 구성 
enum class OgTextureType : uint8
{
	TEX_1D = 0,
	TEX_2D = 1,
	TEX_3D = 2,

	//TEX_1D_ARRAY = 4,
	//TEX_2D_ARRAY = 5,
	//TEX_CUBE_ARRAY = 6,
};

// Image Buffer 어떻게 읽는지
enum class OgTextureViewType : uint8
{
	TEX_1D = 0,
	TEX_2D = 1,
	TEX_3D = 2,
	TEX_CUBE = 3,
	TEX_1D_ARRAY = 4,
	TEX_2D_ARRAY = 5,
	TEX_CUBE_ARRAY = 6,
};

// https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkImageUsageFlagBits.html
// TODO : Vulkan 기준이였으나, GL과 섞이면서 다른 것들이 생겼음
//        따라서 각 플랫폼 별로 convert 하는 메소드 만들어야함
enum class OgTextureUsage : uint16
{
	GPU_LOCAL = 0x00000001,						// No data but only in GPU : 상태 
	STAGING = 0x00000002,						// data but only in GPU : 전송 방법
	SAMPLED = 0x00000004,						// data in GPU and CPU : 상태 (쉐이더에서 읽을 수 있다)
	STORAGE = 0x00000008,						// 상태
	COLOR_ATTACHMENT = 0x00000010,				//		0001 0000	: Layout (상태)
	DEPTH_ATTACHMENT = 0x00000020,				//		0010 0000	: Layout (상태)
	STENCIL_ATTACHMENT = 0x00000040,			//		0100 0000	: Layout (상태)
	DEPTH_STENCIL_ATTACHMENT = 0x00000080,		//		1000 0000	: Layout (상태)
	TRANSIENT_ATTACHMENT = 0x00000100			// 0001 0000 0000	: Layout (상태)
};

inline bool operator!=(OgTextureUsage a, uint16 b)
{
	return static_cast<uint16>(a) != b;
}

inline OgTextureUsage operator&(OgTextureUsage a, OgTextureUsage b)
{
	return static_cast<OgTextureUsage>(static_cast<uint16>(a) & static_cast<uint16>(b));
}

inline OgTextureUsage operator|(OgTextureUsage a, OgTextureUsage b)
{
	return static_cast<OgTextureUsage>(static_cast<uint16>(a) | static_cast<uint16>(b));
}

static bool IsAttachmentUsage(const OgTextureUsage& usage)
{
	if ((usage & OgTextureUsage::COLOR_ATTACHMENT) != 0) return true;
	if ((usage & OgTextureUsage::DEPTH_ATTACHMENT) != 0) return true;
	if ((usage & OgTextureUsage::STENCIL_ATTACHMENT) != 0) return true;
	if ((usage & OgTextureUsage::DEPTH_STENCIL_ATTACHMENT) != 0) return true;
	if ((usage & OgTextureUsage::TRANSIENT_ATTACHMENT) != 0) return true;

	return false;
}

enum class OgFilter : uint8
{
	NEAREST = 0,
	LINEAR = 1,
};

enum class OgSamplerMipmapMode : uint8
{
	NEAREST = 0,
	LINEAR = 1,
};

enum class OgSamplerCoord : uint8
{
	NORMALIZED = 0,
	PIXEL = 1,
};

enum class OgSamplerAddressMode : uint8
{
	REPEAT = 0,
	MIRRORED_REPEAT = 1,
	CLAMP_TO_EDGE = 2,
	//CLAMP_TO_BORDER = 3,
	//MIRROR_CLAMP_TO_EDGE = 4
};

enum class OgColorWriteMask : uint8
{
	RED = 0x00000001,
	GREEN = 0x00000002,
	BLUE = 0x00000004,
	ALPHA = 0x00000008,
};

inline OgColorWriteMask operator&(OgColorWriteMask a, OgColorWriteMask b)
{
	return static_cast<OgColorWriteMask>(static_cast<uint8>(a) & static_cast<uint8>(b));
}

inline OgColorWriteMask operator|(OgColorWriteMask a, OgColorWriteMask b)
{
	return static_cast<OgColorWriteMask>(static_cast<uint8>(a) | static_cast<uint8>(b));
}

// https://m.blog.naver.com/PostView.nhn?blogId=itrainl4&logNo=90188723209&proxyReferer=https%3A%2F%2Fwww.google.com%2F
enum class OgBlendFactor : uint8
{
	ZERO = 0,
	ONE = 1,
	SRC_COLOR = 2,
	ONE_MINUS_SRC_COLOR = 3,
	DST_COLOR = 4,
	ONE_MINUS_DST_COLOR = 5,
	SRC_ALPHA = 6,
	ONE_MINUS_SRC_ALPHA = 7,
	DST_ALPHA = 8,
	ONE_MINUS_DST_ALPHA = 9,
	CONSTANT_COLOR = 10,
	ONE_MINUS_CONSTANT_COLOR = 11,
	CONSTANT_ALPHA = 12,
	ONE_MINUS_CONSTANT_ALPHA = 13,
	SRC_ALPHA_SATURATE = 14,
	SRC1_COLOR = 15,
	ONE_MINUS_SRC1_COLOR = 16,
	SRC1_ALPHA = 17,
	ONE_MINUS_SRC1_ALPHA = 18,
};

enum class OgBlendOp : uint8
{
	ADD = 0,
	SUBTRACT = 1,
	REVERSE_SUBTRACT = 2,
	MIN = 3,
	MAX = 4,
};

enum class OgLogicOp : uint8
{
	CLEAR = 0,
	AND = 1,
	AND_REVERSE = 2,
	COPY = 3,
	AND_INVERTED = 4,
	NO_OP = 5,
	XOR = 6,
	OR = 7,
	NOR = 8,
	EQUIVALENT = 9,
	INVERT = 10,
	OR_REVERSE = 11,
	COPY_INVERTED = 12,
	OR_INVERTED = 13,
	NAND = 14,
	SET = 15,
};

enum class OgPolygonMode : uint8
{
	FILL = 0,
	LINE = 1,
	POINT = 2,
	// FILL_RECTANGLE_NV = 1000153000, // ???
};

enum class OgPrimitiveType : uint8
{
	POINT_LIST = 0,
	LINE_LIST = 1,
	LINE_STRIP = 2,
	TRIANGLE_LIST = 3,
	TRIANGLE_STRIP = 4,
	PATCH_LIST = 10,
};

enum class OgCullMode : uint8
{
	NONE = 0,
	FRONT = 0x00000001,
	BACK = 0x00000002,
	FRONT_AND_BACK = 0x00000003,
};

enum class OgFrontFace : uint8
{
	COUNTER_CLOCKWISE = 0,
	CLOCKWISE = 1,
};

enum class OgCompareOp : uint8
{
	NEVER = 0,
	LESS = 1,
	EQUAL = 2,
	LESS_OR_EQUAL = 3,
	GREATER = 4,
	NOT_EQUAL = 5,
	GREATER_OR_EQUAL = 6,
	ALWAYS = 7,
};

enum class OgStencilOp : uint8
{
	KEEP = 0,
	ZERO = 1,
	REPLACE = 2,
	INCREMENT_AND_CLAMP = 3,
	DECREMENT_AND_CLAMP = 4,
	INVERT = 5,
	INCREMENT_AND_WRAP = 6,
	DECREMENT_AND_WRAP = 7,
};

// http://vulkan-spec-chunked.ahcox.com/ch13s01.html
enum class OgResourceType : uint8
{
	SAMPLER = 0,
	COMBINED_IMAGE_SAMPLER = 1,
	SAMPLED_IMAGE = 2,
	STORAGE_IMAGE = 3,
	UNIFORM_TEXEL_BUFFER = 4,
	STORAGE_TEXEL_BUFFER = 5,
	UNIFORM_BUFFER = 6,
	STORAGE_BUFFER = 7,
	UNIFORM_BUFFER_DYNAMIC = 8,
	STORAGE_BUFFER_DYNAMIC = 9,
	INPUT_ATTACHMENT = 10,
};

class OG_API OgDeviceCapability
{
	// todo
	// https://github.com/bkaradzic/bgfx/blob/master/src/renderer_vk.cpp line 941
};


// https://github.com/KhronosGroup/KTX-Specification/issues/8
// https://android.googlesource.com/platform/external/vulkan-validation-layers/+/HEAD/layers/vk_format_utils.cpp
class OG_API OgFormatSupplement
{

public:

	static uint16 GetSizeInBytes(OgVertexFormat format)
	{
		/// OpenGL TyPe : https://www.khronos.org/opengl/wiki/OpenGL_Type
		/// OpenGL Vertex Spec : https://www.khronos.org/opengl/wiki/Vertex_Specification#Component_type
		/// GLES Precisoin of Types : http://learnwebgl.brown37.net/12_shader_language/glsl_data_types.html
		/// METAL : https://developer.apple.com/documentation/metal/mtOgertexformat 

		switch (format)
		{
		case OgVertexFormat::BYTE2_NORM:
		case OgVertexFormat::BYTE2:
		case OgVertexFormat::SBYTE2_NORM:
		case OgVertexFormat::SBYTE2:
			return 2;

		case OgVertexFormat::BYTE3_NORM:
		case OgVertexFormat::BYTE3:
		case OgVertexFormat::SBYTE3_NORM:
		case OgVertexFormat::SBYTE3:
			return 3;

		case OgVertexFormat::BYTE4_NORM:
		case OgVertexFormat::BYTE4:
		case OgVertexFormat::SBYTE4_NORM:
		case OgVertexFormat::SBYTE4:
		case OgVertexFormat::FLOAT1:
		case OgVertexFormat::UINT1:
		case OgVertexFormat::INT1:
		case OgVertexFormat::SHORT2_NORM:
		case OgVertexFormat::SHORT2:
		case OgVertexFormat::USHORT2_NORM:
		case OgVertexFormat::USHORT2:
		case OgVertexFormat::HALF2:
			return 4;

		case OgVertexFormat::SHORT3_NORM:
		case OgVertexFormat::SHORT3:
		case OgVertexFormat::USHORT3_NORM:
		case OgVertexFormat::USHORT3:
		case OgVertexFormat::HALF3:
			return 6;

		case OgVertexFormat::FLOAT2:
		case OgVertexFormat::UINT2:
		case OgVertexFormat::INT2:
		case OgVertexFormat::SHORT4_NORM:
		case OgVertexFormat::SHORT4:
		case OgVertexFormat::USHORT4_NORM:
		case OgVertexFormat::USHORT4:
		case OgVertexFormat::HALF4:
			return 8;

		case OgVertexFormat::FLOAT3:
		case OgVertexFormat::UINT3:
		case OgVertexFormat::INT3:
			return 12;

		case OgVertexFormat::FLOAT4:
		case OgVertexFormat::UINT4:
		case OgVertexFormat::INT4:
			return 16;
		default:
			return 0;
		}

		return 0;
	}

	static uint16 GetSizeInBytes(OgPixelFormat format)
	{
		switch (format)
		{
		case OgPixelFormat::R8_UNORM:
		case OgPixelFormat::R8_SNORM:
		case OgPixelFormat::R8_UINT:
		case OgPixelFormat::R8_SINT:
		case OgPixelFormat::R8_SRGB:
			return 1;
		case OgPixelFormat::R8G8_UNORM:
		case OgPixelFormat::R8G8_SNORM:
		case OgPixelFormat::R8G8_UINT:
		case OgPixelFormat::R8G8_SINT:
		case OgPixelFormat::R8G8_SRGB:
		case OgPixelFormat::R16_UNORM:
		case OgPixelFormat::R16_SNORM:
		case OgPixelFormat::R16_UINT:
		case OgPixelFormat::R16_SINT:
		case OgPixelFormat::R16_SFLOAT:
			return 2;
		case OgPixelFormat::R8G8B8_UNORM:
		case OgPixelFormat::R8G8B8_SNORM:
		case OgPixelFormat::R8G8B8_UINT:
		case OgPixelFormat::R8G8B8_SINT:
		case OgPixelFormat::R8G8B8_SRGB:
		case OgPixelFormat::B8G8R8_UNORM:
		case OgPixelFormat::B8G8R8_SNORM:
		case OgPixelFormat::B8G8R8_UINT:
		case OgPixelFormat::B8G8R8_SINT:
		case OgPixelFormat::B8G8R8_SRGB:
			return 3;
		case OgPixelFormat::R8G8B8A8_UNORM:
		case OgPixelFormat::R8G8B8A8_SNORM:
		case OgPixelFormat::R8G8B8A8_UINT:
		case OgPixelFormat::R8G8B8A8_SINT:
		case OgPixelFormat::R8G8B8A8_SRGB:
		case OgPixelFormat::B8G8R8A8_UNORM:
		case OgPixelFormat::B8G8R8A8_SNORM:
		case OgPixelFormat::B8G8R8A8_UINT:
		case OgPixelFormat::B8G8R8A8_SINT:
		case OgPixelFormat::B8G8R8A8_SRGB:
		case OgPixelFormat::A8B8G8R8_UNORM_PACK32:
		case OgPixelFormat::A8B8G8R8_SNORM_PACK32:
		case OgPixelFormat::A8B8G8R8_UINT_PACK32:
		case OgPixelFormat::A8B8G8R8_SINT_PACK32:
		case OgPixelFormat::A8B8G8R8_SRGB_PACK32:
		case OgPixelFormat::A2R10G10B10_UNORM_PACK32:
		case OgPixelFormat::A2R10G10B10_SNORM_PACK32:
		case OgPixelFormat::A2R10G10B10_UINT_PACK32:
		case OgPixelFormat::A2R10G10B10_SINT_PACK32:
		case OgPixelFormat::A2B10G10R10_UNORM_PACK32:
		case OgPixelFormat::A2B10G10R10_SNORM_PACK32:
		case OgPixelFormat::A2B10G10R10_UINT_PACK32:
		case OgPixelFormat::A2B10G10R10_SINT_PACK32:
		case OgPixelFormat::R16G16_UNORM:
		case OgPixelFormat::R16G16_SNORM:
		case OgPixelFormat::R16G16_UINT:
		case OgPixelFormat::R16G16_SINT:
		case OgPixelFormat::R16G16_SFLOAT:
		case OgPixelFormat::R32_UINT:
		case OgPixelFormat::R32_SINT:
		case OgPixelFormat::R32_SFLOAT:
		case OgPixelFormat::B10G11R11_UFLOAT_PACK32:
		case OgPixelFormat::E5B9G9R9_UFLOAT_PACK32:
		case OgPixelFormat::D24_UNORM_S8_UINT:
			return 4;
		case OgPixelFormat::D32_SFLOAT_S8_UINT:
			return 5;
		case OgPixelFormat::R16G16B16_UNORM:
		case OgPixelFormat::R16G16B16_SNORM:
		case OgPixelFormat::R16G16B16_UINT:
		case OgPixelFormat::R16G16B16_SINT:
		case OgPixelFormat::R16G16B16_SFLOAT:
			return 6;
		case OgPixelFormat::R16G16B16A16_UNORM:
		case OgPixelFormat::R16G16B16A16_SNORM:
		case OgPixelFormat::R16G16B16A16_UINT:
		case OgPixelFormat::R16G16B16A16_SINT:
		case OgPixelFormat::R16G16B16A16_SFLOAT:
		case OgPixelFormat::R32G32_UINT:
		case OgPixelFormat::R32G32_SINT:
		case OgPixelFormat::R32G32_SFLOAT:
		case OgPixelFormat::R64_UINT:
		case OgPixelFormat::R64_SINT:
		case OgPixelFormat::R64_SFLOAT:
			return 8;
		case OgPixelFormat::R32G32B32_UINT:
		case OgPixelFormat::R32G32B32_SINT:
		case OgPixelFormat::R32G32B32_SFLOAT:
			return 12;
		case OgPixelFormat::R32G32B32A32_UINT:
		case OgPixelFormat::R32G32B32A32_SINT:
		case OgPixelFormat::R32G32B32A32_SFLOAT:
		case OgPixelFormat::R64G64_UINT:
		case OgPixelFormat::R64G64_SINT:
		case OgPixelFormat::R64G64_SFLOAT:
			return 16;
		case OgPixelFormat::R64G64B64_UINT:
		case OgPixelFormat::R64G64B64_SINT:
		case OgPixelFormat::R64G64B64_SFLOAT:
			return 24;
		default:
			LOGE(OG_ID, "GetSizeInBytes should not be used on a compressed format.");
			break;
		}

		return -1;
	}

	static uint GetPixelCount(OgPixelFormat format)
	{
		switch (format)
		{
		case OgPixelFormat::R8_UNORM:
		case OgPixelFormat::R8_SNORM:
		case OgPixelFormat::R8_UINT:
		case OgPixelFormat::R8_SINT:
		case OgPixelFormat::R8_SRGB:
		case OgPixelFormat::R16_UNORM:
		case OgPixelFormat::R16_SNORM:
		case OgPixelFormat::R16_UINT:
		case OgPixelFormat::R16_SINT:
		case OgPixelFormat::R16_SFLOAT:
		case OgPixelFormat::R32_UINT:
		case OgPixelFormat::R32_SINT:
		case OgPixelFormat::R32_SFLOAT:
		case OgPixelFormat::R64_UINT:
		case OgPixelFormat::R64_SINT:
		case OgPixelFormat::R64_SFLOAT:
			return 1;

		case OgPixelFormat::R8G8_UNORM:
		case OgPixelFormat::R8G8_SNORM:
		case OgPixelFormat::R8G8_UINT:
		case OgPixelFormat::R8G8_SINT:
		case OgPixelFormat::R8G8_SRGB:
		case OgPixelFormat::R16G16_UNORM:
		case OgPixelFormat::R16G16_SNORM:
		case OgPixelFormat::R16G16_UINT:
		case OgPixelFormat::R16G16_SINT:
		case OgPixelFormat::R16G16_SFLOAT:
		case OgPixelFormat::R32G32_UINT:
		case OgPixelFormat::R32G32_SINT:
		case OgPixelFormat::R32G32_SFLOAT:
		case OgPixelFormat::R64G64_UINT:
		case OgPixelFormat::R64G64_SINT:
		case OgPixelFormat::R64G64_SFLOAT:
			return 2;

		case OgPixelFormat::R8G8B8_UNORM:
		case OgPixelFormat::R8G8B8_SNORM:
		case OgPixelFormat::R8G8B8_UINT:
		case OgPixelFormat::R8G8B8_SINT:
		case OgPixelFormat::R8G8B8_SRGB:
		case OgPixelFormat::B8G8R8_UNORM:
		case OgPixelFormat::B8G8R8_SNORM:
		case OgPixelFormat::B8G8R8_UINT:
		case OgPixelFormat::B8G8R8_SINT:
		case OgPixelFormat::B8G8R8_SRGB:
		case OgPixelFormat::B10G11R11_UFLOAT_PACK32:
		case OgPixelFormat::R16G16B16_UNORM:
		case OgPixelFormat::R16G16B16_SNORM:
		case OgPixelFormat::R16G16B16_UINT:
		case OgPixelFormat::R16G16B16_SINT:
		case OgPixelFormat::R16G16B16_SFLOAT:
		case OgPixelFormat::R32G32B32_UINT:
		case OgPixelFormat::R32G32B32_SINT:
		case OgPixelFormat::R32G32B32_SFLOAT:
		case OgPixelFormat::R64G64B64_UINT:
		case OgPixelFormat::R64G64B64_SINT:
		case OgPixelFormat::R64G64B64_SFLOAT:
			return 3;

		case OgPixelFormat::R8G8B8A8_UNORM:
		case OgPixelFormat::R8G8B8A8_SNORM:
		case OgPixelFormat::R8G8B8A8_UINT:
		case OgPixelFormat::R8G8B8A8_SINT:
		case OgPixelFormat::R8G8B8A8_SRGB:
		case OgPixelFormat::B8G8R8A8_UNORM:
		case OgPixelFormat::B8G8R8A8_SNORM:
		case OgPixelFormat::B8G8R8A8_UINT:
		case OgPixelFormat::B8G8R8A8_SINT:
		case OgPixelFormat::B8G8R8A8_SRGB:
		case OgPixelFormat::A8B8G8R8_UNORM_PACK32:
		case OgPixelFormat::A8B8G8R8_SNORM_PACK32:
		case OgPixelFormat::A8B8G8R8_UINT_PACK32:
		case OgPixelFormat::A8B8G8R8_SINT_PACK32:
		case OgPixelFormat::A8B8G8R8_SRGB_PACK32:
		case OgPixelFormat::A2R10G10B10_UNORM_PACK32:
		case OgPixelFormat::A2R10G10B10_SNORM_PACK32:
		case OgPixelFormat::A2R10G10B10_UINT_PACK32:
		case OgPixelFormat::A2R10G10B10_SINT_PACK32:
		case OgPixelFormat::A2B10G10R10_UNORM_PACK32:
		case OgPixelFormat::A2B10G10R10_SNORM_PACK32:
		case OgPixelFormat::A2B10G10R10_UINT_PACK32:
		case OgPixelFormat::A2B10G10R10_SINT_PACK32:
		case OgPixelFormat::E5B9G9R9_UFLOAT_PACK32:
		case OgPixelFormat::R16G16B16A16_UNORM:
		case OgPixelFormat::R16G16B16A16_SNORM:
		case OgPixelFormat::R16G16B16A16_UINT:
		case OgPixelFormat::R16G16B16A16_SINT:
		case OgPixelFormat::R16G16B16A16_SFLOAT:
		case OgPixelFormat::R32G32B32A32_UINT:
		case OgPixelFormat::R32G32B32A32_SINT:
		case OgPixelFormat::R32G32B32A32_SFLOAT:
			return 4;

			// The Pixel Count of Depth Stencil Buffer will be 1 (추측)
		case OgPixelFormat::D24_UNORM_S8_UINT:
		case OgPixelFormat::D32_SFLOAT_S8_UINT:
			return 1;

		default:
			LOGE(OG_ID, "GetSizeInBytes should not be used on a compressed format.");
			break;
		}

		return -1;
	}

	static uint16 GetDepthSizeInBytes(OgPixelFormat format)
	{
		switch (format)
		{
		case OgPixelFormat::D16_UNORM:
		case OgPixelFormat::D16_UNORM_S8_UINT:
			return 2;

		case OgPixelFormat::X8_D24_UNORM_PACK32: // Vulkan은 Only-Depth24가 없음. GL은 D24만 지원하는게 있음.
		case OgPixelFormat::D24_UNORM_S8_UINT:
			return 3;

		case OgPixelFormat::D32_SFLOAT:
		case OgPixelFormat::D32_SFLOAT_S8_UINT:
			return 4;

		default:
			LOGD(OG_ID, "You may put the wrong format on getting depth size");
			return 0;
		}

		return -1;
	}

	static uint16 GetDepthSizeInBytes(OgRenderTextureFormat format)
	{
		switch (format)
		{
		case OgRenderTextureFormat::DEPTH16:
		case OgRenderTextureFormat::DEPTH16_STENCIL8:
			return 2;

		case OgRenderTextureFormat::DEPTH24:
		case OgRenderTextureFormat::DEPTH24_STENCIL8:
			return 3;

		case OgRenderTextureFormat::DEPTH32:
		case OgRenderTextureFormat::DEPTH32_STENCIL8:
			return 4;

		default:
			LOGD(OG_ID, "You may put the wrong format on getting depth size");
			return 0;
			break;
		}

		return -1;
	}

	static uint16 GetStencilSizeInBytes(OgPixelFormat format)
	{
		switch (format)
		{
		case OgPixelFormat::S8_UINT:
		case OgPixelFormat::D16_UNORM_S8_UINT:
		case OgPixelFormat::D24_UNORM_S8_UINT:
		case OgPixelFormat::D32_SFLOAT_S8_UINT:
			return 1;

		default:
			LOGD(OG_ID, "You may put the wrong format on getting stencil size");
			return 0;
		}

		return -1;
	}

	static uint16 GetStencilSizeInBytes(OgRenderTextureFormat format)
	{
		switch (format)
		{
		case OgRenderTextureFormat::STENCIL8:
		case OgRenderTextureFormat::DEPTH16_STENCIL8:
		case OgRenderTextureFormat::DEPTH24_STENCIL8:
		case OgRenderTextureFormat::DEPTH32_STENCIL8:
			return 1;

		default:
			LOGD(OG_ID, "You may put the wrong format on getting stencil size");
			return 0;
			break;
		}

		return -1;
	}


	static bool IsDepthFormat(OgPixelFormat format)
	{
		return format == OgPixelFormat::D16_UNORM
			|| format == OgPixelFormat::D32_SFLOAT
			|| format == OgPixelFormat::X8_D24_UNORM_PACK32;
	}

	static bool IsDepthFormat(OgRenderTextureFormat format)
	{
		return format == OgRenderTextureFormat::DEPTH16
			|| format == OgRenderTextureFormat::DEPTH24
			|| format == OgRenderTextureFormat::DEPTH32
			|| format == OgRenderTextureFormat::DEFAULT_DEPTH;
	}

	static bool IsStencilFormat(OgPixelFormat format)
	{
		return format == OgPixelFormat::S8_UINT;
	}

	static bool IsStencilFormat(OgRenderTextureFormat format)
	{
		return format == OgRenderTextureFormat::STENCIL8;
	}

	static bool IsDepthStencilFormat(OgPixelFormat format)
	{
		return format == OgPixelFormat::D16_UNORM_S8_UINT
			|| format == OgPixelFormat::D24_UNORM_S8_UINT
			|| format == OgPixelFormat::D32_SFLOAT_S8_UINT;
	}

	static bool IsDepthStencilFormat(OgRenderTextureFormat format)
	{
		return format == OgRenderTextureFormat::DEPTH16_STENCIL8
			|| format == OgRenderTextureFormat::DEPTH24_STENCIL8
			|| format == OgRenderTextureFormat::DEPTH32_STENCIL8
			|| format == OgRenderTextureFormat::DEFAULT_DEPTH_STENCIL;
	}

	static bool IsCompressedFormat(OgPixelFormat format)
	{
		return format == OgPixelFormat::BC1_RGB_UNORM_BLOCK
			|| format == OgPixelFormat::BC1_RGB_SRGB_BLOCK
			|| format == OgPixelFormat::BC1_RGBA_UNORM_BLOCK
			|| format == OgPixelFormat::BC1_RGBA_SRGB_BLOCK
			|| format == OgPixelFormat::BC2_UNORM_BLOCK
			|| format == OgPixelFormat::BC2_SRGB_BLOCK
			|| format == OgPixelFormat::BC3_UNORM_BLOCK
			|| format == OgPixelFormat::BC3_SRGB_BLOCK
			|| format == OgPixelFormat::BC4_UNORM_BLOCK
			|| format == OgPixelFormat::BC4_SNORM_BLOCK
			|| format == OgPixelFormat::BC5_UNORM_BLOCK
			|| format == OgPixelFormat::BC5_SNORM_BLOCK
			|| format == OgPixelFormat::BC6H_UFLOAT_BLOCK
			|| format == OgPixelFormat::BC6H_SFLOAT_BLOCK
			|| format == OgPixelFormat::BC7_UNORM_BLOCK
			|| format == OgPixelFormat::BC7_SRGB_BLOCK
			|| format == OgPixelFormat::ETC2_R8G8B8_UNORM_BLOCK
			|| format == OgPixelFormat::ETC2_R8G8B8_SRGB_BLOCK
			|| format == OgPixelFormat::ETC2_R8G8B8A1_UNORM_BLOCK
			|| format == OgPixelFormat::ETC2_R8G8B8A1_SRGB_BLOCK
			|| format == OgPixelFormat::ETC2_R8G8B8A8_UNORM_BLOCK
			|| format == OgPixelFormat::ETC2_R8G8B8A8_SRGB_BLOCK;
	}

	static uint GetBlockSizeInBytes(OgPixelFormat format)
	{
		switch (format)
		{
		case OgPixelFormat::BC1_RGB_UNORM_BLOCK:
		case OgPixelFormat::BC1_RGB_SRGB_BLOCK:
		case OgPixelFormat::BC1_RGBA_UNORM_BLOCK:
		case OgPixelFormat::BC1_RGBA_SRGB_BLOCK:
		case OgPixelFormat::BC4_UNORM_BLOCK:
		case OgPixelFormat::BC4_SNORM_BLOCK:
		case OgPixelFormat::ETC2_R8G8B8_UNORM_BLOCK:
			return 8;
		case OgPixelFormat::BC2_UNORM_BLOCK:
		case OgPixelFormat::BC2_SRGB_BLOCK:
		case OgPixelFormat::BC3_UNORM_BLOCK:
		case OgPixelFormat::BC3_SRGB_BLOCK:
		case OgPixelFormat::BC5_UNORM_BLOCK:
		case OgPixelFormat::BC7_UNORM_BLOCK:
		case OgPixelFormat::BC7_SRGB_BLOCK:
		case OgPixelFormat::ETC2_R8G8B8A8_UNORM_BLOCK:
			return 16;
		default:
			LOGE(OG_ID, "Not implement yet");
			break;

		}

		return 0;
	}

	static OgPixelFormat GetViewFamilyFormat(OgPixelFormat format)
	{
		switch (format)
		{
		case OgPixelFormat::R32G32B32A32_SFLOAT:
		case OgPixelFormat::R32G32B32A32_UINT:
		case OgPixelFormat::R32G32B32A32_SINT:
			return OgPixelFormat::R32G32B32A32_SFLOAT;
		case OgPixelFormat::R16G16B16A16_SFLOAT:
		case OgPixelFormat::R16G16B16A16_UNORM:
		case OgPixelFormat::R16G16B16A16_UINT:
		case OgPixelFormat::R16G16B16A16_SINT:
		case OgPixelFormat::R16G16B16A16_SNORM:
			return OgPixelFormat::R16G16B16A16_SFLOAT;
		case OgPixelFormat::R32G32_SFLOAT:
		case OgPixelFormat::R32G32_UINT:
		case OgPixelFormat::R32G32_SINT:
			return OgPixelFormat::R32G32_SFLOAT;
		case OgPixelFormat::A2R10G10B10_UNORM_PACK32:
		case OgPixelFormat::A2R10G10B10_UINT_PACK32:
			return OgPixelFormat::A2R10G10B10_UNORM_PACK32;
		case OgPixelFormat::R8G8B8A8_UNORM:
		case OgPixelFormat::R8G8B8A8_SRGB:
		case OgPixelFormat::R8G8B8A8_SNORM:
		case OgPixelFormat::R8G8B8A8_UINT:
		case OgPixelFormat::R8G8B8A8_SINT:
			return OgPixelFormat::R8G8B8A8_UNORM;
		case OgPixelFormat::R16G16_SFLOAT:
		case OgPixelFormat::R16G16_UNORM:
		case OgPixelFormat::R16G16_SNORM:
		case OgPixelFormat::R16G16_SINT:
		case OgPixelFormat::R16G16_UINT:
			return OgPixelFormat::R16G16_SFLOAT;
		case OgPixelFormat::R32_SFLOAT:
		case OgPixelFormat::R32_UINT:
		case OgPixelFormat::R32_SINT:
			return OgPixelFormat::R32_SFLOAT;
		case OgPixelFormat::R8G8_UNORM:
		case OgPixelFormat::R8G8_UINT:
		case OgPixelFormat::R8G8_SNORM:
		case OgPixelFormat::R8G8_SINT:
			return OgPixelFormat::R8G8_UNORM;
		case OgPixelFormat::R16_SFLOAT:
		case OgPixelFormat::R16_UNORM:
		case OgPixelFormat::R16_UINT:
		case OgPixelFormat::R16_SNORM:
		case OgPixelFormat::R16_SINT:
			return OgPixelFormat::R16_SFLOAT;
		case OgPixelFormat::BC1_RGB_UNORM_BLOCK:
		case OgPixelFormat::BC1_RGB_SRGB_BLOCK:
		case OgPixelFormat::BC1_RGBA_UNORM_BLOCK:
		case OgPixelFormat::BC1_RGBA_SRGB_BLOCK:
			return OgPixelFormat::BC1_RGB_UNORM_BLOCK;
		case OgPixelFormat::BC2_UNORM_BLOCK:
		case OgPixelFormat::BC2_SRGB_BLOCK:
			return OgPixelFormat::BC2_UNORM_BLOCK;
		case OgPixelFormat::BC3_UNORM_BLOCK:
		case OgPixelFormat::BC3_SRGB_BLOCK:
			return OgPixelFormat::BC3_UNORM_BLOCK;
		case OgPixelFormat::BC4_UNORM_BLOCK:
		case OgPixelFormat::BC4_SNORM_BLOCK:
			return OgPixelFormat::BC5_UNORM_BLOCK;
		case OgPixelFormat::BC5_UNORM_BLOCK:
		case OgPixelFormat::BC5_SNORM_BLOCK:
			return OgPixelFormat::BC5_UNORM_BLOCK;
		case OgPixelFormat::B8G8R8A8_UNORM:
		case OgPixelFormat::B8G8R8A8_SNORM:
			return OgPixelFormat::B8G8R8A8_UNORM;
		case OgPixelFormat::BC7_UNORM_BLOCK:
		case OgPixelFormat::BC7_SRGB_BLOCK:
			return OgPixelFormat::BC7_UNORM_BLOCK;
		default:
			LOGE(OG_ID, "Not implement yet");
			return OgPixelFormat::NONE;
		}
	}

	static OgPixelFormat GetPixelFormat(OgRenderTextureFormat format)
	{
		switch (format)
		{
		case OgRenderTextureFormat::STENCIL8:
			return OgPixelFormat::S8_UINT;

		case OgRenderTextureFormat::DEPTH16:
			return OgPixelFormat::D16_UNORM;

		case OgRenderTextureFormat::DEPTH24:
		{
#if !defined(__IOS__) && !defined(__MACOSX__)
			return OgPixelFormat::X8_D24_UNORM_PACK32;
#else
			LOGW(OG_ID, "METAL doesn't support DEPTH24 format. So It's changed DEPTH32 instead of");
			return OgPixelFormat::D32_SFLOAT;
#endif
		}
		case OgRenderTextureFormat::DEFAULT_DEPTH:
		case OgRenderTextureFormat::DEPTH32:
			return OgPixelFormat::D32_SFLOAT;

		case OgRenderTextureFormat::DEPTH16_STENCIL8:
			return OgPixelFormat::D16_UNORM_S8_UINT;

		case OgRenderTextureFormat::DEPTH24_STENCIL8:
			return OgPixelFormat::D24_UNORM_S8_UINT;

		case OgRenderTextureFormat::DEPTH32_STENCIL8:
			return OgPixelFormat::D32_SFLOAT_S8_UINT;

		case OgRenderTextureFormat::R5G6B5:
			return OgPixelFormat::R5G6B5_UNROM_PACK16;

		case OgRenderTextureFormat::R8G8B8:
			return OgPixelFormat::R8G8B8_UNORM;

		case OgRenderTextureFormat::DEFAULT_COLOR:
		case OgRenderTextureFormat::R8G8B8A8_UNORM:
			return OgPixelFormat::R8G8B8A8_UNORM;

		case OgRenderTextureFormat::R8G8B8A8_SNORM:
			return OgPixelFormat::R8G8B8A8_SNORM;

		case OgRenderTextureFormat::R8G8_UNORM:
			return OgPixelFormat::R8G8_UNORM;

		case OgRenderTextureFormat::B8G8R8A8:
			return OgPixelFormat::B8G8R8A8_UNORM;

		case OgRenderTextureFormat::R32G32:
			return OgPixelFormat::R32G32_SFLOAT;

		case OgRenderTextureFormat::R32G32B32:
			return OgPixelFormat::R32G32B32_SFLOAT;

		case OgRenderTextureFormat::R32G32B32A32:
			return OgPixelFormat::R32G32B32A32_SFLOAT;

		default:
			LOGE(OG_ID, "Not Supported Yet");
		}

		return OgPixelFormat::NONE;
	}

	static OgRenderTextureFormat GetRenderTextureFormat(OgPixelFormat format)
	{
		switch (format)
		{
		case OgPixelFormat::S8_UINT:
			return OgRenderTextureFormat::STENCIL8;

		case OgPixelFormat::D16_UNORM:
			return OgRenderTextureFormat::DEPTH16;

		case OgPixelFormat::X8_D24_UNORM_PACK32:
			return OgRenderTextureFormat::DEPTH24;

		case OgPixelFormat::D32_SFLOAT:
			return OgRenderTextureFormat::DEPTH32;

		case OgPixelFormat::D16_UNORM_S8_UINT:
			return OgRenderTextureFormat::DEPTH16_STENCIL8;

		case OgPixelFormat::D24_UNORM_S8_UINT:
			return OgRenderTextureFormat::DEPTH24_STENCIL8;

		case OgPixelFormat::D32_SFLOAT_S8_UINT:
			return OgRenderTextureFormat::DEPTH32_STENCIL8;

		case OgPixelFormat::R5G6B5_UNROM_PACK16:
			return OgRenderTextureFormat::R5G6B5;

		case OgPixelFormat::R8G8B8_UNORM:
			return OgRenderTextureFormat::R8G8B8;

		case OgPixelFormat::R8G8B8A8_UNORM:
			return OgRenderTextureFormat::R8G8B8A8_UNORM;

		case OgPixelFormat::R8G8B8A8_SNORM:
			return OgRenderTextureFormat::R8G8B8A8_SNORM;

		case OgPixelFormat::B8G8R8A8_UNORM:
			return OgRenderTextureFormat::B8G8R8A8;

		case OgPixelFormat::R8G8_UNORM:
			return OgRenderTextureFormat::R8G8_UNORM;

		case OgPixelFormat::R8G8_SNORM:
			return OgRenderTextureFormat::R8G8_SNORM;

		case OgPixelFormat::R32G32B32A32_SFLOAT:
			return OgRenderTextureFormat::R32G32B32A32;

		case OgPixelFormat::R32G32B32_SFLOAT:
			return OgRenderTextureFormat::R32G32B32;

		case OgPixelFormat::R32G32_SFLOAT:
			return OgRenderTextureFormat::R32G32;
		default:
			LOGE(OG_ID, "Not Supported Yet");
		}

		return OgRenderTextureFormat::NONE;
	}
};

class OgResourceSupplement
{
public:

	static bool IsOnlySampler(OgResourceType type)
	{
		return type == OgResourceType::SAMPLER;
	}

	static bool IsImageType(OgResourceType type)
	{
		switch (type)
		{
		case OgResourceType::COMBINED_IMAGE_SAMPLER:
		case OgResourceType::SAMPLED_IMAGE:
		case OgResourceType::STORAGE_IMAGE:
			return true;
		}

		return false;
	}

	static bool IsBufferType(OgResourceType type)
	{
		switch (type)
		{
		case OgResourceType::UNIFORM_TEXEL_BUFFER:
		case OgResourceType::STORAGE_TEXEL_BUFFER:
		case OgResourceType::UNIFORM_BUFFER:
		case OgResourceType::STORAGE_BUFFER:
		case OgResourceType::UNIFORM_BUFFER_DYNAMIC:
		case OgResourceType::STORAGE_BUFFER_DYNAMIC:
			return true;
		}

		return false;
	}
};


// http://devgit.com2us.com/TS/TPact/issues/33

enum class OgMemoryOption : uint8
{
	MAP_MANAGED,
	PRIVATE_GPU,
	STAGING
};

enum class OgBufferUsage : uint8
{
	UNIFORM = 0x00000010,
	INDEX = 0x00000040,
	VERTEX = 0x00000080,
};

enum class OgRenderFeature : uint8
{

};

enum class OgMemoryLayout : uint8
{
	STD140,
	// packed, shared are deprecated soon.
	PACKED,
	SHARED,
	// std430 is not impl yet
	//STD430,
};

struct OgBufferLayout;

enum class OgHandleType : uint8
{
	BUFFER,
	SAMPLER,
	TEXTURE,
	SHADER,
	PROGRAM,
	FRAMEBUFFER,
	RENDERPASS,
	RESOURCE_LAYOUT,
	RESOURCE_SET,
	PIPELINE,
	COMMAND_ENCODER
};

class OG_API OgHandle
{
public:
	OgHandle() = delete;
	OgHandle(OgHandleType type);
	virtual ~OgHandle();

	const char* name;

#if defined(_DEBUG)
	const char* instanceType = nullptr;
#endif
	virtual uint32 GetHashCode() const;

	OG_FORCEINLINE uint32 Retain()
	{
		int32 newRef = (int32)(_refCount += 1);

		//OG_CHECK(newRef > 0, "Wrong Retain Operation");
		return (uint32)newRef;
	}

	OG_FORCEINLINE uint32 Release()
	{
		int32 newRef = (_refCount -= 1);
		if (_refCount == 0)
		{
			_pendingDeleteQueue.push(this);
		}
		//OG_CHECK(newRef >= 0, "Wrong Release Operation");
		return (uint32)newRef;
	}

	OG_FORCEINLINE uint32 GetRefCount() const
	{
		return _refCount;
	}

	OG_FORCEINLINE OgHandleType GetType() const
	{
		return _type;
	}

	// Only 
	static void FlushPendingDeletes(class OgRenderContext* rc, bool forceDeferredDeleteFlush);
	static bool AdvanceFrame();	// next frame overflow

private:
	OgHandleType _type;

	uint32 _refCount;

private:
	static queue<OgHandle*> _pendingDeleteQueue;
	static uint32 _currentFrame;

	struct ResourceToDelete
	{
		list<OgHandle*> handles;
		uint32 frameDelete;
	};
	static list<ResourceToDelete> _deferredDeleteArray;
};

struct OG_API OgBufferHandle : public OgHandle
{
	OgBufferHandle();
	virtual ~OgBufferHandle();

	uint32 size;

	OgBufferUsage usage;

	OgMemoryOption option;

	//uint32 offset;

	virtual void Start(uint32 position = 0);

	virtual void Align(uint32 alignment);

	virtual void Read(void* data, uint32 size);

	virtual void Write(const void* data, uint32 size);

	virtual void End();

	virtual void Reset();
};

// TODO : SoOge inherit problem  
// L1 64KB, L2 2MB. if to allocate into CPU L2 cache, It has to be almost less than 2MB.
// Buffer,
// Align : http://www.songho.ca/misc/alignment/dataalign.html
struct OG_API OgByteBuffer
{
	uint8* pointer;

	uint32 capacity;

	uint32 position;

	bool isFixedSize;

	OgByteBuffer();

	OgByteBuffer(bool fixedSize);

	OgByteBuffer(uint32 capacitySize);

	OgByteBuffer(uint32 capacitySize, bool fixedSize);

	virtual ~OgByteBuffer();

	void Free();

	template<typename T>
	void Write(const T& in)
	{
		Align(Og_ALIGNOF(T));
		Write(reinterpret_cast<const uint8*>(&in), sizeof(T));
	}

	void Write(const void* data, uint32 dataSize);

	void Align(uint32 alignment);

	template<typename T>
	void Read(T& in)
	{
		Align(Og_ALIGNOF(T));
		Read(reinterpret_cast<uint8*>(&in), sizeof(T));
	}

	void Read(void* in, uint32 dataSize);

	template<typename T>
	void Skip()
	{
		Align(Og_ALIGNOF(T));
		Skip(sizeof(T));
	}

	const uint8* Skip(uint32 _size);

	void Reset();

	void Start(uint32 position = 0);

	void End();
};

struct OG_API OgShaderVariable
{
	OgShaderValueType	type = OgShaderValueType::Void;
	uint16				arrayCount = 0;	// array count == 0 means not array type. count >= 1 means [1]
	int16				structTypeDefineIndex = -1;	// type == Struct -> refer structDefineArrays with the index
	OgShaderVariable() { }
	OgShaderVariable(OgShaderValueType type, uint16 arrayCount = 0, int16 structDefineIndex = -1);
};


// https://www.khronos.org/opengl/wiki/Interface_Block_(GLSL)
// Same as Uniform Layout in GL
struct OG_API OgBufferLayout
{
	OgMemoryLayout				memoryLayout = OgMemoryLayout::STD140;
	list<OgShaderVariable>	variables;

	OgBufferLayout() {}

	OgBufferLayout(const OgBufferLayout& b)
		: memoryLayout(b.memoryLayout)
		, variables(b.variables)
	{}

	OgBufferLayout(OgBufferLayout&& b)
		: memoryLayout(b.memoryLayout)
		, variables(std::move(b.variables))
	{}

	OgBufferLayout& operator=(OgBufferLayout&& b)
	{
		if (this != &b)
		{
			memoryLayout = b.memoryLayout;
			variables = std::move(b.variables);
		}

		return *this;
	}

	OgBufferLayout& operator=(const OgBufferLayout& b)
	{
		if (this != &b)
		{
			memoryLayout = b.memoryLayout;
			variables = b.variables;
		}

		return *this;
	}

	int GetSizeInBytes(OgRenderPlatform platform, const list<list<OgShaderVariable>>* structDefineArrays = nullptr, bool metalStd140Layout = false) const;
};

struct OG_API OgUniformWriter
{
	OgBufferHandle& buffer;			// 8 bytes

	const OgBufferLayout& layout;	// 8 bytes

	OgRenderPlatform platform;		// 1 bytes

	uint8 maxAlign;					// 1 bytes

	uint8 valueIndex;				// 1 bytes

	bool metalStd140Layout;

	OgUniformWriter() = delete;

	OgUniformWriter(OgRenderPlatform platform, OgBufferHandle& buffer, const OgBufferLayout& layout,
		const list<list<OgShaderVariable>>* structDefineArray = nullptr, bool useMetalStd140Layout = false);

	void Start();

	void Reset();

	void End();

	// Should care the alignment of each graphics API platform (GLES 3.0 / Vulkan / Metal)
	void Write(void* pointer, uint32 size);

	void WriteBool(bool b, bool isStructMember = false);

	// array count should be equal or more than 1
	void WriteBool(bool* b, uint32 arrayCount, bool isStructMember = false);

	void WriteInt(int32 i, bool isStructMember = false);

	void WriteInt(int32* i, uint32 arrayCount, bool isStructMember = false);

	void WriteUint(uint32 u, bool isStructMember = false);

	void WriteUint(uint32* u, uint32 arrayCount, bool isStructMember = false);

	void WriteFloat(float f, bool isStructMember = false);

	void WriteFloat(float* f, uint32 arrayCount, bool isStructMember = false);
	
	void WriteVec2(const glm::vec2& v, bool isStructMember = false);

	void WriteVec2(glm::vec2* v, uint32 arrayCount, bool isStructMember = false);

	void WriteVec3(const glm::vec3& v, bool isStructMember = false);

	void WriteVec3(glm::vec3* v, uint32 arrayCount, bool isStructMember = false);

	void WriteVec4(const glm::vec4& v, bool isStructMember = false);

	void WriteVec4(glm::vec4* v, uint32 arrayCount, bool isStructMember = false);

	void WriteMat2(const glm::mat2& m, bool isStructMember = false);

	void WriteMat2(glm::mat2* m, uint32 arrayCount, bool isStructMember = false);

	void WriteMat3(const glm::mat3& m, bool isStructMember = false);

	void WriteMat3(glm::mat3* m, uint32 arrayCount, bool isStructMember = false);

	void WriteMat4(const glm::mat4& m, bool isStructMember = false);

	void WriteMat4(glm::mat4* m, uint32 arrayCount, bool isStructMember = false);

	// array count can be 0 or more than 1
	// return type is the calculated unpadded struct size for recursive function
	size_t WriteStruct(void* unpaddedStrcutData, uint32 structIndex, const list<list<OgShaderVariable>>& structDefineArrays, uint32 arrayCount, bool isStructMember = false);
};

struct OG_API OgSamplerInfo
{
	OgSamplerInfo();

	// TEX_CUBE Deprecated.
	OgSamplerType type;								// 1 byte

	OgSamplerCoord coordinate;						// 1 byte

	OgFilter magFilter : 4;

	OgFilter minFilter : 4;							// 1 byte

	OgSamplerMipmapMode mipmapMode : 4;

	OgSamplerAddressMode address : 4;				// 1 byte : 4 align

	OgSamplerAddressMode addressU : 4;

	// TODO : Vulkan 에서는 V 를 Repeat 으로 해야 잘 나온다. 해결해야함.
	OgSamplerAddressMode addressV : 4;				// 1 byte

	OgSamplerAddressMode addressW : 4;

	OgCompareOp compareOp : 4;						// 1 byte

	uint16 isCompareEnable : 1;						// 1 bit

	uint16 mipLevels : 7;							// 7 bit	: 1

	uint16 binding : 7;								// 7 bit

	uint16 isAnisotropyEnable : 1;					// 1 bit	: 1 - > 4 align

	float maxAnisotropy;							// 4 byte

	// TODO : borderColor;
};

struct OG_API OgSamplerHandle : public OgHandle
{
	OgSamplerInfo info;

	OgSamplerHandle();

	~OgSamplerHandle();
};

struct OG_API OgShaderHandle : public OgHandle
{
	OgShaderHandle();

	~OgShaderHandle();
};

struct OG_API OgProgramHandle : public OgHandle
{
	OgProgramHandle();

	~OgProgramHandle();
};

struct OG_API OgTextureInfo
{
	const char* name;						// 8 bytes

	OgTextureUsage usage;					// 2 bytes

	OgPixelFormat format;					// 1 byte

	OgTextureType type;						// 1 byte

	OgTextureViewType viewType;				// 1 byte

	OgSampleCountFlag samples;				// 1 byte

	uint16 isGenerateMipmaps : 1;

	uint16 arrayLayers : 7;					// 1 byte

	uint16 mipLevels : 8;					// 1 byte -> 8 align

	uint32 byteSize;						// 4 byte. byteSize should be enough large

	// TODO : should remove this structure for structure packing
	struct
	{
		uint16 width;
		uint16 height;
		uint16 depth;
	} extent;

	OgTextureInfo();
};


struct OG_API OgTextureHandle : public OgHandle
{
	OgTextureInfo info;

	OgSamplerHandle* sampler;

	OgTextureHandle();

	~OgTextureHandle();
};

// https://community.arm.com/graphics/b/blog/posts/mali-performance-2-how-to-correctly-buffers-framebuffers
struct OG_API OgAttachment
{
	OgAttachment();

	OgRenderTextureFormat format = OgRenderTextureFormat::DEFAULT_COLOR;

	OgRenderBufferLoadAction load = OgRenderBufferLoadAction::DONT_CARE;

	OgRenderBufferStoreAction store = OgRenderBufferStoreAction::STORE;

	bool isDepthStencilAttachment = false;

	OgSampleCountFlag sampleCount = OgSampleCountFlag::COUNT_1; // MSAA

};

struct OG_API OgFrameBufferHandle : OgHandle
{
	uint16 width;

	uint16 height;

	uint16 colorBufferCount : 15;

	uint16 useDepthStencilBuffer : 1;

	OgFrameBufferHandle();

	~OgFrameBufferHandle();
};

struct OG_API OgRenderPassInfo
{
	OgRenderPassInfo();
	~OgRenderPassInfo();

	OgAttachment* outputColorAttachments;

	OgAttachment* resoOgeColorAttachment;

	OgAttachment outputDepthStencilAttachment;

	uint8 outputColorAttachmentCount : 3;

	uint8 resoOgeColorAttachmentCount : 3;

	uint8 useDepthStencilAttacment : 1;

	uint8 isSwapchainRenderPass : 1;

	uint32 GetHashCode() const;

	/* TODO :	Subpass Desccription/Dependency Implementation
		Subpass 구조를 명시할(VkSubpassDescription) 또 다른 클래스 필요
		list<uint> inputColorAttachmentIndices;
		uint inputDepthStencilAttachmentIndex;
		bool useInputDepthStencil;
		list<uint> resoOgeColorAttachmentIndices;
		uint resoOgeDepthStencilAttachmentIndex;
		bool useResoOgeDepthStencil;
	*/
	// 아마 Subpass로 사용될??
	/*
	struct Iterator
	{
		uint index;

		OgAttachment* array;

		uint arrayLength;

		// OgRenderPassHandle* renderPass;

		uint clearFlags;

		Iterator();

		~Iterator();

		// void Set(OgRenderPassHandle& pass);

		bool HasNext();

		OgAttachment& Current();

		void Reset();

		void Clear();
	};
	*/
};

struct OG_API OgRenderPassHandle : OgHandle
{
	// TODO : SubPass
	OgRenderPassHandle();

	~OgRenderPassHandle();

	virtual uint32 GetHashCode() const override;

	OgRenderPassInfo info;
};

struct OG_API OgSwapChain
{
	uint16 bufferCount : 14;

	uint16 useDepthBuffer : 1;

	uint16 useStencilBuffer : 1;				// 2 bytes

	OgPixelFormat colorPixelFormat;				// 1

	OgRenderTextureFormat colorRenderFormat;	// 1

	OgPixelFormat depthPixelFormat;				// 1

	OgRenderTextureFormat depthRenderFormat;	// 1

	OgPixelFormat stencilPixelFormat;			// 1

	OgRenderTextureFormat stencilRenderFormat;	// 1

	OgSwapChain();
};

struct OG_API OgSwapChainInfo
{
	// always true on useColorBuffer of SwapChain

	bool useDepthBuffer = true;

	bool useStencilBuffer = true;

	// colorBufferFormat will be set automatically by engine

	OgRenderTextureFormat depthBufferFormat = OgRenderTextureFormat::DEFAULT_DEPTH_STENCIL;

	OgRenderTextureFormat stencilBufferFormat = OgRenderTextureFormat::DEFAULT_DEPTH_STENCIL;

	// for Multi Sampling Anti Alising(MSAA)
	bool useMSAA = false;

	OgSampleCountFlag msaaSampleCount = OgSampleCountFlag::COUNT_1;

};

struct OG_API OgFrameBufferInfo
{
	OgFrameBufferInfo();

	OgFrameBufferInfo(const OgFrameBufferInfo& o);

	OgRenderPassHandle* renderPass;

	list<OgTextureHandle*> colorBuffers;

	OgTextureHandle* resoOgeColorBuffer = nullptr;

	OgTextureHandle* depthStencilBuffer;

	uint16 width = 1, height = 1;
};

struct OG_API OgStencilOpState
{
	OgStencilOp    failOp = OgStencilOp::KEEP;
	OgStencilOp    passOp = OgStencilOp::KEEP;
	OgStencilOp    depthFailOp = OgStencilOp::KEEP;
	OgCompareOp    compareOp = OgCompareOp::ALWAYS;
	uint8       compareMask = 1; // ???
	uint8       writeMask = 1;
	uint8       reference = 0;
};

struct OG_API OgVertexBufferLayoutDescriptor
{
	OgVertexBufferLayoutDescriptor();
	explicit OgVertexBufferLayoutDescriptor(uint16 binding, uint16 stride);
	explicit OgVertexBufferLayoutDescriptor(uint16 binding, uint16 stride, bool instancing, uint16 stepRate);
	uint16      binding;
	uint16      stride;
	uint16		useInstancing;
	uint16      stepRate;
};

struct OG_API OgVertexAttributeDescriptor
{
	OgVertexAttributeDescriptor();

	explicit OgVertexAttributeDescriptor(uint16 binding, uint8 loc, OgVertexFormat format, uint32 offset);

	uint16				binding;	// 2 byte
	uint8				location;	// 1 bytes
	OgVertexFormat		format;		// 1 byte
	uint32				offset;		// 4 bytes
};

struct OG_API OgVertexInputDescriptor
{
	OgVertexBufferLayoutDescriptor* layouts;

	OgVertexAttributeDescriptor* attributes;

	uint32 layoutCount;

	uint32 attributeCount;

	OgVertexInputDescriptor();

	OgVertexInputDescriptor(const OgVertexInputDescriptor& o);

	~OgVertexInputDescriptor();

	void operator=(const OgVertexInputDescriptor& o);

	void CopyFrom(const OgVertexInputDescriptor& vid);

	void Release();
	bool isAlloced = false;
};

// OgResourceBinding can be used for either Uniform Buffer or Uniform Texture.
// When It's used as a uniform buffer, you should fill the 'OgBufferLayout layout' member
// When It's used as a uniform texture, you should fill the `const char* name' member
struct OG_API OgResourceBinding
{
	OgResourceType			type;

	OgShaderType			stage;

	uint16					binding;	// Similar to (binding = 0) on GLSL

	uint16					arrayCount;	// array가 아니라면 0으로 설정되어야 하고, array([n])이라면 1 이상으로 설정되어야 한다.

	const char*				name;		// Uniform Texture Name / Assume name is a literal (name = "something"). GLES3 Platform에 의해 항상 유지되어야 한다.

	OgResourceBinding();
	OgResourceBinding(const OgResourceBinding& o)
		:type(o.type)
		, stage(o.stage)
		, binding(o.binding)
		, arrayCount(o.arrayCount)
		, name(o.name)
	{

	}

	OgResourceBinding& operator=(const OgResourceBinding& o)
	{
		type = o.type;
		stage = (o.stage);
		binding = o.binding;
		arrayCount = o.arrayCount;
		name = o.name;
		return *this;
	}
};

struct OG_API OgResourceUsage
{
	OgResourceBinding binding;

	union
	{
		struct
		{
			OgTextureHandle** handle;
		} texture;

		struct
		{
			OgBufferHandle** handle;
			uint32* offset;
			uint32* range;
		} buffer;
	};

	OgResourceUsage();
};

struct OG_API OgResourceLayoutHandle : OgHandle
{
	OgResourceLayoutHandle();
};

struct OG_API OgResourceSetHandle : OgHandle
{
	OgResourceSetHandle();
};

struct OG_API OgShaderDescriptor
{
	OgProgramHandle*	program;
	OgShaderHandle*		shaders[2];	// @Chan : 만약 Vertex/Fragment말고 Geometry/Tesellation등을 쓰게 된다면 이것을 더 늘리면 된다.
	uint32				shaderCount;

	OgShaderDescriptor();

	~OgShaderDescriptor();

	void CopyFrom(const OgShaderDescriptor& sd);
};

struct OG_API OgColorBlendDescriptor
{
	struct Attachment
	{
		bool blendEnable = false;
		OgColorWriteMask writeMask = OgColorWriteMask::RED | OgColorWriteMask::GREEN | OgColorWriteMask::BLUE | OgColorWriteMask::ALPHA;
		OgBlendFactor     srcColor = OgBlendFactor::ONE;
		OgBlendFactor     dstColor = OgBlendFactor::ZERO;
		OgBlendOp         colorOp = OgBlendOp::ADD;
		OgBlendFactor     srcAlpha = OgBlendFactor::ONE;
		OgBlendFactor     dstAlpha = OgBlendFactor::ZERO;
		OgBlendOp         alphaOp = OgBlendOp::ADD;
	};

	Attachment attachments[4];

	uint32 attachmentCount;

	OgColorBlendDescriptor();

	~OgColorBlendDescriptor();

	uint32 GetHashCode() const;

	void CopyFrom(const OgColorBlendDescriptor& cbd);
};

struct OG_API OgRasterizationDescriptor
{
	// VkBool32                                   depthClampEnable;?
	OgPolygonMode    polygonMode = OgPolygonMode::FILL;
	OgCullMode       cullMode = OgCullMode::BACK;
	OgFrontFace      frontFace = OgFrontFace::COUNTER_CLOCKWISE;
	OgPrimitiveType  primitiveType = OgPrimitiveType::TRIANGLE_LIST;
	bool             scissorTest = false;
};

// https://open.gl/depthstencils
struct OG_API OgDepthStencilDescriptor
{
	bool                depthTest;
	bool                depthWrite;
	OgCompareOp         depthCompareOp = OgCompareOp::LESS;
	// VkBool32                                  depthBoundsTestEnable;?
	bool                stencilTest = false;
	OgStencilOpState    front;
	OgStencilOpState    back;
	// float                                     minDepthBounds;?
	// float                                     maxDepthBounds;?
};

struct OG_API OgPipelineDescriptor
{
	OgPipelineDescriptor();

	OgPipelineType				type;
	const char*					name;
	OgRenderPassHandle*			renderPass = nullptr;
	OgVertexInputDescriptor		vertexInput; // TODO : Rename vertexInput
	OgResourceLayoutHandle*		resourceLayout;
	OgShaderDescriptor			shader;
	OgRasterizationDescriptor	rasterize;

	// TODO : ColorBlend Descriptor Array
	OgColorBlendDescriptor		colorBlend;
	OgDepthStencilDescriptor	depthStencil;
};

struct OG_API OgPipelineHandle : OgHandle
{
	OgPipelineType type;

	OgRenderPassHandle* renderPass;

	OgResourceLayoutHandle* resourceLayout;

	OgPipelineHandle();
};

// TODO : change OgDrawDataEncoder
struct OG_API OgCommandEncoderHandle : OgHandle
{
	OgCommandEncoderHandle();

	virtual void Begin() = 0;

	virtual void SetViewport
	(
		const float x, const float y,
		const float width, const float height,
		const float minDepth = 0.0f, const float maxDepth = 1.0f
	) = 0;

	virtual void SetScissor(const int32 x, const int32 y, const uint32 width, const uint32 height) = 0;

	// Clear Color에 사용되는 값을 모두 0 ~ 1사이의 값으로 작은 precision을 사용해도 된다.
	// 따라서 넣어줄 때, type casting만 하면된다.
	union ClearValue
	{
		struct
		{
			float value[4];
		} color;

		struct
		{
			float depth;
			float stencil;
		} depthStencil;
	};

	struct Area
	{
		uint16 x, y, width, height;
		Area()
			: x(0), y(0), width(1), height(1)
		{ }
		Area(uint16 x, uint16 y, uint16 w, uint16 h)
			: x(x), y(y), width(w), height(h)
		{ }
	};

	virtual void BeginRenderPass
	(
		const OgRenderPassHandle* renderPass,
		const OgFrameBufferHandle* frameBuffer,
		const Area area,
		const uint8 colorAttachClearCount,
		const ClearValue* colorAttachmentClear,
		const uint8 resoOgeAttachClearCount,
		const ClearValue* resoOgeAttachmentClear,
		const ClearValue* depthAttachmentClear
	) = 0;

	virtual void BindPipeline(const OgPipelineHandle* pipeline) = 0;

	virtual void BindResourceSet(const OgResourceSetHandle* rSet) = 0;

	// const all levels for bufferHandle*
	virtual void BindVertexBuffers(const OgBufferHandle* const * vertexBuffers, const uint32* offsets, const uint8 bufferCount) = 0;

	virtual void BindIndexBuffer(const OgBufferHandle* indexBuffer, const  OgIndexType indexType = OgIndexType::UInt16) = 0;

	// firstIndex : byte offset
	virtual void DrawIndexed(const uint32 firstIndex, const uint32 indexCount, const uint32 instanceCount, const uint32 vertexOffset) = 0;

	// firstVertex : index of the first vertex to draw
	virtual void DrawArrays(const uint32 firstVertex, const  uint32 vertexCount, const  uint32 instanceCount) = 0;

	virtual void EndRenderPass() = 0;

	virtual void BeginDebugMarker(const char* label, float color[4]) = 0;

	virtual void EndDebugMarker() = 0;

	virtual void End() = 0;
};

struct OG_API OgResourceSetPool
{
	virtual OgResourceSetHandle* Allocate(OgResourceLayoutHandle* resourceSetLayoutHandle, OgResourceUsage* usages, uint32 usageCount) = 0;

	virtual void Deallocate(OgResourceSetHandle* resourceSetHandle) = 0;

	virtual void Reset() = 0;

	virtual ~OgResourceSetPool() {};

};

// This class acts as a bridge to connect the inside and outside of the renderer.
// OgBufferManager is class whis is used in Render.
struct OgBufferPendingDelete
{
	static bool AdvanceFrame();
	static void FlushPendingDeleteForFreeBuffer(bool bImmediatly);
};

	
OG_NAMESPACE_RENDER_END

#endif // _OG_RENDER_DEFINITIONS_H__
