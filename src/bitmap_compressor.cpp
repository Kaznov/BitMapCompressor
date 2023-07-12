#include <array>
#include <bit>
#include <cstdio>
#include <expected>
#include <map>
#include <memory>
#include <set>

#include "bitmap_loader.hpp"

std::expected<BitmapData, const char*> compressBitmap(const BitmapData& bmp) {
    if (bmp.bits_per_pixel != 24) {
        return std::unexpected{"The bitmap is already compressed"};
    }

    using uchar = unsigned char;
    struct RGB {
        uchar R;
        uchar G;
        uchar B;
        auto operator<=>(const RGB& other) const = default;
    };

    auto get_color = [&bmp](size_t idx) {
        return RGB{ bmp.data[idx * 3 + 0], bmp.data[idx * 3 + 1], bmp.data[idx * 3 + 2] };
    };

    std::set<RGB> colors;
    for (size_t idx = 0; idx < bmp.width * bmp.height; ++idx) {
        colors.insert(get_color(idx));
    }

    size_t bits_per_pixel =
        colors.size() <= (1 << 1) ? 1 :
        colors.size() <= (1 << 4) ? 4 :
        colors.size() <= (1 << 8) ? 8 :
        24;

    if (bits_per_pixel == 24) {
        return std::unexpected{"The bitmap has too many colors to be compressed"};
    }

    // From now on, bits_per_pixel is either 1, 4 or 8

    BitmapData result;
    result.width = bmp.width;
    result.height = bmp.height;
    result.bits_per_pixel = bits_per_pixel;
    result.color_map = std::make_unique<uchar[]>(result.getColorMapSize());
    result.data = std::make_unique<uchar[]>(result.getDataSize());

    std::map<RGB, uchar> colors_ids;

    for (size_t idx = 0; auto color : colors) {
        result.color_map[idx * 3 + 0] = color.R;
        result.color_map[idx * 3 + 1] = color.G;
        result.color_map[idx * 3 + 2] = color.B;
        colors_ids[color] = static_cast<uchar>(idx);
        ++idx;
    }

    // even though pixels can ocupy a part of a byte, scanlines are byte-aligned
    size_t scan_line_width = (result.width * bits_per_pixel + 8 - 1) / 8;

    for (size_t row_idx = 0; row_idx < result.height; ++row_idx) {
        // byte offset of the row
        size_t row_offset = row_idx * scan_line_width;

        for (size_t col_idx = 0; col_idx < result.width; ++col_idx) {
            // byte offset of the pixel
            size_t line_offset = col_idx * bits_per_pixel / 8;
            // bit offset of the pixel
            size_t in_byte_offset = (8 - (col_idx * bits_per_pixel) % 8 - bits_per_pixel);


            size_t pixel_idx = row_idx * result.width + col_idx;
            uchar color_id = colors_ids[get_color(pixel_idx)];

            // We need to "pack" pixels into bytes, we add a part of it
            uchar shifted_pixel_data = color_id << in_byte_offset;
            result.data[row_offset + line_offset] |= shifted_pixel_data;
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
    auto load_result = loadBitmap(file_in);
    if (!load_result.has_value()) {
        fprintf(stderr, "ERROR: %s\n", load_result.error());
        return EXIT_FAILURE;
    }

    printf("Compressing bitmap...\n");
    auto compression_result = compressBitmap(load_result.value());
    if (!compression_result.has_value()) {
        fprintf(stderr, "ERROR: %s\n", compression_result.error());
        return EXIT_FAILURE;
    }

    printf("Saving compressed bitmap...\n");
    auto save_result = saveBitmap(file_out, compression_result.value());
    if (!save_result.has_value()) {
        fprintf(stderr, "ERROR: %s\n", save_result.error());
        return EXIT_FAILURE;
    }

    printf("Done!\n");
}
