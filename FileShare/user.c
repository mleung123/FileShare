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

pthread_mutex_t user_lock = PTHREAD_MUTEX_INITIALIZER;
char* user_dir_path;
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

//Global variable holding fd for our connection to the server
int connection;
char* return_file_name;
void receive_file(void* arg) ;
void user_send_file(void* filename);

//Creates threads to send and receive messages between connections. Takes in a fd
void* messages_thread(void* arg) {
  int * fd = (int*) arg;
  char * inc_message=NULL;

  char buffer[BUFSIZ];
  FILE *received_file;

  while(1){
    //Reads in name and message to be printed out - if user disconnects prints a message.
    inc_message = receive_message(*fd); 
    
    if (inc_message ==NULL) {
      *fd = -1;
      printf("User left!\n");
      return NULL;
    }
    else if(strcmp(inc_message, "%")==0){ //download file request
      receive_file(fd);
      continue;
    }
    else if(strcmp(inc_message, "<send it over")==0){ //download file request
      user_send_file(return_file_name);
      continue;
    }

    //for every single connection, send the message to the user on the other end of that connection.
    
    // Print the message
    printf("%s\n", inc_message);
    free(inc_message);
  }
}


// Sends a file to the central server
// assume that the file is in the current working directory.
void user_send_file(void* filename){
  char file_size[256];
  //open the file
  FILE *fp = fopen(filename, "r");
  if (fp == NULL){
    perror("Error opening file");
    return;
  }
  int file_fd = fileno(fp);
  // Get file stats 
  struct stat f_stats;
  if (fstat(file_fd, &f_stats) < 0){
    perror("Error obtaining fstat");
    return;
  }
  //set file size
  sprintf(file_size, "%ld", f_stats.st_size);

  

  // Send file size 
  int out = send_message(connection, file_size);
  if (out != 0){
    perror("Error sending file_size");
    return;
  }
  // Send file name
  out = send_message(connection, filename);
  if (out != 0){
    perror("Error sending file_size");
    return;
  }

  //send data from our file_fd to the socket_fd
  long offset = 0;
  int data_remaining = f_stats.st_size;
  // Send file data     
  int sent_bytes;
  while (((sent_bytes = sendfile(connection, file_fd, &offset, BUFSIZ)) > 0) && (data_remaining > 0)){
    //printf("Server sent %d bytes from file's data, offset is now : %ld and remaining data = %d\n", sent_bytes, offset, data_remaining);
    data_remaining -= sent_bytes;
  }
  fclose(fp);
}
void send_file_request(char *filename){
  //send code
  send_message(connection, "0");
  user_send_file(filename);
}
// sends request to central server asking it to create a file with the given name.
void create_file_request( char *filename) {
  send_message(connection, "1");
  send_message(connection, filename);
}

// sends request to central server asking it to check out a given file
void checkout_file_request(char *filename, char* destination){
  send_message(connection, "2");
  send_message(connection, filename);
  send_message(connection, filename);
  send_message(connection, destination);
}

// sends request to central server asking it to delete a given file
void delete_file_request(char *filename){
  send_message(connection, "3");
  send_message(connection, filename);
}

// sends request to central server asking it to send names of all files in directory
void display_all_request(){
  send_message(connection, "4"); 
}
// sends request to central server asking it to download a particular file
void download_file_request(char* filename, char* dir_path){
  send_message(connection, "5");
  send_message(connection, filename);
  send_message(connection, dir_path);
}

// sends request to central server asking it to print a particular file
void print_file_request(char *filename){
  send_message(connection, "6");
  send_message(connection, filename);
}
// sends request to central server asking to return a particular file
void return_file_request(char *filename){
  send_message(connection, "7");
  return_file_name = filename;
  printf("return_file_name: %s\n", return_file_name);
  send_message(connection, filename);

}
// sends request to central server asking it to print a file's status
void status_file_request(char *filename){
  send_message(connection, "8");
  send_message(connection, filename);
}

// receive a file from the central server, and create it in a location specified in return_file_request.
void receive_file(void* arg) {
  int * fd = (int*) arg;
  char * inc_message=NULL;
  
  
  //file size
  inc_message = receive_message(*fd);
  if (inc_message ==NULL) {
    *fd = -1;
    printf("User left!\n");
    
  }
  int f_size = atoi(inc_message);

  //file path
  inc_message = receive_message(*fd);
  
  if (inc_message ==NULL) {
    *fd = -1;
    printf("User left!\n");
    
  }
  char path[MAX_PATH_LENGTH];
  strcpy(path, inc_message);
  //create a new file
  
  FILE *received_file = fopen(path,"w");
  if(received_file==NULL){
    perror("couldn't open file");
    return;
  }
  int data_remaining = f_size;
  //receive the data and write it to the given location
  int bytes_received;
  char buffer[BUFSIZ];
  while ((data_remaining > 0) && ((bytes_received = recv(*fd, buffer, BUFSIZ, 0)) > 0)){
    data_remaining -= bytes_received;
    fwrite(buffer, sizeof(char), bytes_received, received_file);

  }
  fclose(received_file);
  printf("Received file: %s\n", inc_message);
}

int main(int argc, char** argv) {
    
    if (argc != 3) {
        fprintf(stderr, "Usage: <server_name> <port number>\n");
        exit(1);
    }

    //initialize and open a port.
    unsigned short port = 0;
    int server_socket_fd = server_socket_open(&port);
    if (server_socket_fd == -1) {
      perror("Server socket was not opened");
      exit(EXIT_FAILURE);
    }
    
    if (listen(server_socket_fd, 1)) {
      perror("listen failed");
      exit(EXIT_FAILURE);

    }
    // Unpack arguments
    char* peer_hostname = argv[1];
    unsigned short peer_port = atoi(argv[2]);

    //connect to the peer
    pthread_mutex_lock(&user_lock);
    int peer_server_socket_fd = socket_connect(peer_hostname, peer_port);
    
    if(peer_server_socket_fd==-1){
      perror("socket number not found");
      exit(EXIT_FAILURE);
    }
    
    //Creates a starting connection thread, add to connections
    connection = peer_server_socket_fd;
    pthread_t m_thread;
    if (pthread_create(&m_thread, NULL,  messages_thread, (void*) &peer_server_socket_fd)) {
        perror("pthread_create failed");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_unlock(&user_lock);
    
    
    printf("Type 'help' for a list of commands. Type exit to quit.\n");

    char inp[MAX_PATH_LENGTH];
    char* rest;
    while(1){

        fgets(inp, sizeof(inp), stdin);
        inp[strcspn(inp, "\n")] = '\0';

        if(strcmp(inp, "exit") == 0){
            break;
        }
        char * token  = strtok_r(inp, " ", &rest);

        if(strcmp(token, "help")==0){
            printf("Commands: \n");
            printf("download \n");
            printf("display_all \n");
            printf("delete \n");
            printf("upload \n");
            printf("print \n");
            printf("checkout\n");
            printf("return\n");
            printf("status");

        }
        else if(strcmp(token, "display_all")==0){
          display_all_request();
        }
        else if(strcmp(token, "create")==0){
            token = strtok_r(NULL, " ", &rest);
            if(token==NULL){
                printf("Invalid syntax. Proper syntax is: create <filename>\n");
                continue;
            }
            create_file_request(token);
        }
        else if(strcmp(token, "delete")==0){
            token = strtok_r(NULL, " ", &rest);
            if(token==NULL){
                printf("Invalid syntax. Proper syntax is: delete <filename>\n");
                continue;
            }
            delete_file_request(token);      
        }
        
        else if(strcmp(token, "download")==0){
            char* token1 = strtok_r(NULL, " ", &rest);
            char* token2 = strtok_r(NULL, " ", &rest);
            if((token1==NULL) || (token2 == NULL)){
                printf("Invalid syntax. Proper syntax is: download <filename> <directory path>\n");
                continue;
            }
            download_file_request(token1, token2);
        }
        else if(strcmp(token, "upload")==0){
            char* arg1 = strtok_r(NULL, " ", &rest);
            if(arg1==NULL){
                printf("Invalid syntax. Proper syntax is: upload <ffilename>\n");
                continue;
            }
            send_file_request(arg1);
        }
        else if(strcmp(token, "print")==0){
            char* arg1 = strtok_r(NULL, " ", &rest);
            
            if(arg1==NULL){
                printf("Invalid syntax. Proper syntax is: print <filename>\n");
                continue;
            }
            print_file_request(arg1);
        }
        else if(strcmp(token, "checkout")==0){
            char* arg1 = strtok_r(NULL, " ", &rest);
            char* arg2 = strtok_r(NULL, " ", &rest);

            if((arg1==NULL) || (arg2==NULL)){
                printf("Invalid syntax. Proper syntax is: checkout <filename> <targetDirectory>\n");
                continue;
            }
            checkout_file_request(arg1, arg2);
        }
        else if(strcmp(token, "return")==0){
            char* arg1 = strtok_r(NULL, " ", &rest);
            if(arg1==NULL){
                printf("Invalid syntax. Proper syntax is: return <filename>\n");
                continue;
            }
            return_file_request(arg1);
        }
        else if(strcmp(token, "status")==0){
            char* arg1 = strtok_r(NULL, " ", &rest);
            if(arg1==NULL){
                printf("Invalid syntax. Proper syntax is: status <filename>\n");
                continue;
            }
            status_file_request(arg1);
        }
        else{
            printf("Unrecognized command. Type 'help' for a list of valid commands.\n");
        }

    }

    pthread_mutex_destroy(&user_lock);

    return 0;
}

