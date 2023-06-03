#ifndef UTILS_H
#define UTILS_H

typedef struct image{
	int width;
	int height;
	long int size;
	unsigned char * data;
	int * alpha;
} image;

image* imgRgbToGrey(unsigned char * input, int width, int height);

image* redSquare(image * q, image * c, int min_x, int min_y, unsigned char * inputImg);
int isValidFilter(int index, image * q, image * c);
int applyFilter(int index, image * q, image *c);
int isBetterFilter(int index, image * q, image *c, int seuil);

image* sequential(image * q, image * c, unsigned char * inputImg);
image* sequentialLinear(image * q, image * c, unsigned char * inputImg);
image* ompNaif(image * q, image * c, unsigned char * inputImg);
image* ompNaifLinear(image * q, image * c, unsigned char * inputImg);
image* ompTerminator(image * q, image * c, unsigned char * inputImg);
image* ompScoreBoard(image * q, image * c, unsigned char * inputImg);

#endif