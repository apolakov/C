#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "function.h"
#include <png.h>

#define TABLE_SIZE 16384
#define MAX_CHAR 256

unsigned char* readBinaryPayloadData(const char* filename, int* size);
int writeBinaryPayloadData(const char* filename, const unsigned char* data, int size);

    int main() {
    const char* imageFilename = "../bmp24.bmp";
    const char* payloadFilename = "../k.txt";
    const char* outputImageFilename = "../output.bmp";
    const char* decompressedPayloadBaseFilename = "../PAYLOAD2";

    // Step 1: Determine file type and check if it's 24-bit
    if (determineFileTypeAndCheck24Bit(imageFilename)) {
        fprintf(stderr, "File is not a 24-bit BMP or PNG.\n");
        return 1;
    }

    // Step 2: Load payload
    int payloadSize;
    unsigned char* payloadData = readPayloadData(payloadFilename, &payloadSize);
    if (!payloadData) {
        fprintf(stderr, "Failed to load payload data.\n");
        return 1;
    }

    // Step 3: Compress payload
    int compressedSize = payloadSize;
    int* compressedPayload = lzwCompress(payloadData, &compressedSize);
    free(payloadData);

    if (!compressedPayload) {
        fprintf(stderr, "Compression failed.\n");
        return 1;
    }


    // Step 4: Embed payload into image and save the output image
    if (embedPayloadInImage(imageFilename, outputImageFilename, compressedPayload, compressedSize, payloadFilename) != 0) {        fprintf(stderr, "Failed to embed payload into image or create output image.\n");
        free(compressedPayload);
        return 1;
    }
    free(compressedPayload);

    printf("Hiding is done.\n");

    // Step 5: Extract and decompress payload from image
    if (!extractAndDecompressPayload(outputImageFilename, decompressedPayloadBaseFilename)) {
        fprintf(stderr, "Failed to extract and decompress payload from image.\n");
        return 1;
    }

    // Step 6: Load original compressed payload and compare
    int originalCompressedSize;
    int* originalCompressedPayload = loadOriginalCompressedPayload("../compressed_payload_original.bin", &originalCompressedSize);
    if (originalCompressedPayload) {
        if (comparePayloads(compressedPayload, originalCompressedPayload, originalCompressedSize)) {
            printf("Extraction verification successful: The payloads match.\n");
        } else {
            fprintf(stderr, "Extraction verification failed: The payloads do not match.\n");
        }
        free(originalCompressedPayload);
    } else {
        fprintf(stderr, "Failed to load original compressed payload.\n");
    }

    return 0;
}


unsigned int extractSizeFromPixelData(const Pixel* pixels, int numPixels) {
    unsigned int size = 0;
    for (int i = 0; i < 32; ++i) {
        unsigned int bit = pixels[i].blue & 1;
        size |= (bit << i);
    }
    return size;
}

int* extractPayload(const Pixel* pixels, int numPixels, int* compressedPayloadSize) {
    unsigned int payloadBitSize = extractSizeFromPixelData(pixels, numPixels);
    *compressedPayloadSize = (payloadBitSize + 31) / 32;

    int* payload = (int*)malloc(*compressedPayloadSize * sizeof(int));
    if (!payload) {
        fprintf(stderr, "Memory allocation failed for payload extraction.\n");
        return NULL;
    }

    memset(payload, 0, *compressedPayloadSize * sizeof(int));
    int payloadIndex = 0, bitPosition = 0;

    // Start extracting from pixel 32, skipping the first 32 pixels used for the size
    for (int i = 32; i < numPixels && payloadIndex < *compressedPayloadSize; ++i) {
        int bit = pixels[i].blue & 1;
        //printf("Extracting bit %d from pixel %d\n", bit, i); // Debugging statement
        payload[payloadIndex] |= (bit << bitPosition);
        //printf("Extracting bit %d from pixel %d. Index: %d, BitPos: %d\n", bit, i, payloadIndex, bitPosition);

        bitPosition++;
        if (bitPosition == 32) {
            bitPosition = 0;
            payloadIndex++;
        }
    }

    return payload;
}



Pixel* readPixelData(FILE* file, BITMAPFILEHEADER bfh, BITMAPINFOHEADER bih, int* pixelDataSize) {
    // Calculate the padding for each row (BMP rows are aligned to 4-byte boundaries)
    int padding = (4 - (bih.width * sizeof(Pixel)) % 4) % 4;

    // Calculate the size of the pixel data (including padding)
    *pixelDataSize = bih.width * abs(bih.height) * sizeof(Pixel) + padding * abs(bih.height);

    // Allocate memory for pixel data
    Pixel* pixelData = (Pixel*)malloc(*pixelDataSize);
    if (!pixelData) {
        fprintf(stderr, "Memory allocation failed for pixel data.\n");
        fclose(file);
        return NULL;
    }

    // Set the file position to the beginning of pixel data
    fseek(file, bfh.offset, SEEK_SET);

    // Read pixel data row by row
    for (int i = 0; i < abs(bih.height); ++i) {
        // Read one row of pixel data
        if (fread(pixelData + (bih.width * i), sizeof(Pixel), bih.width, file) != bih.width) {
            fprintf(stderr, "Failed to read pixel data.\n");
            free(pixelData);  // Free allocated memory in case of read error
            fclose(file);
            return NULL;
        }
        // Skip over padding
        fseek(file, padding, SEEK_CUR);
    }

    return pixelData;
}

void setLSB(unsigned char* byte, int bitValue) {
    // Ensure the bitValue is either 0 or 1
    bitValue = (bitValue != 0) ? 1 : 0;

    // Set or clear the LSB
    *byte = (*byte & 0xFE) | bitValue;
}

int getBit(const int* data, int size, int position) {
    int byteIndex = position / 32;
    int bitIndex = position % 32;

    if (byteIndex >= size) {
        return 0; // Out of bounds check
    }

    return (data[byteIndex] >> bitIndex) & 1;
}

void embedPayload(Pixel* pixels, int numPixels, const int* compressedPayload, int compressedSize) {
    int totalBits = compressedSize * 32;
    int availableBits = (numPixels - 32) * 8; // Assuming first 32 pixels store metadata

    if (totalBits > availableBits) {
        fprintf(stderr, "Error: Not enough space in the image to embed the payload.\n");
        return;
    }

    // Start embedding the payload after the metadata pixels
    for (int i = 0, bitPosition = 0; i < numPixels && bitPosition < totalBits; ++i) {
        int bit = getBit(compressedPayload, compressedSize, bitPosition++);
        setLSB(&pixels[i + 32].blue, bit); // Start after metadata pixels
    }
}


void embedSize(Pixel* pixels, unsigned int size) {
    for (int i = 0; i < 32; ++i) {
        setLSB(&pixels[i].blue, (size >> i) & 1);
    }
}

int embedFileType(Pixel* pixels, const char* fileType) {
    for (int i = 0; i < 24; ++i) {
        int bit = (fileType[i / 8] >> (i % 8)) & 1;
        setLSB(&pixels[i].blue, bit);
    }
    return 0;
}

int embedPayloadInImage(const char* imageFilename, const char* outputImageFilename, const int* compressedPayload, int compressedSize, const char* payloadFilename) {
    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;
    int pixelDataSize;

    // Open the BMP file
    FILE *inputFile = fopen(imageFilename, "rb");
    if (!inputFile) {
        fprintf(stderr, "Failed to open BMP file.\n");
        return 1;
    }

    // Read the headers
    fread(&bfh, sizeof(BITMAPFILEHEADER), 1, inputFile);
    fread(&bih, sizeof(BITMAPINFOHEADER), 1, inputFile);

    // Calculate the row padding
    int padding = (4 - (bih.width * 3) % 4) % 4;

    // Calculate the total pixel data size, including padding
    pixelDataSize = (bih.width * 3 + padding) * abs(bih.height);

    Pixel *pixels = readPixelData(inputFile, bfh, bih, &pixelDataSize);
    fclose(inputFile);
    if (!pixels) {
        fprintf(stderr, "Failed to read pixel data.\n");
        return 1;
    }

    // Extract the file type from the payload filename
    const char* fileType = getFileExtension(payloadFilename); // payloadFilename needs to be accessible here

    // Check if fileType is not empty
    if (strlen(fileType) == 0) {
        fprintf(stderr, "File type extraction failed.\n");
        free(pixels);
        return 1;
    }


    // Embed file type
    if (embedFileType(pixels, fileType) != 0) {
        fprintf(stderr, "Failed to embed file type.\n");
        free(pixels);
        return 1;
    }

    int numPixels = pixelDataSize / sizeof(Pixel);
    int payloadBits = compressedSize * 32; // Payload size in bits
    int availableBits = numPixels * 8 - 32; // Available bits for payload, minus 32 for the size

    // Check if payload fits into the image
    if (payloadBits > availableBits) {
        fprintf(stderr, "Not enough space in the image for the payload.\n");
        free(pixels);
        return 1;
    }

    // Embed payload size and payload into the image
    int sizeToEmbed = payloadBits;
    embedSize(pixels, sizeToEmbed);
    embedPayload(pixels + 24, numPixels - 24, compressedPayload, compressedSize);

    // Save the modified image
    if (!saveImage(outputImageFilename, bfh, bih, (unsigned char*)pixels, pixelDataSize)) {
        fprintf(stderr, "Failed to create output image with embedded payload.\n");
        free(pixels);
        return 1;
    }

    free(pixels);
    return 0;
}



int extractAndDecompressPayload(const char* inputImageFilename, const char* outputPayloadBaseFilename) {    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;
    int pixelDataSize;
    FILE* inputFile = fopen(inputImageFilename, "rb");
    if (!inputFile) {
        fprintf(stderr, "Failed to open BMP file.\n");
        return 1;
    }
    fread(&bfh, sizeof(BITMAPFILEHEADER), 1, inputFile);
    fread(&bih, sizeof(BITMAPINFOHEADER), 1, inputFile);
    Pixel* pixels = readPixelData(inputFile, bfh, bih, &pixelDataSize);
    fclose(inputFile);
    if (!pixels) {
        fprintf(stderr, "Failed to read pixel data.\n");
        return 1;
    }


    char fileType[4]; // Assuming fileType is 3 characters + null terminator
    if (extractFileType(pixels, fileType) != 0) {
        fprintf(stderr, "Failed to extract file type.\n");
        free(pixels);
        return 1;
    }

    // Construct the correct output filename based on the extracted file type
    char outputPayloadFilename[256]; // Adjust size as necessary
    snprintf(outputPayloadFilename, sizeof(outputPayloadFilename), "%s.%s", outputPayloadBaseFilename, fileType);


    // Step 2: Extract compressed payload from pixel data
    int compressedPayloadSize;
    int* compressedPayload = extractPayload(pixels + 24, (pixelDataSize / sizeof(Pixel)) - 24, &compressedPayloadSize);
    free(pixels);
    if (!compressedPayload) {
        fprintf(stderr, "Failed to extract payload.\n");
        return 1;
    }

    // Step 3: Decompress payload
    int decompressedPayloadSize; // Variable to store the size of the decompressed data
    unsigned char* decompressedPayload = lzwDecompress(compressedPayload, compressedPayloadSize, &decompressedPayloadSize); // Modified call
    free(compressedPayload);
    if (!decompressedPayload) {
        fprintf(stderr, "Failed to decompress payload.\n");
        return 1;
    }

    // Step 4: Save the decompressed payload to a file
    FILE* outputFile = fopen(outputPayloadFilename, "wb");
    if (!outputFile) {
        fprintf(stderr, "Failed to open output file for decompressed payload.\n");
        free(decompressedPayload);
        return 1;
    }
    fwrite(decompressedPayload, 1, decompressedPayloadSize, outputFile);
    fclose(outputFile);
    free(decompressedPayload);

    return 0; // Success
}

int extractFileType(Pixel* pixels, char* fileType) {
    // Extract a 3-character file type from the first 24 pixels
    for (int i = 0; i < 24; ++i) {
        int bit = pixels[i].blue & 1;
        fileType[i / 8] |= (bit << (i % 8));
    }
    fileType[3] = '\0'; // Null-terminate the string
    return 0;
}

const char* getFileExtension(const char* filename) {
    const char* dot = strrchr(filename, '.');
    if (!dot || dot == filename) return ""; // No extension found
    return dot + 1; // Skip the dot
}

// Function to load the original compressed payload from a file
int* loadOriginalCompressedPayload(const char* filename, int* size) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening original compressed payload file");
        return NULL;
    }

    // Get the file size
    fseek(file, 0, SEEK_END);
    *size = ftell(file) / sizeof(int);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the original compressed data
    int* data = (int*)malloc(*size * sizeof(int));
    if (data == NULL) {
        fprintf(stderr, "Memory allocation failed for original compressed payload.\n");
        fclose(file);
        return NULL;
    }

    // Load the data
    fread(data, sizeof(int), *size, file);
    fclose(file);
    return data;
}

// Function to compare two blocks of memory
int comparePayloads(const int* payload1, const int* payload2, int size) {
    for (int i = 0; i < size; i++) {
        if (payload1[i] != payload2[i]) {
            printf("Mismatch at index %d: %d != %d\n", i, payload1[i], payload2[i]);
            return 0; // Return 0 if mismatch
        }
    }
    return 1; // Return 1 if match
}

unsigned char* readBinaryPayloadData(const char* filename, int* size) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open payload file.\n");
        return NULL;
    }

    // Determine the file size
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the payload
    unsigned char* data = (unsigned char*)malloc(*size);
    if (!data) {
        fprintf(stderr, "Memory allocation failed for payload.\n");
        fclose(file);
        return NULL;
    }

    // Read the payload into the buffer
    fread(data, 1, *size, file);
    fclose(file);
    return data;
}

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