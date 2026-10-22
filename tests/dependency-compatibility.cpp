#include "crengine.h"
#include "lvimg.h"
#include "testutil.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <jpeglib.h>
#include <png.h>
#include <zlib.h>
#include <curl/curl.h>

lString16 pbSkinFileName, openedCacheFile, currentCacheDir;
class ImageCheck : public LVImageDecoderCallback {
public:
    int lines = 0;
    bool good = true, ended = false;
    void OnStartDecode(LVImageSource *image) {
        good = image->GetWidth() == 16 && image->GetHeight() == 16;
    }
    bool OnLineDecoded(LVImageSource *, int y, lUInt32 *pixels) {
        good = good && y == lines && pixels;
        for (int x = 0; x < 16; ++x)
            good = good && ((pixels[x] & 0xffffff) == 0xffffff);
        ++lines;
        return true;
    }
    void OnEndDecode(LVImageSource *, bool errors) { good = good && !errors; ended = true; }
};
static int decode(const char *path) {
    LVStreamRef stream = LVOpenFileStream(path, LVOM_READ);
    CHECK(!stream.isNull());
    LVImageSourceRef image = LVCreateStreamImageSource(stream);
    CHECK(!image.isNull());
    ImageCheck callback;
    CHECK(image->Decode(&callback));
    CHECK(callback.good && callback.ended && callback.lines == 16);
    return 0;
}
int main(int argc, char **argv) {
    CHECK(argc == 2);
    CHECK(LIBJPEG_TURBO_VERSION_NUMBER == 3002000);
    FT_Library ft;
    CHECK(!FT_Init_FreeType(&ft));
    FT_Int major, minor, patch;
    FT_Library_Version(ft, &major, &minor, &patch);
    CHECK(major == 2 && minor == 15 && patch == 0);
    FT_Done_FreeType(ft);
    CHECK(!std::strcmp(png_get_libpng_ver(NULL), "1.6.59"));
    CHECK(!std::strcmp(zlibVersion(), "1.3.2"));
    CHECK(!std::strcmp(curl_version_info(CURLVERSION_NOW)->version, "8.23.0"));
    const char *const *protocol = curl_version_info(CURLVERSION_NOW)->protocols;
    int protocols = 0;
    for (; *protocol; ++protocol, ++protocols)
        CHECK(!std::strcmp(*protocol, "http") || !std::strcmp(*protocol, "https"));
    CHECK(protocols == 2);

    // Exercise the engine's custom stream adapters, not just the upstream APIs.
    for (int mode = 0; mode < 3; ++mode) {
        std::string path = std::string(argv[1]) + "/image-" + char('0' + mode) + ".jpg";
        FILE *output = std::fopen(path.c_str(), "wb"); CHECK(output);
        jpeg_compress_struct encoder; jpeg_error_mgr error;
        encoder.err = jpeg_std_error(&error); jpeg_create_compress(&encoder);
        jpeg_stdio_dest(&encoder, output);
        encoder.image_width = encoder.image_height = 16;
        encoder.input_components = mode == 2 ? 1 : 3;
        encoder.in_color_space = mode == 2 ? JCS_GRAYSCALE : JCS_RGB;
        jpeg_set_defaults(&encoder);
        if (mode == 1) jpeg_simple_progression(&encoder);
        jpeg_start_compress(&encoder, TRUE);
        unsigned char row[48]; std::memset(row, 255, sizeof(row));
        JSAMPROW scanline = row;
        while (encoder.next_scanline < encoder.image_height)
            jpeg_write_scanlines(&encoder, &scanline, 1);
        jpeg_finish_compress(&encoder); jpeg_destroy_compress(&encoder);
        CHECK(!std::fclose(output)); CHECK(!decode(path.c_str()));
    }
    std::string path = std::string(argv[1]) + "/image.png";
    png_image image; std::memset(&image, 0, sizeof(image));
    image.version = PNG_IMAGE_VERSION; image.width = image.height = 16;
    image.format = PNG_FORMAT_RGBA;
    std::vector<unsigned char> pixels(16 * 16 * 4, 255);
    CHECK(png_image_write_to_file(&image, path.c_str(), 0, pixels.data(), 0, NULL));
    png_image_free(&image);
    CHECK(!decode(path.c_str()));
    puts("PASS: pinned runtime versions, HTTP(S)-only curl, engine baseline/progressive/grayscale JPEG and RGBA PNG decoding");
    return 0;
}
