#include "sorted-list.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * Initialize a sorted list.
 *
 * \param lst This is a pointer to space that should be initialized as a sorted list. The caller of
 * this function retains ownership of the memory that lst points to (meaning the caller must free it
 * if the pointer was returned from malloc)
 */
void sorted_list_init(sorted_list_t* lst) {
  lst->head = NULL;
  
}

/**
 * Destroy a sorted list. Free any memory allocated to store the list, but not the list itself.
 *
 * \param lst This is a pointer to the space that holds the list being destroyred. This function
 * should free any memory used to represent the list, but the caller retains ownership of the lst
 * pointer itself.
 */
void sorted_list_destroy(sorted_list_t* lst) {
  int_node_t *current = lst->head; 
  int_node_t *prev = lst->head;

  while(current != NULL){ 
    
    current = current->next;
    free(prev);

  }
}
/**
 * Checks if a sorted list is empty, returns 1 if empty, 0 otherwise
 *
 * \param lst   The list that is being inserted to
 */
int isEmpty(sorted_list_t* lst){
  return lst->head == NULL;
}

/**
 * Add an element to a sorted list, maintaining the lowest-to-highest sorted order.
 *
 * \param lst   The list that is being inserted to
 * \param value The value being inserted
 */

void sorted_list_insert(sorted_list_t* lst, int value) {
  
  //initialize a Node
  int_node_t *newNode = malloc(sizeof(int_node_t));
  newNode->data = value;
  
  //if our list is empty, put the new Node at the head and return
  if(isEmpty(lst)){
    
    lst->head = newNode;
    return;
  }
  int_node_t *current = lst->head;

  while(current != NULL){  //Traverse  the list until we reach the end
    
    
    if(value == current->data){ //If we reach a Node whose value is equal to our new value
      
      newNode->next = current->next; // Insert our new Node into the list ahead of current and return
      current->next = newNode;
      return;
    }
    if(value > current->data){ //If we reach a Node whose value is less than our new value
      int_node_t *prev = current;

      while(current != NULL && value > current->data){ //Traverse the list until we reach a value for which this is not the case
        prev = current;
        current = current->next; 
      }
      
      newNode->next = current; // Insert our new Node into the list behind current and return
      prev->next = newNode;
      return;
    }
    current = current->next; //go to next node in the list
  }
  //If the value to insert was never greater than or equal to anye of the other values
  //It must have been less than all other values, and therefore should be at the head.
  newNode->next = lst->head; // Insert our new Node at the head of the list
  lst->head = newNode; 

}

/**
 * Count how many times a value appears in a sorted list.
 *
 * \param lst The list being searched
 * \param value The value being counted
 * \returns the number of times value appears in lst
 */

 
int sorted_list_count(sorted_list_t* lst, int value) {
  
  int count = 0;
  int_node_t *current = lst->head;

  while (current != NULL){ //Traverse  the list until we reach the end

    if(current->data == value){ //If we locate our target value

      while(current != NULL && current->data == value){ //Traverse  the list until we are past said value
        count ++; //increment count
        current = current->next; //go to next node in the list
      }

      return count; // Once we are past our target value, return
    }

    current = current->next; //go to next node in the list
  }
 
  return count;
}

/**
 * Print the values in a sorted list in ascending order, separated by spaces and followed by a
 * newline.
 *
 * \param lst The list to print
 */
void sorted_list_print(sorted_list_t* lst) {
  int_node_t *current = lst->head;

  while (current != NULL){ //Traverse  the list until we reach the end
    printf("%d ", current->data);  // Print the current element
    current = current->next; //go to next node in the list
  }
  printf("\n");
  
}

