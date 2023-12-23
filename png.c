#include "files.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
//#include "function.h"
#include <stdlib.h>
#include "lzw.h"
#include "bmp.h"
#include "pngs.h"
#include <png.h>



PNGPixel* read_png_file(const char* file_name, png_uint_32* width, png_uint_32* height) {
    FILE *fp = fopen(file_name, "rb");
    if (!fp) {
        fprintf(stderr, "Could not open file %s for reading.\n", file_name);
        return NULL;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fprintf(stderr, "Failed to create PNG read structure.\n");
        fclose(fp);
        return NULL;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        fprintf(stderr, "Failed to create PNG info structure.\n");
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(fp);
        return NULL;
    }

    if (setjmp(png_jmpbuf(png))) {
        fprintf(stderr, "Error during PNG read initialization.\n");
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return NULL;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    *width = png_get_image_width(png, info);
    *height = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    // Ensure the PNG is in 8-bit RGB format
    if (color_type != PNG_COLOR_TYPE_RGB || bit_depth != 8) {
        fprintf(stderr, "The image is not a 24-bit depth RGB PNG.\n");
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return NULL;
    }

    // Update the png info struct.
    png_read_update_info(png, info);

    // Allocate memory for the PNG pixel data
    PNGPixel* pixel_data = (PNGPixel*)malloc((*width) * (*height) * sizeof(PNGPixel));
    if (!pixel_data) {
        fprintf(stderr, "Could not allocate memory for pixel data.\n");
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return NULL;
    }

    // Allocate memory for reading the png rows
    png_bytep row_pointers[*height];
    for (png_uint_32 y = 0; y < *height; y++) {
        row_pointers[y] = (png_bytep)(pixel_data + y * (*width));
    }

    // Read the image data
    if (setjmp(png_jmpbuf(png))) {
        fprintf(stderr, "Error during reading PNG.\n");
        free(pixel_data);
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return NULL;
    }

    png_read_image(png, row_pointers);

    // Cleanup
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);

    return pixel_data;
}

void print_pixel_data(PNGPixel* pixels, png_uint_32 width, png_uint_32 height) {
    // Print the RGB values of the first few pixels as a test
    for (int y = 0; y < height && y < 10; ++y) {
        for (int x = 0; x < width && x < 10; ++x) {
            PNGPixel pixel = pixels[y * width + x];
            printf("Pixel at (%d,%d): R=%d, G=%d, B=%d\n", x, y, pixel.red, pixel.green, pixel.blue);
        }
    }
}