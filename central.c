#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <dirent.h>
#include <fcntl.h>

//#include "file_commands.c"
#include "message.h"
#include "socket.h"

pthread_mutex_t central_lock = PTHREAD_MUTEX_INITIALIZER;
char* dir_path;
#define INITIAL_SIZE 10
#define MAX_PATH_LENGTH 100
#define RECEIVE_ID 0
#define CREATE_ID 1
#define CHECKOUT_ID 2
#define DELETE_ID 3
#define DISPLAY_ALL_ID 4
#define DOWNLOAD_ID 5
#define PRINT_CONTENTS_ID 6
#define RETURN_ID 7
#define STATUS_ID 8



//Setup for an array of nodes to connect between each other for our chat
int connections_current_index=0;
int * connections;
int connections_max_size = INITIAL_SIZE;


//Doubles array size to increase maximum number of clients in server.
void double_arr_size(){
  //Replacement array, with check for if creation failed.
  int *new_array = (int *)malloc(2* connections_max_size*sizeof(int));

  if(new_array==NULL){
    perror("Malloc failed\n");
    exit(1);
  }

  //Adds items from old array into new one
  for(int i=0; i<connections_max_size;i++){
    new_array[i] = connections[i];
  }

  //Delete old array, return new one
  free(connections);

  connections = new_array;
  connections_max_size *= 2;
}

//misc signatures
void* messages_thread(void* arg);
void receive_file(int fd);
void display_all_files(int fd);
void delete_file(int fd);
void remove_file(char * filename);
void create_file(int fd);
void central_send_file(int fd);
void print_file(int fd);
void checkout_file(int fd);
void get_file_status(int fd);
void return_file(int fd);
//Sets up thread to establish connections
void* connections_thread(void* arg) {
    int* server_socket_fd = (int*) arg;
    
    while(1){
      //check for connections
      
      int client_socket_fd = server_socket_accept(*server_socket_fd);
      if (client_socket_fd==-1) {
        perror("accept failed");
        exit(EXIT_FAILURE);
      }      
      //add new connection to the array and create a thread managing the sending and recieving of messages for it.
      pthread_mutex_lock(&central_lock);
      if(connections_current_index+1 > connections_max_size){
        double_arr_size();
      }
      connections[connections_current_index]=client_socket_fd;
      
      //Create thread for new connection, add to connections array
      pthread_t message_thread;
      if (pthread_create(&message_thread, NULL, messages_thread, (void*) &connections[connections_current_index])) {
        perror("pthread_create failed");
        exit(EXIT_FAILURE);
      }
      connections_current_index++;        
      pthread_mutex_unlock(&central_lock);
      char welcome_msg[25];
      sprintf(welcome_msg, "Welcome, your id is: %d", client_socket_fd);
      send_message(client_socket_fd, welcome_msg);
    }
    return NULL;
}

 

//Creates threads to send and receive messages between connections. Takes in a fd
void* messages_thread(void* arg) {
  int * fd = (int*) arg;
  char * inc_message=NULL;

  while(1){
    //Reads in name and message to be printed out - if user disconnects prints a message.
    inc_message = receive_message(*fd); 
    if (inc_message ==NULL) {
      *fd = -1;
      printf("User left!\n");
      return NULL;
    }
    switch(atoi(inc_message)){
      case RECEIVE_ID: //upload file request
        receive_file(*fd);
        break;
      case CREATE_ID: //create file request
        create_file(*fd);
        break;
      case CHECKOUT_ID: //checkout file request
        checkout_file(*fd);
        break;
      case DELETE_ID: //delete file request
        delete_file(*fd);
        break;
      case DISPLAY_ALL_ID: //display all files request
        display_all_files(*fd);
        break;
      case DOWNLOAD_ID: //download file request
        central_send_file(*fd);
        break;
      case PRINT_CONTENTS_ID: //print file request
        print_file(*fd);
        break;  
      case RETURN_ID: //return file request
        return_file(*fd);
        break;
      case STATUS_ID: //return file request
        get_file_status(*fd);
        break;    
      default:
        printf("Warning: Received invalid id\n");
    }
    
    free(inc_message);
  }
}

char* get_lock_name(char* full_path){
  //locate the file extension
  char* extension_loc = strrchr(full_path, '.');
  if(extension_loc==NULL){
    perror("invalid file name");
    return NULL;
  }
  // create a string that is filename_lock.txt
  int extension_index = extension_loc - full_path;
  char* lock_file_path = malloc(sizeof(char)*MAX_PATH_LENGTH);
  strncpy(lock_file_path, full_path, extension_index);
  strcpy(lock_file_path+extension_index,"_lock.txt");
  return lock_file_path;
}

void checkout_file(int fd){
  pthread_mutex_lock(&central_lock);

  //recieve the filename
  char* filename = receive_message(fd);
  char full_path[MAX_PATH_LENGTH]; 
  strcpy(full_path, dir_path);
  strcat(full_path, "/");
  strcat(full_path, filename);


  char * lock_file_path= get_lock_name(full_path);
  //printf("%s\n", lock_file_path);
  FILE *fp = fopen(lock_file_path, "r");
  if (fp == NULL){
    perror("Error opening file");
    pthread_mutex_unlock(&central_lock);

    return;
  }

  char* rest1;

  char line1[20];
  if(fgets(line1, sizeof(line1), fp)==NULL){
    perror("lock file invalid");
    pthread_mutex_unlock(&central_lock);

    return;
  }
  char * token  = strtok_r(line1, " ", &rest1);
  if(atoi(token) == 1){
    send_message(fd,"File is checked out right now.");
  }
  else{
    fclose(fp);
    fp =  fopen(lock_file_path, "w");
    fprintf(fp,"1 %d", fd); //checkout the file
    printf("file has been checked out by user %d\n", fd);
    send_message(fd,"file has been checked out");

    fclose(fp);
    central_send_file(fd);

  }
  pthread_mutex_unlock(&central_lock);

  return;

}

void return_file(int fd){
  char* filename = receive_message(fd);
  char full_path[MAX_PATH_LENGTH]; 
  strcpy(full_path, dir_path);
  strcat(full_path, "/");
  strcat(full_path, filename);

  char * lock_file_path= get_lock_name(full_path);

  FILE *fp = fopen(lock_file_path, "r");
  if (fp == NULL){
    perror("Error opening file");
    return;
  }
  char* rest1;

  char line1[20];
  if(fgets(line1, sizeof(line1), fp)==NULL){
    perror("lock file invalid");
    return;
  }
  char * token  = strtok_r(line1, " ", &rest1);
  int checkout =atoi(token);
  if(checkout==0){
    send_message(fd, "This file is not checked out.");
  }
  else{
    token  = strtok_r(NULL, " ", &rest1);
    if(atoi(token)!=fd){
      send_message(fd, "Someone else checked this file out!");
    }
    else{
      send_message(fd, "<send it over");
      receive_file(fd);
      send_message(fd, "File successfully returned.");
      //close fp and reopen in write mode to reset it
      fclose(fp);
      
      
    }
  }
}

//Given a particular file, print out whether or not it's checked out
void get_file_status(int fd){
  char* filename = receive_message(fd);
  char full_path[MAX_PATH_LENGTH]; 
  strcpy(full_path, dir_path);
  strcat(full_path, "/");
  strcat(full_path, filename);

  char* lock_file_path= get_lock_name(full_path);

  FILE *fp = fopen(lock_file_path, "r");
  if (fp == NULL){
    perror("Error opening file");
    return;
  }
  char* rest1;

  char line1[20];
  if(fgets(line1, sizeof(line1), fp)==NULL){
    perror("lock file invalid");
    return;
  }
  char * token  = strtok_r(line1, " ", &rest1);
  int checkout =atoi(token);
  if(checkout==0){
    send_message(fd, "This file is not checked out.");
  }
  else{
    token  = strtok_r(NULL, " ", &rest1);
    char check_msg[100];
    sprintf(check_msg,"This file is checked out by user %s", token);
    send_message(fd, check_msg);
  }
}


//deletes a file and its lock file from the server.
void delete_file(int fd){
  char* filename = receive_message(fd);
  char full_path[MAX_PATH_LENGTH]; 
  strcpy(full_path, dir_path);
  strcat(full_path, "/");
  strcat(full_path, filename);

  if(remove(full_path)==0){
    send_message(fd, "removed file successfully");
  }
  else{
    send_message(fd, "file does not exist/couldn't remove file");
  }

  char* lock_file_path= get_lock_name(full_path);
  if(remove(lock_file_path)==0){
    send_message(fd, "removed lock file successfully");
  }
  else{
    send_message(fd, "lock file does not exist/couldn't remove file");
  }
}

//recieves a file from the fd to write into the directory, overwriting any existing file with the same name. 
void receive_file(int fd) {
  
  //receive the filesize
  char* file_size = receive_message(fd);
  if (file_size ==NULL) {
    fd = -1;
    printf("User left!\n");
    
  }
  int f_size = atoi(file_size);
  
  //receive the filename
  char* filename = receive_message(fd);

  if (filename ==NULL) {
    fd = -1;
    printf("User left!\n");
    
  }
  //create the path
  char full_path[MAX_PATH_LENGTH];
  strcpy(full_path, dir_path);
  strcat(full_path, "/");
  strcat(full_path, filename);
  
  //create new file to be written to
  FILE *received_file = fopen(full_path,"w");
  if(received_file==NULL){
    perror("couldn't open file");
    exit(1);
  }
  int data_remaining = f_size;
  //receive the data
  int bytes_received;
  char buffer[BUFSIZ];
  while ((data_remaining > 0) && ((bytes_received = recv(fd, buffer, BUFSIZ, 0)) > 0)){

    fwrite(buffer, sizeof(char), bytes_received, received_file);
    data_remaining -= bytes_received;
  }
  //close the file
  fclose(received_file);  
  printf("Received file: %s\n", full_path);
  
  //create the lock file
  char* lock_file_path= get_lock_name(full_path);
  FILE *lock_file = fopen(lock_file_path, "w");
      
  if (lock_file == NULL) {
    // If file creation failed
    perror("Error creating file");
    return;

    // If file creation succeeded, print messages and close the file
  }
  fputs("0", lock_file); //set checkout to 0.
  //printf("Lock file created successfully: %s\n", lock_file_path);
  fclose(lock_file);
  send_message(fd, "received file successfully");
}
//send the names of all files in storage to a given user
void display_all_files(int fd){
  
  struct dirent * current;
  DIR * dir = opendir(dir_path);

  if(dir==NULL){
    perror("Error in display, couldn't open\n");
    exit(1);
  }
  send_message(fd, "");

  while((current = readdir(dir))!=NULL){
    if(current->d_type == DT_REG){
      send_message(fd, current->d_name);
    }
  }
  printf("display_all complete \n");
}

//prints contents of a file for the user to see.
// NOTE: should only be used on text files.
void print_file(int fd){
  char full_path[MAX_PATH_LENGTH]; 
  char *filename = receive_message(fd);
  //get full path
  strcpy(full_path, dir_path);
  strcat(full_path, "/");
  strcat(full_path, filename);
  FILE * file = fopen(full_path, "r");
  if(file==NULL){
    perror("Couldn't open file");
    return;
  }
  //while we haven't reached the end of the file, send the file's contents to the user
  char buffer[BUFSIZ];
  while(fgets(buffer,sizeof(buffer), file) !=NULL){
    send_message(fd, buffer);
  }
  fclose(file);
}

//creates a file. Filename received as message from given fd.
void create_file(int fd ) {

    char *filename = receive_message(fd);
    // Concatenate folder path and filename to get the full path
    char full_path[MAX_PATH_LENGTH]; 
    strcpy(full_path, dir_path);
    strcat(full_path, "/");
    strcat(full_path, filename);

   
    // create a string that is filename_lock.txt
    char* lock_file_path= get_lock_name(full_path);
    // Try to open the file for writing
    if (access(full_path, F_OK) == 0) {
      printf("This file already exists, pick a different name\n");
      return;
    }
    else{ //make the file
      FILE *file = fopen(full_path, "w");
      FILE *lock_file = fopen(lock_file_path, "w");
      
      if (file == NULL || lock_file == NULL) {
          // If file creation failed
          perror("Error creating file");
          return;
      } else {
          // If file creation succeeded, print messages and close the file

          fputs("0", lock_file); //set checkout to 0.

          printf("File created successfully: %s\n", full_path);
          printf("Lock file created successfully: %s\n", lock_file_path);
          fclose(file);
          fclose(lock_file);
          return;
      }
    
    }
    
}

//sends a file from the central server to the user whose fd is specified in arg
//filename and location are learned via message from the user
void central_send_file(int fd){
  

  char* filename = receive_message(fd);
  if(filename==NULL){
    perror("error receiving filename");
    return;
  }
  char* dest_path = receive_message(fd);
  if(dest_path==NULL){
    perror("error receiving dest_path");
    return;
  }
  //get source path
  char source_full_path[MAX_PATH_LENGTH]; 
  strcpy(source_full_path, dir_path);
  strcat(source_full_path, "/");
  strcat(source_full_path, (char*)filename);

  //get dest path
  char dest_full_path[MAX_PATH_LENGTH]; 
  strcpy(dest_full_path, dest_path);
  strcat(dest_full_path, "/");
  strcat(dest_full_path, (char*)filename);

  //open up the file
  FILE *fp = fopen(source_full_path, "r");
  if(fp==NULL){
    perror("Couldn't open file\n");
    return;
  }
  
  int file_fd = fileno(fp);
  if (file_fd == -1){
    perror("Error opening file ");
    return;
  }

  // Get file stats 
  struct stat f_stats;
  if (fstat(file_fd, &f_stats) < 0){
    perror("Error obtaining fstat ");
    return;
  }

  int out;

  //send code
  out = send_message(fd, "%");
  if (out != 0){
    perror("Error sending code");
    return;
  }
  char file_size[256];
  sprintf(file_size, "%ld", f_stats.st_size);
  
  // Send file size 
  out = send_message(fd, file_size);
  if (out != 0){
    perror("Error sending file_size");
    return;
  }
  // Send file path
  out = send_message(fd, dest_full_path);
  if (out != 0){
    perror("Error sending file_size");
    return;
  }
  //send data from our file_fd to the socket_fd
  long offset = 0;
  int data_remaining = f_stats.st_size;
  // Send file data     
  int sent_bytes;
  while (((sent_bytes = sendfile(fd, file_fd, &offset, BUFSIZ)) > 0) && (data_remaining > 0)){
    data_remaining -= sent_bytes;

  }
  printf("sending finished\n");
  fclose(fp);
}

// void copy_file( char *source_path) {f
//     // Get the filename from the source_path
//     char *filename = strrchr(source_path, '/');
//     if (filename == NULL) {
//         filename = source_path; // No path separator found, use the whole path as filename
//     } else {
//         filename++; // Move past the path separator
//     }

//     // Create the full destination path
//     char destination_path[MAX_PATH_LENGTH]; 
//     strcpy(destination_path, dir_path);
//     strcat(destination_path, "/");
//     strcat(destination_path, filename);

//     // Open and read the source file
//     FILE *source_file = fopen(source_path, "rb");
//     if (source_file == NULL) {
//         perror("Couldn't open source file");
//         return;
//     }

//     // Open the destination file
//     FILE *destination_file = fopen(destination_path, "w");
//     if (destination_file == NULL) {
//         perror("Couldn't open destination file");
//         fclose(source_file);
//         return;
//     }

//     // Copy the contents of the source to the destination 
//     int ch;
//     while ((ch = fgetc(source_file)) != EOF) {
//         fputc(ch, destination_file);
//     }

//     // Close the files
//     fclose(source_file);
//     fclose(destination_file);
//     printf("File copied from %s to %s\n", source_path, destination_path);
// }

int main(int argc, char** argv) {
  connections =malloc(sizeof(int)*connections_max_size);
  if (argc != 2 && argc != 4) {
    fprintf(stderr, "Usage: %s <directory path> <peer> <port number>, OR %s <directory path>\n", argv[0], argv[0]);
    return 0;
  }
  
  dir_path = argv[1];  

  //check if storage dir exists
  DIR *dir = opendir(dir_path);
  if(dir){
    printf("Directory found. Files will be stored in %s\n", dir_path);
    closedir(dir);
  }
  else if(ENOENT ==errno){ //give user option of creating dir if it doesn't exist.
    printf("Directory does not exist. Create new one? [y]/[n] ");
    char inp[MAX_PATH_LENGTH];
    
    while(1){
      fgets(inp, sizeof(inp), stdin);
      inp[strcspn(inp, "\n")] = '\0';

      if(strcmp(inp, "y") == 0){
        mkdir(dir_path, 0700);
        printf("Directory created. Files will be stored in %s\n", dir_path);
       
        break;
      }
      else if (strcmp(inp, "n") == 0){
        return 0;
      }
    }
  }


  //initialize and open a port.
  unsigned short port = 0;

  //Initialize connections array
  for(int i = 0; i< connections_max_size; i++){
    connections[i] = 0;
  }

  // Set up a server socket to accept incoming connections
  int server_socket_fd = server_socket_open(&port);
  if (server_socket_fd == -1) {
    perror("Server socket was not opened");
    exit(EXIT_FAILURE);
  }

  if (listen(server_socket_fd, 1)) {
    perror("listen failed");
    exit(EXIT_FAILURE);

  }
  
  //Creates our first thread to start connecting
  pthread_t connection_thread;
  if (pthread_create(&connection_thread, NULL, connections_thread, (void*) &server_socket_fd)) {
    perror("pthread_create failed");
    exit(EXIT_FAILURE);
  }

  printf("Port Number: %d\n", port);
  printf("Server active. Type exit to quit.\n");
  
  char inp[MAX_PATH_LENGTH];
  char* rest;
  
  //keep the server up until 'exit' is input into the console
  while(1){

    fgets(inp, sizeof(inp), stdin);
    inp[strcspn(inp, "\n")] = '\0';
    
    if(strcmp(inp, "exit") == 0){
      break;
    }
  }

  pthread_mutex_destroy(&central_lock);
  free(connections);

  return 0;
}

