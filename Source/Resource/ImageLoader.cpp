#include "Resource/ImageLoader.h"
#include <string>
#include <filesystem>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include <stb_image.h>


ImageLoader::ImageLoader(const char* RelativeFilePath): TempData(nullptr), Width(0), Height(0), BytesPerPixel(BYTES_PER_PIXEL){
	std::filesystem::path FullPath { ASSETS_PATH };
	FullPath.append(RelativeFilePath);
	TempData = stbi_load(FullPath.string().c_str(), &Width, &Height, &BytesPerPixel, BYTES_PER_PIXEL);
}

ImageLoader::~ImageLoader() {
	if(TempData) {
		stbi_image_free(TempData);
	}
}

void ImageLoader::GetData(std::vector<uint8>& OutData) const {
	const size_t ByteSize = (size_t)Width * Height * BytesPerPixel;
	OutData.resize(ByteSize);
	memcpy(OutData.data(), TempData, ByteSize);
}
