#pragma once
#include <vector>
#include "Core/Defines.h"

class ImageLoader{
public:
    ImageLoader(const char* RelativeFilePath);
    ~ImageLoader();
    bool IsValid() const{return !!TempData;}
    int GetWidth() const {return Width;}
    int GetHeight() const {return Height;}
    int GetBytesPerPixel() const{return BytesPerPixel; }
    void GetData(std::vector<uint8>& OutData) const;

private:
    void* TempData;
    int Width;
    int Height;
    int BytesPerPixel;
    static constexpr int BYTES_PER_PIXEL = 3;

};