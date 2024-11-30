#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 8192
#define PNG_SIGNATURE "\211PNG"  // Standard PNG signature (‰PNG)
#define PNG_END "IEND"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <input.snagx>\n", argv[0]);
        return 1;
    }

    // Open input file
    FILE *input = fopen(argv[1], "rb");
    if (!input) {
        printf("Error: Cannot open input file %s\n", argv[1]);
        return 1;
    }

    // Create output filename by replacing .snagx with .png
    char output_filename[256];
    strcpy(output_filename, argv[1]);
    char *ext = strstr(output_filename, ".snagx");
    if (ext) {
        strcpy(ext, ".png");
    } else {
        strcat(output_filename, ".png");
    }

    FILE *output = NULL;
    unsigned char buffer[BUFFER_SIZE];
    size_t bytes_read;
    int found_start = 0;
    int found_end = 0;
    long start_pos = 0;

    // First pass: find PNG signature position
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, input)) > 0) {
        for (size_t i = 0; i < bytes_read - 4; i++) {
            if (memcmp(&buffer[i], PNG_SIGNATURE, 4) == 0) {
                start_pos = ftell(input) - bytes_read + i;
                found_start = 1;
                break;
            }
        }
        if (found_start) break;
    }

    if (!found_start) {
        printf("Error: PNG signature not found in file\n");
        fclose(input);
        return 1;
    }

    // Open output file and seek to start position
    output = fopen(output_filename, "wb");
    if (!output) {
        printf("Error: Cannot create output file %s\n", output_filename);
        fclose(input);
        return 1;
    }

    fseek(input, start_pos, SEEK_SET);

    // Copy data until we find IEND chunk
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, input)) > 0) {
        fwrite(buffer, 1, bytes_read, output);
        
        // Check for IEND chunk
        for (size_t i = 0; i < bytes_read - 4; i++) {
            if (memcmp(&buffer[i], PNG_END, 4) == 0) {
                // Write a bit more to include the IEND chunk's CRC
                fwrite(buffer + bytes_read, 1, 4, output);
                found_end = 1;
                break;
            }
        }
        if (found_end) break;
    }

    fclose(input);
    fclose(output);

    if (!found_end) {
        printf("Warning: PNG end marker not found, file might be incomplete\n");
        return 1;
    }

    printf("Successfully converted %s to %s\n", argv[1], output_filename);
    return 0;
}
