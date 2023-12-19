#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sorted-list.h"

int main(int argc, char** argv) {
  // Set up and initialize a sorted list
  sorted_list_t lst;
  sorted_list_init(&lst);

  // Read lines until the end of stdin
  char* line = NULL;
  size_t line_size = 0;
  while (getline(&line, &line_size, stdin) != EOF) {
    int num;

    // Which command is this?
    if (strcmp(line, "done\n") == 0) {
      // The user is done so we can exit
      break;

    } else if (strcmp(line, "print\n") == 0) {
      // Print the list
      sorted_list_print(&lst);

    } else if (sscanf(line, "insert %d\n", &num) == 1) {
      // Add a value to the list
      sorted_list_insert(&lst, num);

    } else if (sscanf(line, "count %d\n", &num) == 1) {
      // How many times does the provided value appear in the list?
      size_t count = sorted_list_count(&lst, num);
      printf("%lu\n", count);

    } else {
      printf("unrecognized command.\n");
    }
  }

  // Free the space allocated by getline
  free(line);

  // Clean up the queue
  sorted_list_destroy(&lst);

  return 0;
}
