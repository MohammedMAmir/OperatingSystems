#include "request.h"
#include "server_thread.h"
#include "common.h"

struct server {
	int nr_threads;
	int max_requests;
	int max_cache_size;
	int exiting;
	
        /* add any other parameters you need */
                  
                  struct cache* cache;
                  pthread_mutex_t* cache_lock;
                  
	int *conn_buf;
	pthread_t *threads;
	int request_head;
	int request_tail;
	pthread_mutex_t mutex;
	pthread_cond_t prod_cond;
	pthread_cond_t cons_cond;	
};

/*LRU Node that stores the file name, whether the file is being used by another thread, and the next LRU Node*/
struct LRU_node{
    char* file_name;
    int in_use;
    int file_size;
    
    struct LRU_node* next;
};

/*file node that is stored in hash table with name, data, and next file at same hash bucket*/
struct file{
    struct file_data* data;
    
    struct file* next;
};

/*overall cache struct that contains the head of the hash table in cache_head, LRU most used node, LRU least used Node
 the overall size of the cache, and the remaining size of the cache*/
struct cache{
    struct file** cache_head;
    struct LRU_node* head_least_used;
    struct LRU_node* tail_most_used;
    int cache_size;
    int remaining_cache;
};

unsigned int hashfun(char* word, int cache_size){
    unsigned int hash, i;
    for(hash = i = 0; i < strlen(word); i++){
        hash += word[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    
    hash += (hash<<3);
    hash ^= (hash>>11);
    hash += (hash << 15);
    return (hash%cache_size);
}

int check_file(struct cache* cache, char* file_name){
    struct file* tmpptr = cache->cache_head[hashfun(file_name, cache->cache_size)];
    while(tmpptr != NULL){
        if(strcmp(tmpptr->data->file_name, file_name) == 0){
            return 1;
        }else{
            tmpptr = tmpptr->next;
        }
    }
    return 0;
}

void LRU_using_plus(char* file_name, struct server* sv){
    /*if there's only one thing in the LRU queue, and its what we were looking for, mark it as in use*/
        if(sv->cache->head_least_used == sv->cache->tail_most_used && strcmp(sv->cache->head_least_used->file_name, file_name) == 0){
            sv->cache->head_least_used->in_use++;
            return;
        /*if its at head of the LRU queue, change the head to the next thing and move the existing file node to the
         tail of the queue*/
        }else if(strcmp(sv->cache->head_least_used->file_name, file_name) == 0){
            struct LRU_node* tmpptr = sv->cache->head_least_used;
            sv->cache->head_least_used = tmpptr->next;
            sv->cache->tail_most_used->next = tmpptr;
            tmpptr->next = NULL;
            tmpptr->in_use++;
            sv->cache->tail_most_used = tmpptr;
            tmpptr = NULL;
            return;
         /*if what we're looking for is at the tail of the queue, mark it as in use*/
        }else if (strcmp(sv->cache->tail_most_used->file_name, file_name) == 0){
            sv->cache->tail_most_used->in_use++;
            return;
        /*otherwise, our file is somewhere deeper in the LRU queue. Find it, unlink it, put it at the tail, and mark it as in use */    
        }else{
            struct LRU_node* prevptr = sv->cache->head_least_used;
            while(prevptr->next != NULL){
                if(strcmp(prevptr->next->file_name, file_name) == 0){
                    struct LRU_node* tmpptr = prevptr->next;
                    prevptr->next = prevptr->next->next;
                    sv->cache->tail_most_used->next = tmpptr;
                    tmpptr->next = NULL;
                    tmpptr->in_use++;
                    sv->cache->tail_most_used = tmpptr;
                    tmpptr = NULL;
                    prevptr = NULL;
                    return;
                }else{
                    prevptr = prevptr->next;
                }
            }
            // something when horribly wrong because the file wasn't in the queue;
            return;
        }
}

void LRU_using_minus(char* file_name, struct server* sv){
        /*if there's only one thing in the LRU queue, and its what we were looking for, mark it as no longer in use*/
        if(sv->cache->head_least_used == sv->cache->tail_most_used && strcmp(sv->cache->head_least_used->file_name, file_name) == 0){
            sv->cache->head_least_used->in_use--;
            return;
        /*if its at head of the LRU queue, mark it as no longer in use*/
        }else if(strcmp(sv->cache->head_least_used->file_name, file_name) == 0){
            sv->cache->head_least_used->in_use--;
            return;
         /*if what we're looking for is at the tail of the queue, mark it as no longer in use*/
        }else if (strcmp(sv->cache->tail_most_used->file_name, file_name) == 0){
            sv->cache->tail_most_used->in_use--;
            return;
        /*otherwise, our file is somewhere deeper in the LRU queue. Find it and mark it as no longer in use */    
        }else{
            struct LRU_node* tmpptr = sv->cache->head_least_used;
            while(tmpptr->next != NULL){
                if(strcmp(tmpptr->file_name, file_name) == 0){
                    tmpptr->in_use--;
                    tmpptr = NULL;
                    return;
                }else{
                    tmpptr =   tmpptr->next;
                }
            }
            // something when horribly wrong because the file wasn't in the queue;
            return;
        }
}

/*overall function that looks up whether a file is in cache and returns associated file data. Returns NULL if lookup
 not successful*/
struct file_data* cache_lookup(char* file_name, struct server* sv){
    int ret = check_file(sv->cache, file_name);
    /*if the return is 0, the file isn't in our cache, so return NULL pointer*/
    if(ret == 0){
        return NULL;
    /*otherwise, the file is in our cache, find it, reorganize the LRU and return with a pointer to the data */
    }else{
        struct file* tmpptr = sv->cache->cache_head[hashfun(file_name, sv->cache->cache_size)];
        while(tmpptr != NULL){
            if(strcmp(tmpptr->data->file_name, file_name) == 0){
                return tmpptr->data;
            }else{
                tmpptr = tmpptr->next;
            }
        }
    }
    return NULL;
}

/*hash evict helper function that deletes hash file node and returns the amount of data freed*/
int hash_evict(char* file_name, struct server* sv){
    int index = hashfun(file_name, sv->cache->cache_size);
    struct file* prevptr = sv->cache->cache_head[index];
    /*the node we need to delete is at the head of cache_head[index]*/
    if(strcmp(prevptr->data->file_name, file_name) == 0){
        sv->cache->cache_head[index] = prevptr->next;
        int ret = prevptr->data->file_size;
        free(prevptr->data->file_buf);
        free(prevptr->data->file_name);
        free(prevptr->data);
        prevptr->next = NULL;
        free(prevptr);
        return ret;
    /*otherwise, the node is deeper in cache_head[index]*/    
    }else{
        while(prevptr->next != NULL){
            if(strcmp(prevptr->next->data->file_name, file_name) == 0){
                struct file* tmpptr = prevptr->next;
                prevptr->next = tmpptr->next;
                tmpptr->next = NULL;
                int ret = tmpptr->data->file_size;
                free(tmpptr->data->file_buf);
                free(tmpptr->data->file_name);
                free(tmpptr->data);
                free(tmpptr);
                prevptr = NULL;
                return ret;
            }else{
                prevptr = prevptr->next;
            }
        }
    }
    return 0;
}

/*LRU evict helper function that deletes least recently used LRU queue node if not in use and return file name of 
 cache file deleted (careful of segfaults)*/
char* LRU_evict(struct server* sv){
    char* evicted;
    /*if head of the LRU is not in use, pop it off and return it*/
    if(sv->cache->head_least_used->in_use == 0){
        /*if there's only on thing in the LRU, we need to change the tail too*/
        if(sv->cache->head_least_used == sv->cache->tail_most_used){
            struct LRU_node* tmpptr = sv->cache->head_least_used;
            sv->cache->head_least_used = NULL;
            sv->cache->tail_most_used = NULL;
            evicted = strdup(tmpptr->file_name);
            free(tmpptr->file_name);
            tmpptr->next = NULL;
            free(tmpptr);
            return evicted;
        /*otherwise, there are at least 2 Nodes in the LRU, take off the head*/
        }else{
            struct LRU_node* tmpptr = sv->cache->head_least_used;
            sv->cache->head_least_used = sv->cache->head_least_used->next;
            evicted = strdup(tmpptr->file_name);
            free(tmpptr->file_name);
            tmpptr->next = NULL;
            free(tmpptr);
            return evicted;
        }
    /*otherwise, the head is use and we need to pop off the next thing not in use*/
    }else{
        struct LRU_node* prevptr = sv->cache->head_least_used;
        while(prevptr->next != NULL){
            if(prevptr->next->in_use == 0){
                struct LRU_node* tmpptr = prevptr->next;
                prevptr->next = prevptr->next->next;
                evicted = strdup(tmpptr->file_name);
                free(tmpptr->file_name);
                tmpptr->next = NULL;
                free(tmpptr);
                prevptr = NULL;
                return evicted;
            }else{
                prevptr = prevptr->next;
            }
        }
    }
    return NULL;
}

/*overall eviction function that calls LRU_evict and hash_evict*/
int cache_evict(int amount_to_evict, struct server* sv){
   /*check if we even have the required amount to evict available in total*/
    //printf("entered cache_evict\n");
    int available_for_eviction = 0;
    struct LRU_node* tmpptr = sv->cache->head_least_used;
    while(tmpptr != NULL){
        if(tmpptr->in_use == 0){
            available_for_eviction += tmpptr->file_size;
            tmpptr = tmpptr->next;
        }else{
            //printf("%s in use - don't evict\n", tmpptr->file_name);
            tmpptr = tmpptr->next;
        }
    }
    
    /*if the total available amount to evict is less than the amount we need, return -1, otherwise evict*/
    if(available_for_eviction < amount_to_evict){
        //printf("not enough memory available\n");
        return -1;
    }else{
        int freed = 0;
        while(freed < amount_to_evict){
            char* evicted = LRU_evict(sv);
            //printf("freeing %s\n", evicted);
            int bytes_freed = hash_evict(evicted, sv);
            freed += bytes_freed;
            sv->cache->remaining_cache += bytes_freed;
            free(evicted);
        }
        return 0;
    }
    return 0;
}

/*helper function that inserts file into hash table if not already in there. If it's already in there, just return 1. If we 
 can't evict enough space for it return 2. If we successfully insert it return 0*/
int hash_insert(struct file_data* file, struct server* sv){
    struct cache* cache = sv->cache;
    int index = hashfun(file->file_name, cache->cache_size);
    
    if(cache->cache_head[index] != NULL){
        /*check if the file is already in the cache*/
        if(check_file(cache, file->file_name) != 1){
            /*if the size of the remaining cache is smaller than the size of the file evict some stuff if you can*/
            if(cache->remaining_cache < file->file_size){
                if(cache_evict(file->file_size - cache->remaining_cache, sv) == 0){
                    struct file* tmpptr = cache->cache_head[index];
                    cache->cache_head[index] = (struct file*)malloc(sizeof(struct file));
                    //cache->cache_head[index]->data->file_name = strdup(file->file_name);
                    //cache->cache_head[index]->data->file_buf = strdup(file->file_buf);
                    //cache->cache_head[index]->data->file_size = file->file_size;
                    cache->cache_head[index]->data = file;
                    cache->remaining_cache = cache->remaining_cache - file->file_size;
                    cache->cache_head[index]->next = tmpptr;
                    tmpptr = NULL;
                    cache = NULL;
                    return 0;
                /*otherwise, just return 2*/
                }else{
                    cache = NULL;
                    return 2;
                }
            /*otherwise just insert it normally*/    
            }else{     
                struct file* tmpptr = cache->cache_head[index];
                cache->cache_head[index] = (struct file*)malloc(sizeof(struct file));
                //cache->cache_head[index]->data->file_name = strdup(file->file_name);
                //cache->cache_head[index]->data->file_buf = strdup(file->file_buf);
                //cache->cache_head[index]->data->file_size = file->file_size;
                cache->cache_head[index]->data = file;
                cache->remaining_cache = cache->remaining_cache - file->file_size;
                cache->cache_head[index]->next = tmpptr;
                tmpptr = NULL;
                cache = NULL;
                return 0;
            }
            /*since the file is already in the cache return 1*/
        }else{
            cache = NULL;
            return 1;
        }
     /*cache index head is equal to NULL so just plug that file at the index*/   
    }else{
       if(cache->remaining_cache < file->file_size){
                if(cache_evict(file->file_size - cache->remaining_cache, sv) == 0){
                    cache->cache_head[index] = (struct file*)malloc(sizeof(struct file));
                    //cache->cache_head[index]->data->file_name = strdup(file->file_name);
                    //cache->cache_head[index]->data->file_buf = strdup(file->file_buf);
                    //cache->cache_head[index]->data->file_size = file->file_size;
                    cache->cache_head[index]->data = file;
                    cache->remaining_cache = cache->remaining_cache - file->file_size;
                    cache->cache_head[index]->next = NULL;
                    cache = NULL;
                    return 0;
                /*otherwise, just return 2*/
                }else{
                    cache = NULL;
                    return 2;
                }
            /*otherwise just insert it normally*/    
        }else{     
            cache->cache_head[index] = (struct file* ) malloc(sizeof(struct file));
            //cache->cache_head[index]->data->file_name = strdup(file->file_name);
            //cache->cache_head[index]->data->file_buf = strdup(file->file_buf);
            //cache->cache_head[index]->data->file_size = file->file_size;
            cache->cache_head[index]->data = file;
            cache->remaining_cache = cache->remaining_cache - file->file_size;
            cache->cache_head[index]->next = NULL;
            cache = NULL;
            return 0;
        }
    }
    cache = NULL;
    return 0;
}

/*LRU insert function that inserts file_name into LRU queue if not already in there. If already in there, just reorders
 list to make referred function the most recently used*/
int LRU_insert(struct file_data* file_name, struct server* sv, int hash_ret){
    if(hash_ret == 2){
        return 2;
    }
    /*case where LRU is empty and we are inserting first file*/
    if(sv->cache->head_least_used == NULL && sv->cache->tail_most_used == NULL){
        /*hash can return 2 in case we don't have space for some reason, or 0 in case we inserted into the hash
         table correctly. This handles both cases*/
        if(hash_ret == 2){
            return 2;
        }else if(hash_ret == 0){
            sv->cache->head_least_used = (struct LRU_node*)malloc(sizeof(struct LRU_node));
            sv->cache->tail_most_used = sv->cache->head_least_used;
            sv->cache->head_least_used->next = NULL;
            sv->cache->head_least_used->file_name = strdup(file_name->file_name);
            sv->cache->head_least_used->file_size = file_name->file_size;
            sv->cache->head_least_used->in_use = 1;
            return 0;
        }else{
            return 1;
        }
    /*if hash_ret = 1, our file was already in the hash table and should already be in the queue. Find it and move it
     to the tail_most_used*/
    }else if(hash_ret == 1){
        /*if there's only one thing in the LRU queue, and its what we were looking for, don't do anything, we're all good*/
        if(sv->cache->head_least_used == sv->cache->tail_most_used && strcmp(sv->cache->head_least_used->file_name, file_name->file_name) == 0){
            sv->cache->head_least_used->in_use++;
            return 1;
        /*if its at head of the LRU queue, change the head to the next thing and move the existing file node to the
         tail of the queue*/
        }else if(strcmp(sv->cache->head_least_used->file_name, file_name->file_name) == 0){
            struct LRU_node* tmpptr = sv->cache->head_least_used;
            sv->cache->head_least_used = tmpptr->next;
            sv->cache->tail_most_used->next = tmpptr;
            tmpptr->next = NULL;
            tmpptr->in_use++;
            sv->cache->tail_most_used = tmpptr;
            tmpptr = NULL;
            return 1;
         /*if what we're looking for is at the tail of the queue, do nothing, we're all good*/
        }else if (strcmp(sv->cache->tail_most_used->file_name, file_name->file_name) == 0){
            sv->cache->tail_most_used->in_use++;
            return 1;
        /*otherwise, our file is somewhere deeper in the LRU queue. Find it, unlink it, and put it at the tail*/    
        }else{
            struct LRU_node* prevptr = sv->cache->head_least_used;
            while(prevptr->next != NULL){
                if(strcmp(prevptr->next->file_name, file_name->file_name) == 0){
                    struct LRU_node* tmpptr = prevptr->next;
                    prevptr->next = prevptr->next->next;
                    sv->cache->tail_most_used->next = tmpptr;
                    tmpptr->next = NULL;
                    tmpptr->in_use++;
                    sv->cache->tail_most_used = tmpptr;
                    tmpptr = NULL;
                    prevptr = NULL;
                    return 1;
                }else{
                    prevptr = prevptr->next;
                }
            }
            // something when horribly wrong because the file wasn't in the queue;
            return 2;
        }
    /*Our hash_ret must have been 0 so all we have to do is create a new LRU node and add it to the end of the
     LRU queue*/
    }else{
        struct LRU_node* tmpptr = (struct LRU_node*)malloc(sizeof(struct LRU_node));
        tmpptr->file_name = strdup(file_name->file_name);
        tmpptr->file_size = file_name->file_size;
        tmpptr->in_use = 1;
        tmpptr->next = NULL;
        sv->cache->tail_most_used->next = tmpptr;
        sv->cache->tail_most_used = tmpptr;
        tmpptr = NULL;
        return 0;
    }
    return 0;
}

/*overall cache insert function that calls LRU insert and hash insert*/
int cache_insert(struct file_data* file, struct server* sv){
    int ret = hash_insert(file, sv);
    int ret2 = LRU_insert(file, sv, ret);
    return ret2;
}

/* static functions */

/*Initialize cache using wc_init setup function*/
struct cache* cache_init(int cache_size){
    struct cache* cache;
    cache = (struct cache* )malloc(sizeof(struct cache));
    assert(cache);
    
    cache->cache_head = (struct file** )malloc(cache_size*sizeof(struct file* ));
    assert(cache->cache_head);
    cache->cache_size = cache_size;
    cache->remaining_cache = cache_size;
    cache->head_least_used = NULL;
    cache->tail_most_used = NULL;
    return cache;
}

/*Destroy the cache so we don't waste memory*/
void cache_destroy(struct cache* cache){
    for (int i; i < cache->cache_size; i++){
        if(cache->cache_head[i] != NULL){
            while(cache->cache_head[i]->next != NULL){
                struct file* tmpptr;
                tmpptr = cache->cache_head[i];
                cache->cache_head[i] = cache->cache_head[i]->next;
                free(tmpptr->data->file_name);
                free(tmpptr->data->file_buf);
                free(tmpptr->data);
                free(tmpptr);
            }
            free(cache->cache_head[i]->data->file_buf);
            free(cache->cache_head[i]->data->file_name);
            free(cache->cache_head[i]->data);
            free(cache->cache_head[i]);
        }
    }
    
    free(cache->cache_head);
    
    if(cache->head_least_used != NULL){
        struct LRU_node* tmpptr;
        tmpptr = cache->head_least_used;
        while(tmpptr->next != NULL){
            cache->head_least_used = cache->head_least_used->next;
            free(tmpptr->file_name);
            tmpptr->next = NULL;
            free(tmpptr);
            tmpptr = cache->head_least_used;
        }
        tmpptr = NULL;
        cache->tail_most_used = NULL;
        free(cache->head_least_used->file_name);
        free(cache->head_least_used);
    }
   
    free(cache);
}
/* initialize file data */
static struct file_data *
file_data_init(void)
{
	struct file_data *data;

	data = Malloc(sizeof(struct file_data));
	data->file_name = NULL;
	data->file_buf = NULL;
	data->file_size = 0;
	return data;
}

/* free all file data */
static void
file_data_free(struct file_data *data)
{
	free(data->file_name);
	free(data->file_buf);
	free(data);
}

static void
do_server_request(struct server *sv, int connfd)
{
	int ret;
	struct request *rq;
	struct file_data *data;
                  struct file_data* cache_ret;
                  int insert_ret = -1;
                  int data_free = 1;
	data = file_data_init();

	/* fill data->file_name with name of the file being requested */
	rq = request_init(connfd, data);
	if (!rq) {
		file_data_free(data);
		return;
	}
                  
	/* read file, 
	 * fills data->file_buf with the file contents,
	 * data->file_size with file size. */
                  if(sv->max_cache_size > 0){
                      pthread_mutex_lock(sv->cache_lock);
                      cache_ret = cache_lookup(data->file_name, sv);
                      if(cache_ret == NULL){
                          pthread_mutex_unlock(sv->cache_lock);
                          
                          ret = request_readfile(rq);
                          
                          pthread_mutex_lock(sv->cache_lock);
                          insert_ret = cache_insert(data, sv);
                          if(insert_ret != 2){
                              LRU_using_plus(data->file_name, sv);
                          }
                          pthread_mutex_unlock(sv->cache_lock);
                      }else{
                          data_free = 0;
                          data->file_buf = cache_ret->file_buf;
                          data->file_size = cache_ret->file_size;
                          LRU_using_plus(data->file_name, sv);
                          pthread_mutex_unlock(sv->cache_lock);
                          
                          request_sendfile(rq);
                          
                          pthread_mutex_lock(sv->cache_lock);
                          LRU_using_minus(data->file_name, sv);
                          data->file_buf = NULL;
                          pthread_mutex_unlock(sv->cache_lock);
                          goto out;
                      }
                      if(ret == 0){
                          goto out;
                      }
                      
                      request_sendfile(rq);
                      
                      if(insert_ret != 2){
                          pthread_mutex_lock(sv->cache_lock);
                          LRU_using_minus(data->file_name, sv);
                          if(insert_ret == 0){
                              data = NULL;
                          }
                          pthread_mutex_unlock(sv->cache_lock);
                      }
                  }else{
                      ret = request_readfile(rq);
                      if(ret == 0) { /* couldn't read file */
                          goto out;
                      }
                      request_sendfile(rq); /*send file to client */
                  }
out:
	request_destroy(rq);
                  if(insert_ret != 0 && data_free != 0){
                      file_data_free(data);
                  }else if(data_free == 0){
                      free(data->file_name);
                      free(data);
                  }
                  
}

static void *
do_server_thread(void *arg)
{
	struct server *sv = (struct server *)arg;
	int connfd;

	while (1) {
		pthread_mutex_lock(&sv->mutex);
		while (sv->request_head == sv->request_tail) {
			/* buffer is empty */
			if (sv->exiting) {
				pthread_mutex_unlock(&sv->mutex);
				goto out;
			}
			pthread_cond_wait(&sv->cons_cond, &sv->mutex);
		}
		/* get request from tail */
		connfd = sv->conn_buf[sv->request_tail];
		/* consume request */
		sv->conn_buf[sv->request_tail] = -1;
		sv->request_tail = (sv->request_tail + 1) % sv->max_requests;
		
		pthread_cond_signal(&sv->prod_cond);
		pthread_mutex_unlock(&sv->mutex);
		/* now serve request */
		do_server_request(sv, connfd);
	}
out:
	return NULL;
}

/* entry point functions */

struct server *
server_init(int nr_threads, int max_requests, int max_cache_size)
{
	struct server *sv;
	int i;

	sv = Malloc(sizeof(struct server));
	sv->nr_threads = nr_threads;
	/* we add 1 because we queue at most max_request - 1 requests */
	sv->max_requests = max_requests + 1;
	sv->max_cache_size = max_cache_size;
	sv->exiting = 0;

	/* Lab 4: create queue of max_request size when max_requests > 0 */
	sv->conn_buf = Malloc(sizeof(*sv->conn_buf) * sv->max_requests);
	for (i = 0; i < sv->max_requests; i++) {
		sv->conn_buf[i] = -1;
	}
	sv->request_head = 0;
	sv->request_tail = 0;

	/* Lab 5: init server cache and limit its size to max_cache_size */
                  sv->cache_lock = (pthread_mutex_t* )malloc(sizeof(pthread_mutex_t));
                  pthread_mutex_init(sv->cache_lock, NULL);
                  if(max_cache_size > 0){
                        sv->cache = cache_init(max_cache_size);
                  }
	/* Lab 4: create worker threads when nr_threads > 0 */
	pthread_mutex_init(&sv->mutex, NULL);
	pthread_cond_init(&sv->prod_cond, NULL);
	pthread_cond_init(&sv->cons_cond, NULL);	
	sv->threads = Malloc(sizeof(pthread_t) * nr_threads);
	for (i = 0; i < nr_threads; i++) {
		SYS(pthread_create(&(sv->threads[i]), NULL, do_server_thread,
				   (void *)sv));
	}
	return sv;
}

void
server_request(struct server *sv, int connfd)
{
	if (sv->nr_threads == 0) { /* no worker threads */
		do_server_request(sv, connfd);
	} else {
		/*  Save the relevant info in a buffer and have one of the
		 *  worker threads do the work. */

		pthread_mutex_lock(&sv->mutex);
		while (((sv->request_head - sv->request_tail + sv->max_requests)
			% sv->max_requests) == (sv->max_requests - 1)) {
			/* buffer is full */
			pthread_cond_wait(&sv->prod_cond, &sv->mutex);
		}
		/* fill conn_buf with this request */
		assert(sv->conn_buf[sv->request_head] == -1);
		sv->conn_buf[sv->request_head] = connfd;
		sv->request_head = (sv->request_head + 1) % sv->max_requests;
		pthread_cond_signal(&sv->cons_cond);
		pthread_mutex_unlock(&sv->mutex);
	}
}

void
server_exit(struct server *sv)
{
	int i;
	/* when using one or more worker threads, use sv->exiting to indicate to
	 * these threads that the server is exiting. make sure to call
	 * pthread_join in this function so that the main server thread waits
	 * for all the worker threads to exit before exiting. */
	pthread_mutex_lock(&sv->mutex);
	sv->exiting = 1;
	pthread_cond_broadcast(&sv->cons_cond);
	pthread_mutex_unlock(&sv->mutex);
	for (i = 0; i < sv->nr_threads; i++) {
		pthread_join(sv->threads[i], NULL);
	}

	/* make sure to free any allocated resources */
	cache_destroy(sv->cache);
                  free(sv->cache_lock);
                  free(sv->conn_buf);
	free(sv->threads);
	free(sv);
}
