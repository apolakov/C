#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "function.h"

bool is_png(FILE *file) {
    uint8_t png_signature[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    uint8_t buffer[8];

    if (fread(buffer, 1, 8, file) == 8) {
        return memcmp(buffer, png_signature, 8) == 0;
    }
    return false;
}

// Function to check if the file is a BMP.
bool is_bmp(FILE *file) {
    uint8_t bmp_signature[2] = {0x42, 0x4D};
    uint8_t buffer[2];

    if (fread(buffer, 1, 2, file) == 2) {
        return memcmp(buffer, bmp_signature, 2) == 0;
    }
    return false;
}

// Function to open the file and determine its type
void check_file_type(const char *filename) {
    FILE *file = fopen(filename, "rb");

    if (!file) {
        perror("File opening failed");
        return;
    }

    if (is_png(file)) {
        printf("%s is a PNG file.\n", filename);
    } else {
        rewind(file);
        if (is_bmp(file)) {
            printf("%s is a BMP file.\n", filename);
        } else {
            printf("%s is neither a PNG nor a BMP file.\n", filename);
        }
    }

    fclose(file);
}


bool is_24bit_bmp(FILE *file) {
    rewind(file);

    // Skip the BMP header which is 14 bytes
    if (fseek(file, 14, SEEK_SET) != 0) {
        return false;
    }

    // Read the size of the DIB header
    uint32_t dib_header_size;
    if (fread(&dib_header_size, sizeof(dib_header_size), 1, file) != 1) {
        return false;
    }

    // The bits_per_pixel field is located 14 bytes after the start of the DIB header.
    // Since we have already read 4 bytes for the dib_header_size, we need to move additional 10 bytes
    if (fseek(file, 10, SEEK_CUR) != 0) {
        return false;
    }

    uint16_t bits_per_pixel;
    if (fread(&bits_per_pixel, sizeof(bits_per_pixel), 1, file) != 1) {
        return false;
    }

    // The BMP is 24-bit if bits_per_pixel is 24
    return bits_per_pixel == 24;
}


bool is_24bit_png(FILE *file) {
    rewind(file);

    // PNG signature is 8 bytes, but we'll skip it because we've already checked if it's a PNG
    if (fseek(file, 8, SEEK_SET) != 0) {
        return false;
    }

    // Read IHDR chunk length and type
    uint32_t chunk_length;
    char chunk_type[5];
    if (fread(&chunk_length, sizeof(chunk_length), 1, file) != 1) {
        return false;
    }
    if (fread(chunk_type, 1, 4, file) != 4) {
        return false;
    }
    chunk_type[4] = '\0'; // Null-terminate the string

    // Check if it is IHDR chunk
    if (strcmp(chunk_type, "IHDR") != 0) {
        return false;
    }

    // Skip width and height (4 bytes each)
    if (fseek(file, 8, SEEK_CUR) != 0) {
        return false;
    }

    uint8_t bit_depth;
    if (fread(&bit_depth, sizeof(bit_depth), 1, file) != 1) {
        return false;
    }

    uint8_t color_type;
    if (fread(&color_type, sizeof(color_type), 1, file) != 1) {
        return false;
    }

    // Check for 24-bit RGB: color type 2 and bit depth 8
    return color_type == 2 && bit_depth == 8;
}


int analyze_file_format(const char *filename) {
    int answer = 0;
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("File opening failed");
        return -1;
    }

    // Check if it's a PNG file
    if (is_png(file)) {
        printf("%s is a PNG file.\n", filename);
        // Now check if it's a 24-bit PNG file
        if (is_24bit_png(file)) {
            printf("%s is a 24-bit RGB PNG file.\n", filename);
            answer = 1;
        } else {
            printf("%s is not a 24-bit RGB PNG file.\n", filename);
            answer = 2;
        }
    }
        // Check if it's a BMP file
    else {
        rewind(file); // Rewind the file pointer for BMP check
        if (is_bmp(file)) {
            printf("%s is a BMP file.\n", filename);
            // Now check if it's a 24-bit BMP file
            if (is_24bit_bmp(file)) {
                printf("%s is a 24-bit RGB BMP file.\n", filename);
                answer = 3;
            } else {
                printf("%s is not a 24-bit RGB BMP file.\n", filename);
                answer = 4;
            }
        } else {
            printf("%s is neither a PNG nor a BMP file.\n", filename);
        }
    }

    fclose(file);
    return answer;
}


