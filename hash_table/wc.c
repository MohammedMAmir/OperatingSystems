#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "wc.h"
#include <string.h>


#define MAX_WORD 256
#define HSIZE 10000000

unsigned int hashfun(char* word){
    unsigned int hash, i;
    for(hash = i = 0; i < strlen(word); i++){
        hash += word[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    
    hash += (hash<<3);
    hash ^= (hash>>11);
    hash += (hash << 15);
    return hash % HSIZE;
}

struct wc {
	struct Word* hash_table[HSIZE];
};

struct Word{
	char word[MAX_WORD];
	int count;
	struct Word* nextWord;
};

int check_word(struct wc* wc, char* word){
    struct Word* tmpptr = wc->hash_table[hashfun(word)];
    while(tmpptr != NULL){
        if(strcmp(tmpptr->word, word) == 0){
            tmpptr->count += 1;
            return 1;
        }else{
            tmpptr = tmpptr->nextWord;
        }
    }
    return 0;
}
struct wc *
wc_init(char *word_array, long size)
{
	struct wc *wc;
	wc = (struct wc *)malloc(sizeof(struct wc));
	assert(wc);

        for(int i = 0; i<HSIZE; i++){
            wc->hash_table[i] = NULL;
        }
        
        char* dup = strdup(word_array);
        char delim[] = "\n\t\r \0";
        char* ptr = strtok(dup, delim);
        while(ptr != NULL){
            if(wc->hash_table[hashfun(ptr)] != NULL){
                if(check_word(wc, ptr) != 1){
                    struct Word* tmpptr = wc->hash_table[hashfun(ptr)];
                    wc->hash_table[hashfun(ptr)]=(struct Word*)malloc(sizeof(struct Word));
                    strcpy(wc->hash_table[hashfun(ptr)]->word, ptr);
                    wc->hash_table[hashfun(ptr)]->count = 1;
                    wc->hash_table[hashfun(ptr)]->nextWord = tmpptr;
                    tmpptr = NULL;
                }
            }else{
                wc->hash_table[hashfun(ptr)]=(struct Word*)malloc(sizeof(struct Word));
                strcpy(wc->hash_table[hashfun(ptr)]->word, ptr);
                wc->hash_table[hashfun(ptr)]->count = 1;
                wc->hash_table[hashfun(ptr)]->nextWord = NULL;
            }  
            ptr = strtok(NULL, delim);
        }
        free(dup);
	return wc;
}

void
wc_output(struct wc *wc)
{
    for(int i = 0; i<HSIZE; i++){
        if(wc->hash_table[i] != NULL){
            struct Word* tmpptr = wc->hash_table[i];
            while(tmpptr != NULL){
                printf("%s:%d\n",tmpptr->word,tmpptr->count);
                tmpptr= tmpptr->nextWord;
            }
        }
    }
}

void
wc_destroy(struct wc *wc)
{   
    for(int i = 0; i<HSIZE; i++){
         if(wc->hash_table[i] != NULL){ 
            struct Word* tmpptr;
            while(wc->hash_table[i]->nextWord != NULL ){
                tmpptr = wc->hash_table[i];
                wc->hash_table[i] = wc->hash_table[i]->nextWord;
                free(tmpptr);
            }
            free(wc->hash_table[i]);
         }
    }
        //free(wc->hash_table);
	free(wc);
}
