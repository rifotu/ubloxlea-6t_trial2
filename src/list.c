#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include "list.h"


// private structures
typedef struct lnode
{
  struct lnode* prev;
  struct lnode* next;
  void *data;
} node;

// private functions
static node* create_node(void *data);



/** create list 
 * Allocate memory on heap
 * initialize parameters   */
list* create_list(void)
{
  list *l = malloc(sizeof(list));
  l->head = NULL;
  l->size = 0;
  pthread_mutex_init(&(l->mutex),NULL);
  return l;
}



/* Create node 
 * Create a node with the provided data
 * return a pointer to the created node  */
static node* create_node(void* data)
{
  node *n = malloc(sizeof(node));
  n->data = data;
  n->prev = NULL;
  n->next = NULL;
  return n;
}



/* push_front
 * Add incoming data to the linked list,
 * entry is from back  */
void push_front(list* llist, void* data)
{
  node *n = create_node(data);

  pthread_mutex_lock(&(llist->mutex));

  // if the list size is 0 then next and prev nodes
  // point to the current node
  if(!llist->size){
    n->next = n;
    n->prev = n;
  } else {
    node *head = llist->head;
    node *prev = head->prev;

    // set the new nodes next and prev pointers
    n->next = head;
    n->prev = head->prev;

    // set the prev and next pointers to the new node
    head->prev = n;
    prev->next = n;

  }

  // set the prev and next pointers to the new node
  llist->head = n;
  llist->size++;
  pthread_mutex_unlock(&(llist->mutex));

}



/* push back
 * Add incoming data to the linked list
 * entry is from back  */
void push_back(list* llist, void *data)
{
  node *n = create_node(data);

  pthread_mutex_lock(&(llist->mutex));
  //if the list size is 0
  if(!llist->size){
    // set the next and prev to the new node
    n->next = n;
    n->prev = n;

    // since there are no other nodes, assign new head
    llist->head = n;
  } else {

    node *head = llist->head;
    node *prev = head->prev;

    // insert the new node to the back of the list
    n->next = head;
    n->prev = head->prev;

    // update the next and prev pointers to the current node
    head->prev = n;
    prev->next = n;
  }

  llist->size++;
  pthread_mutex_unlock(&(llist->mutex));
}



/* remove front
 * Remove the node at the front of the linked list
 */
int remove_front(list *llist, list_op free_func)
{

  pthread_mutex_lock(&(llist->mutex));

  if(!llist->size){
    pthread_mutex_unlock(&(llist->mutex));
    return -1;  // Nothing to remove, list is already empty
  }

  node *head = llist->head;

  if(llist->size == 1){
    // if the size is 1, set the head to NULL 
    llist->head = NULL;
  } else {
    node *next = head->next;
    node *prev = head->prev;

    // update the head
    llist->head = next;

    // update the pointers
    next->prev = prev;
    prev->next = next;
  }

  // free the data and the node
  free_func(head->data);
  free(head);

  llist->size--;
  pthread_mutex_unlock(&(llist->mutex));

  return 0;
}




/* remove back
 * Removes the node at the back of the linked list
 */
int remove_back(list* llist, list_op free_func)
{


  pthread_mutex_lock(&(llist->mutex));
  if(!llist->size){
    pthread_mutex_unlock(&(llist->mutex));
    return -1;  // Nothing to remove, list is already empty
  }

  node *head = llist->head;
  node *tbr  = head->prev;  // to be removed
  node *nb   = tbr->prev;   // new node to act as the back

  // if there is only a single node
  if(llist->size == 1){

    llist->head = NULL;    // make the head null

  } else {
    // update the pointers 
    head->prev = nb;
    nb->next = head;
  }

  // free the data and the node
  free_func(tbr->data);
  free(tbr);

  llist->size--;
  pthread_mutex_unlock(&(llist->mutex));

  return 0;
}




/* front
 * Get the data at the front of the linked list.
 * If the list is empty return NULL
 */
void* front(list* llist)
{
  uint8_t *buf = NULL;

  pthread_mutex_lock(&(llist->mutex));
  if(llist->size){
    // if the list is not empty, return the data at the head
    buf = llist->head->data;

  } else {
    // list is empty
    buf = NULL;
  }
  pthread_mutex_unlock(&(llist->mutex));
  return (void *)buf;
}




/* back
 * Get the data at the back of the linked list.
 * If the list is empty return NULL
 */
void *back(list* llist)
{
  uint8_t *buf = NULL;

  pthread_mutex_lock(&(llist->mutex));
  // if the list is empty, simply return NULL
  if(!llist->size){
    buf = NULL;
  } else {
      // list is not empty, return the back node's data
    buf = llist->head->prev->data;
  }
  pthread_mutex_unlock(&(llist->mutex));
  return (void *)buf;
}



/* is_empty
 * Check whether the list is empty or not
 */
int is_empty(list* llist)
{
  int res = 1;

  pthread_mutex_lock(&(llist->mutex));
  if(llist->size == 0 && llist->head == NULL){
    res = 1;
  } else {
    res = 0;
  }
  pthread_mutex_unlock(&(llist->mutex));
  return res;
}



/* size
 * Get the size of the linked list
 */
int size(list *llist)
{
  int size = 0;
  pthread_mutex_lock(&(llist->mutex));
  size = llist->size;
  pthread_mutex_unlock(&(llist->mutex));
  return size;
}




/* empty_list
 * Empties the linked list. 
 */
void empty_list(list *llist, list_op free_func)
{
  pthread_mutex_lock(&(llist->mutex));

  if(!llist->size){

    pthread_mutex_unlock(&(llist->mutex));
    return;  // simply return if the size is already 0 

  } else {

    node *current = llist->head;
    node *next    = current->next;

    // loop through the list and free all the nodes
    for(int i=0; i < llist->size; i++){
      free_func(current->data);
      free(current);
      current = next;

      // if this is not the last node, then get the next node's address
      if(i < llist->size-1) next = current->next;
    }

    // reset the head and size
    llist->head = NULL;
    llist->size = 0;
  }

  pthread_mutex_unlock(&(llist->mutex));

}

/* get index of the node that has a cumulative data length 
   greater than "len"    */
int payloadLen_2_index(list *llist, int len )
{
    uint8_t *array = NULL;
    int sum,i = 0;
    int index = -1;
    pthread_mutex_lock(&(llist->mutex));

    node *current = llist->head;

    if(len <= 0){  // don't accept negative numbers
        printf("negative number to payloadLen_2_index\n");
        
    }else{

       for(i = 0; i < llist->size; i++){
           array = (uint8_t *)(current->data);
           sum +=  array[0];
           //printf("sum: %d target length: %d\n", sum, len);
           current = current->next;
           if(sum >= len){ 
               index = i;
               break; 
           }
       }
    }
    pthread_mutex_unlock(&(llist->mutex));
    return index;
}
        

/* traverse
 * traverses the linked list, calling a function on each node's data
 */
void traverse(list* llist, list_op do_func)
{
  pthread_mutex_lock(&(llist->mutex));

  node *current = llist->head;
  for(int i = 0; i < llist->size; i++){
    do_func(current->data);
    current = current->next;
  }
  pthread_mutex_unlock(&(llist->mutex));

}









