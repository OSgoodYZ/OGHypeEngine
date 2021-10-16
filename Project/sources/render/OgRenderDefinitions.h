#pragma warning(disable:4251)
#pragma once
#ifndef _LV_RENDER_DEFINITIONS_H__
#define _LV_RENDER_DEFINITIONS_H__

#include "system/LvQueue.h"
#include "system/LvList.h"
#include "system/LvHashtable.h"
#include "system/math/LvVec2f.h"
#include "system/math/LvVec3f.h"
#include "system/math/LvVec4f.h"
#include "system/math/LvMat2f.h"
#include "system/math/LvMat3f.h"
#include "system/math/LvMat4f.h"

// TODO : https://docs.microsoft.com/ko-kr/cpp/cpp/explicitly-defaulted-and-deleted-functions?view=vs-2017
// TODO Alignment : http://www.songho.ca/misc/alignment/dataalign.html

namespace Lv { namespace Render { class LvRenderContext; } }

LV_NS_RENDER_BEGIN

	// Notice : derive enum class from uint8/uint16/uint32 
	// in order to decrease memory space
	
	enum class LvShaderType : uint8
	{
		VERTEX = 0x00000001,
		TESSELLATION_CONTROL = 0x00000002,
		TESSELLATION_EVALUATION = 0x00000004,
		GEOMETRY = 0x00000008,
		FRAGMENT = 0x00000010,
		COMPUTE = 0x00000020,
		// ALL_GRAPHICS = 0x0000001F,
		// ALL = 0x7FFFFFFF,
		// FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
	};

	enum class LvShaderValueType : uint8
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

	enum class LvRenderTextureFormat : uint8
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
	enum class LvPixelFormat : uint8
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
	enum class LvTextureFormat : uint8
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

	enum class LvVertexFormat : uint8
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

	enum class LvIndexType : uint8
	{
		UInt16,
		UInt32
	};

	// If your application will render all pixels of the attachment for a given frame, use the default load action MTLLoadActionDontCare.
	// The MTLLoadActionDontCare action allows the GPU to avoid loading the existing contents of the texture, ensuring the best performance.
	// Otherwise, you can use the MTLLoadActionClear action to clear the previous contents of the attachment, or the MTLLoadActionLoad action to preserve them.
	// The MTLLoadActionClear action also avoids loading the existing texture contents, but it incurs the cost of filling the destination with a solid color.
	enum class LvRenderBufferLoadAction : uint8
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
	enum class LvRenderBufferStoreAction : uint8
	{
		STORE,
		DONT_CARE
		//RESOLVE // TODO: resolve is not used in engine code
	};

	// Graphic Pipeline Command.
	enum class LvCommandType : uint8
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

	inline const char* lv_get_command_type_string(LvCommandType e)
	{
		switch (e)
		{
			case LvCommandType::END:						return "END";
			case LvCommandType::BEGIN:						return "BEGIN";
			case LvCommandType::BIND_PIPELINE:              return "BIND_PIPELINE";
			case LvCommandType::SET_VIEWPORT:               return "SET_VIEWPORT";
			case LvCommandType::SET_SCISSOR:                return "SET_SCISSOR";
			case LvCommandType::BEGIN_RENDERPASS:           return "BEGIN_RENDERPASS";
			case LvCommandType::END_RENDERPASS:             return "END_RENDERPASS";
			case LvCommandType::BIND_VERTEX_BUFFERS:        return "BIND_VERTEX_BUFFERS";
			case LvCommandType::BIND_INDEX_BUFFER:          return "BIND_INDEX_BUFFER";
			case LvCommandType::DRAW_INDEXED:               return "DRAW_INDEXED";
            case LvCommandType::DRAW_ARRAYS:                return "DRAW_ARRAYS";
			case LvCommandType::BIND_RESOURCESET:			return "BIND_RESOURCESET";
        }

		return "Invaild Enum";
	}

	//LV_DECLARE_ENUM(LvRenderPlatform, GLES2, GLES3, VULKAN, METAL, DX11, DX12)

	enum class LvRenderPlatform : uint8
	{
		NONE,
		GLES2,
		GLES3,
		VULKAN,
		METAL,
		DX11,
	};

	enum class LvPipelineType : uint8
	{
		GRAPHICS_PIPELINE,
		COMPUTE_PIPELINE
	};

	// https://www.khronos.org/opengl/wiki/Sampler_(GLSL)

	enum class LvSamplerType : uint8
	{
		TEX_1D = 0,
		TEX_2D = 1,
		TEX_3D = 2,
		TEX_CUBE = 3,
	};

	enum class LvSampleCountFlag : uint8
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
	enum class LvTextureType : uint8
	{
		TEX_1D = 0,
		TEX_2D = 1,
		TEX_3D = 2,

		//TEX_1D_ARRAY = 4,
		//TEX_2D_ARRAY = 5,
		//TEX_CUBE_ARRAY = 6,
	};

	// Image Buffer 어떻게 읽는지
	enum class LvTextureViewType : uint8
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
	enum class LvTextureUsage : uint16
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

	inline bool operator!=(LvTextureUsage a, uint16 b)
	{
		return static_cast<uint16>(a) != b;
	}

	inline LvTextureUsage operator&(LvTextureUsage a, LvTextureUsage b)
	{
		return static_cast<LvTextureUsage>(static_cast<uint16>(a) & static_cast<uint16>(b));
	}

	inline LvTextureUsage operator|(LvTextureUsage a, LvTextureUsage b)
	{
		return static_cast<LvTextureUsage>(static_cast<uint16>(a) | static_cast<uint16>(b));
	}

    static bool IsAttachmentUsage(const LvTextureUsage& usage)
    {
        if ((usage & LvTextureUsage::COLOR_ATTACHMENT) != 0) return true;
        if ((usage & LvTextureUsage::DEPTH_ATTACHMENT) != 0) return true;
        if ((usage & LvTextureUsage::STENCIL_ATTACHMENT) != 0) return true;
        if ((usage & LvTextureUsage::DEPTH_STENCIL_ATTACHMENT) != 0) return true;
        if ((usage & LvTextureUsage::TRANSIENT_ATTACHMENT) != 0) return true;

        return false;
    }

	enum class LvFilter : uint8
	{
		NEAREST = 0,
		LINEAR = 1,
	};

	enum class LvSamplerMipmapMode : uint8
	{
		NEAREST = 0,
		LINEAR = 1,
	};

	enum class LvSamplerCoord : uint8
	{
		NORMALIZED = 0,
		PIXEL = 1,
	};

	enum class LvSamplerAddressMode : uint8
	{
		REPEAT = 0,
		MIRRORED_REPEAT = 1,
		CLAMP_TO_EDGE = 2,
		//CLAMP_TO_BORDER = 3,
		//MIRROR_CLAMP_TO_EDGE = 4
	};

	enum class LvColorWriteMask : uint8
	{
		RED = 0x00000001,
		GREEN = 0x00000002,
		BLUE = 0x00000004,
		ALPHA = 0x00000008,
	};

	inline LvColorWriteMask operator&(LvColorWriteMask a, LvColorWriteMask b)
	{
		return static_cast<LvColorWriteMask>(static_cast<uint8>(a) & static_cast<uint8>(b));
	}

	inline LvColorWriteMask operator|(LvColorWriteMask a, LvColorWriteMask b)
	{
		return static_cast<LvColorWriteMask>(static_cast<uint8>(a) | static_cast<uint8>(b));
	}

	// https://m.blog.naver.com/PostView.nhn?blogId=itrainl4&logNo=90188723209&proxyReferer=https%3A%2F%2Fwww.google.com%2F
	enum class LvBlendFactor : uint8
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

	enum class LvBlendOp : uint8
	{
		ADD = 0,
		SUBTRACT = 1,
		REVERSE_SUBTRACT = 2,
		MIN = 3,
		MAX = 4,
	};

	enum class LvLogicOp : uint8
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

	enum class LvPolygonMode : uint8
	{
		FILL = 0,
		LINE = 1,
		POINT = 2,
		// FILL_RECTANGLE_NV = 1000153000, // ???
	};

	enum class LvPrimitiveType : uint8
	{
		POINT_LIST = 0,
		LINE_LIST = 1,
		LINE_STRIP = 2,
		TRIANGLE_LIST = 3,
		TRIANGLE_STRIP = 4,
		PATCH_LIST = 10,
	};

	enum class LvCullMode : uint8
	{
		NONE = 0,
		FRONT = 0x00000001,
		BACK = 0x00000002,
		FRONT_AND_BACK = 0x00000003,
	};

	enum class LvFrontFace : uint8
	{
		COUNTER_CLOCKWISE = 0,
		CLOCKWISE = 1,
	};

	enum class LvCompareOp : uint8
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

	enum class LvStencilOp : uint8
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
	enum class LvResourceType : uint8
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

	class LV_API LvDeviceCapability
	{
		// todo
		// https://github.com/bkaradzic/bgfx/blob/master/src/renderer_vk.cpp line 941
	};


	// https://github.com/KhronosGroup/KTX-Specification/issues/8
	// https://android.googlesource.com/platform/external/vulkan-validation-layers/+/HEAD/layers/vk_format_utils.cpp
	class LV_API LvFormatSupplement
	{

	public:

		static uint16 GetSizeInBytes(LvVertexFormat format)
		{
			/// OpenGL TyPe : https://www.khronos.org/opengl/wiki/OpenGL_Type
			/// OpenGL Vertex Spec : https://www.khronos.org/opengl/wiki/Vertex_Specification#Component_type
			/// GLES Precisoin of Types : http://learnwebgl.brown37.net/12_shader_language/glsl_data_types.html
			/// METAL : https://developer.apple.com/documentation/metal/mtlvertexformat 

			switch (format)
			{
				case LvVertexFormat::BYTE2_NORM:
				case LvVertexFormat::BYTE2:
				case LvVertexFormat::SBYTE2_NORM:
				case LvVertexFormat::SBYTE2:
					return 2;

				case LvVertexFormat::BYTE3_NORM:
				case LvVertexFormat::BYTE3:
				case LvVertexFormat::SBYTE3_NORM:
				case LvVertexFormat::SBYTE3:
					return 3;

				case LvVertexFormat::BYTE4_NORM:
				case LvVertexFormat::BYTE4:
				case LvVertexFormat::SBYTE4_NORM:
				case LvVertexFormat::SBYTE4:
				case LvVertexFormat::FLOAT1:
				case LvVertexFormat::UINT1:
				case LvVertexFormat::INT1:
				case LvVertexFormat::SHORT2_NORM:
				case LvVertexFormat::SHORT2:
				case LvVertexFormat::USHORT2_NORM:
				case LvVertexFormat::USHORT2:
				case LvVertexFormat::HALF2:
					return 4;

				case LvVertexFormat::SHORT3_NORM:
				case LvVertexFormat::SHORT3:
				case LvVertexFormat::USHORT3_NORM:
				case LvVertexFormat::USHORT3:
				case LvVertexFormat::HALF3:
					return 6;

				case LvVertexFormat::FLOAT2:
				case LvVertexFormat::UINT2:
				case LvVertexFormat::INT2:
				case LvVertexFormat::SHORT4_NORM:
				case LvVertexFormat::SHORT4:
				case LvVertexFormat::USHORT4_NORM:
				case LvVertexFormat::USHORT4:
				case LvVertexFormat::HALF4:
					return 8;

				case LvVertexFormat::FLOAT3:
				case LvVertexFormat::UINT3:
				case LvVertexFormat::INT3:
					return 12;

				case LvVertexFormat::FLOAT4:
				case LvVertexFormat::UINT4:
				case LvVertexFormat::INT4:
					return 16;
				default:
					return 0;
			}

			return 0;
		}

		static uint16 GetSizeInBytes(LvPixelFormat format)
		{
			switch (format)
			{
			case LvPixelFormat::R8_UNORM:
			case LvPixelFormat::R8_SNORM:
			case LvPixelFormat::R8_UINT:
			case LvPixelFormat::R8_SINT:
			case LvPixelFormat::R8_SRGB:
				return 1;
			case LvPixelFormat::R8G8_UNORM:
			case LvPixelFormat::R8G8_SNORM:
			case LvPixelFormat::R8G8_UINT:
			case LvPixelFormat::R8G8_SINT:
			case LvPixelFormat::R8G8_SRGB:
			case LvPixelFormat::R16_UNORM:
			case LvPixelFormat::R16_SNORM:
			case LvPixelFormat::R16_UINT:
			case LvPixelFormat::R16_SINT:
			case LvPixelFormat::R16_SFLOAT:
				return 2;
			case LvPixelFormat::R8G8B8_UNORM:
			case LvPixelFormat::R8G8B8_SNORM:
			case LvPixelFormat::R8G8B8_UINT:
			case LvPixelFormat::R8G8B8_SINT:
			case LvPixelFormat::R8G8B8_SRGB:
			case LvPixelFormat::B8G8R8_UNORM:
			case LvPixelFormat::B8G8R8_SNORM:
			case LvPixelFormat::B8G8R8_UINT:
			case LvPixelFormat::B8G8R8_SINT:
			case LvPixelFormat::B8G8R8_SRGB:
				return 3;
			case LvPixelFormat::R8G8B8A8_UNORM:
			case LvPixelFormat::R8G8B8A8_SNORM:
			case LvPixelFormat::R8G8B8A8_UINT:
			case LvPixelFormat::R8G8B8A8_SINT:
			case LvPixelFormat::R8G8B8A8_SRGB:
			case LvPixelFormat::B8G8R8A8_UNORM:
			case LvPixelFormat::B8G8R8A8_SNORM:
			case LvPixelFormat::B8G8R8A8_UINT:
			case LvPixelFormat::B8G8R8A8_SINT:
			case LvPixelFormat::B8G8R8A8_SRGB:
			case LvPixelFormat::A8B8G8R8_UNORM_PACK32:
			case LvPixelFormat::A8B8G8R8_SNORM_PACK32:
			case LvPixelFormat::A8B8G8R8_UINT_PACK32:
			case LvPixelFormat::A8B8G8R8_SINT_PACK32:
			case LvPixelFormat::A8B8G8R8_SRGB_PACK32:
			case LvPixelFormat::A2R10G10B10_UNORM_PACK32:
			case LvPixelFormat::A2R10G10B10_SNORM_PACK32:
			case LvPixelFormat::A2R10G10B10_UINT_PACK32:
			case LvPixelFormat::A2R10G10B10_SINT_PACK32:
			case LvPixelFormat::A2B10G10R10_UNORM_PACK32:
			case LvPixelFormat::A2B10G10R10_SNORM_PACK32:
			case LvPixelFormat::A2B10G10R10_UINT_PACK32:
			case LvPixelFormat::A2B10G10R10_SINT_PACK32:
			case LvPixelFormat::R16G16_UNORM:
			case LvPixelFormat::R16G16_SNORM:
			case LvPixelFormat::R16G16_UINT:
			case LvPixelFormat::R16G16_SINT:
			case LvPixelFormat::R16G16_SFLOAT:
			case LvPixelFormat::R32_UINT:
			case LvPixelFormat::R32_SINT:
			case LvPixelFormat::R32_SFLOAT:
			case LvPixelFormat::B10G11R11_UFLOAT_PACK32:
			case LvPixelFormat::E5B9G9R9_UFLOAT_PACK32:
			case LvPixelFormat::D24_UNORM_S8_UINT:
				return 4;
			case LvPixelFormat::D32_SFLOAT_S8_UINT:
				return 5;
			case LvPixelFormat::R16G16B16_UNORM:
			case LvPixelFormat::R16G16B16_SNORM:
			case LvPixelFormat::R16G16B16_UINT:
			case LvPixelFormat::R16G16B16_SINT:
			case LvPixelFormat::R16G16B16_SFLOAT:
				return 6;
			case LvPixelFormat::R16G16B16A16_UNORM:
			case LvPixelFormat::R16G16B16A16_SNORM:
			case LvPixelFormat::R16G16B16A16_UINT:
			case LvPixelFormat::R16G16B16A16_SINT:
			case LvPixelFormat::R16G16B16A16_SFLOAT:
			case LvPixelFormat::R32G32_UINT:
			case LvPixelFormat::R32G32_SINT:
			case LvPixelFormat::R32G32_SFLOAT:
			case LvPixelFormat::R64_UINT:
			case LvPixelFormat::R64_SINT:
			case LvPixelFormat::R64_SFLOAT:
				return 8;
			case LvPixelFormat::R32G32B32_UINT:
			case LvPixelFormat::R32G32B32_SINT:
			case LvPixelFormat::R32G32B32_SFLOAT:
				return 12;
			case LvPixelFormat::R32G32B32A32_UINT:
			case LvPixelFormat::R32G32B32A32_SINT:
			case LvPixelFormat::R32G32B32A32_SFLOAT:
			case LvPixelFormat::R64G64_UINT:
			case LvPixelFormat::R64G64_SINT:
			case LvPixelFormat::R64G64_SFLOAT:
				return 16;
			case LvPixelFormat::R64G64B64_UINT:
			case LvPixelFormat::R64G64B64_SINT:
			case LvPixelFormat::R64G64B64_SFLOAT:
				return 24;
			default:
				LOGE(LV_ID, "GetSizeInBytes should not be used on a compressed format.");
				break;
			}

			return -1;
		}

		static uint GetPixelCount(LvPixelFormat format)
		{
			switch (format)
			{
			case LvPixelFormat::R8_UNORM:
			case LvPixelFormat::R8_SNORM:
			case LvPixelFormat::R8_UINT:
			case LvPixelFormat::R8_SINT:
			case LvPixelFormat::R8_SRGB:
			case LvPixelFormat::R16_UNORM:
			case LvPixelFormat::R16_SNORM:
			case LvPixelFormat::R16_UINT:
			case LvPixelFormat::R16_SINT:
			case LvPixelFormat::R16_SFLOAT:
			case LvPixelFormat::R32_UINT:
			case LvPixelFormat::R32_SINT:
			case LvPixelFormat::R32_SFLOAT:
			case LvPixelFormat::R64_UINT:
			case LvPixelFormat::R64_SINT:
			case LvPixelFormat::R64_SFLOAT:
				return 1;

			case LvPixelFormat::R8G8_UNORM:
			case LvPixelFormat::R8G8_SNORM:
			case LvPixelFormat::R8G8_UINT:
			case LvPixelFormat::R8G8_SINT:
			case LvPixelFormat::R8G8_SRGB:
			case LvPixelFormat::R16G16_UNORM:
			case LvPixelFormat::R16G16_SNORM:
			case LvPixelFormat::R16G16_UINT:
			case LvPixelFormat::R16G16_SINT:
			case LvPixelFormat::R16G16_SFLOAT:
			case LvPixelFormat::R32G32_UINT:
			case LvPixelFormat::R32G32_SINT:
			case LvPixelFormat::R32G32_SFLOAT:
			case LvPixelFormat::R64G64_UINT:
			case LvPixelFormat::R64G64_SINT:
			case LvPixelFormat::R64G64_SFLOAT:
				return 2;

			case LvPixelFormat::R8G8B8_UNORM:
			case LvPixelFormat::R8G8B8_SNORM:
			case LvPixelFormat::R8G8B8_UINT:
			case LvPixelFormat::R8G8B8_SINT:
			case LvPixelFormat::R8G8B8_SRGB:
			case LvPixelFormat::B8G8R8_UNORM:
			case LvPixelFormat::B8G8R8_SNORM:
			case LvPixelFormat::B8G8R8_UINT:
			case LvPixelFormat::B8G8R8_SINT:
			case LvPixelFormat::B8G8R8_SRGB:
			case LvPixelFormat::B10G11R11_UFLOAT_PACK32:
			case LvPixelFormat::R16G16B16_UNORM:
			case LvPixelFormat::R16G16B16_SNORM:
			case LvPixelFormat::R16G16B16_UINT:
			case LvPixelFormat::R16G16B16_SINT:
			case LvPixelFormat::R16G16B16_SFLOAT:
			case LvPixelFormat::R32G32B32_UINT:
			case LvPixelFormat::R32G32B32_SINT:
			case LvPixelFormat::R32G32B32_SFLOAT:
			case LvPixelFormat::R64G64B64_UINT:
			case LvPixelFormat::R64G64B64_SINT:
			case LvPixelFormat::R64G64B64_SFLOAT:
				return 3;

			case LvPixelFormat::R8G8B8A8_UNORM:
			case LvPixelFormat::R8G8B8A8_SNORM:
			case LvPixelFormat::R8G8B8A8_UINT:
			case LvPixelFormat::R8G8B8A8_SINT:
			case LvPixelFormat::R8G8B8A8_SRGB:
			case LvPixelFormat::B8G8R8A8_UNORM:
			case LvPixelFormat::B8G8R8A8_SNORM:
			case LvPixelFormat::B8G8R8A8_UINT:
			case LvPixelFormat::B8G8R8A8_SINT:
			case LvPixelFormat::B8G8R8A8_SRGB:
			case LvPixelFormat::A8B8G8R8_UNORM_PACK32:
			case LvPixelFormat::A8B8G8R8_SNORM_PACK32:
			case LvPixelFormat::A8B8G8R8_UINT_PACK32:
			case LvPixelFormat::A8B8G8R8_SINT_PACK32:
			case LvPixelFormat::A8B8G8R8_SRGB_PACK32:
			case LvPixelFormat::A2R10G10B10_UNORM_PACK32:
			case LvPixelFormat::A2R10G10B10_SNORM_PACK32:
			case LvPixelFormat::A2R10G10B10_UINT_PACK32:
			case LvPixelFormat::A2R10G10B10_SINT_PACK32:
			case LvPixelFormat::A2B10G10R10_UNORM_PACK32:
			case LvPixelFormat::A2B10G10R10_SNORM_PACK32:
			case LvPixelFormat::A2B10G10R10_UINT_PACK32:
			case LvPixelFormat::A2B10G10R10_SINT_PACK32:
			case LvPixelFormat::E5B9G9R9_UFLOAT_PACK32:
			case LvPixelFormat::R16G16B16A16_UNORM:
			case LvPixelFormat::R16G16B16A16_SNORM:
			case LvPixelFormat::R16G16B16A16_UINT:
			case LvPixelFormat::R16G16B16A16_SINT:
			case LvPixelFormat::R16G16B16A16_SFLOAT:
			case LvPixelFormat::R32G32B32A32_UINT:
			case LvPixelFormat::R32G32B32A32_SINT:
			case LvPixelFormat::R32G32B32A32_SFLOAT:
				return 4;

			// The Pixel Count of Depth Stencil Buffer will be 1 (추측)
			case LvPixelFormat::D24_UNORM_S8_UINT:
			case LvPixelFormat::D32_SFLOAT_S8_UINT:
				return 1;

			default:
				LOGE(LV_ID, "GetSizeInBytes should not be used on a compressed format.");
				break;
			}

			return -1;
		}

		static uint16 GetDepthSizeInBytes(LvPixelFormat format)
		{
			switch (format)
			{
			case LvPixelFormat::D16_UNORM:
			case LvPixelFormat::D16_UNORM_S8_UINT:
				return 2;
			
			case LvPixelFormat::X8_D24_UNORM_PACK32: // Vulkan은 Only-Depth24가 없음. GL은 D24만 지원하는게 있음.
			case LvPixelFormat::D24_UNORM_S8_UINT:
				return 3;

			case LvPixelFormat::D32_SFLOAT:
			case LvPixelFormat::D32_SFLOAT_S8_UINT:
				return 4;

			default:
				LOGD(LV_ID, "You may put the wrong format on getting depth size");
				return 0;
			}

			return -1;
		}

		static uint16 GetDepthSizeInBytes(LvRenderTextureFormat format)
		{
			switch (format)
			{
			case LvRenderTextureFormat::DEPTH16:
			case LvRenderTextureFormat::DEPTH16_STENCIL8:
				return 2;

			case LvRenderTextureFormat::DEPTH24:
			case LvRenderTextureFormat::DEPTH24_STENCIL8:
				return 3;

			case LvRenderTextureFormat::DEPTH32:
			case LvRenderTextureFormat::DEPTH32_STENCIL8:
				return 4;

			default:
				LOGD(LV_ID, "You may put the wrong format on getting depth size");
				return 0;
				break;
			}

			return -1;
		}

		static uint16 GetStencilSizeInBytes(LvPixelFormat format)
		{
			switch (format)
			{
			case LvPixelFormat::S8_UINT:
			case LvPixelFormat::D16_UNORM_S8_UINT:
			case LvPixelFormat::D24_UNORM_S8_UINT:
			case LvPixelFormat::D32_SFLOAT_S8_UINT:
				return 1;

			default:
				LOGD(LV_ID, "You may put the wrong format on getting stencil size");
				return 0;
			}
			
			return -1;
		}

		static uint16 GetStencilSizeInBytes(LvRenderTextureFormat format)
		{
			switch (format)
			{
			case LvRenderTextureFormat::STENCIL8:
			case LvRenderTextureFormat::DEPTH16_STENCIL8:
			case LvRenderTextureFormat::DEPTH24_STENCIL8:
			case LvRenderTextureFormat::DEPTH32_STENCIL8:
				return 1;

			default:
				LOGD(LV_ID, "You may put the wrong format on getting stencil size");
				return 0;
				break;
			}

			return -1;
		}


		static bool IsDepthFormat(LvPixelFormat format)
		{
			return format == LvPixelFormat::D16_UNORM
				|| format == LvPixelFormat::D32_SFLOAT
				|| format == LvPixelFormat::X8_D24_UNORM_PACK32;
		}

		static bool IsDepthFormat(LvRenderTextureFormat format)
		{
			return format == LvRenderTextureFormat::DEPTH16
				|| format == LvRenderTextureFormat::DEPTH24
				|| format == LvRenderTextureFormat::DEPTH32
				|| format == LvRenderTextureFormat::DEFAULT_DEPTH;
		}

		static bool IsStencilFormat(LvPixelFormat format)
		{
			return format == LvPixelFormat::S8_UINT;
		}

		static bool IsStencilFormat(LvRenderTextureFormat format)
		{
			return format == LvRenderTextureFormat::STENCIL8;
		}

		static bool IsDepthStencilFormat(LvPixelFormat format)
		{
			return format == LvPixelFormat::D16_UNORM_S8_UINT
				|| format == LvPixelFormat::D24_UNORM_S8_UINT
				|| format == LvPixelFormat::D32_SFLOAT_S8_UINT;
		}

		static bool IsDepthStencilFormat(LvRenderTextureFormat format)
		{
			return format == LvRenderTextureFormat::DEPTH16_STENCIL8
				|| format == LvRenderTextureFormat::DEPTH24_STENCIL8
				|| format == LvRenderTextureFormat::DEPTH32_STENCIL8
				|| format == LvRenderTextureFormat::DEFAULT_DEPTH_STENCIL;
		}

		static bool IsCompressedFormat(LvPixelFormat format)
		{
			return format == LvPixelFormat::BC1_RGB_UNORM_BLOCK
				|| format == LvPixelFormat::BC1_RGB_SRGB_BLOCK
				|| format == LvPixelFormat::BC1_RGBA_UNORM_BLOCK
				|| format == LvPixelFormat::BC1_RGBA_SRGB_BLOCK
				|| format == LvPixelFormat::BC2_UNORM_BLOCK
				|| format == LvPixelFormat::BC2_SRGB_BLOCK
				|| format == LvPixelFormat::BC3_UNORM_BLOCK
				|| format == LvPixelFormat::BC3_SRGB_BLOCK
				|| format == LvPixelFormat::BC4_UNORM_BLOCK
				|| format == LvPixelFormat::BC4_SNORM_BLOCK
				|| format == LvPixelFormat::BC5_UNORM_BLOCK
				|| format == LvPixelFormat::BC5_SNORM_BLOCK
				|| format == LvPixelFormat::BC6H_UFLOAT_BLOCK
				|| format == LvPixelFormat::BC6H_SFLOAT_BLOCK
				|| format == LvPixelFormat::BC7_UNORM_BLOCK
				|| format == LvPixelFormat::BC7_SRGB_BLOCK
				|| format == LvPixelFormat::ETC2_R8G8B8_UNORM_BLOCK
				|| format == LvPixelFormat::ETC2_R8G8B8_SRGB_BLOCK
				|| format == LvPixelFormat::ETC2_R8G8B8A1_UNORM_BLOCK
				|| format == LvPixelFormat::ETC2_R8G8B8A1_SRGB_BLOCK
				|| format == LvPixelFormat::ETC2_R8G8B8A8_UNORM_BLOCK
				|| format == LvPixelFormat::ETC2_R8G8B8A8_SRGB_BLOCK;
		}

		static uint GetBlockSizeInBytes(LvPixelFormat format)
		{
			switch (format)
			{
			case LvPixelFormat::BC1_RGB_UNORM_BLOCK:
			case LvPixelFormat::BC1_RGB_SRGB_BLOCK:
			case LvPixelFormat::BC1_RGBA_UNORM_BLOCK:
			case LvPixelFormat::BC1_RGBA_SRGB_BLOCK:
			case LvPixelFormat::BC4_UNORM_BLOCK:
			case LvPixelFormat::BC4_SNORM_BLOCK:
			case LvPixelFormat::ETC2_R8G8B8_UNORM_BLOCK:
				return 8;
			case LvPixelFormat::BC2_UNORM_BLOCK:
			case LvPixelFormat::BC2_SRGB_BLOCK:
			case LvPixelFormat::BC3_UNORM_BLOCK:
			case LvPixelFormat::BC3_SRGB_BLOCK:
			case LvPixelFormat::BC5_UNORM_BLOCK:
			case LvPixelFormat::BC7_UNORM_BLOCK:
			case LvPixelFormat::BC7_SRGB_BLOCK:
			case LvPixelFormat::ETC2_R8G8B8A8_UNORM_BLOCK:
				return 16;
			default:
				LOGE(LV_ID, "Not implement yet");
				break;

			}

			return 0;
		}

		static LvPixelFormat GetViewFamilyFormat(LvPixelFormat format)
		{
			switch (format)
			{
			case LvPixelFormat::R32G32B32A32_SFLOAT:
			case LvPixelFormat::R32G32B32A32_UINT:
			case LvPixelFormat::R32G32B32A32_SINT:
				return LvPixelFormat::R32G32B32A32_SFLOAT;
			case LvPixelFormat::R16G16B16A16_SFLOAT:
			case LvPixelFormat::R16G16B16A16_UNORM:
			case LvPixelFormat::R16G16B16A16_UINT:
			case LvPixelFormat::R16G16B16A16_SINT:
			case LvPixelFormat::R16G16B16A16_SNORM:
				return LvPixelFormat::R16G16B16A16_SFLOAT;
			case LvPixelFormat::R32G32_SFLOAT:
			case LvPixelFormat::R32G32_UINT:
			case LvPixelFormat::R32G32_SINT:
				return LvPixelFormat::R32G32_SFLOAT;
			case LvPixelFormat::A2R10G10B10_UNORM_PACK32:
			case LvPixelFormat::A2R10G10B10_UINT_PACK32:
				return LvPixelFormat::A2R10G10B10_UNORM_PACK32;
			case LvPixelFormat::R8G8B8A8_UNORM:
			case LvPixelFormat::R8G8B8A8_SRGB:
			case LvPixelFormat::R8G8B8A8_SNORM:
			case LvPixelFormat::R8G8B8A8_UINT:
			case LvPixelFormat::R8G8B8A8_SINT:
				return LvPixelFormat::R8G8B8A8_UNORM;
			case LvPixelFormat::R16G16_SFLOAT:
			case LvPixelFormat::R16G16_UNORM:
			case LvPixelFormat::R16G16_SNORM:
			case LvPixelFormat::R16G16_SINT:
			case LvPixelFormat::R16G16_UINT:
				return LvPixelFormat::R16G16_SFLOAT;
			case LvPixelFormat::R32_SFLOAT:
			case LvPixelFormat::R32_UINT:
			case LvPixelFormat::R32_SINT:
				return LvPixelFormat::R32_SFLOAT;
			case LvPixelFormat::R8G8_UNORM:
			case LvPixelFormat::R8G8_UINT:
			case LvPixelFormat::R8G8_SNORM:
			case LvPixelFormat::R8G8_SINT:
				return LvPixelFormat::R8G8_UNORM;
			case LvPixelFormat::R16_SFLOAT:
			case LvPixelFormat::R16_UNORM:
			case LvPixelFormat::R16_UINT:
			case LvPixelFormat::R16_SNORM:
			case LvPixelFormat::R16_SINT:
				return LvPixelFormat::R16_SFLOAT;
			case LvPixelFormat::BC1_RGB_UNORM_BLOCK:
			case LvPixelFormat::BC1_RGB_SRGB_BLOCK:
			case LvPixelFormat::BC1_RGBA_UNORM_BLOCK:
			case LvPixelFormat::BC1_RGBA_SRGB_BLOCK:
				return LvPixelFormat::BC1_RGB_UNORM_BLOCK;
			case LvPixelFormat::BC2_UNORM_BLOCK:
			case LvPixelFormat::BC2_SRGB_BLOCK:
				return LvPixelFormat::BC2_UNORM_BLOCK;
			case LvPixelFormat::BC3_UNORM_BLOCK:
			case LvPixelFormat::BC3_SRGB_BLOCK:
				return LvPixelFormat::BC3_UNORM_BLOCK;
			case LvPixelFormat::BC4_UNORM_BLOCK:
			case LvPixelFormat::BC4_SNORM_BLOCK:
				return LvPixelFormat::BC5_UNORM_BLOCK;
			case LvPixelFormat::BC5_UNORM_BLOCK:
			case LvPixelFormat::BC5_SNORM_BLOCK:
				return LvPixelFormat::BC5_UNORM_BLOCK;
			case LvPixelFormat::B8G8R8A8_UNORM:
			case LvPixelFormat::B8G8R8A8_SNORM:
				return LvPixelFormat::B8G8R8A8_UNORM;
			case LvPixelFormat::BC7_UNORM_BLOCK:
			case LvPixelFormat::BC7_SRGB_BLOCK:
				return LvPixelFormat::BC7_UNORM_BLOCK;
			default:
				LOGE(LV_ID, "Not implement yet");
				return LvPixelFormat::NONE;
			}
		}

		static LvPixelFormat GetPixelFormat(LvRenderTextureFormat format)
		{
			switch (format)
			{
			case LvRenderTextureFormat::STENCIL8:
				return LvPixelFormat::S8_UINT;

			case LvRenderTextureFormat::DEPTH16:
				return LvPixelFormat::D16_UNORM;

			case LvRenderTextureFormat::DEPTH24:
            {
#if !defined(__IOS__) && !defined(__MACOSX__)
                return LvPixelFormat::X8_D24_UNORM_PACK32;
#else
				LOGW(LV_ID, "METAL doesn't support DEPTH24 format. So It's changed DEPTH32 instead of");
				return LvPixelFormat::D32_SFLOAT;
#endif
            }
			case LvRenderTextureFormat::DEFAULT_DEPTH:
			case LvRenderTextureFormat::DEPTH32:
				return LvPixelFormat::D32_SFLOAT;

			case LvRenderTextureFormat::DEPTH16_STENCIL8:
				return LvPixelFormat::D16_UNORM_S8_UINT;

			case LvRenderTextureFormat::DEPTH24_STENCIL8:
				return LvPixelFormat::D24_UNORM_S8_UINT;

			case LvRenderTextureFormat::DEPTH32_STENCIL8:
				return LvPixelFormat::D32_SFLOAT_S8_UINT;

			case LvRenderTextureFormat::R5G6B5:
				return LvPixelFormat::R5G6B5_UNROM_PACK16;

			case LvRenderTextureFormat::R8G8B8:
				return LvPixelFormat::R8G8B8_UNORM;

			case LvRenderTextureFormat::DEFAULT_COLOR:
			case LvRenderTextureFormat::R8G8B8A8_UNORM:
				return LvPixelFormat::R8G8B8A8_UNORM;

			case LvRenderTextureFormat::R8G8B8A8_SNORM:
				return LvPixelFormat::R8G8B8A8_SNORM;

			case LvRenderTextureFormat::R8G8_UNORM:
				return LvPixelFormat::R8G8_UNORM;

			case LvRenderTextureFormat::B8G8R8A8:
				return LvPixelFormat::B8G8R8A8_UNORM;

			case LvRenderTextureFormat::R32G32:
				return LvPixelFormat::R32G32_SFLOAT;

			case LvRenderTextureFormat::R32G32B32:
				return LvPixelFormat::R32G32B32_SFLOAT;

			case LvRenderTextureFormat::R32G32B32A32:
				return LvPixelFormat::R32G32B32A32_SFLOAT;

			default:
				LOGE(LV_ID, "Not Supported Yet");
			}

			return LvPixelFormat::NONE;
		}

		static LvRenderTextureFormat GetRenderTextureFormat(LvPixelFormat format)
		{
			switch (format)
			{
			case LvPixelFormat::S8_UINT:
				return LvRenderTextureFormat::STENCIL8;

			case LvPixelFormat::D16_UNORM:
				return LvRenderTextureFormat::DEPTH16;

			case LvPixelFormat::X8_D24_UNORM_PACK32:
				return LvRenderTextureFormat::DEPTH24;

			case LvPixelFormat::D32_SFLOAT:
				return LvRenderTextureFormat::DEPTH32;

			case LvPixelFormat::D16_UNORM_S8_UINT:
				return LvRenderTextureFormat::DEPTH16_STENCIL8;

			case LvPixelFormat::D24_UNORM_S8_UINT:
				return LvRenderTextureFormat::DEPTH24_STENCIL8;

			case LvPixelFormat::D32_SFLOAT_S8_UINT:
				return LvRenderTextureFormat::DEPTH32_STENCIL8;

			case LvPixelFormat::R5G6B5_UNROM_PACK16:
				return LvRenderTextureFormat::R5G6B5;

			case LvPixelFormat::R8G8B8_UNORM:
				return LvRenderTextureFormat::R8G8B8;

			case LvPixelFormat::R8G8B8A8_UNORM:
				return LvRenderTextureFormat::R8G8B8A8_UNORM;

			case LvPixelFormat::R8G8B8A8_SNORM:
				return LvRenderTextureFormat::R8G8B8A8_SNORM;

			case LvPixelFormat::B8G8R8A8_UNORM:
				return LvRenderTextureFormat::B8G8R8A8;

			case LvPixelFormat::R8G8_UNORM:
				return LvRenderTextureFormat::R8G8_UNORM;

			case LvPixelFormat::R8G8_SNORM:
				return LvRenderTextureFormat::R8G8_SNORM;

			case LvPixelFormat::R32G32B32A32_SFLOAT:
				return LvRenderTextureFormat::R32G32B32A32;

			case LvPixelFormat::R32G32B32_SFLOAT:
				return LvRenderTextureFormat::R32G32B32;

			case LvPixelFormat::R32G32_SFLOAT:
				return LvRenderTextureFormat::R32G32;
			default:
				LOGE(LV_ID, "Not Supported Yet");
			}
		
			return LvRenderTextureFormat::NONE;
		}
	};

	class LvResourceSupplement
	{
	public:

		static bool IsOnlySampler(LvResourceType type)
		{
			return type == LvResourceType::SAMPLER;
		}

		static bool IsImageType(LvResourceType type)
		{
			switch (type)
			{
			case LvResourceType::COMBINED_IMAGE_SAMPLER:
			case LvResourceType::SAMPLED_IMAGE:
			case LvResourceType::STORAGE_IMAGE:
				return true;
			}

			return false;
		}

		static bool IsBufferType(LvResourceType type)
		{
			switch (type)
			{
			case LvResourceType::UNIFORM_TEXEL_BUFFER:
			case LvResourceType::STORAGE_TEXEL_BUFFER:
			case LvResourceType::UNIFORM_BUFFER:
			case LvResourceType::STORAGE_BUFFER:
			case LvResourceType::UNIFORM_BUFFER_DYNAMIC:
			case LvResourceType::STORAGE_BUFFER_DYNAMIC:
				return true;
			}

			return false;
		}
	};


	// http://devgit.com2us.com/TS/TPact/issues/33

	enum class LvMemoryOption : uint8
	{
		MAP_MANAGED,
		PRIVATE_GPU,
		STAGING
	};

	enum class LvBufferUsage : uint8
	{
		UNIFORM = 0x00000010,
		INDEX = 0x00000040,
		VERTEX = 0x00000080,
	};

	enum class LvRenderFeature : uint8
	{

	};

	enum class LvMemoryLayout : uint8
	{
		STD140,
		// packed, shared are deprecated soon.
		PACKED,
		SHARED,
		// std430 is not impl yet
		//STD430,
	};

	struct LvBufferLayout;

	enum class LvHandleType : uint8
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

	class LV_API LvHandle
	{
	public:
		LvHandle() = delete;
		LvHandle(LvHandleType type);
		virtual ~LvHandle();

		const char* name;

#if defined(_DEBUG)
		const char* instanceType = nullptr;
#endif
		virtual uint32 GetHashCode() const;

		LV_FORCEINLINE uint32 Retain()
		{
			int32 newRef = (int32)(_refCount += 1);
			LV_CHECK(newRef > 0, "Wrong Retain Operation");
			return (uint32)newRef;
		}

		LV_FORCEINLINE uint32 Release()
		{
			int32 newRef = (_refCount -= 1);
			if (_refCount == 0)
			{
				_pendingDeleteQueue.Enqueue(this);
			}
			LV_CHECK(newRef >= 0, "Wrong Release Operation");
			return (uint32)newRef;
		}

		LV_FORCEINLINE uint32 GetRefCount() const
		{
			return _refCount;
		}

		LV_FORCEINLINE LvHandleType GetType() const
		{
			return _type;
		}

		// Only 
		static void FlushPendingDeletes(LvRenderContext* rc, bool forceDeferredDeleteFlush);
		static bool AdvanceFrame();	// next frame overflow

	private:
		LvHandleType _type;

		uint32 _refCount;

	private:
		static LvQueue<LvHandle*> _pendingDeleteQueue;
		static uint32 _currentFrame;

		struct ResourceToDelete
		{
			LvList<LvHandle*> handles;
			uint32 frameDelete;
		};
		static LvList<ResourceToDelete> _deferredDeleteArray;
	};

	struct LV_API LvBufferHandle : public LvHandle
	{
		LvBufferHandle();
		virtual ~LvBufferHandle();

		uint32 size;

		LvBufferUsage usage;

		LvMemoryOption option;

		//uint32 offset;

		virtual void Start(uint32 position = 0);

		virtual void Align(uint32 alignment);

		virtual void Read(void* data, uint32 size);

		virtual void Write(const void* data, uint32 size);

		virtual void End();

		virtual void Reset();
	};

	// TODO : Solve inherit problem  
	// L1 64KB, L2 2MB. if to allocate into CPU L2 cache, It has to be almost less than 2MB.
	// Buffer,
	// Align : http://www.songho.ca/misc/alignment/dataalign.html
	struct LV_API LvByteBuffer
	{
		uint8* pointer;

		uint32 capacity;

		uint32 position;

		bool isFixedSize;

		LvByteBuffer();

		LvByteBuffer(bool fixedSize);

		LvByteBuffer(uint32 capacitySize);

		LvByteBuffer(uint32 capacitySize, bool fixedSize);

		virtual ~LvByteBuffer();

		void Free();

		template<typename T>
		void Write(const T& in)
		{
			Align(LV_ALIGNOF(T));
			Write(reinterpret_cast<const uint8*>(&in), sizeof(T));
		}

		void Write(const void* data, uint32 dataSize);

		void Align(uint32 alignment);

		template<typename T>
		void Read(T& in)
		{
			Align(LV_ALIGNOF(T));
			Read(reinterpret_cast<uint8*>(&in), sizeof(T));
		}

		void Read(void* in, uint32 dataSize);

		template<typename T>
		void Skip()
		{
			Align(LV_ALIGNOF(T));
			Skip(sizeof(T));
		}

		const uint8* Skip(uint32 _size);

		void Reset();

		void Start(uint32 position = 0);

		void End();
	};

	struct LV_API LvShaderVariable
	{
		LvShaderValueType	type = LvShaderValueType::Void;
		uint16				arrayCount = 0;	// array count == 0 means not array type. count >= 1 means [1]
		int16				structTypeDefineIndex = -1;	// type == Struct -> refer structDefineArrays with the index
		LvShaderVariable() { }
		LvShaderVariable(LvShaderValueType type, uint16 arrayCount = 0, int16 structDefineIndex = -1);
	};


	// https://www.khronos.org/opengl/wiki/Interface_Block_(GLSL)
	// Same as Uniform Layout in GL
	struct LV_API LvBufferLayout
	{
		LvMemoryLayout				memoryLayout = LvMemoryLayout::STD140;
		LvList<LvShaderVariable>	variables;

		LvBufferLayout() {}

		LvBufferLayout(const LvBufferLayout& b)
			: memoryLayout(b.memoryLayout)
			, variables(b.variables)
		{}

		LvBufferLayout(LvBufferLayout&& b) 
			: memoryLayout(b.memoryLayout)
			, variables(std::move(b.variables)) 
		{}

		LvBufferLayout& operator=(LvBufferLayout&& b)
		{
			if (this != &b)
			{
				memoryLayout = b.memoryLayout;
				variables = std::move(b.variables);
			}

			return *this;
		}

		LvBufferLayout& operator=(const LvBufferLayout& b)
		{
			if (this != &b)
			{
				memoryLayout = b.memoryLayout;
				variables = b.variables;
			}

			return *this;
		}

		int GetSizeInBytes(LvRenderPlatform platform, const LvList<LvList<LvShaderVariable>>* structDefineArrays = nullptr, bool metalStd140Layout = false) const;
	};

	struct LV_API LvUniformWriter
	{
		LvBufferHandle& buffer;			// 8 bytes

		const LvBufferLayout& layout;	// 8 bytes

		LvRenderPlatform platform;		// 1 bytes

		uint8 maxAlign;					// 1 bytes

		uint8 valueIndex;				// 1 bytes
        
        bool metalStd140Layout;

		LvUniformWriter() = delete;
		
		LvUniformWriter(LvRenderPlatform platform, LvBufferHandle& buffer, const LvBufferLayout& layout,
                        const LvList<LvList<LvShaderVariable>>* structDefineArray = nullptr, bool useMetalStd140Layout = false);

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

		void WriteVec2(const System::Math::LvVec2f& v, bool isStructMember = false);

		void WriteVec2(System::Math::LvVec2f* v, uint32 arrayCount, bool isStructMember = false);

		void WriteVec3(const System::Math::LvVec3f& v, bool isStructMember = false);

		void WriteVec3(System::Math::LvVec3f* v, uint32 arrayCount, bool isStructMember = false);

		void WriteVec4(const System::Math::LvVec4f& v, bool isStructMember = false);

		void WriteVec4(System::Math::LvVec4f* v, uint32 arrayCount, bool isStructMember = false);

		void WriteMat2(const System::Math::LvMat2f& m, bool isStructMember = false);

		void WriteMat2(System::Math::LvMat2f* m, uint32 arrayCount, bool isStructMember = false);

		void WriteMat3(const System::Math::LvMat3f& m, bool isStructMember = false);

		void WriteMat3(System::Math::LvMat3f* m, uint32 arrayCount, bool isStructMember = false);

		void WriteMat4(const System::Math::LvMat4f& m, bool isStructMember = false);

		void WriteMat4(System::Math::LvMat4f* m, uint32 arrayCount, bool isStructMember = false);

		// array count can be 0 or more than 1
		// return type is the calculated unpadded struct size for recursive function
		size_t WriteStruct(void* unpaddedStrcutData, uint32 structIndex, const LvList<LvList<LvShaderVariable>>& structDefineArrays, uint32 arrayCount, bool isStructMember = false);
	};

	struct LV_API LvSamplerInfo
	{
		LvSamplerInfo();
		
		// TEX_CUBE Deprecated.
		LvSamplerType type;								// 1 byte

		LvSamplerCoord coordinate;						// 1 byte

		LvFilter magFilter : 4;

		LvFilter minFilter : 4;							// 1 byte

		LvSamplerMipmapMode mipmapMode : 4;				

		LvSamplerAddressMode address : 4;				// 1 byte : 4 align

		LvSamplerAddressMode addressU : 4;

		// TODO : Vulkan 에서는 V 를 Repeat 으로 해야 잘 나온다. 해결해야함.
		LvSamplerAddressMode addressV : 4;				// 1 byte

		LvSamplerAddressMode addressW : 4;				

		LvCompareOp compareOp : 4;						// 1 byte

		uint16 isCompareEnable : 1;						// 1 bit

		uint16 mipLevels : 7;							// 7 bit	: 1

		uint16 binding : 7;								// 7 bit

		uint16 isAnisotropyEnable : 1;					// 1 bit	: 1 - > 4 align

		float maxAnisotropy;							// 4 byte

		// TODO : borderColor;
	};

	struct LV_API LvSamplerHandle : public LvHandle
	{
		LvSamplerInfo info;

		LvSamplerHandle();

		~LvSamplerHandle();
	};

	struct LV_API LvShaderHandle : public LvHandle
	{
		LvShaderHandle();

		~LvShaderHandle();
	};

	struct LV_API LvProgramHandle : public LvHandle
	{
		LvProgramHandle();

		~LvProgramHandle();
	};

	struct LV_API LvTextureInfo
	{
		const char* name;						// 8 bytes

		LvTextureUsage usage;					// 2 bytes

		LvPixelFormat format;					// 1 byte
		
		LvTextureType type;						// 1 byte

		LvTextureViewType viewType;				// 1 byte

		LvSampleCountFlag samples;				// 1 byte

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

		LvTextureInfo();
	};


	struct LV_API LvTextureHandle : public LvHandle
	{
		LvTextureInfo info;

		LvSamplerHandle* sampler;

		LvTextureHandle();

		~LvTextureHandle();
	};

	// https://community.arm.com/graphics/b/blog/posts/mali-performance-2-how-to-correctly-buffers-framebuffers
	struct LV_API LvAttachment
	{
		LvAttachment();

		LvRenderTextureFormat format = LvRenderTextureFormat::DEFAULT_COLOR;

		LvRenderBufferLoadAction load = LvRenderBufferLoadAction::DONT_CARE;

		LvRenderBufferStoreAction store = LvRenderBufferStoreAction::STORE;

		bool isDepthStencilAttachment = false;

		LvSampleCountFlag sampleCount = LvSampleCountFlag::COUNT_1; // MSAA

	};

	struct LV_API LvFrameBufferHandle : LvHandle
	{
		uint16 width;
		
		uint16 height;

		uint16 colorBufferCount : 15;

		uint16 useDepthStencilBuffer : 1;

		LvFrameBufferHandle();

		~LvFrameBufferHandle();
	};

	struct LV_API LvRenderPassInfo
	{
		LvRenderPassInfo();
		~LvRenderPassInfo();
        
		LvAttachment* outputColorAttachments;

		LvAttachment* resolveColorAttachment;

		LvAttachment outputDepthStencilAttachment;

		uint8 outputColorAttachmentCount : 3;			

		uint8 resolveColorAttachmentCount : 3;

		uint8 useDepthStencilAttacment : 1;
				
		uint8 isSwapchainRenderPass : 1;

		uint32 GetHashCode() const;

		/* TODO :	Subpass Desccription/Dependency Implementation
			Subpass 구조를 명시할(VkSubpassDescription) 또 다른 클래스 필요
			LvList<uint> inputColorAttachmentIndices;
			uint inputDepthStencilAttachmentIndex;
			bool useInputDepthStencil;
			LvList<uint> resolveColorAttachmentIndices;
			uint resolveDepthStencilAttachmentIndex;
			bool useResolveDepthStencil;
		*/
		// 아마 Subpass로 사용될??
		/*
		struct Iterator
		{
			uint index;

			LvAttachment* array;

			uint arrayLength;

			// LvRenderPassHandle* renderPass;

			uint clearFlags;

			Iterator();

			~Iterator();

			// void Set(LvRenderPassHandle& pass);

			bool HasNext();

			LvAttachment& Current();

			void Reset();

			void Clear();
		};
		*/
	};

	struct LV_API LvRenderPassHandle : LvHandle
	{
		// TODO : SubPass
		LvRenderPassHandle();

		~LvRenderPassHandle();
		
		virtual uint32 GetHashCode() const override;

		LvRenderPassInfo info;
	};

	struct LV_API LvSwapChain
	{
		uint16 bufferCount : 14;

		uint16 useDepthBuffer : 1;

		uint16 useStencilBuffer : 1;				// 2 bytes

		LvPixelFormat colorPixelFormat;				// 1

		LvRenderTextureFormat colorRenderFormat;	// 1

		LvPixelFormat depthPixelFormat;				// 1

		LvRenderTextureFormat depthRenderFormat;	// 1

		LvPixelFormat stencilPixelFormat;			// 1

		LvRenderTextureFormat stencilRenderFormat;	// 1

		LvSwapChain();
	};

	struct LV_API LvSwapChainInfo
	{
		// always true on useColorBuffer of SwapChain

		bool useDepthBuffer = true;
		
		bool useStencilBuffer = true;

		// colorBufferFormat will be set automatically by engine

		LvRenderTextureFormat depthBufferFormat = LvRenderTextureFormat::DEFAULT_DEPTH_STENCIL;

		LvRenderTextureFormat stencilBufferFormat = LvRenderTextureFormat::DEFAULT_DEPTH_STENCIL;

		// for Multi Sampling Anti Alising(MSAA)
		bool useMSAA = false;

		LvSampleCountFlag msaaSampleCount = LvSampleCountFlag::COUNT_1;

	};

	struct LV_API LvFrameBufferInfo
	{
		LvFrameBufferInfo();

		LvFrameBufferInfo(const LvFrameBufferInfo& o);

		LvRenderPassHandle* renderPass;

		LvList<LvTextureHandle*> colorBuffers;
		
		LvTextureHandle* resolveColorBuffer = nullptr;

		LvTextureHandle* depthStencilBuffer;

		uint16 width = 1, height = 1;
	};

	struct LV_API LvStencilOpState
	{
		LvStencilOp    failOp = LvStencilOp::KEEP;
		LvStencilOp    passOp = LvStencilOp::KEEP;
		LvStencilOp    depthFailOp = LvStencilOp::KEEP;
		LvCompareOp    compareOp = LvCompareOp::ALWAYS;
		uint8       compareMask = 1; // ???
		uint8       writeMask = 1;
		uint8       reference = 0;
	};

	struct LV_API LvVertexBufferLayoutDescriptor
	{
		LvVertexBufferLayoutDescriptor();
		explicit LvVertexBufferLayoutDescriptor(uint16 binding, uint16 stride);
		explicit LvVertexBufferLayoutDescriptor(uint16 binding, uint16 stride, bool instancing, uint16 stepRate);
		uint16      binding;
		uint16      stride;
		uint16		useInstancing;
		uint16      stepRate;
	};

	struct LV_API LvVertexAttributeDescriptor
	{
		LvVertexAttributeDescriptor();
		
		explicit LvVertexAttributeDescriptor(uint16 binding, uint8 loc, LvVertexFormat format, uint32 offset);

		uint16				binding;	// 2 byte
		uint8				location;	// 1 bytes
		LvVertexFormat		format;		// 1 byte
		uint32				offset;		// 4 bytes
	};

	struct LV_API LvVertexInputDescriptor
	{
		LvVertexBufferLayoutDescriptor* layouts;

		LvVertexAttributeDescriptor* attributes;

		uint32 layoutCount;

		uint32 attributeCount;

		LvVertexInputDescriptor();

		LvVertexInputDescriptor(const LvVertexInputDescriptor& o);

		~LvVertexInputDescriptor();

		void operator=(const LvVertexInputDescriptor& o);
	
		void CopyFrom(const LvVertexInputDescriptor& vid);

		void Release();
		bool isAlloced = false;
	};

	// LvResourceBinding can be used for either Uniform Buffer or Uniform Texture.
	// When It's used as a uniform buffer, you should fill the 'LvBufferLayout layout' member
	// When It's used as a uniform texture, you should fill the `const char* name' member
	struct LV_API LvResourceBinding
	{
		LvResourceType			type;

		LvShaderType			stage;

		uint16					binding;	// Similar to (binding = 0) on GLSL

		uint16					arrayCount;	// array가 아니라면 0으로 설정되어야 하고, array([n])이라면 1 이상으로 설정되어야 한다.

		const char*				name;		// Uniform Texture Name / Assume name is a literal (name = "something"). GLES3 Platform에 의해 항상 유지되어야 한다.
		
		LvResourceBinding();
		LvResourceBinding(const LvResourceBinding& o)
			:type(o.type)
			, stage(o.stage)
			, binding(o.binding)
			, arrayCount(o.arrayCount)
			, name(o.name)
		{

		}

		LvResourceBinding& operator=(const LvResourceBinding& o)
		{
			type = o.type;
			stage = (o.stage);
			binding = o.binding;
			arrayCount = o.arrayCount;
			name = o.name;
			return *this;
		}
	};

	struct LV_API LvResourceUsage
	{
		LvResourceBinding binding;

		union
		{
			struct
			{
				LvTextureHandle** handle;
			} texture;

			struct
			{
				LvBufferHandle** handle;
				uint32* offset;
				uint32* range;
			} buffer;
		};

		LvResourceUsage();
	};

	struct LV_API LvResourceLayoutHandle : LvHandle
	{
		LvResourceLayoutHandle();
	};

	struct LV_API LvResourceSetHandle : LvHandle
	{
		LvResourceSetHandle();
	};

	struct LV_API LvShaderDescriptor
	{
		LvProgramHandle*	program;
		LvShaderHandle*		shaders[2];	// @Chan : 만약 Vertex/Fragment말고 Geometry/Tesellation등을 쓰게 된다면 이것을 더 늘리면 된다.
		uint32				shaderCount;

		LvShaderDescriptor();

		~LvShaderDescriptor();

		void CopyFrom(const LvShaderDescriptor& sd);
	};

	struct LV_API LvColorBlendDescriptor
	{
		struct Attachment
		{
			bool blendEnable = false;
			LvColorWriteMask writeMask = LvColorWriteMask::RED | LvColorWriteMask::GREEN | LvColorWriteMask::BLUE | LvColorWriteMask::ALPHA;
			LvBlendFactor     srcColor = LvBlendFactor::ONE;
			LvBlendFactor     dstColor = LvBlendFactor::ZERO;
			LvBlendOp         colorOp = LvBlendOp::ADD;
			LvBlendFactor     srcAlpha = LvBlendFactor::ONE;
			LvBlendFactor     dstAlpha = LvBlendFactor::ZERO;
			LvBlendOp         alphaOp = LvBlendOp::ADD;
		};

		Attachment attachments[4];
		
		uint32 attachmentCount;

		LvColorBlendDescriptor();

		~LvColorBlendDescriptor();

		uint32 GetHashCode() const;

		void CopyFrom(const LvColorBlendDescriptor& cbd);
	};

	struct LV_API LvRasterizationDescriptor
	{
		// VkBool32                                   depthClampEnable;?
		LvPolygonMode    polygonMode = LvPolygonMode::FILL;
		LvCullMode       cullMode = LvCullMode::BACK;
		LvFrontFace      frontFace = LvFrontFace::COUNTER_CLOCKWISE;
		LvPrimitiveType  primitiveType = LvPrimitiveType::TRIANGLE_LIST;
		bool             scissorTest = false;
	};

	// https://open.gl/depthstencils
	struct LV_API LvDepthStencilDescriptor
	{
		bool                depthTest;
		bool                depthWrite;
		LvCompareOp         depthCompareOp = LvCompareOp::LESS;
		// VkBool32                                  depthBoundsTestEnable;?
		bool                stencilTest	= false;
		LvStencilOpState    front;
		LvStencilOpState    back;
		// float                                     minDepthBounds;?
		// float                                     maxDepthBounds;?
	};

	struct LV_API LvPipelineDescriptor
	{
		LvPipelineDescriptor();

		LvPipelineType				type;
		const char*					name;
		LvRenderPassHandle*			renderPass = nullptr;
		LvVertexInputDescriptor		vertexInput; // TODO : Rename vertexInput
		LvResourceLayoutHandle*		resourceLayout;
		LvShaderDescriptor			shader;
		LvRasterizationDescriptor	rasterize;

		// TODO : ColorBlend Descriptor Array
		LvColorBlendDescriptor		colorBlend;
		LvDepthStencilDescriptor	depthStencil;
	};

	struct LV_API LvPipelineHandle : LvHandle
	{
		LvPipelineType type;
		
		LvRenderPassHandle* renderPass;

		LvResourceLayoutHandle* resourceLayout;

		LvPipelineHandle();
	};

	// TODO : change LvDrawDataEncoder
	struct LV_API LvCommandEncoderHandle : LvHandle
	{
		LvCommandEncoderHandle();

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
			const LvRenderPassHandle* renderPass, 
			const LvFrameBufferHandle* frameBuffer, 
			const Area area, 
			const uint8 colorAttachClearCount,
			const ClearValue* colorAttachmentClear, 
			const uint8 resolveAttachClearCount,
			const ClearValue* resolveAttachmentClear,
			const ClearValue* depthAttachmentClear
		) = 0;

		virtual void BindPipeline(const LvPipelineHandle* pipeline) = 0;

		virtual void BindResourceSet(const LvResourceSetHandle* rSet) = 0;

		// const all levels for bufferHandle*
		virtual void BindVertexBuffers(const LvBufferHandle* const * vertexBuffers, const uint32* offsets, const uint8 bufferCount) = 0;

		virtual void BindIndexBuffer(const LvBufferHandle* indexBuffer, const  LvIndexType indexType = LvIndexType::UInt16) = 0;

		// firstIndex : byte offset
		virtual void DrawIndexed(const uint32 firstIndex, const uint32 indexCount, const uint32 instanceCount, const uint32 vertexOffset) = 0;

		// firstVertex : index of the first vertex to draw
		virtual void DrawArrays(const uint32 firstVertex, const  uint32 vertexCount, const  uint32 instanceCount) = 0;

		virtual void EndRenderPass() = 0;

		virtual void BeginDebugMarker(const char* label, float color[4]) = 0;

		virtual void EndDebugMarker() = 0;

		virtual void End() = 0;
	};

	struct LV_API LvResourceSetPool
	{
		virtual LvResourceSetHandle* Allocate(LvResourceLayoutHandle* resourceSetLayoutHandle, LvResourceUsage* usages, uint32 usageCount) = 0;

		virtual void Deallocate(LvResourceSetHandle* resourceSetHandle) = 0;

		virtual void Reset() = 0;
		
		virtual ~LvResourceSetPool() {};
		
	};

	// This class acts as a bridge to connect the inside and outside of the renderer.
	// LvBufferManager is class whis is used in Render.
	struct LvBufferPendingDelete
	{
		static bool AdvanceFrame();
		static void FlushPendingDeleteForFreeBuffer(bool bImmediatly);
	};

LV_NS_RENDER_END

#endif
