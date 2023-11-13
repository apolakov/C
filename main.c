#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABLE_SIZE 4096
#define MAX_CHAR 256
int nextAvailableCode = MAX_CHAR;

typedef struct {
    char *str;  // String
    int code;   // Corresponding code
} DictionaryEntry;

DictionaryEntry *table[TABLE_SIZE];

void initializeDictionary();
void freeDictionary();
void addStringToDictionary(const char *str);
int findStringCode(const char *str);
int* lzwCompress(const char *input, int *size);
char* lzwDecompress(const int *codes, int size);
char* getStringFromCode(int code);

int main() {
    char *input = "this is a test";
    int compressedSize;

    int *compressed = lzwCompress(input, &compressedSize);
    printf("Compressed Data:\n");
    for (int i = 0; i < compressedSize; i++) {
        printf("%d ", compressed[i]);
    }
    printf("\n");

/*
    for (int i = 0; i < TABLE_SIZE; i++) {
        if (table[i] != NULL) {
            printf("%d: %s -> %d\n", i, table[i]->str, table[i]->code);
        }
    }
    */


    char *decompressed = lzwDecompress(compressed, compressedSize);
    printf("Decompressed Data: %s\n", decompressed);


    free(compressed);
    free(decompressed);
    freeDictionary();

    return 0;
}

void initializeDictionary() {
    for (int i = 0; i < MAX_CHAR; i++) {
        table[i] = (DictionaryEntry *)malloc(sizeof(DictionaryEntry));
        if (!table[i]) {
            fprintf(stderr, "Memory allocation failed for ASCII character %d.\n", i);
            exit(EXIT_FAILURE);
        }
        table[i]->str = (char *)malloc(2 * sizeof(char)); // Space for character and null-terminator
        if (!table[i]->str) {
            fprintf(stderr, "String allocation failed for ASCII character %d.\n", i);
            free(table[i]);
            exit(EXIT_FAILURE);
        }
        table[i]->str[0] = (char)i;
        table[i]->str[1] = '\0';
        table[i]->code = i;
    }
    nextAvailableCode = MAX_CHAR; // Start adding new strings from here
}

void freeDictionary() {
    for (int i = 0; i < TABLE_SIZE; i++) {

        if (table[i]) {
            free(table[i]->str);
            free(table[i]);
        }
    }
}



void addStringToDictionary(const char *str) {
    if (nextAvailableCode < TABLE_SIZE) {
        table[nextAvailableCode] = (DictionaryEntry *)malloc(sizeof(DictionaryEntry));
        if (!table[nextAvailableCode]) {
            fprintf(stderr, "Memory allocation failed.\n");
            exit(EXIT_FAILURE);
        }
        table[nextAvailableCode]->str = strdup(str);
        if (!table[nextAvailableCode]->str) {
            fprintf(stderr, "String duplication failed.\n");
            free(table[nextAvailableCode]);
            exit(EXIT_FAILURE);
        }
        table[nextAvailableCode]->code = nextAvailableCode;
        nextAvailableCode++;
    }
}

int findStringCode(const char *str) {
    for (int i = 0; i < nextAvailableCode; i++) {
        if (table[i] && strcmp(table[i]->str, str) == 0) {
            return i;
        }
    }
    return -1; // String not found
}

int* lzwCompress(const char *input, int *size) {
    initializeDictionary();

    char currentString[3] = {0}; // Buffer for current string
    int *output = (int *)malloc(TABLE_SIZE * sizeof(int));
    if (!output) {
        fprintf(stderr, "Memory allocation failed for output.\n");
        exit(EXIT_FAILURE);
    }
    int outputSize = 0;

    for (size_t i = 0; input[i] != '\0'; i++) {
        char nextString[4] = {0}; // Buffer for next string
        sprintf(nextString, "%s%c", currentString, input[i]);

        int code = findStringCode(nextString);
        if (code != -1) {
            strcpy(currentString, nextString);
        } else {
            output[outputSize++] = findStringCode(currentString);
            addStringToDictionary(nextString);
            sprintf(currentString, "%c", input[i]);
        }
    }

    if (currentString[0] != '\0') {
        output[outputSize++] = findStringCode(currentString);
    }

    *size = outputSize;
    int* temp = realloc(output, outputSize * sizeof(int));
    if (temp == NULL) {
        free(output);
        fprintf(stderr, "Memory reallocation failed for output.\n");
        exit(EXIT_FAILURE);
    }
    return temp;
}


char* lzwDecompress(const int *codes, int size) {
    initializeDictionary();

    int bufferSize = TABLE_SIZE;
    char *decompressed = (char *)malloc(bufferSize * sizeof(char));
    if (decompressed == NULL) {
        fprintf(stderr, "Failed to allocate memory for decompression.\n");
        exit(EXIT_FAILURE);
    }
    decompressed[0] = '\0';

    int prevCode = codes[0];
    char prevString[MAX_CHAR] = {0};
    strcpy(prevString, getStringFromCode(prevCode));
    strncat(decompressed, prevString, bufferSize - strlen(decompressed) - 1);

    for (int i = 1; i < size; i++) {
        int currentCode = codes[i];
        char currentString[MAX_CHAR] = {0};

        if (getStringFromCode(currentCode) != NULL) {
            strncpy(currentString, getStringFromCode(currentCode), MAX_CHAR - 1);
        } else {
            strncpy(currentString, prevString, MAX_CHAR - 1);
            currentString[strlen(currentString) + 1] = '\0';
            currentString[strlen(currentString)] = prevString[0]; // Append the first character of the previous string
        }

        // Check buffer size and expand if necessary
        if (strlen(decompressed) + strlen(currentString) + 1 > bufferSize) {
            bufferSize *= 2;
            char *temp = realloc(decompressed, bufferSize * sizeof(char));
            if (temp == NULL) {
                free(decompressed);
                fprintf(stderr, "Failed to reallocate memory for decompression.\n");
                exit(EXIT_FAILURE);
            }
            decompressed = temp;
        }

        strncat(decompressed, currentString, bufferSize - strlen(decompressed) - 1);

        // Add new entry to dictionary
        char newEntry[MAX_CHAR] = {0};
        snprintf(newEntry, sizeof(newEntry), "%s%c", prevString, currentString[0]);
        addStringToDictionary(newEntry);

        strcpy(prevString, currentString);
    }

    return decompressed;
}

char* getStringFromCode(int code) {
    if (code >= 0 && code < nextAvailableCode && table[code]) {

        return table[code]->str;
    }
    return NULL;
}
