#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "function.h"
#include <stdlib.h>
int nextAvailableCode = MAX_CHAR; // Variable to track the next available code in LZW compression
DictionaryEntry *table[TABLE_SIZE]; // Dictionary for LZW compression



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



// Function to find the code for a given byte sequence in the dictionary
int findBytesCode(const unsigned char *bytes, int length) {
    for (int i = 0; i < nextAvailableCode; i++) {
        if (table[i] && table[i]->length == length && memcmp(table[i]->bytes, bytes, length) == 0) {
            return table[i]->code;
        }
    }
    return -1; // Byte array not found
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

// Function to compress data using LZW algorithm
int* lzwCompress(const unsigned char *input, int *size) {
    // Initialize the dictionary for compression
    initializeDictionary();

    unsigned char currentString[MAX_CHAR + 1] = {0}; // Buffer for current string, increased size
    int currentLength = 0; // Current length of the string
    int *output = (int *)malloc(TABLE_SIZE * sizeof(int)); // Allocate memory for compressed output
    if (!output) {
        fprintf(stderr, "Memory allocation failed for output.\n");
        exit(EXIT_FAILURE);
    }
    int outputSize = 0; // Variable to track the size of the output

    // Main compression loop
    for (size_t i = 0; i < *size; i++) {
        unsigned char nextString[MAX_CHAR + 1] = {0}; // Buffer for next string, increased size
        memcpy(nextString, currentString, currentLength); // Copy current string to next string
        nextString[currentLength] = input[i]; // Append the next character
        int nextLength = currentLength + 1; // Increase the length for next string

        int code = findBytesCode(nextString, nextLength);
        if (code != -1) {
            // String found in dictionary, update current string
            memcpy(currentString, nextString, nextLength);
            currentLength = nextLength;
        } else {
            // String not found, output code for current string
            int currentStringCode = findBytesCode(currentString, currentLength);
            if (currentStringCode == -1) {
                fprintf(stderr, "Error: Current string not found in dictionary.\n");
                free(output);
                exit(EXIT_FAILURE);
            }
            output[outputSize++] = currentStringCode;

            // Add new string to dictionary
            if (nextAvailableCode < TABLE_SIZE) {
                addBytesToDictionary(nextString, nextLength);
            }

            // Reset currentString
            currentString[0] = input[i];
            currentLength = 1;
        }
    }

    // Handle the last string
    if (currentLength > 0) {
        int currentStringCode = findBytesCode(currentString, currentLength);
        if (currentStringCode == -1) {
            fprintf(stderr, "Error: Current string not found in dictionary.\n");
            free(output);
            exit(EXIT_FAILURE);
        }
        output[outputSize++] = currentStringCode;
    }

    *size = outputSize; // Update the size of the compressed data
    int* temp = realloc(output, outputSize * sizeof(int)); // Reallocate memory to fit the output
    if (temp == NULL) {
        free(output);
        fprintf(stderr, "Memory reallocation failed for output.\n");
        exit(EXIT_FAILURE);
    }

    /*
    // Print compressed data for debugging
    printf("LZW Compressed Data: ");
    for (int i = 0; i < outputSize; i++) {
        printf("%d ", temp[i]);
    }
    printf("\n");
     */

    return temp; // Return the compressed data
}



/*
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
 */

/*
// Function to open a BMP file and validate its format
FILE* openAndValidateBMP(const char* filename, BITMAPFILEHEADER* bfh, BITMAPINFOHEADER* bih) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Unable to open file %s.\n", filename);
        return NULL;
    }

    fread(bfh, sizeof(BITMAPFILEHEADER), 1, file);
    fread(bih, sizeof(BITMAPINFOHEADER), 1, file);

    if (bfh->bfType != 0x4D42) {  // 'BM' in little-endian
        fprintf(stderr, "Not a BMP file.\n");
        fclose(file);
        return NULL;
    }

    printf("Width: %d\n", bih->width);
    printf("Height: %d\n", bih->height);

    return file;
}
 int compareFiles(const char *file1, const char *file2) {
    FILE *fp1 = fopen(file1, "rb");
    FILE *fp2 = fopen(file2, "rb");

    if (fp1 == NULL || fp2 == NULL) {
        fprintf(stderr, "File open error.\n");
        exit(EXIT_FAILURE);
    }

    int ch1, ch2;
    int flag = 0; // File are identical

    while (((ch1 = fgetc(fp1)) != EOF) && ((ch2 = fgetc(fp2)) != EOF)) {
        if (ch1 != ch2) {
            flag = 1; // Files are not identical
            break;
        }
    }

    fclose(fp1);
    fclose(fp2);
    return flag;
}

  */

void saveDecompressedPayload(const unsigned char* decompressedPayload, int decompressedPayloadSize) {
    if (decompressedPayloadSize < 4) {
        fprintf(stderr, "Decompressed payload is too small to contain a valid file extension.\n");
        return;
    }

    char fileExtension[5];
    strncpy(fileExtension, (const char*)decompressedPayload, 4);
    fileExtension[4] = '\0';

    printf("Extracted file extension: %s\n", fileExtension);

    // Hardcoded filename for testing
    FILE* file = fopen("../test.png", "wb");
    if (!file) {
        perror("Error opening hardcoded file");
        return;
    }


    printf("First few bytes of payload data: ");
    for (int i = 0; i < 15 && i < decompressedPayloadSize - 4; ++i) {
        printf("%02x ", decompressedPayload[i + 4]);
    }
    printf("\n");

    printf("Size of decompressed payload: %d\n", decompressedPayloadSize - 4);


    size_t written = fwrite(decompressedPayload + 4, 1, decompressedPayloadSize - 4, file);
    if (written != decompressedPayloadSize - 4) {
        fprintf(stderr, "Error writing to hardcoded file 'test.png'. Written: %zu, Expected: %d\n", written, decompressedPayloadSize - 4);
    }

    fclose(file);
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

    int padding = (4 - (bih.width * 3) % 4) % 4;
    unsigned char pad[3] = {0};

    for (int i = 0; i < abs(bih.height); i++) {
        // Calculate the position to write from
        unsigned char* rowData = pixelData + (bih.width * 3 + padding) * i;

        // Write one row of pixels at a time
        size_t bytesWritten = fwrite(rowData, 3, bih.width, file);
        if (bytesWritten != bih.width) {
            fprintf(stderr, "Failed to write pixel data.\n");
            fclose(file);
            return 0;
        }

        // Write the padding
        fwrite(pad, 1, padding, file);
    }

    fclose(file);
    return 1;
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


//MAIN I AM NOT USING RIGHT NOW
/*
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

    FILE* compressedPayloadFile = fopen("../compressed_payload.bin", "wb");
    if (!compressedPayloadFile) {
        perror("Failed to open file to save compressed payload");
        return 1;
    }
    fwrite(compressedPayload, sizeof(int), compressedSize, compressedPayloadFile);
    fclose(compressedPayloadFile);

    //printLZWCompressedData(* compressedPayload, 10);
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

    // Save the decompressed payload to a file
    FILE* decompressedPayloadFile = fopen("../decompressed_payload.bin", "wb");
    if (!decompressedPayloadFile) {
        perror("Failed to open file to save decompressed payload");
        return 1;
    }
    fwrite(decompressedPayload, 1, decompressedPayloadSize, decompressedPayloadFile);
    fclose(decompressedPayloadFile);

    if (compareFiles("../compressed_payload.bin", "../decompressed_payload.bin") == 0) {
        printf("Success: The embedded and extracted payloads are identical.\n");
    } else {
        printf("Error: The payloads do not match!\n");
    }

    // Clean up
    free(extractedCompressedPayload);
    */