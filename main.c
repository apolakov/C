#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "function.h"

#define TABLE_SIZE 16384 // Size of the dictionary used in LZW compression
#define MAX_CHAR 256 // Maximum number of characters (ASCII range)

void embedSize(unsigned char* pixelData, int size);


int main() {
    const char *inputFilename = "../bmp24.bmp";
    const char *outputFilename = "../output.bmp";
    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;
    int pixelDataSize;

    // Open and read the original BMP file
    FILE *inputFile = fopen(inputFilename, "rb");
    if (!inputFile) {
        fprintf(stderr, "Failed to open input BMP file.\n");
        return 1;
    }
    fread(&bfh, sizeof(BITMAPFILEHEADER), 1, inputFile);
    fread(&bih, sizeof(BITMAPINFOHEADER), 1, inputFile);
    unsigned char *pixelData = readPixelData(inputFile, bfh, bih, &pixelDataSize);
    fclose(inputFile);

    // Embed a known payload into the pixel data
    int originalPayload[] = {0x12345678, 0x9ABCDEF0};  // Example known payload
    int compressedSize = sizeof(originalPayload) / sizeof(originalPayload[0]);

    int sizeToEmbed = compressedSize * 32; // Assuming each int is 32 bits
    embedSize(pixelData, sizeToEmbed);
    embedPayload(pixelData, pixelDataSize, originalPayload, compressedSize);

    // Save the modified pixel data to a new BMP file
    FILE *outputFile = fopen(outputFilename, "wb");
    if (!outputFile) {
        fprintf(stderr, "Failed to open output BMP file.\n");
        free(pixelData);
        return 1;
    }
    fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, outputFile);
    fwrite(&bih, sizeof(BITMAPINFOHEADER), 1, outputFile);
    fwrite(pixelData, pixelDataSize, 1, outputFile);
    fclose(outputFile);
    free(pixelData);

    // Read the new BMP file and extract its pixel data
    FILE *modifiedFile = fopen(outputFilename, "rb");
    if (!modifiedFile) {
        fprintf(stderr, "Failed to open modified BMP file.\n");
        return 1;
    }
    fread(&bfh, sizeof(BITMAPFILEHEADER), 1, modifiedFile);
    fread(&bih, sizeof(BITMAPINFOHEADER), 1, modifiedFile);
    unsigned char *modifiedPixelData = readPixelData(modifiedFile, bfh, bih, &pixelDataSize);
    fclose(modifiedFile);

    // Extract the payload from the modified pixel data
    int extractedPayloadSize;
    int* extractedPayload = extractPayload(modifiedPixelData, pixelDataSize, &extractedPayloadSize);

    // Validate the extracted payload
    if (extractedPayloadSize != compressedSize) {
        printf("extractedPayloadSize %d \n", extractedPayloadSize);
        printf("compressedSize %d \n", compressedSize);
        fprintf(stderr, "Error: Extracted payload size does not match the original.\n");
    } else {
        int validPayload = 1;
        for (int i = 0; i < compressedSize; i++) {
            if (originalPayload[i] != extractedPayload[i]) {
                fprintf(stderr, "Error: Extracted payload does not match the original at index %d.\n", i);
                validPayload = 0;
                break;
            }
        }
        if (validPayload) {
            printf("Success: Extracted payload matches the original payload.\n");
        }
    }

    free(modifiedPixelData);
    free(extractedPayload);

    return 0;
}




unsigned int extractSizeFromPixelData(unsigned char* pixelData) {
    unsigned int size = 0;
    int bitIndex = 0;
    for (int i = 2; i < 96; i += 3) {
        unsigned int bit = pixelData[i] & 1;
        printf("Byte %d (value %02x), Extracted bit: %u, Current size: %u\n", i, pixelData[i], bit, size);
        if (bitIndex < 32) {
            size |= (bit << bitIndex);
        }
        bitIndex++;
        if (bitIndex >= 32) {
            break;
        }
    }
    printf("Final extracted size: %u\n", size);
    return size;
}



int* extractPayload(unsigned char* pixelData, int pixelDataSize, int* compressedPayloadSize) {
    printf("PixelSize in the beginning: %d\n", pixelDataSize); // 260064
    unsigned int payloadBitSize = extractSizeFromPixelData(pixelData);
    printf("Extracted payload bit size: %u\n", payloadBitSize);
    if (payloadBitSize <= 0 || payloadBitSize > (pixelDataSize * 8)) {
        fprintf(stderr, "Error: Invalid payload bit size extracted.\n");
        return NULL;
    }
    *compressedPayloadSize = (payloadBitSize + 31) / 32;

    int* payload = (int*)malloc(*compressedPayloadSize * sizeof(int));
    if (!payload) {
        fprintf(stderr, "Memory allocation failed for payload extraction.\n");
        return NULL;
    }

    memset(payload, 0, *compressedPayloadSize * sizeof(int));

    int payloadIndex = 0;

    for (int i = 96; i < pixelDataSize * 8 && payloadIndex < *compressedPayloadSize * 32; i++) {
        int pixelIndex = (i - 96) / 8; // Calculate the index of the pixel
        int bit = (pixelData[pixelIndex * 3 + 2] & 1); // Extract LSB from the blue channel
        payload[payloadIndex / 32] |= (bit << (payloadIndex % 32));
        payloadIndex++;
    }

    printf("PixelSize in the end of the extract payload function: %d\n", pixelDataSize);

    return payload;
}



// Function to read pixel data from a BMP file
unsigned char* readPixelData(FILE* file, BITMAPFILEHEADER bfh, BITMAPINFOHEADER bih, int* pixelDataSize) {
    // Calculate the padding for each row
    int padding = (4 - (bih.width * 3) % 4) % 4;

    // Calculate the total size of the pixel data including padding
    *pixelDataSize = (bih.width * 3 + padding) * abs(bih.height);

    // Allocate memory for pixel data
    unsigned char *pixelData = malloc(*pixelDataSize);
    if (!pixelData) {
        fprintf(stderr, "Memory allocation failed for pixel data.\n");
        fclose(file);
        return NULL;
    }

    // Read the pixel data from the file
    fseek(file, bfh.offset, SEEK_SET); // Go to the start of the pixel data
    for (int i = 0; i < abs(bih.height); i++) {
        // Calculate the position to read into
        unsigned char* rowData = pixelData + (bih.width * 3 + padding) * i;

        // Read one row of pixels at a time
        size_t bytesRead = fread(rowData, 3, bih.width, file);
        if (bytesRead != bih.width) {
            fprintf(stderr, "Failed to read pixel data.\n");
            free(pixelData);
            fclose(file);
            return NULL;
        }
        fseek(file, padding, SEEK_CUR); // Skip the padding
    }

    return pixelData;
}

void setLSB(unsigned char* byte, int bitValue) {
    if (bitValue == 0) {
        *byte &= 0xFE; // Clear the LSB (1111 1110)
    } else {
        *byte |= 0x01; // Set the LSB (0000 0001)
    }
}

int getBit(const int* data, int size, int position) {
    int byteIndex = position / 32; // There are 32 bits in an int
    int bitIndex = position % 32;
    if (byteIndex < size) {
        return (data[byteIndex] >> bitIndex) & 1;
    }
    return 0; // Return 0 if position is outside the data size
}

void embedPayload(unsigned char* pixelData, int pixelDataSize, const int* compressedPayload, int compressedSize) {
    int totalBits = compressedSize * 32;  // Total bits in the compressed payload
    int availableBits = pixelDataSize / 3;  // Only one out of every three bytes (blue channel)

    printf("Total bits in payload: %d\n", totalBits);
    printf("Available bits in image: %d\n", availableBits);


    if (totalBits > availableBits) {
        fprintf(stderr, "Error: Not enough space in the image to embed the payload.\n");
        return;  // Insufficient space
    }

    int bitPosition = 0;

    for (int i = 2; i < pixelDataSize && bitPosition < totalBits; i += 3) {
        int bit = getBit(compressedPayload, compressedSize, bitPosition++);
        setLSB(&pixelData[i], bit); // Modify the LSB of the blue channel
    }
    printf("Embedded Data (first 32 bytes):\n");
    for (int i = 0; i < 32; ++i) {
        printf("%02x ", pixelData[i]);
    }
    printf("\n");
    printf("Embedded size: %d\n", pixelDataSize); // After embedding the size

}

void embedSize(unsigned char* pixelData, int size) {
    for (int i = 0; i < 32; i++) {
        int bit = (size >> i) & 1;
        if (bit == 1) {
            pixelData[i] = pixelData[i] | 1;  // Set the LSB
        } else {
            pixelData[i] = pixelData[i] & (~1); // Clear the LSB
        }
    }
    printf("Size being embedded: %d\n", size);
}