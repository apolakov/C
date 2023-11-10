#include <stdio.h>
#include <string.h>
#include "function.h"
#include <stdlib.h>

typedef struct {
    char *str;  // String
    int code;   // Corresponding code
} DictionaryEntry;


int main() {
    const char *filename = "../bmp24.bmp";
    const char* inputFilename = "input.file";

    //analyze_file_format(filename);

    size_t length;
    void* data = readFile("../input.png", &length);

    if (data) {


        printf(data);
        printf("Compressed file length: %zu bytes\n", length);
        free(data);
    }

    //png24 = 1, png = 2, bmp24 = 3, png24=4

    return 0;
}

void* readFile(const char* filename, size_t* length) {
    // Open the file in binary read mode.
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("File opening failed");
        return NULL;
    }

    // Determine the file size.
    fseek(file, 0, SEEK_END);
    *length = ftell(file); // The length of the data is set here.
    fseek(file, 0, SEEK_SET);

    // Allocate memory to store the file's contents.
    void* data = malloc(*length);
    if (!data) {
        perror("Memory allocation failed");
        fclose(file);
        return NULL;
    }

    // Read the file's contents into memory.
    if (fread(data, 1, *length, file) < *length) {
        perror("File read failed");
        free(data);
        fclose(file);
        return NULL;
    }

    // Close the file and return the data.
    fclose(file);
    return data;
}