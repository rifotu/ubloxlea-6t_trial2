#ifndef LIST_H
#define LIST_H

#include <pthread.h>

/* linked list struct. Has a head pointer */
#pragma pack(1)
typedef struct llist
{
  struct lnode *head;
  unsigned int size;
  pthread_mutex_t mutex;

}list;
#pragma pack()

/* Function pointer types to be passed to linked list library functions  */
typedef void (*list_op)(void *);
typedef int (*list_pred)(const void *);
typedef int (*equal_op)(const void*, const void*);


/*  Prototypes for linked list library functions */

// Create a list 
list* create_list(void);

// Add data to list
void push_front(list* llist, void* data);
void push_back(list* llist, void* data);

// Removing elements from list
int remove_front(list* llist, list_op free_func);
int remove_index(list* llist, int index, list_op free_func);
int remove_back(list* llist, list_op free_func);

// Querying through the list
void* front(list* llist);
void* back(list* llist);
int is_empty(list* llist);
int size(list* llist);

// Freeing stuff
void empty_list(list* llist, list_op free_func);

// get total payload size
int payloadLen_2_index(list *llist, int len );

// Traversal
void traverse(list* llist, list_op do_func);

#endif
