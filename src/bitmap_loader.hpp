#ifndef BITMAP_OADER_HPP
#define BITMAP_LOADER_HPP

#include <expected>
#include <vector>

struct BitmapData {
    std::vector<unsigned char> data;
    std::vector<unsigned char> color_map;
    size_t width;
    size_t height;
    size_t bits_per_pixel;

    size_t getColorMapSize() const {
        return bits_per_pixel == 24 ? 0 : (1 << bits_per_pixel) * 3;
    }

    size_t getDataSize() const {
        return height * getScanlineWidth();
    }

    size_t getScanlineWidth() const {
        // scanlines of bitpacked bitmaps have to be aligned to 4-byte boundaries
        return (width * bits_per_pixel + 32 - 1) / 32 * 4;
    }
};

std::expected<BitmapData, const char*> loadBitmap(const char* filename);
std::expected<void, const char*> saveBitmap(const char* filename, const BitmapData& bitmap);


#endif // BIRMAL_LOADER_HPP
