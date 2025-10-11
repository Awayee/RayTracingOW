#include "RHI/RHICommon.h"
class NullRHI {
public:
	NullRHI(uint32 WindowWidth, uint32 WindowHeight) {}
	~NullRHI() = default;
	// public func
	RHITextureHandle CreateTexture(uint32 width, uint32 height, uint16 layer, uint16 mip, ERHIFormat format){ return nullptr;}
	void UpdateTextureData(RHITextureHandle Handle, const void* Data, uint64 ByteSize) {}
	void DestroyTexture(RHITextureHandle Handle) {}
	bool DrawTexture(RHITextureHandle TextureHandle){return false;}
};