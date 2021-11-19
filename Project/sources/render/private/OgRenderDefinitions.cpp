#include "OgPrecompile.h"

#include "system/OgMemory.h"
#include "system/OgHashCode.h"

#include "render/OgRenderDefinitions.h"
#include "render/OgRenderContext.h"

OG_NAMESPACE_RENDER_BEGIN

std::queue<OgHandle*> OgHandle::_pendingDeleteQueue;

static OG_FORCEINLINE void delete_handle(OgRenderContext* rc, OgHandle* handle)
{
	switch (handle->GetType())
	{
	case OgHandleType::BUFFER:
		rc->DestroyBuffer((OgBufferHandle*)handle);
		break;
	case OgHandleType::SAMPLER:
		rc->DestroySampler((OgSamplerHandle*)handle);
		break;
	case OgHandleType::TEXTURE:
		rc->DestroyTexture((OgTextureHandle*)handle);
		break;
	case OgHandleType::SHADER:
		rc->DestroyShader((OgShaderHandle*)handle);
		break;
	case OgHandleType::PROGRAM:
		rc->DestroyProgram((OgProgramHandle*)handle);
		break;
	case OgHandleType::FRAMEBUFFER:
		rc->DestroyFrameBuffer((OgFrameBufferHandle*)handle);
		break;
	case OgHandleType::RENDERPASS:
		rc->DestroyRenderPass((OgRenderPassHandle*)handle);
		break;
	case OgHandleType::RESOURCE_LAYOUT:
		rc->DestroyResourceLayout((OgResourceLayoutHandle*)handle);
		break;
	case OgHandleType::RESOURCE_SET:
		rc->DestroyResourceSet((OgResourceSetHandle*)handle);
		break;
	case OgHandleType::PIPELINE:
		rc->DestroyPipeline((OgPipelineHandle*)handle);
		break;
	case OgHandleType::COMMAND_ENCODER:
		rc->DestroyCommandEncoder((OgCommandEncoderHandle*)handle);
		break;
	default:
		LOGE(LV_ID, "Wrong Type");
		break;
	}
}

OgHandle::OgHandle(OgHandleType type)
	: name(nullptr)
	, _type(type)
	, _refCount(0)
{}

OgHandle::~OgHandle()
{
	OG_CHECK(_refCount == 0, "This Handle is being destroyed with more than one ref count");
}

uint32 OgHandle::GetHashCode() const
{
	// TODO: system에 hash code 작업 필요
	// return PointerHash(this);
	return 0;
}

/// LvBufferHandle
OgBufferHandle::OgBufferHandle() : OgHandle(OgHandleType::BUFFER), size(0) {}
OgBufferHandle::~OgBufferHandle() { }

void OgBufferHandle::Start(uint32 position)
{
	LOGE(OG_ID, "It couldn't start");
}

void OgBufferHandle::Read(void* data, uint32 size)
{
	LOGE(OG_ID, "It couldn't start");
}

void OgBufferHandle::Align(uint32 alignment)
{
	LOGE(OG_ID, "It couldn't start");
}

void OgBufferHandle::Write(const void* data, uint32 size)
{
	LOGE(OG_ID, "It couldn't start");
}

void OgBufferHandle::End()
{
	LOGE(OG_ID, "It couldn't start");
}

void OgBufferHandle::Reset()
{
	LOGE(OG_ID, "It couldn't start");
}

void OgHandle::FlushPendingDeletes(class OgRenderContext* rc, bool forceDeferredDeleteFlush)
{
	auto deleteFunc = [&](OgHandle* handle)
	{
		if (handle->GetRefCount() == 0)
		{
			delete_handle(rc, handle);
		}
	};

	size_t pendingDeleteCount = _pendingDeleteQueue.size();
	if (pendingDeleteCount > 0)
	{
		ResourceToDelete rtd;
		rtd.frameDelete = _currentFrame;
		rtd.handles.Resize(pendingDeleteCount);
		for (size_t i = 0; i < pendingDeleteCount; ++i)
		{
			rtd.handles[i] = _pendingDeleteQueue.back();
			_pendingDeleteQueue.pop();
		}
		_deferredDeleteArray.Add(rtd);
	}

	constexpr uint32 handleExpirePeriod = 3u;

	if (size_t deferredArrayCount = _deferredDeleteArray.Size())
	{
		if (forceDeferredDeleteFlush == true)
		{
			rc->WaitDeviceIdle();

			for (size_t i = 0; i < deferredArrayCount; ++i)
			{
				ResourceToDelete& rtd = _deferredDeleteArray[i];

				for (size_t j = 0, handleMax = rtd.handles.Size(); j < handleMax; ++j)
				{
					deleteFunc(rtd.handles[j]);
				}
			}

			_deferredDeleteArray.Clear();
		}
		else
		{
			size_t deletedRtd = 0;
			for (size_t i = 0; i < deferredArrayCount; ++i)
			{
				ResourceToDelete& rtd = _deferredDeleteArray[i];

				if (rtd.frameDelete + handleExpirePeriod < _currentFrame)
				{
					++deletedRtd;
					for (size_t j = 0, handleMax = rtd.handles.Size(); j < handleMax; ++j)
					{
						deleteFunc(rtd.handles[j]);
					}
				}
				else
				{
					break;
				}
			}
		}
	}
}

bool OgHandle::AdvanceFrame()
{
	uint32 nextFrame = _currentFrame + 1;

	// overflow
	if (nextFrame < _currentFrame)
	{
		_currentFrame = 0;
		return true;
	}
	else
	{
		_currentFrame = nextFrame;
		return false;
	}
}

static void do_std_140_align(const OgShaderVariable var, int32& outTotal, const OgVector<OgVector<OgShaderVariable>>* structDefineArray)
{
	/* https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_uniform_buffer_object.txt
	* 1. If the member is a scalar consuming N basic machine units, the base alignment is N. (bool, float, int, uint is 4 basic machin units)
	* 2. If the member is a two- or four-component vector with components consuming N basic machine units,
	*	 the base alignment is 2N or 4N, respectively.
	* 3. If the member is a three-component vector with components consuming N basic machine units,
	*	 the base alignment is 4N.
	* 4. If the member is an array of scalars or vectors, the base alignment and array
	*	 stride are set to match the base alignment of a single array element, according
	*	 to rules (1), (2), and (3), and rounded up to the base alignment of a vec4. The
	*	 array may have padding at the end; the base offset of the member following
	*	 the array is rounded up to the next multiple of the base alignment
	* 5. If the member is a column-major matrix with C columns and R rows, the
	*	 matrix is stored identically to an array of C column vectors with R components each,
	*	 according to rule (4)
	* 6. If the member is an array of S column-major matrices with C columns and
	*	 R rows, the matrix is stored identically to a row of S x C column vectors
	*	 with R components each, according to reule (4).
	* 7. If the member is a row-major matrix with C columns and R rows, the matrix
	*	 is stored identically to an array of R row vectors with C components each,
	*	 according to rule (4).
	* 8. If the member is an array of S row-major matrices with C columns and R
	*	 rows, the matrix is stored identically to a row of S X R row vectors with C
	*	 components each, according to rule(4).
	* 9. If the member is a structure, the base alignment of the structure is N, where
	*	 N is the largest base alignment value of any of its members, and rounded
	*	 up to the base alignment of a vec4. The individual members of this sub-
	*	 structure are then assigned offsets by applying this set of rules recursively,
	*	 where the base offset of the first member of the sub-structure is equal to the
	*	 aligned offset of the structure. The structure may have padding at the end;
	*	 The base offset of the member following the sub-structure is rounded up to
	*	 the next multiple of the base alignment of the structure.
	* 10. If the member is an array of S structures, the S elements of the array are laid
	*	  out in order, according to rule(9).
	*
	*/
	switch (var.type)
	{
	case OgShaderValueType::Bool:
	case OgShaderValueType::Float1:
	case OgShaderValueType::Int1:
	case OgShaderValueType::Uint1:
	{
		constexpr int32 baseAlign = 4;
		constexpr int32 baseMask = baseAlign - 1;
		constexpr int32 arrayBaseAlign = 16;
		constexpr int32 arrayBaseMask = arrayBaseAlign - 1;
		constexpr int32 size = 4;

		if (var.arrayCount >= 1)
		{
			// round for each array elements
			for (uint32 i = 0; i < var.arrayCount; ++i)
			{
				outTotal = ((outTotal + arrayBaseMask) & (~arrayBaseMask)) + size;
			}

			// padding at the end
			outTotal = ((outTotal + arrayBaseMask) & (~arrayBaseMask));
		}
		else
		{
			outTotal = ((outTotal + baseMask) & (~baseMask)) + size;
		}
		break;
	}
	case OgShaderValueType::Vec2:
	{
		constexpr int32 baseAlign = 8;
		constexpr int32 baseMask = baseAlign - 1;
		constexpr int32 arrayBaseAlign = 16;
		constexpr int32 arrayBaseMask = arrayBaseAlign - 1;
		constexpr int32 size = 8;

		if (var.arrayCount >= 1)
		{
			for (uint32 i = 0; i < var.arrayCount; ++i)
			{
				outTotal = ((outTotal + arrayBaseMask) & (~arrayBaseMask)) + size;
			}

			// padding at the end
			outTotal = ((outTotal + arrayBaseMask) & (~arrayBaseMask));
		}
		else
		{
			outTotal = ((outTotal + baseMask) & (~baseMask)) + size;
		}

		break;
	}
	case OgShaderValueType::Vec3:
	{
		constexpr int32 align = 16;
		constexpr int32 mask = align - 1;
		constexpr int32 size = 12;

		if (var.arrayCount >= 1)
		{
			for (uint32 i = 0; i < var.arrayCount; ++i)
			{
				outTotal = ((outTotal + mask) & (~mask)) + size;
			}

			// padding at the end
			outTotal = ((outTotal + mask) & (~mask));
		}
		else
		{
			outTotal = ((outTotal + mask) & (~mask)) + size;
		}


		break;
	}
	case OgShaderValueType::Vec4:
	{
		constexpr int32 align = 16;
		constexpr int32 mask = align - 1;
		constexpr int32 size = 16;

		if (var.arrayCount >= 1)
		{
			for (uint32 i = 0; i < var.arrayCount; ++i)
			{
				outTotal = ((outTotal + mask) & (~mask)) + size;
			}

			// padding at the end
			outTotal = ((outTotal + mask) & (~mask));
		}
		else
		{
			outTotal = ((outTotal + mask) & (~mask)) + size;
		}

		break;
	}
	case OgShaderValueType::Mat2:
	{
		constexpr int32 align = 16;
		constexpr int32 mask = align - 1;
		constexpr int32 vectorSize = 8;

		if (var.arrayCount >= 1)
		{
			// rule 6 -> rule 4
			for (uint32 i = 0; i < var.arrayCount; ++i)
			{
				for (uint32 j = 0; j < 2; ++j)
				{
					outTotal = ((outTotal + mask) & (~mask)) + vectorSize;
				}

				// padding at the end by rule 4
				outTotal = ((outTotal + mask) & (~mask));
			}
		}
		else
		{
			// rule 5 -> rule 4
			for (uint32 j = 0; j < 2; ++j)
			{
				outTotal = ((outTotal + mask) & (~mask)) + vectorSize;
			}

			// padding at the end by rule 4
			outTotal = ((outTotal + mask) & (~mask));
		}

		break;
	}
	case OgShaderValueType::Mat3:
	{
		constexpr int32 align = 16;
		constexpr int32 mask = align - 1;
		constexpr int32 vectorSize = 12;

		if (var.arrayCount >= 1)
		{
			for (uint32 i = 0; i < var.arrayCount; ++i)
			{
				for (uint32 j = 0; j < 3; ++j)
				{
					outTotal = ((outTotal + mask) & (~mask)) + vectorSize;
				}

				outTotal = ((outTotal + mask) & (~mask));
			}
		}
		else
		{
			for (uint32 j = 0; j < 3; ++j)
			{
				outTotal = ((outTotal + mask) & (~mask)) + vectorSize;
			}

			outTotal = ((outTotal + mask) & (~mask));
		}

		break;
	}
	case OgShaderValueType::Mat4:
	{
		constexpr int32 align = 16;
		constexpr int32 mask = align - 1;
		constexpr int32 vectorSize = 16;

		if (var.arrayCount >= 1)
		{
			for (uint32 i = 0; i < var.arrayCount; ++i)
			{
				for (uint32 j = 0; j < 4; ++j)
				{
					outTotal = ((outTotal + mask) & (~mask)) + vectorSize;
				}

				outTotal = ((outTotal + mask) & (~mask));
			}
		}
		else
		{
			for (uint32 j = 0; j < 4; ++j)
			{
				outTotal = ((outTotal + mask) & (~mask)) + vectorSize;
			}

			outTotal = ((outTotal + mask) & (~mask));
		}

		break;
	}
	case OgShaderValueType::Struct:
	{
		constexpr int32 align = 16;
		constexpr int32 mask = align - 1;
		OG_CHECK(structDefineArray != nullptr, "No Input Struct Define Array");
		OG_CHECK(var.structTypeDefineIndex >= 0 && var.structTypeDefineIndex < (*structDefineArray).Size(), "Wrong Struct Type Define Index");

		const OgVector<OgShaderVariable>& typeDefines = (*structDefineArray)[var.structTypeDefineIndex];

		if (var.arrayCount >= 1)
		{
			for (uint32 i = 0; i < var.arrayCount; ++i)
			{
				// align begin
				outTotal = ((outTotal + mask) & (~mask));

				for (size_t i = 0; i < typeDefines.Size(); ++i)
				{
					do_std_140_align(typeDefines[i], outTotal, structDefineArray);
				}

				// pad end
				outTotal = ((outTotal + mask) & (~mask));
			}
		}
		else
		{
			// align begin
			outTotal = ((outTotal + mask) & (~mask));

			for (size_t i = 0; i < typeDefines.Size(); ++i)
			{
				do_std_140_align(typeDefines[i], outTotal, structDefineArray);
			}

			// pad end
			outTotal = ((outTotal + mask) & (~mask));
		}

		break;
	}
	default:
	{
		LOGE(OG_ID, "Not Yet Implemeneted");
		break;
	}
	}
}

/// OgBufferLayout
// https://learnopengl.com/Advanced-OpenGL/Advanced-GLSL
int OgBufferLayout::GetSizeInBytes(OgRenderPlatform platform, const OgVector<OgVector<OgShaderVariable>>* structDefineArrays, bool metalStd140Layout) const
{
	int32 total = 0;
	size_t variableCount = (size_t)variables.Size();

	if (platform == OgRenderPlatform::GLES3 || platform == OgRenderPlatform::VULKAN)
	{
		for (size_t variableIndex = 0; variableIndex < variableCount; ++variableIndex)
		{
			const OgShaderVariable var = variables[variableIndex];
			int32 elemCount = var.arrayCount == 0 ? 1 : var.arrayCount;
			if (memoryLayout == OgMemoryLayout::STD140)
			{
				do_std_140_align(var, total, structDefineArrays);
			}
			else
			{
				switch (var.type)
				{
				case OgShaderValueType::Bool:
					total += 1 * elemCount;
					break;
				case OgShaderValueType::Float1:
				case OgShaderValueType::Int1:
				case OgShaderValueType::Uint1:
					total += 4 * elemCount;
					break;
				case OgShaderValueType::Vec2:
					total += 8 * elemCount;
					break;
				case OgShaderValueType::Vec3:
					total += 12 * elemCount;
					break;
				case OgShaderValueType::Vec4:
					total += 16 * elemCount;
					break;
				case OgShaderValueType::Mat3:
					total += 36 * elemCount;
					break;
				case OgShaderValueType::Mat4:
					total += 64 * elemCount;
					break;
				default:
					LOGE(OG_ID, "Not Yet Implemeneted");
					break;
				}
				break;
			}
		}
	}
	else if (platform == OgRenderPlatform::METAL)
	{
		// TODO
	}
	else if (platform == OgRenderPlatform::GLES2)
	{
		
		for (uint32 variableIndex = 0; variableIndex < variableCount; ++variableIndex)
		{
			const OgShaderVariable var = variables[variableIndex];
			int32 elemCount = var.arrayCount == 0 ? 1 : var.arrayCount;

			switch (var.type)
			{
			case OgShaderValueType::Bool:
				total += 1 * elemCount;
				break;
			case OgShaderValueType::Float1:
			case OgShaderValueType::Int1:
			case OgShaderValueType::Uint1:
				total += 4 * elemCount;
				break;
			case OgShaderValueType::Vec2:
				total += 8 * elemCount;
				break;
			case OgShaderValueType::Vec3:
				total += 12 * elemCount;
				break;
			case OgShaderValueType::Vec4:
				total += 16 * elemCount;
				break;
			case OgShaderValueType::Mat3:
				total += 36 * elemCount;
				break;
			case OgShaderValueType::Mat4:
				total += 64 * elemCount;
				break;
			default:
				LOGE(OG_ID, "Not Yet Implemeneted");
				break;
			}
		}
	}

	return total;
}


/// ByteBuffer
constexpr uint32 byteBufferDefaultCapacity = 128;
OgByteBuffer::OgByteBuffer()
	: capacity(byteBufferDefaultCapacity)
	, isFixedSize(false)
{
	pointer = (uint8*)og_malloc(capacity);
}

OgByteBuffer::OgByteBuffer(bool fixedSize)
	: capacity(byteBufferDefaultCapacity)
	, isFixedSize(fixedSize)
{
	pointer = (uint8*)og_malloc(capacity);
}

OgByteBuffer::OgByteBuffer(uint32 capacitySize)
	: capacity(capacitySize)
	, isFixedSize(false)
{
	pointer = (uint8*)og_malloc(capacity);
}

OgByteBuffer::OgByteBuffer(uint32 capacitySize, bool fixedSize)
	: capacity(capacitySize)
	, isFixedSize(fixedSize)
{
	pointer = (uint8*)og_malloc(capacity);
}

OgByteBuffer::~OgByteBuffer()
{
	Free();
}

void OgByteBuffer::Write(const void* data, uint32 dataSize)
{
	//OG_CHECK(size == LV_MAX_BUFFER_SIZE, "Called write outside start/finish?");
	OG_CHECK(isFixedSize == true ? position + dataSize <= capacity : true, "Buffer Align Error (pos : %u, capacity %u)", position + dataSize, capacity);

	if (isFixedSize == false && position + dataSize > capacity)
	{
		uint32 newCapacity = capacity;
		while (newCapacity < position + dataSize)
			newCapacity = (newCapacity << 1);

		capacity = newCapacity;

		uint8* tmp = (uint8*)og_realloc(pointer, capacity);
		OG_CHECK(tmp, "Eror on reallocating memory");
		pointer = tmp;
	}

	memcpy(&pointer[position], data, dataSize);
	position += dataSize;
}

void OgByteBuffer::Align(uint32 alignment)
{
	const uint32 mask = alignment - 1;
	const uint32 pos = (position + mask) & (~mask);

	OG_CHECK(isFixedSize == true ? pos <= capacity : true, "Buffer Align Error (pos : %u, capacity %u)", pos, capacity);

	if (isFixedSize == false && pos > capacity)
	{
		uint32 newCapacity = capacity;
		while (newCapacity < pos)
			newCapacity = (newCapacity << 1);

		capacity = newCapacity;

		uint8* tmp = (uint8*)og_realloc(pointer, capacity);
		OG_CHECK(tmp, "Eror on reallocating memory");
		pointer = tmp;
	}

	position = pos;
}

void OgByteBuffer::Read(void* in, uint32 dataSize)
{
	OG_CHECK(position + dataSize <= capacity, "CommandBuffer::read error (pos: %d, capacity: %d).", position, capacity);
	memcpy(in, &pointer[position], dataSize);
	position += dataSize;
}


const uint8* OgByteBuffer::Skip(uint32 _size)
{
	OG_CHECK(position < capacity, "Buffer::skip error (pos: %d, capacity: %d).", position, capacity);
	const uint8* result = &pointer[position];
	position += _size;
	return result;
}

void OgByteBuffer::Reset()
{
	position = 0;
}

void OgByteBuffer::Start(uint32 position)
{
	this->position = position;
}

void OgByteBuffer::Free()
{
	if (pointer != nullptr)
	{
		og_free(pointer);
		pointer = nullptr;
	}
}

void OgByteBuffer::End()
{
	position = 0;
}


/// LvSamplerInfo

OgSamplerInfo::OgSamplerInfo()
	: // type()
	coordinate(OgSamplerCoord::NORMALIZED)
	, magFilter(OgFilter::LINEAR)
	, minFilter(OgFilter::LINEAR)
	, mipmapMode(OgSamplerMipmapMode::LINEAR)
	, address(OgSamplerAddressMode::REPEAT)
	, addressU(OgSamplerAddressMode::REPEAT)
	, addressV(OgSamplerAddressMode::REPEAT)
	, addressW(OgSamplerAddressMode::REPEAT)
	, compareOp(OgCompareOp::NEVER)
	, isCompareEnable(false)
	, isAnisotropyEnable(false)
	, maxAnisotropy(1)
	, mipLevels(1)
	, binding(0)
{}

/// LvSamplerHandle
OgSamplerHandle::OgSamplerHandle() : OgHandle(OgHandleType::SAMPLER) {}
OgSamplerHandle::~OgSamplerHandle() { }

/// LvTextureInfo
OgTextureInfo::OgTextureInfo()
	: name(nullptr)
	, type(OgTextureType::TEX_2D)
	, viewType(OgTextureViewType::TEX_2D)
	, samples(OgSampleCountFlag::COUNT_1)
	, isGenerateMipmaps(false)
	, arrayLayers(1)
	, mipLevels(1)
	, byteSize(0)
	, extent{ 0, 0, 1 }
{}

/// LvTextureHandle
OgTextureHandle::OgTextureHandle() : OgHandle(OgHandleType::TEXTURE), sampler(nullptr) {}
OgTextureHandle::~OgTextureHandle() { sampler = nullptr; }


/// LvShaderHandle
OgShaderHandle::OgShaderHandle() : OgHandle(OgHandleType::SHADER) {}
OgShaderHandle::~OgShaderHandle() { }

/// LvProgramHandle
OgProgramHandle::OgProgramHandle() : OgHandle(OgHandleType::PROGRAM) {}
OgProgramHandle::~OgProgramHandle() { }

/// LvAttachment
OgAttachment::OgAttachment() { }



/// LvRenderPassInfo 
OgRenderPassInfo::OgRenderPassInfo()
	: outputColorAttachments(nullptr)
	, outputColorAttachmentCount(0)
	, useDepthStencilAttacment(false)
	, isSwapchainRenderPass(false)
{}

OgRenderPassInfo::~OgRenderPassInfo() {}

uint32 OgRenderPassInfo::GetHashCode() const
{

	// TODO
	//uint32 hash = CRCHash::TypeCrc32(isSwapchainRenderPass ? true : false, 0);
	//if (useDepthStencilAttacment == true)
	//{
	//	hash = CRCHash::MemCrc32((void*)&(outputDepthStencilAttachment), sizeof(LvAttachment), 0);
	//}

	//if (resolveColorAttachmentCount > 0)
	//{
	//	hash = CRCHash::MemCrc32(resolveColorAttachment, sizeof(LvAttachment) * resolveColorAttachmentCount, hash);
	//}

	//if (outputColorAttachmentCount > 0)
	//{
	//	hash = CRCHash::MemCrc32(outputColorAttachments, sizeof(LvAttachment) * outputColorAttachmentCount, hash);
	//}

	// renderArea는 Hash에 필요없다.
	
	//return hash;

	return 0;
}

/// LvRenderPassHandle
OgRenderPassHandle::OgRenderPassHandle() : OgHandle(OgHandleType::RENDERPASS) { }
OgRenderPassHandle::~OgRenderPassHandle() {}

uint32 OgRenderPassHandle::GetHashCode() const
{
	return info.GetHashCode();
}


OgRenderPassHandle::OgRenderPassHandle(const OgRenderPassInfo& info)
	: OgHandle(OgHandleType::RENDERPASS)
{
	this->info.isSwapchainRenderPass = info.isSwapchainRenderPass;
	this->info.outputColorAttachmentCount = info.outputColorAttachmentCount;

	if (info.outputColorAttachmentCount > 0)
	{
		this->info.outputColorAttachments = new OgAttachment[info.outputColorAttachmentCount];
		memcpy(this->info.outputColorAttachments, info.outputColorAttachments, info.outputColorAttachmentCount * sizeof(OgAttachment));
	}

	this->info.useDepthStencilAttacment = info.useDepthStencilAttacment;
	this->info.outputDepthStencilAttachment = info.outputDepthStencilAttachment;

	// TODO MSAA

}


/// OgFrameBufferInfo
OgFrameBufferInfo::OgFrameBufferInfo() : renderPass(nullptr), depthStencilBuffer(nullptr) {}

OgFrameBufferInfo::OgFrameBufferInfo(const OgFrameBufferInfo& o)
	: renderPass(o.renderPass)
	, colorBuffers(std::move(o.colorBuffers))
	, depthStencilBuffer(o.depthStencilBuffer)
	, width(o.width)
	, height(o.height)
{}

OgFrameBufferHandle::OgFrameBufferHandle()
	: OgHandle(OgHandleType::FRAMEBUFFER)
	, width(0)
	, height(0)
	, colorBufferCount(0)
	, useDepthStencilBuffer(false)
{

}

OgFrameBufferHandle::~OgFrameBufferHandle()
{

}

OgFrameBufferHandle::OgFrameBufferHandle(uint32 useDepthStencil, uint32  w, uint32 h)
	: OgHandle(OgHandleType::FRAMEBUFFER)
	, useDepthStencilBuffer(useDepthStencil)
	, width(w)
	, height(h)
{
	// TODO: reconsider

}

OgFrameBufferHandle::OgFrameBufferHandle(const OgFrameBufferInfo& info)
	: OgHandle(OgHandleType::FRAMEBUFFER)
	, isSwapchainFrameBuffer(false)
	, framebufferInfo(info)
{
	this->colorBufferCount = info.colorBuffers.size();
	this->useDepthStencilBuffer = info.depthStencilBuffer == nullptr ? false : true;
	this->width = info.width;
	this->height = info.height;
}

/// OgSwapChain
OgSwapChain::OgSwapChain()
	: bufferCount(0)
	, colorPixelFormat(OgPixelFormat::NONE)
	, colorRenderFormat(OgRenderTextureFormat::NONE)
	, useDepthBuffer(false)
	, useStencilBuffer(false)
	, depthPixelFormat(OgPixelFormat::NONE)
	, depthRenderFormat(OgRenderTextureFormat::NONE)
	, stencilPixelFormat(OgPixelFormat::NONE)
	, stencilRenderFormat(OgRenderTextureFormat::NONE)
{

}

/// OgVertexBufferLayoutDescriptor
OgVertexBufferLayoutDescriptor::OgVertexBufferLayoutDescriptor()
	: binding(0)
	, stride(0)
	, useInstancing(false)
	, stepRate(0)
{}

OgVertexBufferLayoutDescriptor::OgVertexBufferLayoutDescriptor(uint16 binding, uint16 stride)
	: binding(binding)
	, stride(stride)
	, useInstancing(false)
	, stepRate(0)
{}

OgVertexBufferLayoutDescriptor::OgVertexBufferLayoutDescriptor(uint16 binding, uint16 stride, bool instancing, uint16 stepRate)
	: binding(binding)
	, stride(stride)
	, useInstancing(instancing)
	, stepRate(stepRate)
{ }


/// OgVertexAttributeDescriptor
OgVertexAttributeDescriptor::OgVertexAttributeDescriptor() {}
OgVertexAttributeDescriptor::OgVertexAttributeDescriptor(uint16 binding, uint8 loc, OgVertexFormat format, uint32 offset)
	: binding(binding), location(loc), format(format), offset(offset) {}


/// OgVertexInputDescriptor

OgVertexInputDescriptor::OgVertexInputDescriptor()
	: layouts(nullptr)
	, attributes(nullptr)
	, layoutCount(0)
	, attributeCount(0)
{}

OgVertexInputDescriptor::OgVertexInputDescriptor(const OgVertexInputDescriptor& o)
	: OgVertexInputDescriptor()
{
	CopyFrom(o);
}


void OgVertexInputDescriptor::operator=(const OgVertexInputDescriptor& o)
{
	CopyFrom(o);
}

OgVertexInputDescriptor::~OgVertexInputDescriptor()
{
	if (isAlloced)
		Release();
}

void OgVertexInputDescriptor::Release()
{
	if (this->layouts != nullptr)
	{
		delete[] this->layouts;
		this->layouts = nullptr;
	}

	if (this->attributes != nullptr)
	{
		delete[] this->attributes;
		this->attributes = nullptr;
	}
}

void OgVertexInputDescriptor::CopyFrom(const OgVertexInputDescriptor& vid)
{
	if (vid.layoutCount > 0)
	{
		this->layouts = new OgVertexBufferLayoutDescriptor[vid.layoutCount];
		memcpy(this->layouts, vid.layouts, sizeof(OgVertexBufferLayoutDescriptor) * vid.layoutCount);
	}

	if (vid.attributeCount > 0)
	{
		this->attributes = new OgVertexAttributeDescriptor[vid.attributeCount];
		memcpy(this->attributes, vid.attributes, sizeof(OgVertexAttributeDescriptor) * vid.attributeCount);
	}

	this->layoutCount = vid.layoutCount;
	this->attributeCount = vid.attributeCount;
	isAlloced = true;
}


/// OgResourceBinding
OgResourceBinding::OgResourceBinding()
	: binding(0)
	, arrayCount(0)
	, name(nullptr)
{}


///OgResourceUsage
OgResourceUsage::OgResourceUsage()
	: buffer{ nullptr, nullptr, nullptr }
{}

OgResourceLayoutHandle::OgResourceLayoutHandle() 
	: OgHandle(OgHandleType::RESOURCE_LAYOUT)
{
}

OgResourceLayoutHandle::OgResourceLayoutHandle(OgResourceBinding* bindings, uint8 count)
	: OgHandle(OgHandleType::RESOURCE_LAYOUT)
	, bindings(nullptr)
	, bindingCount(count)
	, bufferUsageCount(0)
	, textureUsageCount(0)
	, bufferCount(0)
	, textureCount(0)
{
	if (count > 0)
	{
		bindings = new OgResourceBinding[count];
		memcpy(bindings, bindings, sizeof(OgResourceBinding) * count);

		for (uint8 i = 0; i < count; ++i)
		{
			switch (bindings[i].type)
			{
			case OgResourceType::UNIFORM_BUFFER:
			{
				++bufferUsageCount;

				bufferCount += bindings[i].arrayCount == 0 ? 1 : bindings[i].arrayCount;

				break;
			}
			case OgResourceType::COMBINED_IMAGE_SAMPLER:
			{
				++textureUsageCount;

				textureCount += bindings[i].arrayCount == 0 ? 1 : bindings[i].arrayCount;

				break;
			}
			default:
				LOGE(OG_ID, "Not Supported Yet");
				break;
			}
		}
	}
}

OgResourceLayoutHandle::~OgResourceLayoutHandle()
{
	if (this->bindingCount > 0)
	{
		delete[] this->bindings;
		this->bindings = nullptr;
		this->bindingCount = 0;
	}
}

bool IsCompatible(const OgResourceBinding* a, const OgResourceBinding* b)
{
	ASSERT(a != nullptr && b != nullptr);

	if (a->type != b->type) return false;
	if (a->stage != b->stage) return false;
	if (a->binding != b->binding) return false;

	return true;
}

bool OgResourceLayoutHandle::IsCompatibleLayoutWithSet(OgResourceUsage* usages, uint32 usageCount) const
{
	if (this->bindingCount != usageCount) return false;
	for (uint32 bindingIndex = 0; bindingIndex < this->bindingCount; ++bindingIndex)
	{
		const uint16 layoutbinding = this->bindings[bindingIndex].binding;

		int8 matchedBindingIndex = -1;
		for (uint32 usageIndex = 0; usageIndex < usageCount; ++usageIndex)
		{
			if (layoutbinding == usages[usageIndex].binding.binding)
			{
				matchedBindingIndex = usageIndex;
				break;
			}
		}
		// No Matched Binding Index
		if (matchedBindingIndex == -1) return false;
		// If Binding Index can matched, we should check LvResourceBinding.		
		if (!IsCompatible(&(usages[matchedBindingIndex].binding), &(this->bindings[bindingIndex]))) return false;

	}
	return true;
}

OgResourceSetHandle::OgResourceSetHandle() 
	: OgHandle(OgHandleType::RESOURCE_SET) 
{
}

OgResourceSetHandle::OgResourceSetHandle(OgResourceUsage* usages, uint32 usageCount)
	: OgHandle(OgHandleType::RESOURCE_SET)
	, resourceUsages(nullptr), 
	resourceUsageCount(usageCount)
{
	// No Deep copy on LvResourceBinding on LvResourceBufferUsage
	// Becuase LvResourceLayoutHandle does have the information.
	if (resourceUsageCount > 0)
	{
		this->resourceUsages = new OgResourceUsage[resourceUsageCount];
		memcpy(this->resourceUsages, usages, sizeof(OgResourceUsage) * resourceUsageCount);

		for (uint32 i = 0; i < usageCount; ++i)
		{
			const OgResourceUsage& srcUsage = usages[i];
			OgResourceUsage& destUsage = resourceUsages[i];

			uint32 count = srcUsage.binding.arrayCount == 0 ? 1 : srcUsage.binding.arrayCount;
			switch (srcUsage.binding.type)
			{
			case OgResourceType::UNIFORM_BUFFER:
			{
				destUsage.buffer.handle = (OgBufferHandle**)og_malloc((sizeof(OgBufferHandle*) + sizeof(uint32) + sizeof(uint32)) * count);
				destUsage.buffer.offset = (uint32*)(((uint8*)destUsage.buffer.handle) + sizeof(OgBufferHandle*) * count);
				destUsage.buffer.range = (uint32*)(((uint8*)destUsage.buffer.offset) + sizeof(uint32) * count);
				memcpy(destUsage.buffer.handle, srcUsage.buffer.handle, sizeof(OgBufferHandle*) * count);
				memcpy(destUsage.buffer.offset, srcUsage.buffer.offset, sizeof(uint32) * count);
				memcpy(destUsage.buffer.range, srcUsage.buffer.range, sizeof(uint32) * count);
				break;
			}
			case OgResourceType::COMBINED_IMAGE_SAMPLER:
			{
				destUsage.texture.handle = (OgTextureHandle**)og_malloc(sizeof(OgTextureHandle*) * count);
				memcpy(destUsage.texture.handle, srcUsage.texture.handle, sizeof(OgTextureHandle*) * count);
				break;
			}
			}
		}
	}
}
OgResourceSetHandle::~OgResourceSetHandle()
{
	if (this->resourceUsageCount > 0)
	{
		for (uint32 i = 0; i < this->resourceUsageCount; ++i)
		{
			OgResourceUsage& u = this->resourceUsages[i];

			switch (u.binding.type)
			{
			case OgResourceType::UNIFORM_BUFFER:
			{
				og_free(u.buffer.handle);
				break;
			}
			case OgResourceType::COMBINED_IMAGE_SAMPLER:
			{
				og_free(u.texture.handle);
				break;
			}
			}
		}

		delete[] this->resourceUsages;
		this->resourceUsages = nullptr;
		this->resourceUsageCount = 0;
	}
}

/// LvColorBlendDescriptor
OgColorBlendDescriptor::OgColorBlendDescriptor() : attachmentCount(0)
{}

OgColorBlendDescriptor::~OgColorBlendDescriptor() {}

uint32 OgColorBlendDescriptor::GetHashCode() const
{
	//uint32 hash = CRCHash::TypeCrc32(attachmentCount, 0);
	//if (attachmentCount > 0)
	//{
	//	hash = CRCHash::MemCrc32((void*)attachments, (uint32)sizeof(Attachment) * attachmentCount, hash);
	//}
	//return hash;
	return 0;
}

void OgColorBlendDescriptor::CopyFrom(const OgColorBlendDescriptor& cbd)
{
	if (cbd.attachmentCount > 0)
	{
		memcpy(this->attachments, cbd.attachments, sizeof(OgColorBlendDescriptor::Attachment) * cbd.attachmentCount);
	}

	this->attachmentCount = cbd.attachmentCount;
}


/// LvShaderDescriptor
OgShaderDescriptor::OgShaderDescriptor()
	: program(nullptr)
	, shaders{ nullptr, nullptr }
	, shaderCount(0)
{}

OgShaderDescriptor::~OgShaderDescriptor() {}

void OgShaderDescriptor::CopyFrom(const OgShaderDescriptor& sd)
{
	this->program = sd.program;
	this->shaders[0] = sd.shaders[0];
	this->shaders[1] = sd.shaders[1];
	this->shaderCount = sd.shaderCount;
}

OgPipelineHandle::OgPipelineHandle()
	: OgHandle(OgHandleType::PIPELINE)
	, renderPass(nullptr)
{
	
}
OgPipelineHandle::OgPipelineHandle(const OgPipelineDescriptor& descriptor)
	: OgHandle(OgHandleType::PIPELINE)
	, renderPass(nullptr)
{
	this->type = descriptor.type;
	this->name = descriptor.name;
	this->renderPass = descriptor.renderPass;
	this->resourceLayout = descriptor.resourceLayout;

	this->vertexInputDescriptor.CopyFrom(descriptor.vertexInput);

	this->shaderDescriptor.CopyFrom(descriptor.shader);

	this->colorBlendDescriptor.CopyFrom(descriptor.colorBlend);

	memcpy(&(this->rasterizationDescriptor), &(descriptor.rasterize), sizeof(OgRasterizationDescriptor));

	memcpy(&(this->depthStencilDescriptor), &(descriptor.depthStencil), sizeof(OgDepthStencilDescriptor));
}

OgPipelineHandle::~OgPipelineHandle()
{
	this->vertexInputDescriptor.Release();
}

OgCommandEncoderHandle::OgCommandEncoderHandle()
	: OgHandle(OgHandleType::COMMAND_ENCODER)
{

}


OG_NAMESPACE_RENDER_END