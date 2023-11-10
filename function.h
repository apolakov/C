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

#endif