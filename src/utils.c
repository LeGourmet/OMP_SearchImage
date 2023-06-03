#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "utils.h"

image* imgRgbToGrey(unsigned char * input, int width, int height){
    int size = width * height;

    //création d'une image en niveau de gris 
    //size grey = size RGB/nbChannels
    image * output = (image *) malloc(sizeof(image));
    output->width = width;
    output->height = height;
    output->size = size;
    output->data = (unsigned char*)malloc(size * sizeof(unsigned char));
    output->alpha = (int*)malloc(size * sizeof(int));

    #pragma omp parallel for
    for(int i=0; i<size ; i++){
        int x  = i*4;
        output->data[i] = 0.299 * input[x] + 0.587 * input[x + 1] + 0.114 * input[x + 2];
        output->alpha[i] = input[x+3]==255.f ? 1 : 0;
    }

    return output;
}

image* redSquare(image * q, image * c, int min_x, int min_y, unsigned char * inputImg){

    //copie les données de inputImg dans une variable output
    image * output = (image *) malloc(sizeof(image));
    output->width = q->width;
    output->height = q->height;
    output->size = q->size*4;
    output->data = (unsigned char*)malloc(output->size * sizeof(unsigned char));

    memcpy(output->data, inputImg, output->size * sizeof(unsigned char));
    
    if(min_x==-1 ||min_x==-1){
        printf("image not found !\n");
        return output;
    }

    //Tracer les bordures horizontales de la redBox
    int cursor;
    #pragma omp parallel for private(cursor)
    for(int i= min_x; i < min_x+c->width ;i++){
        cursor = (min_y*q->width+i)*4;
        output->data[cursor] = 255;
        output->data[cursor +1] = 0;
        output->data[cursor +2] = 0; 
        output->data[cursor +3] = 255; 

        cursor = ((min_y+c->height-1)*q->width+i)*4;
        output->data[cursor] = 255;
        output->data[cursor +1] = 0;
        output->data[cursor +2] = 0; 
        output->data[cursor +3] = 255; 
    }
    //Tracer les bordures verticales de la redBox
    #pragma omp parallel for private(cursor)
    for(int i= min_y; i < min_y+c->height ;i++){
        cursor = (i*q->width+min_x)*4;
        output->data[cursor] = 255;
        output->data[cursor +1] = 0;
        output->data[cursor +2] = 0; 
        output->data[cursor +3] = 255;   

        cursor = (i*q->width+min_x+c->width-1)*4;
        output->data[cursor] = 255;
        output->data[cursor +1] = 0;
        output->data[cursor +2] = 0; 
        output->data[cursor +3] = 255; 
    }

    return output;
}


//retourne 1 si la sous images corresponds au pattern 0 sinon
int isValidFilter(int index, image * q, image *c){
    for(int filter = 0; filter < c->size; filter++){
        if(filter%c->width == 0 && filter != 0)  
            index += q->width-c->width;
        
        if(c->alpha[filter]*(q->data[index] - c->data[filter]) != 0)
            return 0;

        index++;
    }
    return 1;
}

//renvoi la valeur calculé du patch si elle est inférieur à l'ancienne sinon renvoi l'ancienne
int isBetterFilter(int index, image * q, image *c, int seuil){
    int value = 0;
    for(int filter = 0; filter < c->size; filter++){
        if(value >= seuil)
            return seuil;
        
        if(filter%c->width == 0 && filter != 0)  
            index += q->width-c->width;
        
        if(c->alpha[filter]){
            int tmp = (q->data[index] - c->data[filter]);
            value += tmp*tmp;
        }

        index++;
    }
    return value;
}

//renvoi la valeur du patch testé
int applyFilter(int index ,image * q, image * c){
    int res = 0;

    for(int filter = 0; filter < c->size; filter++){
        if(filter%c->width == 0 && filter != 0)  
            index += q->width-c->width;

        if(c->alpha[filter]){
            int tmp = q->data[index] - c->data[filter];
            res += tmp*tmp;
        }

        index++;
    }

    return res;
}

// =====================================================================================
// ====================================  SEQUENCIAL ==================================== 
// =====================================================================================


image* sequential(image * q, image * c, unsigned char * inputImg){
    int min_x, min_y;    
    int min_value = INT_MAX;
    //parcour de l'image source 
    for(int y=0; y <= q->height - c->height ;y++){
        for(int x=0; x <= q->width - c->width ;x++){
            int value = 0;
            //parcour de la sous-image pour trouver le pattern
            for(int j=0; j< c->height ;j++){
                for(int i=0; i< c->width ;i++){
                    if(c->alpha[j*c->width+i]){
                        int tmp = q->data[(x+i) + q->width*(y+j)] - c->data[j*c->width+i]; 
                        value += tmp*tmp;
                    }
                }
            }
            
            if(value<min_value){
                min_x = x;
                min_y = y;
                min_value = value;
            }

        }
    }

    return redSquare(q,c,min_x,min_y,inputImg);
}

image* sequentialLinear(image * q, image * c, unsigned char * inputImg){
    int min_x = -1, min_y = -1;
    int newLine = q->width - c->width;

    for(int cursor = 0; cursor < q->size - c->height * q->width; cursor++ ){
        if(cursor % q->width == newLine)
            cursor += c->width - 1;
            
        if(isValidFilter(cursor,q,c)){
            min_x = cursor % q->width;
            min_y = (int)cursor/q->width;
            break;
        }
    }

    return redSquare(q,c,min_x,min_y,inputImg);
}

// =====================================================================================
// ====================================== OMP NAIF ===================================== 
// =====================================================================================

image* ompNaif(image * q, image * c, unsigned char * inputImg){
    int min_x, min_y;    
    int min_value = INT_MAX;

    #pragma omp parallel for schedule(dynamic)
    for(int y=0; y <= q->height - c->height ;y++){
        for(int x=0; x <= q->width - c->width ;x++){
            int value = 0;

            for(int j=0; j<c->height ;j++){
                for(int i=0; i<c->width ;i++){
                    if(c->alpha[j*c->width+i]){
                        int tmp = q->data[(x+i) + q->width*(y+j)] - c->data[j*c->width+i]; 
                        #pragma omp atomic
                        value += tmp*tmp;
                    }
                }
            }
            
            #pragma omp critical
            if(value<min_value){
                min_x = x;
                min_y = y;
                min_value = value;
            }

        }
    }

    return redSquare(q,c,min_x,min_y,inputImg);
}

image* ompNaifLinear(image * q, image * c, unsigned char * inputImg){
    int min_x=-1, min_y=-1;    
    int cursor;
    int flag = 1;
    //flag permettant de transmettre aux autres thread que le pattern est trouvé
    #pragma omp parallel for schedule(dynamic) shared(flag) 
    for(cursor = 0; cursor < q->size - c->height * q->width; cursor ++ ){
        if(flag && (cursor % q->width < q->width - c->width)){
            if(isValidFilter(cursor, q, c)){
                flag = 0;
                min_x = cursor % q->width;
                min_y = (int)cursor/q->width;
            }
        }
    }

    return redSquare(q,c,min_x,min_y,inputImg);
}


image* ompTerminator(image * q, image * c, unsigned char * inputImg){
    int min_x=-1, min_y=-1;    
    int cursor;
    int bestSeuil = INT_MAX;
    
    #pragma omp parallel for schedule(dynamic) shared(bestSeuil) 
    for(cursor = 0; cursor < q->size - c->height * q->width; cursor ++ ){
        if(cursor % q->width < q->width - c->width){
            int seuil = isBetterFilter(cursor, q, c, bestSeuil);

            // bestSeuil share => implicit critical barrière
            if(seuil < bestSeuil){
                min_x = cursor % q->width;
                min_y = (int)cursor/q->width;
                bestSeuil = seuil;
            }
        }
    }

    return redSquare(q,c,min_x,min_y,inputImg);
}

// =====================================================================================
// ===================================== OMP OTHER ===================================== 
// =====================================================================================

image* ompScoreBoard(image * q, image * c, unsigned char * inputImg){
    int min_x, min_y, cursor;    

    int min_value = INT_MAX;
    int size = q->size - c->height * q->width;
    int *res = (int*) malloc(size*sizeof(int));

    #pragma omp parallel for schedule(dynamic)
    for(cursor=0; cursor<size; cursor++ )
        if(cursor % q->width < q->width - c->width)
            res[cursor] = applyFilter(cursor, q, c);
    
    #pragma omp parallel for schedule(dynamic) shared(min_value)
    for (cursor=0; cursor<size ;cursor++){
        if(cursor % q->width < q->width - c->width){
            if(res[cursor]<min_value){
                min_x = cursor % q->width;
                min_y = (int)cursor/q->width;
                min_value = res[cursor];
            }
        }
    }

    free(res);
    return redSquare(q,c,min_x,min_y,inputImg);
}
