#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#include "../lib/stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../lib/stb_image/stb_image_write.h"
#include "omp.h"

#include "utils.h"

// gcc -fopenmp -o main main.c utils.c -lm
// ./main ../img/beach.png ../img/goat.png


int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        printf("Invalid arguments !\n");
        return EXIT_FAILURE;
    }

    // Get image paths from arguments.
    const char* inputImgPath = argv[1];
    const char* searchImgPath = argv[2];

    // ==================================== Loading input image.
    int inputImgWidth;
    int inputImgHeight;
    int inputImgNbChannels;
    unsigned char* inputImg = stbi_load(inputImgPath, &inputImgWidth, &inputImgHeight, &inputImgNbChannels, 4);
    if (inputImg == NULL)
    {
        printf("Cannot load image %s", inputImgPath);
        return EXIT_FAILURE;
    }
    printf("Input image %s: %dx%d\n", inputImgPath, inputImgWidth, inputImgHeight);

    // ====================================  Loading search image.
    int searchImgWidth;
    int searchImgHeight;
    int searchImgNbChannels;
    unsigned char* searchImg = stbi_load(searchImgPath, &searchImgWidth, &searchImgHeight, &searchImgNbChannels, 4);
    if (searchImg == NULL)
    {
        printf("Cannot load image %s", searchImgPath);
        return EXIT_FAILURE;
    }
    printf("Search image %s: %dx%d\n", searchImgPath, searchImgWidth, searchImgHeight);

    double start = omp_get_wtime();
    image * q = imgRgbToGrey(inputImg, inputImgWidth, inputImgHeight);
    image * c = imgRgbToGrey(searchImg, searchImgWidth, searchImgHeight);
    double stop = omp_get_wtime();
    printf("Conversion RGB->GREY done in : %fs\n",stop-start);

    // ====================================  Search SearchImage in input image

    start = omp_get_wtime();
    //image * res = sequential(q, c, inputImg);
    //image * res = sequentialLinear(q, c, inputImg);
    //image * res = ompNaif(q, c, inputImg);
    image * res = ompNaifLinear(q, c, inputImg);
    //image * res = ompTerminator(q, c, inputImg);
    //image * res = ompScoreBoard(q, c, inputImg);
    stop = omp_get_wtime();
    printf("Image match calculate in : %fs\n",stop-start);

    // ====================================  Save result
    unsigned char* resultImg = (unsigned char*)malloc(res->size* sizeof(unsigned char));
    memcpy(resultImg, res->data, res->size * sizeof(unsigned char));
    stbi_write_png("../img/result.png", res->width, res->height, 4, resultImg, res->width * 4 );

    free(resultImg);
    free(q->data);
    free(q->alpha);
    free(q);
    free(c->data);
    free(c->alpha);
    free(c);
    free(res->data);
    free(res);
    stbi_image_free(inputImg);
    stbi_image_free(searchImg);

    printf("Good bye!\n");

    return EXIT_SUCCESS;
}




