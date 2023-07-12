#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <expected>
#include <memory>

#include "bitmap_loader.hpp"


using u16 = uint16_t;
using i16 = int16_t;
using u32 = uint32_t;
using i32 = int32_t;

// The fields in the structure are not packed! Exposition only!
// struct BitmapFileHeader {
//   i16 bfType;
//   u32 bfSize;
//   i16 bfReserved1;
//   i16 bfReserved2;
//   u32 bfOffBits;
// };

struct BitmapCoreInfoHeader {
    u32 size;
    u16 width;
    u16 height;
    u16 planes;
    u16 bitCount;
};
static_assert(sizeof(BitmapCoreInfoHeader) == 12);

struct BitmapInfoHeader {
    u32 size;
    i32 width;
    i32 height;
    u16 planes;
    u16 bitCount;
    u32 compression;
    u32 sizeImage;
    i32 xPelsPerMeter;
    i32 yPelsPerMeter;
    u32 clrUsed;
    u32 clrImportant;
};
static_assert(sizeof(BitmapInfoHeader) == 40);

struct BitmapInfoHeaderV4 {
    u32 size;
    i32 width;
    i32 height;
    u16 planes;
    u16 bitCount;
    u32 compression;
    u32 sizeImage;
    i32 xPelsPerMeter;
    i32 yPelsPerMeter;
    u32 clrUsed;
    u32 clrImportant;
    u32 redMask;
    u32 greenMask;
    u32 blueMask;
    u32 alphaMask;
    u32 CSType;
    u32 endpoints[9];
    u32 gammaRed;
    u32 gammaGreen;
    u32 gammaBlue;
};
static_assert(sizeof(BitmapInfoHeaderV4) == 108);

struct BitmapInfoHeaderV5 {
    u32 size;
    i32 width;
    i32 height;
    u16 planes;
    u16 bitCount;
    u32 compression;
    u32 sizeImage;
    i32 xPelsPerMeter;
    i32 yPelsPerMeter;
    u32 clrUsed;
    u32 clrImportant;
    u32 redMask;
    u32 greenMask;
    u32 blueMask;
    u32 alphaMask;
    u32 CSType;
    u32 endpoints[9];
    u32 gammaRed;
    u32 gammaGreen;
    u32 gammaBlue;
    u32 intent;
    u32 profileData;
    u32 profileSize;
    u32 reserved;
};
static_assert(sizeof(BitmapInfoHeaderV5) == 124);

struct FileGuard {
    FILE* f;
    ~FileGuard() { fclose(f); }
};

std::expected<void, const char*> saveBitmap(const char* filename, const BitmapData& bitmap) {
    static constexpr size_t file_header_size = 14;
    size_t file_size_z = file_header_size + sizeof(BitmapCoreInfoHeader) + bitmap.getColorMapSize() + bitmap.getDataSize();
    if (file_size_z > UINT32_MAX) {
        return std::unexpected{"Bitmap is too big"};
    }
    u32 file_size = static_cast<u32>(file_size_z);

    if (bitmap.height > UINT16_MAX || bitmap.width > UINT16_MAX) {
        return std::unexpected{"Bitmap dimentions are too big"};
    }
    u16 height = static_cast<u16>(bitmap.height);
    u16 width = static_cast<u16>(bitmap.width);

    FILE* f = fopen(filename, "w");
    if (f == nullptr) {
        return std::unexpected{"Cannot open the file for writing"};
    }

    // closes file on function exit
    FileGuard guard{f};

    // Write file header (see struct BitmapFileHeader on top of the file )
    fwrite("BM", 2, 1, f);
    fwrite(&file_size, sizeof(u32), 1, f);
    u32 reserved_zero = 0;
    fwrite(&reserved_zero, sizeof(u32), 1, f);
    u32 data_offset = static_cast<u32>(file_header_size + sizeof(BitmapCoreInfoHeader) + bitmap.getColorMapSize());
    fwrite(&data_offset, sizeof(u32), 1, f);

    BitmapCoreInfoHeader info_header;
    info_header.size = sizeof(BitmapCoreInfoHeader);
    info_header.width = width;
    info_header.height = height;
    info_header.planes = 1;
    info_header.bitCount = static_cast<u16>(bitmap.bits_per_pixel);

    fwrite(&info_header, sizeof(info_header), 1, f);
    fwrite(bitmap.color_map.get(), bitmap.getColorMapSize(), 1, f);
    fwrite(bitmap.data.get(), bitmap.getDataSize(), 1, f);

    if (ferror(f)) {
        return std::unexpected{"Writing to bitmap file failed"};
    }

    return {};
}

std::expected<BitmapData, const char*> loadBitmap(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (f == nullptr) {
        return std::unexpected{"Cannot open the file for reading"};
    }

    // closes file on function exit
    FileGuard guard{f};

    unsigned char header_bytes[18];
    if (fread(header_bytes, 18, 1, f) != 1) {
        return std::unexpected{"The file is too short for a bitmap file"};
    }
    if (header_bytes[0] != 'B' || header_bytes[1] != 'M') {
        return std::unexpected{"The file is not a bitmap file"};
    }

    u32 file_size;
    u32 data_offset;
    u32 header_size;

    memcpy(&file_size, header_bytes + 2, sizeof(u32));
    memcpy(&data_offset, header_bytes + 10, sizeof(u32));
    memcpy(&header_size, header_bytes + 14, sizeof(u32));

    u16 width;
    u16 height;
    u16 bits_per_pixel;

    // a lambda template - reads from into the InfoHeader
    // gets some basic info from the info header
    // it's a template, so that it can be used for different headers
    auto read_info_header = [&]<typename Header>()
        -> std::expected<void, const char*> {
        Header info_header;
        unsigned char info_header_bytes[sizeof(info_header)]{};
        if (fread(info_header_bytes + sizeof(u32), sizeof(info_header) - sizeof(u32), 1, f) != 1) {
            return std::unexpected{"Cannot read bitmap info header"};
        }
        memcpy(&info_header, info_header_bytes, sizeof(info_header));

        if (info_header.width < 0 || info_header.width > UINT16_MAX ||
            info_header.height < 0 || info_header.height > UINT16_MAX) {
            return std::unexpected{"Incorrect bitmap size in the header, bitmap too big?"};
        }

        if constexpr (!std::is_same_v<Header, BitmapCoreInfoHeader>) {
            if (info_header.compression != 0) {
                return std::unexpected{"This program doesn't accept compressed bitmaps"};
            }
        }

        width = static_cast<u16>(info_header.width);
        height = static_cast<u16>(info_header.height);
        bits_per_pixel = info_header.bitCount;
        return {};
    };

    std::expected<void, const char*> read_info_header_result;

    if (header_size == sizeof(BitmapCoreInfoHeader)) {
        read_info_header_result = read_info_header.operator()<BitmapCoreInfoHeader>();
    }
    else if (header_size == sizeof(BitmapInfoHeader)) {
        read_info_header_result = read_info_header.operator()<BitmapInfoHeader>();
    }
    else if (header_size == sizeof(BitmapInfoHeaderV4)) {
        read_info_header_result = read_info_header.operator()<BitmapInfoHeaderV4>();
    }
    else if (header_size == sizeof(BitmapInfoHeaderV5)) {
        read_info_header_result = read_info_header.operator()<BitmapInfoHeaderV5>();
    } else {
        return std::unexpected{"Unknown info header type"};
    }

    if (!read_info_header_result.has_value()) {
        return std::unexpected{read_info_header_result.error()};
    }

    if (bits_per_pixel != 1 &&
        bits_per_pixel != 4 &&
        bits_per_pixel != 8 &&
        bits_per_pixel != 24) {
        return std::unexpected{"The number of bits per pixel is not supported"};
    }

    BitmapData result;
    result.bits_per_pixel = bits_per_pixel;
    result.height = height;
    result.width = width;

    // If the representation is not the standard triplet, we need to read the color map
    size_t color_map_size = result.getColorMapSize();
    result.color_map = std::make_unique<unsigned char[]>(color_map_size);
    if (fread(result.color_map.get(), 1, color_map_size, f) != color_map_size) {
        return std::unexpected{"Cannot read the color map"};
    }

    size_t data_size = result.getDataSize();
    result.data = std::make_unique<unsigned char[]>(data_size);
    fseek(f, data_offset, SEEK_SET);
    if (fread(result.data.get(), 1, data_size, f) != data_size) {
        return std::unexpected{"Cannot read pixel data"};
    }

    return result;
}
