#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

bool is_png(FILE *file);
bool is_bmp(FILE *file);
bool is_24bit_bmp(FILE *file);
bool is_24bit_png(FILE *file);
int analyze_file_format(const char *filename);
void check_file_type(const char *filename);
void* readFile(const char* filename, size_t* length);
void initializeDictionary();
char* lzwDecompress(int *codes, int size);
int* lzwCompress(char *input, int *size);
int findStringCode(char *str);
void addStringToDictionary(char *str, int code);
int hashFunction(char *str);
char* getStringFromCode(int code);

#endif