#include <iostream>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <chrono>

// using std::cout;
// using std::endl;
// using std::ifstream;
// using std::ofstream;

using namespace std;

#pragma pack(1)
// #pragma once

typedef int LONG;
typedef unsigned short WORD;
typedef unsigned int DWORD;

#ifndef NUM_THREADS
#define NUM_THREADS 4
#endif

typedef struct tagBITMAPFILEHEADER
{
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

int rows;
int cols;

vector<vector<vector<int>>> pixels;
vector<vector<vector<int>>> smoothed_pixels;

bool fillAndAllocate(char *&buffer, const char *fileName, int &rows, int &cols, int &bufferSize)
{
    std::ifstream file(fileName);

    if (file)
    {
        file.seekg(0, std::ios::end);
        std::streampos length = file.tellg();
        file.seekg(0, std::ios::beg);

        buffer = new char[length];
        file.read(&buffer[0], length);

        PBITMAPFILEHEADER file_header;
        PBITMAPINFOHEADER info_header;

        file_header = (PBITMAPFILEHEADER)(&buffer[0]);
        info_header = (PBITMAPINFOHEADER)(&buffer[0] + sizeof(BITMAPFILEHEADER));
        rows = info_header->biHeight;
        cols = info_header->biWidth;
        bufferSize = file_header->bfSize;
        return 1;
    }
    else
    {
        cout << "File" << fileName << " doesn't exist!" << endl;
        return 0;
    }
}

char *fileReadBuffer;

void getPixlesFromBMP24(int end)
{
    int count = 1;
    int extra = cols % 4;
    for (int i = 0; i < rows; i++)
    {
        count += extra;
        for (int j = cols - 1; j >= 0; j--)
            for (int k = 0; k < 3; k++)
            {
                pixels[i][j][k] = fileReadBuffer[end - count];
                if (pixels[i][j][k] < 0)
                {
                    pixels[i][j][k] += 256;
                }
                count++;
            }
    }
}

void writeOutBmp24(vector<vector<vector<int>>> &filtered_pixels, char *fileBuffer, const char *nameOfFileToCreate, int bufferSize)
{
    std::ofstream write(nameOfFileToCreate);
    if (!write)
    {
        cout << "Failed to write " << nameOfFileToCreate << endl;
        return;
    }
    int count = 1;
    int extra = cols % 4;
    for (int i = 0; i < rows; i++)
    {
        count += extra;
        for (int j = cols - 1; j >= 0; j--)
            for (int k = 0; k < 3; k++)
            {
                switch (k)
                {
                case 0:
                    fileBuffer[bufferSize - count] = filtered_pixels[i][j][0];
                    break;
                case 1:
                    fileBuffer[bufferSize - count] = filtered_pixels[i][j][1];
                    break;
                case 2:
                    fileBuffer[bufferSize - count] = filtered_pixels[i][j][2];
                    break;
                }
                count++;
            }
    }
    write.write(fileBuffer, bufferSize);
}

void applyWashedOut()
{
    int redMean = 0, greenMean = 0, blueMean = 0;
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            for (int k = 0; k < 3; k++)
            {
                switch (k)
                {
                case 0:
                    redMean += pixels[i][j][k];
                    break;
                case 1:
                    greenMean += pixels[i][j][k];
                    break;
                case 2:
                    blueMean += pixels[i][j][k];
                    break;
                }
            }
        }
    }
    redMean /= (rows * cols);
    greenMean /= (rows * cols);
    blueMean /= (rows * cols);

    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            for (int k = 0; k < 3; k++)
            {
                switch (k)
                {
                case 0:
                    pixels[i][j][k] = (pixels[i][j][k] * 0.4) + (redMean * 0.6);
                    break;
                case 1:
                    pixels[i][j][k] = (pixels[i][j][k] * 0.4) + (greenMean * 0.6);
                    break;
                case 2:
                    pixels[i][j][k] = (pixels[i][j][k] * 0.4) + (blueMean * 0.6);
                    break;
                }
            }
        }
    }
}

void applyCrossLine()
{

    for (int i = 1; i < rows; i++)
    {
        for (int k = 0; k < 3; k++)
        {
            pixels[i][i - 1][k] = 255;
            pixels[i][i][k] = 255;
            pixels[i][i + 1][k] = 255;
            pixels[i][rows - i - 1][k] = 255;
            pixels[i][rows - i][k] = 255;
            pixels[i][rows - i + 1][k] = 255;
        }
    }
}

int getMean(int i, int j, int k)
{
    int sum = 0;
    for (int r = i - 1; r < i + 2; r++)
    {
        for (int c = j - 1; c < j + 2; c++)
        {
            sum += pixels[r][c][k];
        }
    }
    return (sum / 9);
}

void *applySmoothing(void *startRowPositionInp)
{
    long startRowPosition = (long)startRowPositionInp;
    for (int i = ((startRowPosition * (rows / NUM_THREADS))); i < ((startRowPosition * (rows / NUM_THREADS))) + (rows / NUM_THREADS); i++)
    {
        if (i == rows - 1)
        {
            break;
        }
        if (i == 0)
        {
            continue;
        }
        for (int j = 1; j < cols - 1; j++)
        {
            for (int k = 0; k < 3; k++)
            {
                smoothed_pixels[i][j][k] = getMean(i, j, k);
            }
        }
    }
}

void *applySepia(void *startRowPositionInp)
{
    long startRowPosition = (long)startRowPositionInp;
    for (int i = ((startRowPosition * (rows / NUM_THREADS))); i < ((startRowPosition * (rows / NUM_THREADS))) + (rows / NUM_THREADS); i++)
    {
        for (int j = 1; j < cols - 1; j++)
        {
            for (int k = 0; k < 3; k++)
            {
                switch (k)
                {
                case 0:
                    pixels[i][j][0] = ((float)smoothed_pixels[i][j][0] * 0.393) + ((float)smoothed_pixels[i][j][1] * 0.769) + ((float)smoothed_pixels[i][j][2] * 0.189);
                    if (pixels[i][j][k] > 255)
                    {
                        pixels[i][j][k] = 255;
                    }
                    break;
                case 1:
                    pixels[i][j][1] = ((float)smoothed_pixels[i][j][0] * 0.349) + ((float)smoothed_pixels[i][j][1] * 0.686) + ((float)smoothed_pixels[i][j][2] * 0.168);
                    if (pixels[i][j][k] > 255)
                    {
                        pixels[i][j][k] = 255;
                    }
                    break;
                case 2:
                    pixels[i][j][2] = ((float)smoothed_pixels[i][j][0] * 0.272) + ((float)smoothed_pixels[i][j][1] * 0.534) + ((float)smoothed_pixels[i][j][2] * 0.131);
                    if (pixels[i][j][k] > 255)
                    {
                        pixels[i][j][k] = 255;
                    }
                    break;
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    char *fileBuffer;
    int bufferSize;
    char *fileName = argv[1];
    auto start = chrono::high_resolution_clock::now();
    if (!fillAndAllocate(fileBuffer, fileName, rows, cols, bufferSize))
    {
        cout << "File read error" << endl;
        return 1;
    }
    pixels.resize(rows + 1, vector<vector<int>>(cols + 1, vector<int>(3)));
    smoothed_pixels.resize(rows + 1, vector<vector<int>>(cols + 1, vector<int>(3)));
    fileReadBuffer = fileBuffer;
    getPixlesFromBMP24(bufferSize);
    pthread_t threads[NUM_THREADS];
    for (long t = 0; t < NUM_THREADS; t++)
    {
        pthread_create(&threads[t], NULL, applySmoothing, (void *)t);
    }
    for (long t = 0; t < NUM_THREADS; t++)
    {
        pthread_join(threads[t], NULL);
    }
    pthread_t sepiaThreads[NUM_THREADS];
    for (long t_sep = 0; t_sep < NUM_THREADS; t_sep++)
    {
        pthread_create(&sepiaThreads[t_sep], NULL, applySepia, (void *)t_sep);
    }
    for (long t = 0; t < NUM_THREADS; t++)
    {
        pthread_join(sepiaThreads[t], NULL);
    }
    applyWashedOut();
    applyCrossLine();
    const char *nameOfFileToCreate = "output.bmp";
    writeOutBmp24(pixels, fileBuffer, nameOfFileToCreate, bufferSize);
    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
    cout << "Executation Time: " << duration.count() << " milliseconds" << endl;
    return 0;
}