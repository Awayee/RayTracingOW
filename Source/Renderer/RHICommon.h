#pragma once
#include "Core/Defines.h"

#define NOT_IMPLEMENTED

typedef void* AppInstanceHandle; 
typedef void* RHITextureHandle;
typedef void* RHIPSOHandle;

enum class ERHIFormat {
	R8G8B8A8_UNorm,
};

class NullRHI {
public:
	NullRHI(AppInstanceHandle Window, uint32 WindowWidth, uint32 WindowHeight) {}
	~NullRHI() = default;
	// public func
	RHITextureHandle CreateTexture(uint32 width, uint32 height, uint16 layer, uint16 mip, ERHIFormat format){ return nullptr;}
	void UpdateTextureData(RHITextureHandle Handle, const void* Data, uint64 ByteSize) {}
	void DestroyTexture(RHITextureHandle Handle) {}
	bool DrawTexture(RHITextureHandle TextureHandle){return false;}
};
