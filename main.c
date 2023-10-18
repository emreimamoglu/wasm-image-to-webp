#include <stdlib.h>
#include <emscripten.h>
#include "libwebp/src/webp/encode.h"
#include <png.h>
#include <string.h>
#include <jpeglib.h>
#include <setjmp.h>


unsigned char *decode_jpeg(const unsigned char *jpeg_data, int data_length, int *out_width, int *out_height)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPARRAY buffer;
    int row_stride;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    jpeg_mem_src(&cinfo, jpeg_data, data_length);
    jpeg_read_header(&cinfo, TRUE);

    jpeg_start_decompress(&cinfo);

    *out_width = cinfo.output_width;
    *out_height = cinfo.output_height;
    row_stride = cinfo.output_width * cinfo.output_components;

    unsigned char *rgb_data = (unsigned char *)malloc(row_stride * cinfo.output_height);
    unsigned char *rgba_data = (unsigned char *)malloc(cinfo.output_width * cinfo.output_height * 4);

    buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

    while (cinfo.output_scanline < cinfo.output_height)
    {
        jpeg_read_scanlines(&cinfo, buffer, 1);
        memcpy(rgb_data + (cinfo.output_scanline - 1) * row_stride, buffer[0], row_stride);
    }

    for (int i = 0; i < cinfo.output_width * cinfo.output_height; i++)
    {
        rgba_data[i * 4 + 0] = rgb_data[i * 3 + 0];
        rgba_data[i * 4 + 1] = rgb_data[i * 3 + 1];
        rgba_data[i * 4 + 2] = rgb_data[i * 3 + 2];
        rgba_data[i * 4 + 3] = 255;
    }

    free(rgb_data);
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    return rgba_data;
}

EMSCRIPTEN_KEEPALIVE
unsigned char *decode_png(const unsigned char *png_data, int data_length, int *out_width, int *out_height)
{
    png_image image;
    memset(&image, 0, sizeof(image));
    image.version = PNG_IMAGE_VERSION;

    if (!png_image_begin_read_from_memory(&image, png_data, data_length))
    {
        return NULL;
    }

    image.format = PNG_FORMAT_RGBA;

    *out_width = image.width;
    *out_height = image.height;

    unsigned char *buffer = (unsigned char *)malloc(PNG_IMAGE_SIZE(image));
    if (buffer == NULL)
    {
        return NULL;
    }

    if (!png_image_finish_read(&image, NULL, buffer, 0, NULL))
    {
        free(buffer);
        return NULL;
    }

    return buffer;
}

EMSCRIPTEN_KEEPALIVE
uint8_t *convert_to_webp(const uint8_t *img_data, int width, int height, int has_alpha, float quality_factor, size_t *output_size)
{
    uint8_t *webp_data = NULL;

    if (has_alpha)
    {
        *output_size = WebPEncodeRGBA(img_data, width, height, width * 4, quality_factor, &webp_data);
    }
    else
    {
        *output_size = WebPEncodeRGB(img_data, width, height, width * 3, quality_factor, &webp_data);
    }

    return webp_data;
}

EMSCRIPTEN_KEEPALIVE
uint8_t *decode_and_convert_to_webp(const unsigned char *image_data, int data_length, const char *format, float quality_factor, size_t *output_size)
{
    int width, height;
    unsigned char *decoded_data = NULL;

    if (strcmp(format, "image/png") == 0)
    {
        decoded_data = decode_png(image_data, data_length, &width, &height);
    }
    else if (strcmp(format, "image/jpeg") == 0 || strcmp(format, "image/jpg") == 0)
    {
        decoded_data = decode_jpeg(image_data, data_length, &width, &height);
    }
    else
    {
        
        return NULL;
    }

    if (!decoded_data)
    {
        return NULL;
    }

    uint8_t *webp_data = convert_to_webp(decoded_data, width, height, 1, quality_factor, output_size);
    free(decoded_data);

    return webp_data;
}

EMSCRIPTEN_KEEPALIVE
void free_memory(uint8_t *data)
{
    free(data);
}

EMSCRIPTEN_KEEPALIVE
uint8_t *allocate_memory(size_t size)
{
    return malloc(size);
}