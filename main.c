#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "function.h"

#define TABLE_SIZE 16384 // Size of the dictionary used in LZW compression
#define MAX_CHAR 256 // Maximum number of characters (ASCII range)

int nextAvailableCode = MAX_CHAR; // Variable to track the next available code in LZW compression
DictionaryEntry *table[TABLE_SIZE]; // Dictionary for LZW compression

// Function prototype for reading pixel data from a BMP file
unsigned char* readPixelData(FILE* file, BITMAPFILEHEADER bfh, BITMAPINFOHEADER bih, int* pixelDataSize);

int main() {
    // Bitmap file and info headers
    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;

    // Open the BMP file
    FILE *file = fopen("../bmp24.bmp", "rb");
    if (!file) {
        fprintf(stderr, "Unable to open file.\n");
        return 1;
    }

    // Read the bitmap file and info headers
    fread(&bfh, sizeof(BITMAPFILEHEADER), 1, file);
    fread(&bih, sizeof(BITMAPINFOHEADER), 1, file);

    // Ensure this is a BMP file
    if (bfh.type != 0x4D42) {
        fprintf(stderr, "Not a BMP file.\n");
        fclose(file);
        return 1;
    }

    // Print image dimensions
    printf("Width: %d\n", bih.width);
    printf("Height: %d\n", bih.height);

    // Read pixel data from the BMP file
    int pixelDataSize;
    unsigned char *pixelData = readPixelData(file, bfh, bih, &pixelDataSize);
    if (!pixelData) {
        // Handle error...
        return 1;
    }

    // Open the payload file (the file to be embedded in the image)
    FILE *payloadFile = fopen("../input.png", "rb");
    if (!payloadFile) {
        fprintf(stderr, "Failed to open payload file.\n");
        return 1;
    }

    // Determine the size of the payload
    fseek(payloadFile, 0, SEEK_END);
    long payloadSize = ftell(payloadFile);
    fseek(payloadFile, 0, SEEK_SET);

    // Allocate memory for the payload
    unsigned char *payload = malloc(payloadSize);
    if (!payload) {
        fprintf(stderr, "Memory allocation failed for payload.\n");
        fclose(payloadFile);
        return 1;
    }

    // Read the payload into memory
    fread(payload, 1, payloadSize, payloadFile);

    // Compress the payload using LZW algorithm
    int compressedSize = payloadSize;
    int *compressedPayload = lzwCompress(payload, &compressedSize);
    if (!compressedPayload) {
        fprintf(stderr, "Payload compression failed.\n");
        free(payload);
        return 1;
    }

    // Calculate the number of pixels and bits required for embedding the payload
    int numberOfPixels = bih.width * abs(bih.height);
    int bitsNeeded = compressedSize * sizeof(int) * 8;
    int bitsAvailable = numberOfPixels * 24;

    // Check if the image can store the compressed payload
    if (bitsAvailable < bitsNeeded) {
        fprintf(stderr, "Not enough pixels to store the payload.\n");
        free(payload);
        free(compressedPayload);
        return 1;
    }

    // Embed the compressed payload into the pixel data
    embedPayload(pixelData, pixelDataSize, compressedPayload, compressedSize);

    // Open a new file to write the modified image with the embedded payload
    FILE *outputFile = fopen("../output.bmp", "wb");
    if (!outputFile) {
        fprintf(stderr, "Unable to open file for writing.\n");
        free(pixelData);
        free(compressedPayload);
        return 1;
    }

    // Write the modified pixel data with the embedded payload to the new file
    fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, outputFile);
    fwrite(&bih, sizeof(BITMAPINFOHEADER), 1, outputFile);
    fwrite(pixelData, 1, pixelDataSize, outputFile);

    // Close the output file
    fclose(outputFile);

    // Open the file with the embedded payload for reading
    FILE *modifiedFile = fopen("../output.bmp", "rb");
    if (!modifiedFile) {
        fprintf(stderr, "Unable to open modified file.\n");
        return 1;
    }

    // Read the modified pixel data
    fread(&bfh, sizeof(BITMAPFILEHEADER), 1, modifiedFile);
    fread(&bih, sizeof(BITMAPINFOHEADER), 1, modifiedFile);
    unsigned char *modifiedPixelData = readPixelData(modifiedFile, bfh, bih, &pixelDataSize);
    if (!modifiedPixelData) {
        fprintf(stderr, "Failed to read pixel data from modified file.\n");
        fclose(modifiedFile);
        return 1;
    }

    // Allocate memory for the extracted payload
    int *extractedPayload = malloc(compressedSize * sizeof(int));
    if (!extractedPayload) {
        fprintf(stderr, "Memory allocation failed for extracted payload.\n");
        fclose(modifiedFile);
        free(modifiedPixelData);
        return 1;
    }

    // Extract the payload from the modified image
    extractPayload(modifiedPixelData, pixelDataSize, extractedPayload, compressedSize);

    // Decompress the extracted payload
    unsigned char *decompressedData = lzwDecompress(extractedPayload, compressedSize);
    if (!decompressedData) {
        fprintf(stderr, "Decompression failed.\n");
        fclose(modifiedFile);
        free(modifiedPixelData);
        free(extractedPayload);
        return 1;
    }

    // Save the decompressed data to a file
    FILE *decompressedFile = fopen("../decompressed_output.dat", "wb");
    if (!decompressedFile) {
        fprintf(stderr, "Unable to open file for writing decompressed data.\n");
        fclose(modifiedFile);
        free(modifiedPixelData);
        free(extractedPayload);
        free(decompressedData);
        return 1;
    }

    fwrite(decompressedData, 1, compressedSize, decompressedFile);
    fclose(decompressedFile);

    // Clean up resources
    fclose(modifiedFile);
    free(modifiedPixelData);
    free(extractedPayload);
    free(decompressedData);

    return 0;
}

// Function to open a BMP file and validate its format
int openAndValidateBMP(const char* filename, BITMAPFILEHEADER* bfh, BITMAPINFOHEADER* bih) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Unable to open file %s.\n", filename);
        return 0;
    }

    fread(bfh, sizeof(BITMAPFILEHEADER), 1, file);
    fread(bih, sizeof(BITMAPINFOHEADER), 1, file);

    if (bfh->type != 0x4D42) {
        fprintf(stderr, "Not a BMP file.\n");
        fclose(file);
        return 0;
    }

    printf("Width: %d\n", bih->width);
    printf("Height: %d\n", bih->height);

    return (int)(long)file;  // Casting FILE* to int for simplicity
}

// Function to initialize the dictionary for LZW compression
void initializeDictionary() {
    for (int i = 0; i < MAX_CHAR; i++) {
        table[i] = (DictionaryEntry *)malloc(sizeof(DictionaryEntry));
        if (!table[i]) {
            fprintf(stderr, "Memory allocation failed for dictionary entry %d.\n", i);
            exit(EXIT_FAILURE);
        }
        table[i]->bytes = (unsigned char *)malloc(sizeof(unsigned char)); // one byte for one char
        if (!table[i]->bytes) {
            fprintf(stderr, "Memory allocation failed for dictionary bytes %d.\n", i);
            free(table[i]);
            exit(EXIT_FAILURE);
        }
        table[i]->bytes[0] = (unsigned char)i; // Cast is not needed, but clarifies the intention
        table[i]->length = 1; // one byte
        table[i]->code = i;
    }
    nextAvailableCode = MAX_CHAR; // Start adding new byte arrays from here
}

// Function to free the dictionary used in LZW compression
void freeDictionary() {
    for (int i = 0; i < TABLE_SIZE; i++) {
        if (table[i]) {
            free(table[i]->bytes); // Free the byte array
            free(table[i]); // Then free the struct
            table[i] = NULL; // Prevent dangling pointer
        }
    }
}

// Function to add new bytes to the dictionary for LZW compression
void addBytesToDictionary(const unsigned char *bytes, int length) {
    if (nextAvailableCode >= TABLE_SIZE) {
        fprintf(stderr, "Dictionary is full.\n");
        exit(EXIT_FAILURE);
    }

    table[nextAvailableCode] = (DictionaryEntry *)malloc(sizeof(DictionaryEntry));
    if (!table[nextAvailableCode]) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }

    table[nextAvailableCode]->bytes = (unsigned char *)malloc(length);
    if (!table[nextAvailableCode]->bytes) {
        fprintf(stderr, "Memory allocation failed for dictionary bytes.\n");
        free(table[nextAvailableCode]);
        exit(EXIT_FAILURE);
    }

    memcpy(table[nextAvailableCode]->bytes, bytes, length);
    table[nextAvailableCode]->length = length;
    table[nextAvailableCode]->code = nextAvailableCode;
    nextAvailableCode++;
}

// Function to find the code for a given byte sequence in the dictionary
int findBytesCode(const unsigned char *bytes, int length) {
    for (int i = 0; i < nextAvailableCode; i++) {
        if (table[i] && table[i]->length == length && memcmp(table[i]->bytes, bytes, length) == 0) {
            return table[i]->code;
        }
    }
    return -1; // Byte array not found
}

// Function to compress data using LZW algorithm
int* lzwCompress(const unsigned char *input, int *size) {
    // Initialize the dictionary for compression
    initializeDictionary();

    unsigned char currentString[3] = {0}; // Buffer for current string
    int currentLength = 0; // Current length of the string
    int *output = (int *)malloc(TABLE_SIZE * sizeof(int)); // Allocate memory for compressed output
    if (!output) {
        fprintf(stderr, "Memory allocation failed for output.\n");
        exit(EXIT_FAILURE);
    }
    int outputSize = 0; // Variable to track the size of the output

    // Main compression loop
    for (size_t i = 0; i < *size; i++) {
        unsigned char nextString[4] = {0}; // Buffer for next string
        memcpy(nextString, currentString, currentLength); // Copy current string to next string
        nextString[currentLength] = input[i]; // Append the next character
        int nextLength = currentLength + 1; // Increase the length for next string

        int code = findBytesCode(nextString, nextLength);
        if (code != -1) {
            // String found in dictionary, update current string
            memcpy(currentString, nextString, nextLength);
            currentLength = nextLength;
        } else {
            // String not found, output code for current string and add new string to dictionary
            output[outputSize++] = findBytesCode(currentString, currentLength);
            addBytesToDictionary(nextString, nextLength);
            // Reset currentString
            currentString[0] = input[i];
            currentLength = 1;
        }
    }

    // Handle the last string
    if (currentLength > 0) {
        output[outputSize++] = findBytesCode(currentString, currentLength);
    }

    *size = outputSize; // Update the size of the compressed data
    int* temp = realloc(output, outputSize * sizeof(int)); // Reallocate memory to fit the output
    if (temp == NULL) {
        free(output);
        fprintf(stderr, "Memory reallocation failed for output.\n");
        exit(EXIT_FAILURE);
    }
    return temp; // Return the compressed data
}

// Function to decompress data using LZW algorithm
unsigned char* lzwDecompress(const int *codes, int size) {
    // Initialize the dictionary for decompression
    initializeDictionary();

    int bufferSize = TABLE_SIZE; // Initial buffer size for decompressed data
    char *decompressed = (char *)malloc(bufferSize * sizeof(char)); // Allocate memory for decompressed data
    if (decompressed == NULL) {
        fprintf(stderr, "Failed to allocate memory for decompression.\n");
        exit(EXIT_FAILURE);
    }
    decompressed[0] = '\0'; // Null-terminate the decompressed string

    int prevCode = codes[0]; // Previous code
    char prevString[MAX_CHAR] = {0}; // Buffer for previous string
    int length; // Variable to store the length of the byte sequence
    unsigned char* byteData = getBytesFromCode(prevCode, &length); // Get byte sequence from code
    memcpy(prevString, byteData, length); // Copy byte sequence to previous string
    prevString[length] = '\0'; // Null-terminate the string
    strncat(decompressed, prevString, bufferSize - strlen(decompressed) - 1); // Append to decompressed data

    // Main decompression loop
    for (int i = 1; i < size; i++) {
        int currentCode = codes[i]; // Current code
        char currentString[MAX_CHAR] = {0}; // Buffer for current string

        unsigned char* byteData = getBytesFromCode(currentCode, &length); // Get byte sequence from code
        if (byteData != NULL) {
            memcpy(currentString, byteData, length); // Copy byte sequence to current string
            currentString[length] = '\0'; // Null-terminate the string
        } else {
            // Special case: code not found in dictionary
            strncpy(currentString, prevString, MAX_CHAR - 1); // Copy previous string to current string
            currentString[strlen(currentString) + 1] = '\0'; // Null-terminate the string
            currentString[strlen(currentString)] = prevString[0]; // Append the first character of previous string
        }

        // Check buffer size and expand if necessary
        if (strlen(decompressed) + strlen(currentString) + 1 > bufferSize) {
            bufferSize *= 2; // Double the buffer size
            char *temp = realloc(decompressed, bufferSize * sizeof(char)); // Reallocate memory for decompressed data
            if (temp == NULL) {
                free(decompressed);
                fprintf(stderr, "Failed to reallocate memory for decompression.\n");
                exit(EXIT_FAILURE);
            }
            decompressed = temp;
        }

        strncat(decompressed, currentString, bufferSize - strlen(decompressed) - 1); // Append current string to decompressed data

        // Add new entry to dictionary
        char newEntry[MAX_CHAR] = {0}; // Buffer for new dictionary entry
        snprintf(newEntry, sizeof(newEntry), "%s%c", prevString, currentString[0]); // Create new entry combining previous and current strings
        addBytesToDictionary((unsigned char *)newEntry, strlen(newEntry)); // Add new entry to dictionary

        strcpy(prevString, currentString); // Update previous string for next iteration
    }

    return (unsigned char*)decompressed; // Return the decompressed data
}

unsigned char* getBytesFromCode(int code, int *length) {
    if (code >= 0 && code < nextAvailableCode && table[code]) {
        // Set the length to the length of the byte array in the dictionary entry
        *length = table[code]->length;
        // Return a pointer to the byte array
        return table[code]->bytes;
    }
    // If the code is not found, set length to 0 to indicate no data
    *length = 0;
    return NULL;
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
        // Read one row of pixels at a time
        if (fread(pixelData + (bih.width * 3 + padding) * i, 3, bih.width, file) != bih.width) {
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
    int totalBits = compressedSize * 32; // 32 bits per int
    int bitPosition = 0;

    for (int i = 0; i < pixelDataSize && bitPosition < totalBits; ++i) {
        // Get the next bit from the compressed payload
        int bit = getBit(compressedPayload, compressedSize, bitPosition++);

        // Embed the bit into the LSB of the current byte of the pixel data
        setLSB(&pixelData[i], bit);
    }

    // Handle potential overflow
    if (bitPosition != totalBits) {
        fprintf(stderr, "Error: Not enough space in the image to embed the payload.\n");
        // Handle the error, such as aborting the operation
    }
}

void extractPayload(unsigned char* pixelData, int pixelDataSize, int* extractedPayload, int compressedSize) {
    int totalBits = compressedSize * 32; // 32 bits per int
    int bitPosition = 0;

    for (int i = 0; i < pixelDataSize && bitPosition < totalBits; ++i) {
        // Extract the bit from the LSB of the current byte of the pixel data
        int bit = pixelData[i] & 1;

        // Place the bit in the correct position in the extracted payload
        int byteIndex = bitPosition / 32;
        int bitIndex = bitPosition % 32;

        if (bitIndex == 0) {
            extractedPayload[byteIndex] = 0; // Initialize to zero before setting bits
        }

        extractedPayload[byteIndex] |= (bit << bitIndex);

        bitPosition++;
    }

    // Handle potential underflow or errors
    if (bitPosition != totalBits) {
        fprintf(stderr, "Error: Not all payload bits were successfully extracted.\n");
        // Handle the error, such as aborting the operation
    }
}