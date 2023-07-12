#ifndef BITMAP_OADER_HPP
#define BITMAP_LOADER_HPP

#include <expected>
#include <memory>

struct BitmapData {
    std::unique_ptr<unsigned char[]> data;
    std::unique_ptr<unsigned char[]> color_map;
    size_t bits_per_pixel;
    size_t width;
    size_t height;

    size_t getColorMapSize() const {
        return bits_per_pixel == 24 ? 0 : (1 << bits_per_pixel) * 3;
    }

    size_t getDataSize() const {
        // scanlines have to be aligned to byte boundaries,
        // (w * bpp + 7) / 8 rounds up scanline to a full byte
        return height * ((width * bits_per_pixel + 8 - 1) / 8);
    }
};

std::expected<BitmapData, const char*> loadBitmap(const char* filename);
std::expected<void, const char*> saveBitmap(const char* filename, const BitmapData& bitmap);


#endif // BIRMAL_LOADER_HPP
