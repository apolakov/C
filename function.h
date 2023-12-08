#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define TABLE_SIZE 16384
#define MAX_CHAR 256

typedef struct {
    unsigned char *bytes;  // Byte array
    int length;            // Length of the byte array
    int code;              // Corresponding code
} DictionaryEntry;



#pragma pack(push, 1)
typedef struct {
    unsigned short type;              // Magic identifier: 0x4d42
    unsigned int size;                // File size in bytes
    unsigned short reserved1;         // Not used
    unsigned short reserved2;         // Not used
    unsigned int offset;              // Offset to image data in bytes from beginning of file
} BITMAPFILEHEADER;

typedef struct {
    unsigned int size;                // Header size in bytes
    int width,height;                 // Width and height of image
    unsigned short planes;            // Number of colour planes
    unsigned short bits;              // Bits per pixel
    unsigned int compression;         // Compression type
    unsigned int imagesize;           // Image size in bytes
    int xresolution,yresolution;      // Pixels per meter
    unsigned int ncolours;            // Number of colours
    unsigned int importantcolours;    // Important colours
} BITMAPINFOHEADER;
#pragma pack(pop)




bool is_png(FILE *file);
bool is_bmp(FILE *file);
bool is_24bit_bmp(FILE *file);
bool is_24bit_png(FILE *file);
int analyze_file_format(const char *filename);
void check_file_type(const char *filename);
void* readFile(const char* filename, size_t* length);
void initializeDictionary();
int findStringCode(char *str);
void addStringToDictionary(char *str, int code);
int hashFunction(char *str);
char* getStringFromCode(int code);
unsigned char* readPayloadData(const char* filename, int* size);


void freeDictionary();
void addBytesToDictionary(const unsigned char *bytes, int length);
int findBytesCode(const unsigned char *bytes, int length);
int* lzwCompress(const unsigned char *input, int *size);
unsigned char* lzwDecompress(const int *codes, int size);
unsigned char* getBytesFromCode(int code, int *length);
void setLSB(unsigned char* byte, int bitValue);
int getBit(const int* data, int size, int position);
void embedPayload(unsigned char* pixelData, int pixelDataSize, const int* compressedPayload, int compressedSize);
//void extractPayload(unsigned char* pixelData, int pixelDataSize, int* extractedPayload, int compressedSize);
unsigned char* readPixelData(FILE* file, BITMAPFILEHEADER bfh, BITMAPINFOHEADER bih, int* pixelDataSize);
int openAndValidateBMP(const char* filename, BITMAPFILEHEADER* bfh, BITMAPINFOHEADER* bih);
// Function prototype for reading pixel data from a BMP file
unsigned char* readPixelData(FILE* file, BITMAPFILEHEADER bfh, BITMAPINFOHEADER bih, int* pixelDataSize);
int saveImage(const char* filename, BITMAPFILEHEADER bfh, BITMAPINFOHEADER bih, unsigned char* pixelData, int pixelDataSize);
int* extractPayload(unsigned char* pixelData, int pixelDataSize, int* compressedPayloadSize);
int extractSizeFromPixelData(unsigned char* pixelData);
//void saveDecompressedPayload(const char* baseFilename, const unsigned char* decompressedPayload, int decompressedPayloadSize);
void saveDecompressedPayload(const unsigned char* decompressedPayload, int decompressedPayloadSize);

#endif