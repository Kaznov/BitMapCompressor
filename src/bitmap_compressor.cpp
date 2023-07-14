#include <cstdio>
#include <expected>
#include <map>
#include <set>

#include "bitmap_loader.hpp"

using uchar = unsigned char;
struct RGB {
    uchar R;
    uchar G;
    uchar B;
    auto operator<=>(const RGB& other) const = default;
};

RGB getPixel(const BitmapData& bmp, size_t row_idx, size_t col_idx) {
    // assert(bmp.bits_per_pixel == 24);
    const size_t scanline_width = bitmapScanlineWidth(bmp.width, bmp.bits_per_pixel);
    size_t offset = row_idx * scanline_width + col_idx * 3;
    return RGB{ bmp.data[offset + 2], bmp.data[offset + 1], bmp.data[offset + 0] };
}

std::set<RGB> getBitmapColors(const BitmapData& bmp) {
    std::set<RGB> colors;

    for (size_t row_idx = 0; row_idx < bmp.height; ++row_idx) {
        for (size_t col_idx = 0; col_idx < bmp.width; ++col_idx) {
            colors.insert(getPixel(bmp, row_idx, col_idx));
        }
    }

    return colors;
}


std::expected<BitmapData, const char*> compressBitmap(const BitmapData& bmp) {
    if (bmp.bits_per_pixel != 24) {
        return std::unexpected{"The bitmap is already compressed"};
    }

    const size_t width = bmp.width;
    const size_t height = bmp.height;

    const std::set<RGB> colors = getBitmapColors(bmp);
    const size_t bits_per_pixel =
        colors.size() <= (1 << 1) ? 1 :
        colors.size() <= (1 << 4) ? 4 :
        colors.size() <= (1 << 8) ? 8 :
        24;

    if (bits_per_pixel == 24) {
        return std::unexpected{"The bitmap has too many colors to be compressed"};
    }

    // From now on, bits_per_pixel is either 1, 4 or 8

    BitmapData result(width, height, bits_per_pixel);

    std::map<RGB, uchar> colors_ids;
    // write color map into buffer and save colors ids
    for (size_t idx = 0; auto color : colors) {
        result.color_map[idx * 3 + 0] = color.B;
        result.color_map[idx * 3 + 1] = color.G;
        result.color_map[idx * 3 + 2] = color.R;
        colors_ids[color] = static_cast<uchar>(idx);
        ++idx;
    }

    const size_t out_scanline_width = bitmapScanlineWidth(width, bits_per_pixel);

    for (size_t row_idx = 0; row_idx < height; ++row_idx) {
        // byte offset of the row
        const size_t row_offset = row_idx * out_scanline_width;

        for (size_t col_idx = 0; col_idx < width; ++col_idx) {
            // byte offset of the pixel
            const size_t line_offset = col_idx * bits_per_pixel / 8;
            const size_t offset = row_offset + line_offset;

            // bit offset of the pixel
            const size_t in_byte_offset = (8 - (col_idx * bits_per_pixel) % 8 - bits_per_pixel);

            const RGB color = getPixel(bmp, row_idx, col_idx);
            const uchar color_id = colors_ids.find(color)->second;

            // We need to "pack" pixels into bytes, we add a part of it
            const uchar shifted_pixel_data = color_id << in_byte_offset;
            result.data[offset] |= shifted_pixel_data;
        }
    }

    return result;
}

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        fputs("Usage: ./bitmap_compressor file_in file_out\n", stderr);
        return EXIT_FAILURE;
    }

    const char* file_in  = argv[1];
    const char* file_out = argv[2];

    printf("Reading bitmap from file %s...\n", file_in);
    const auto load_result = loadBitmap(file_in);
    if (!load_result.has_value()) {
        fprintf(stderr, "ERROR: %s\n", load_result.error());
        return EXIT_FAILURE;
    }

    printf("Compressing bitmap...\n");
    const auto compression_result = compressBitmap(load_result.value());
    if (!compression_result.has_value()) {
        fprintf(stderr, "ERROR: %s\n", compression_result.error());
        return EXIT_FAILURE;
    }

    printf("Saving compressed bitmap...\n");
    const auto save_result = saveBitmap(file_out, compression_result.value());
    if (!save_result.has_value()) {
        fprintf(stderr, "ERROR: %s\n", save_result.error());
        return EXIT_FAILURE;
    }

    printf("Done!\n");
}
