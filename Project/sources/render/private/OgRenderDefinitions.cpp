#include "OgPrecompile.h"

#include "system/OgMemory.h"
#include "system/OgHashCode.h"

#include "render/OgRenderDefinitions.h"

OG_NAMESPACE_RENDER_BEGIN

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

OgResourceLayoutHandle::OgResourceLayoutHandle() : OgHandle(OgHandleType::RESOURCE_LAYOUT) { }

OgResourceSetHandle::OgResourceSetHandle() : OgHandle(OgHandleType::RESOURCE_SET) { }

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


OG_NAMESPACE_RENDER_END