#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "function.h"

#define TABLE_SIZE 16384 // Size of the dictionary used in LZW compression
#define MAX_CHAR 256 // Maximum number of characters (ASCII range)

int nextAvailableCode = MAX_CHAR; // Variable to track the next available code in LZW compression
DictionaryEntry *table[TABLE_SIZE]; // Dictionary for LZW compression
void printLZWCompressedData(const int *compressedData, int compressedSize);


        int main() {
    const char *inputFilename = "../bmp24.bmp";
    const char *outputFilename = "../output.bmp";
    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;
    int pixelDataSize;

    // Analyze the file format to ensure it's a 24-bit BMP file
    int format = analyze_file_format(inputFilename);
    if (format != 3) { // If the file is not a 24-bit RGB BMP file
        fprintf(stderr, "The file is not a 24-bit RGB BMP file.\n");
        return 1;
    }

    // Open the file for reading pixel data
    FILE *inputFile = fopen(inputFilename, "rb");
    if (!inputFile) {
        fprintf(stderr, "Failed to open BMP file.\n");
        return 1;
    }
    // Read the headers (they have already been checked by analyze_file_format)
    fread(&bfh, sizeof(BITMAPFILEHEADER), 1, inputFile);
    fread(&bih, sizeof(BITMAPINFOHEADER), 1, inputFile);

    unsigned char *pixelData = readPixelData(inputFile, bfh, bih, &pixelDataSize);
    fclose(inputFile); // Close the file after reading pixel data

    if (!pixelData) {
        fprintf(stderr, "Failed to read pixel data from BMP file.\n");
        return 1;
    }

    const char* payloadFilename = "../png.png";

    unsigned char* payloadData;
    int payloadSize;
    payloadData = readPayloadData(payloadFilename, &payloadSize);


    int compressedSize = payloadSize; // Initialize to payload size, will be updated by lzwCompress
    int* compressedPayload = lzwCompress(payloadData, &compressedSize);

    printLZWCompressedData(* compressedPayload, 10);
    free(payloadData); // Free the original payload data as it is no longer needed

    if (pixelDataSize < compressedSize * 8) {
        fprintf(stderr, "Not enough space in the image for the payload.\n");
        return 3;
    }

    embedPayload(pixelData, pixelDataSize, compressedPayload, compressedSize);
    free(compressedPayload); // Free the compressed payload as it has been embedded

    saveImage(outputFilename, bfh, bih, pixelData, pixelDataSize);
    free(pixelData);

    FILE *modifiedFile = fopen(outputFilename, "rb");
    if (!modifiedFile) {
        fprintf(stderr, "Failed to open the modified BMP file.\n");
        return 1;
    }
    // Read the headers again from the modified file
    fread(&bfh, sizeof(BITMAPFILEHEADER), 1, modifiedFile);
    fread(&bih, sizeof(BITMAPINFOHEADER), 1, modifiedFile);

// Read the pixel data from the modified file
    unsigned char *modifiedPixelData = readPixelData(modifiedFile, bfh, bih, &pixelDataSize);
    fclose(modifiedFile);

    if (!modifiedPixelData) {
        fprintf(stderr, "Failed to read pixel data from the modified BMP file.\n");
        return 1;
    }

    int compressedPayloadSize;
    int *extractedCompressedPayload = extractPayload(modifiedPixelData, pixelDataSize, &compressedPayloadSize);
    free(modifiedPixelData);  // Free the modified pixel data after extracting the payload

    int decompressedPayloadSize;
    unsigned char* decompressedPayload = lzwDecompress(extractedCompressedPayload, compressedPayloadSize);


    saveDecompressedPayload(decompressedPayload, decompressedPayloadSize);

    // Clean up
    free(extractedCompressedPayload);

    return 0;
}

void printLZWCompressedData(const int *compressedData, int compressedSize) {
    printf("LZW Compressed Data: [");
    for (int i = 0; i < compressedSize; i++) {
        printf("%d", compressedData[i]);
        if (i < compressedSize - 1) {
            printf(", ");
        }
    }
    printf("]\n");
}

int extractSizeFromPixelData(unsigned char* pixelData) {
    // Assuming the size is stored in the first 32 bits (4 bytes)
    // Each LSB of the first 32 bytes of pixel data represents one bit of the size integer
    int size = 0;
    for (int i = 0; i < 32; ++i) {
        size |= ((pixelData[i] & 0x01) << i); // Extract the LSB and shift it to the correct position
    }
    return size;
}


int* extractPayload(unsigned char* pixelData, int pixelDataSize, int* compressedPayloadSize) {
    int payloadBitSize = extractSizeFromPixelData(pixelData);
    *compressedPayloadSize = (payloadBitSize + 31) / 32; // Calculate size in ints

    int* payload = (int*)malloc(*compressedPayloadSize * sizeof(int));
    if (!payload) {
        fprintf(stderr, "Memory allocation failed for payload extraction.\n");
        return NULL;
    }

    memset(payload, 0, *compressedPayloadSize * sizeof(int)); // Initialize payload to zeros

    int payloadIndex = 0; // Index in the payload array
    int bitPosition = 0;  // Bit position within the current integer

    // Start extracting payload bits from the 33rd byte, as we assume the first 32 bytes contain the payload size
    for (int i = 32; i < pixelDataSize && payloadIndex < *compressedPayloadSize; ++i) {
        // Extract LSB from the blue channel
        int bit = pixelData[i] & 0x01;
        payload[payloadIndex] |= (bit << bitPosition); // Set the appropriate bit in the payload

        // Update bit position and payload index
        bitPosition++;
        if (bitPosition == 32) { // Move to the next integer when we have filled one up
            bitPosition = 0;
            payloadIndex++;
        }
    }

    return payload;
}

unsigned char* readPayloadData(const char* filename, int* size) {
    FILE* file = fopen(filename, "rb"); // Open the file in binary mode
    if (!file) {
        fprintf(stderr, "Unable to open payload file.\n");
        return NULL;
    }

    // Seek to the end of the file to determine the size
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET); // Seek back to the beginning of the file

    // Allocate memory for the payload data
    unsigned char* data = (unsigned char*)malloc(*size);
    if (!data) {
        fprintf(stderr, "Memory allocation failed for payload data.\n");
        fclose(file);
        return NULL;
    }

    // Read the file into memory
    fread(data, 1, *size, file);
    fclose(file); // Close the file

    return data;
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

int saveImage(const char* filename, BITMAPFILEHEADER bfh, BITMAPINFOHEADER bih, unsigned char* pixelData, int pixelDataSize) {
    FILE* file = fopen(filename, "wb");  // Open the file in binary write mode
    if (!file) {
        fprintf(stderr, "Unable to open output file.\n");
        return 0;  // Return 0 on failure
    }

    // Write the BMP file header
    fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, file);

    // Write the BMP info header
    fwrite(&bih, sizeof(BITMAPINFOHEADER), 1, file);

    // Write the pixel data
    fwrite(pixelData, 1, pixelDataSize, file);

    fclose(file);  // Close the file

    return 1;  // Return 1 on success
}

void saveDecompressedPayload(const unsigned char* decompressedPayload, int decompressedPayloadSize) {
    if (decompressedPayloadSize < 4) {
        fprintf(stderr, "Decompressed payload is too small to contain a valid file extension.\n");
        return;
    }

    // Extract the file extension from the decompressed payload
    char fileExtension[5];
    strncpy(fileExtension, (const char*)decompressedPayload, 4);
    fileExtension[4] = '\0';  // Ensure null termination

    // Construct the filename with a fixed prefix and the extracted extension
    char filename[260];
    snprintf(filename, sizeof(filename), "result_%s", fileExtension);

    // Debug print the filename
    printf("Attempting to save to file: '%s'\n", filename);

    // Open the file for writing
    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("Error opening file");  // This will print a more descriptive error message
        return;
    }

    // Write the payload to the file, skipping the first 4 bytes which contain the file extension
    size_t written = fwrite(decompressedPayload + 4, 1, decompressedPayloadSize - 4, file);
    if (written != decompressedPayloadSize - 4) {
        fprintf(stderr, "Error writing to file '%s'. Written: %zu, Expected: %d\n", filename, written, decompressedPayloadSize - 4);
    }

    fclose(file);
}