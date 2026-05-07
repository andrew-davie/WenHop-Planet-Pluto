/***************************************************/
/*************** wav2raw2600 utility ***************/
/***************************************************/
/*      Craig Daniels - Gamax Software - 2026      */
/***************************************************/
/* Will convert 16 bit mono/stereo WAV files to    */
/* a raw 4 bit nybble packed for for the 2600      */
/*                                                 */
/* Text prompt/input driven                        */
/*                                                 */
/* Output available in binary, include or both     */
/***************************************************/

#define _CRT_SECURE_NO_WARNINGS

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**************************** Functions ****************************/
int ends_with_wav(const char *str);
void write_output_byte(unsigned char ch, FILE *binFile, FILE *incFile,
                       unsigned int *outputByteCount);

/**************************** Variables ****************************/
char filename[256];
char id[5] = {0};
unsigned int sampleRate;
unsigned short format;
unsigned short channels;
unsigned short blockAlign;
unsigned int dataSize;
unsigned int sampleCount;
short in16;
short left16;
short right16;
short biggest = 0;
short smallest = 0;
float scale;
char outname[256];
unsigned int outSampleRate = 0;
char input[32];
int outputMode = 3;

/***************************************************/
bool verbose = false; // to control extra data output
/***************************************************/

/************************************************* Begin
 * *************************************************/
int main() {
  FILE *file;

  while (1) {
    /*********** Get input WAV file ***********/
    printf("WAV file to convert ('x' to exit): ");
    fgets(filename, sizeof(filename), stdin);

    // Strip newline
    filename[strcspn(filename, "\n")] = 0;

    // Exit
    if (strcmp(filename, "x") == 0 || strcmp(filename, "X") == 0)
      break;

    /*********** Append .wav if missing ***********/
    if (!ends_with_wav(filename)) {
      strcat(filename, ".wav");
    }

    file = fopen(filename, "rb");

    if (file == NULL) {
      printf("Error: Could not open '%s'\n\n", filename);
      continue;
    }

    if (verbose)
      printf("Opened '%s' successfully!\n\n", filename);

    /*********** Check for RIFF ***********/
    fseek(file, 0, SEEK_SET);
    fread(id, 1, 4, file);

    if (strncmp(id, "RIFF", 4) != 0) {
      printf("Invalid WAV file. File must be a standard uncompressed PCM "
             "WAV.\n\n");
      fclose(file);
      continue;
    }

    /*********** Check for WAVE ***********/
    fseek(file, 8, SEEK_SET);
    fread(id, 1, 4, file);

    if (strncmp(id, "WAVE", 4) != 0) {
      printf("Invalid WAV file. File must be a standard uncompressed PCM "
             "WAV.\n\n");
      fclose(file);
      continue;
    }

    /*********** Check for PCM ***********/
    fseek(file, 20, SEEK_SET);
    fread(&format, 2, 1, file);

    if (format != 1) {
      printf("Invalid WAV: must be PCM (uncompressed)\n\n");
      fclose(file);
      continue;
    }

    /*********** Check channels ***********/
    fseek(file, 22, SEEK_SET);
    fread(&channels, 2, 1, file);

    if (channels != 1 && channels != 2) {
      printf("WAV file must be mono or stereo.\n\n");
      fclose(file);
      continue;
    }

    /*********** Get sample rate ***********/
    fseek(file, 24, SEEK_SET);
    fread(&sampleRate, 4, 1, file);

    if (verbose)
      printf("Sample Rate: %u Hz\n", sampleRate);

    /*********** Block check ***********/
    fseek(file, 32, SEEK_SET);
    fread(&blockAlign, 2, 1, file);

    if (blockAlign != channels * 2) {
      printf("WAV must be 16-bit PCM with valid block alignment.\n\n");
      fclose(file);
      continue;
    }

    /*********** Check "data" ID ***********/
    fseek(file, 36, SEEK_SET);
    fread(id, 1, 4, file);

    if (strncmp(id, "data", 4) != 0) {
      printf("Invalid WAV file. File must be a standard uncompressed PCM "
             "WAV.\n\n");
      fclose(file);
      continue;
    }

    /*********** Check "data" ID ***********/
    fseek(file, 40, SEEK_SET);
    fread(&dataSize, 4, 1, file);

    sampleCount = dataSize / blockAlign;

    if (verbose) {
      printf("Data Size: %u bytes\n", dataSize);
      printf("Samples:   %u\n", sampleCount);
    }

    /*********** Normalization pass ***********/
    fseek(file, 44, SEEK_SET);

    biggest = 0;
    smallest = 0;

    for (unsigned int i = 0; i < sampleCount; i++) {
      if (channels == 1) {
        fread(&in16, 2, 1, file);
      } else {
        fread(&left16, 2, 1, file);
        fread(&right16, 2, 1, file);

        in16 = (short)(((int)left16 + (int)right16) / 2);
      }

      if (in16 < smallest)
        smallest = in16;
      if (in16 > biggest)
        biggest = in16;
    }

    if (biggest == 0 && smallest == 0) {
      scale = 1.0f; // silence
    } else if (biggest > -smallest) {
      scale = 32500.0f / biggest;
    } else {
      scale = -32500.0f / smallest;
    }

    /*********** Get output filename ***********/
    printf("\n\nInput file: %s\n", filename);
    printf("Input sample rate: %u\n", sampleRate);
    printf("Input sample count: %u\n\n", sampleCount);

    printf("Highest value in WAV: %d\n", biggest);
    printf("Smallest value in WAV: %d\n", smallest);
    printf("Resulting in a scale factor of %.6f to achieve full volume\n\n",
           scale);

    printf("Output name (no file ext) [leave empty to exit program]: ");
    fgets(outname, sizeof(outname), stdin);

    // Strip newline
    outname[strcspn(outname, "\n")] = 0;

    if (outname[0] == '\0') {
      fclose(file);
      break;
    }

    /*********** Get output sample rate ***********/
    printf("\nOutput sample rate [leave empty to exit program]: ");
    fgets(input, sizeof(input), stdin);

    // Strip newline
    input[strcspn(input, "\n")] = 0;

    // Empty input → treat as 0
    if (input[0] == '\0') {
      outSampleRate = 0;
    } else {
      outSampleRate = (unsigned int)atoi(input);
    }

    // Exit condition
    if (outSampleRate == 0) {
      fclose(file);
      break;
    }

    /*********** Get output type(s) ***********/
    outputMode = 3; // default = both

    printf("\nOutput type:\n");
    printf("1 - Binary (.bin)\n");
    printf("2 - C include (.inc)\n");
    printf("3 - Both [default]\n");
    printf("> ");

    fgets(input, sizeof(input), stdin);

    // Strip newline
    input[strcspn(input, "\n")] = 0;

    // Decide mode
    if (input[0] == '1') {
      outputMode = 1;
    } else if (input[0] == '2') {
      outputMode = 2;
    } else if (input[0] == '3' || input[0] == '\0') {
      outputMode = 3;
    } else {
      printf("Invalid selection. Defaulting to BOTH.\n");
      outputMode = 3;
    }

    /*********** Convert sample ***********/
    float soundTime = 0.0f;
    float soundTimeStep = 1.0f / sampleRate;
    float soundOutStep = 1.0f / outSampleRate;
    float soundTargetTime = soundOutStep;

    int nyb = 0;
    int conv;
    unsigned char ch1 = 0;
    unsigned char ch2 = 0;
    unsigned char ch;
    unsigned int outputByteCount = 0;

    FILE *binFile = NULL;
    FILE *incFile = NULL;
    char binFilename[300];
    char incFilename[300];

    sprintf(binFilename, "%s.bin", outname);
    sprintf(incFilename, "%s.inc", outname);

    if (outputMode == 1 || outputMode == 3) {
      binFile = fopen(binFilename, "wb");

      if (binFile == NULL) {
        printf("Error: Could not create '%s'\n\n", binFilename);
        fclose(file);
        continue;
      }
    }

    if (outputMode == 2 || outputMode == 3) {
      incFile = fopen(incFilename, "w");

      if (incFile == NULL) {
        printf("Error: Could not create '%s'\n\n", incFilename);

        if (binFile != NULL)
          fclose(binFile);
        fclose(file);
        continue;
      }
    }

    fseek(file, 44, SEEK_SET);

    for (unsigned int i = 0; i < sampleCount; i++) {
      if (channels == 1) {
        fread(&in16, 2, 1, file);
      } else {
        fread(&left16, 2, 1, file);
        fread(&right16, 2, 1, file);

        in16 = (short)(((int)left16 + (int)right16) / 2);
      }

      soundTime += soundTimeStep;

      if (soundTime > soundTargetTime) {

        //  conv = (int)(((float)in16 * scale + 32768.0f) / 4096.0f);
        conv = (in16 * scale + 32768.0f) / 4096.0f;

        if (conv < 0)
          conv = 0;
        if (conv > 15)
          conv = 15;

        if (nyb == 0) {
          nyb = 1;
          ch1 = (unsigned char)conv;
        } else {
          nyb = 0;
          ch2 = (unsigned char)conv;

          ch = (ch1 << 4) | ch2;

          write_output_byte(ch, binFile, incFile, &outputByteCount);
        }

        soundTargetTime += soundOutStep;
      }
    }

    if (nyb == 1) {
      ch = (ch1 << 4) | ch1;

      write_output_byte(ch, binFile, incFile, &outputByteCount);
    }

    while ((outputByteCount % 4) != 0) {
      ch = 0x88;

      write_output_byte(ch, binFile, incFile, &outputByteCount);
    }

    if (incFile != NULL) {
      fprintf(incFile, "\n");
      fclose(incFile);
      incFile = NULL;
    }

    if (binFile != NULL) {
      fclose(binFile);
      binFile = NULL;
    }

    printf("\nConversion complete.\n");
    printf("Output bytes: %u\n\n", outputByteCount);

    fclose(file);
  }

  return 0;
}

/*******************************************************************/
/**************************** Functions ****************************/
int ends_with_wav(const char *str) {
  int len = strlen(str);
  if (len < 4)
    return 0;

  char c1 = tolower(str[len - 4]);
  char c2 = tolower(str[len - 3]);
  char c3 = tolower(str[len - 2]);
  char c4 = tolower(str[len - 1]);

  return (c1 == '.' && c2 == 'w' && c3 == 'a' && c4 == 'v');
}

void write_output_byte(unsigned char ch, FILE *binFile, FILE *incFile,
                       unsigned int *outputByteCount) {
  if (binFile != NULL) {
    fwrite(&ch, 1, 1, binFile);
  }

  if (incFile != NULL) {
    fprintf(incFile, "0x%02X,", ch);
  }

  (*outputByteCount)++;

  if (incFile != NULL && (*outputByteCount % 32) == 0) {
    fprintf(incFile, "\n");
  }
}