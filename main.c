#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "function.h"
#include "lzw.h"
#include "files.h"
#include "bmp.h"
#include <png.h>
#include "pngs.h"

#define TABLE_SIZE 16384
#define MAX_CHAR 256

int writeBinaryPayloadData(const char* filename, const unsigned char* data, int size);
void printData(const unsigned char* data, int size, const char* label);

int main() {

    /*
    const char* imageFilename = "../bmp24.bmp";
    const char* payloadFilename = "../k.txt";
    const char* outputImageFilename = "../output.bmp";
    const char* decompressedPayloadFilename = "../decompressed_payload.txt";

    // Step 1: Determine file type and check if it's 24-bit
    printf("start step1\n");
    if (determineFileTypeAndCheck24Bit(imageFilename)) {
        fprintf(stderr, "File is not a 24-bit BMP or PNG.\n");
        return 1;
    }

    // Step 2: Load payload
    printf("start step2\n");
    int payloadSize;
    unsigned char* payloadData = readBinaryPayloadData(payloadFilename, &payloadSize);
    if (!payloadData) {
        fprintf(stderr, "Failed to load payload data.\n");
        return 1;
    }

    // Step 3: Compress payload
    printf("start step3\n");
    int compressedSize;
    int* compressedPayload = lzwCompress(payloadData, payloadSize, &compressedSize);
    free(payloadData); // Assume payloadData is no longer needed after this point

    if (!compressedPayload) {
        fprintf(stderr, "Compression failed.\n");
        return 1;
    }

    // Step 4: Embed payload into image and save the output image
    printf("start step4\n");
    if (embedPayloadInImage(imageFilename, outputImageFilename, compressedPayload, compressedSize, payloadFilename) != 0) {
        fprintf(stderr, "Failed to embed payload into image or create output image.\n");
        free(compressedPayload);
        return 1;
    }
    free(compressedPayload);

    // Step 5: Extract and decompress payload from image
    printf("start step5\n");
    int extractionSuccess = extractAndDecompressPayload(outputImageFilename, decompressedPayloadFilename);
    if (extractionSuccess != 0) {
        fprintf(stderr, "Failed to extract and decompress payload from image.\n");
        return 1;
    }

    /*
    // Step 6: Compare the original and decompressed payloads
    printf("start step6\n");
    int newPayloadSize;
    unsigned char* newPayload = readBinaryPayloadData(decompressedPayloadFilename, &newPayloadSize);
    printf("Original Payload:\n");
    for (int i = 0; i < payloadSize; i++) {
        printf("%02x ", payloadData[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");

    printf("Decompressed Payload:\n");
    for (int i = 0; i < newPayloadSize; i++) {
        printf("%02x ", newPayload[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
    if (newPayloadSize != payloadSize || memcmp(payloadData, newPayload, payloadSize) != 0) {
        fprintf(stderr, "The original and decompressed payloads do not match.\n");
        free(newPayload);
        return 1;
    }


    printf("The original and decompressed payloads match. Success!\n");

    free(newPayload);
     */


    const char* file_name = "../images.png";

    if (determineFileTypeAndCheck24Bit(file_name)) {
        fprintf(stderr, "File is not a 24-bit BMP or PNG.\n");
        return 1;
    }

    FILE *fp = fopen(file_name, "rb");
    if (!fp) {
        fprintf(stderr, "Could not open file %s for reading.\n", file_name);
        return 1;
    }


    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fprintf(stderr, "Failed to create PNG read structure.\n");
        fclose(fp);
        return 1;
    }


    png_infop info = png_create_info_struct(png);
    if (!info) {
        fprintf(stderr, "Failed to create PNG info structure.\n");
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(fp);
        return 1;
    }

    if (setjmp(png_jmpbuf(png))) {
        fprintf(stderr, "Error during PNG read initialization.\n");
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return 1;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
    return 0;



}











int comparePayloads(const int* payload1, const int* payload2, int size) {
    for (int i = 0; i < size; i++) {
        if (payload1[i] != payload2[i]) {
            printf("Mismatch at index %d: %d != %d\n", i, payload1[i], payload2[i]);
            return 0; // Return 0 if mismatch
        }
    }
    return 1; // Return 1 if match
}




/*



// Function to compare two blocks of memory




int writeBinaryPayloadData(const char* filename, const unsigned char* data, int size) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Failed to open file for writing payload.\n");
        return 1;
    }

    fwrite(data, 1, size, file);
    fclose(file);
    return 0;
}

void printData(const unsigned char* data, int size, const char* label) {
    printf("%s - First few bytes: ", label);
    for (int i = 0; i < size; ++i) {
        printf("%02x ", data[i]);
    }

    printf("\n");
}
 */