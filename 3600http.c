/*
 *   CS3600 Project 4: A HTTP Web Server
 */

#include <math.h>
#include <ctype.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


//top level
char* root_directory = "/tmp";
int id = 0;


////////////////
////////////////Functions to create the final packet/subparts of the packet
////////////////
   
//Return a header with the correct message
char* get_header_status(int code){
    char* code_word;

    if (code == 200){
        code_word = "OK";
    }
    if (code == 404){
        code_word = "Not Found";
    }
    if (code == 500){
        code_word = "Other Error";
    }

    char* h_status = calloc(40, sizeof(char));
    snprintf(h_status, 30, "%s%d%s%s%s", "HTTP/1.1 ", code, " ", code_word, "\r\n");
    return h_status;
}
 
//Return the current date in GMT
char* get_header_date(){
    char* pre_h_date = calloc(50, sizeof(char));
    char* h_date = calloc(50, sizeof(char));
    time_t t = time(NULL);
    snprintf(pre_h_date, 31, "%s%s", "Date: ", asctime(gmtime(&t)));
    snprintf(h_date, 38, "%s%s", pre_h_date, " GMT\r\n");

    free(pre_h_date);
    return h_date;
}
    
//Return the current server
char* get_header_server(){
    //TODO
    return (char *) -1;
}

//Return a formatted length line
char* get_content_length(int length){
    char* h_length = calloc(40, sizeof(char));
    snprintf(h_length, 39, "%s%d%s", "Content-Length: ", length, "\r\n");
    return h_length;
}

//Return a formatted type line
char* get_content_type(char* type){
    int len = 20 + strlen(type);
    char* h_type = calloc(len, sizeof(char));
    snprintf(h_type, len, "%s%s%s", "Content-Type: ", type, "\r\n\r\n");
    return h_type;
}
    

//Return a finished packet for the client
void send_packet(int cl_sock, int code, int length, char* type, char* data){ 
    char* h_status = get_header_status(code); 
    char* h_date = get_header_date();
    char* h_server = "Server: Ubuntu/9.10\r\n"; //TODO generate this
    char* h_connection = "Connection: close\r\n";
    char* h_length = get_content_length(length);
    char* h_type = get_content_type(type);
    char* h_data = data;

    //get total length of message
    int total_packet_length = strlen(h_status) + strlen(h_date) + strlen(h_server) + strlen(h_connection) + strlen(h_length) + strlen(h_type) + length + 3;

    char* packet = calloc(total_packet_length, sizeof(char));

    //copy everything 
   snprintf(packet, total_packet_length, "%s%s%s%s%s%s%s%s", h_status, h_date, h_server, h_connection, h_length, h_type, h_data, "\r\n\0");

    //free everything
    free(h_status);
    free(h_date);
    free(h_length);
    free(h_type);
    free(h_data);

    if( send( cl_sock, packet,total_packet_length, 0 ) )
      printf("<%d> DELIVERED\n", id);
    else
      printf("<%d> ERROR <can't send>\n", id );
}

////////////////
////////////////Functions to get data from the system
////////////////

//gets the mimetype of a file given its name 
char* get_mime_type(char* filename){
  //get mimetype
  char* mime_type; 
  int filename_length = strlen(filename);

  char* file_command = "mimetype ";
  char* file_pipe = " | sed 's/.* //'";
  int mime_request_length = filename_length + strlen( file_command ) + strlen( file_pipe ) + 9;
  char* mime_request = calloc( mime_request_length, sizeof(char) );
  sprintf( mime_request, "%s%s%s", file_command, filename, file_pipe );

  FILE *f = popen( mime_request, "r" );
  if( f == NULL ){
    printf( "Mime Type Error" );
    mime_type = "text\\html"; //default
  }else{
    char line[100];

    fgets( line, sizeof( line ), f );

    mime_type = calloc( strlen( line ), sizeof( char ) );
    strncpy( mime_type, line, strlen( line ) - 1);
    mime_type[strlen(line) - 1] = '\0';
    printf( "MIME: %d \"%s\"",strlen( mime_type ), mime_type );
  }
  pclose( f );

  return mime_type;
}
 
//gets the data from a file
//loads the data into data
//returns the length of the data
//returns -1 if not found
int get_file( char * filename, char** data ){
  int length = 0;

  FILE* f;
  char* file_buffer = calloc(100000, sizeof(char)); //TODO get size somehow?
  if (file_buffer){ //allocated correctly
      f = fopen(filename, "r"); //open for reading
      if(f){
        length = fread(file_buffer, 1, 100000, f); //TODO use read() instead?
        printf("reading file\n");
      }else{ //can't find file
        return -1;
        printf("<%d> ERROR <incorrect request format> %s\n", id);
      }
      *data = file_buffer;  
  }else{
      printf("didn't allocate space\n");
      *data = file_buffer; //set anyway so it can be freed later 
      length = 0;
  }
  return length;
}


//looks for a file in a directory, loads it into data
//used for checking for index.html, index.htm
//returns length if file found, returns -1 otherwise
int check_for_file_named(char* name, char* dir, char** data){
  printf("Checking for : %s\n", name);

  char* full_path = calloc( strlen(dir) + strlen(name) + 1, sizeof( char ) );
  strcpy( full_path, dir );
  strcat( full_path, name );

  int data_length = get_file(full_path, data);
  return data_length;
}


//serve the content of a directory
//HAXXX
int get_directory(char* path, char** data){
    int path_len = strlen(path);
 
    printf( "DIRECTORY WITH NO INDEX\n" );
    
	srand( time(NULL) );
	int tmpp = rand();
	char* tmp_file = calloc( 40, sizeof( char ) );
	
    snprintf(tmp_file, 32, "%s%d", "/tmp/", tmpp);
	
    char* get_ls = calloc( path_len + 10 + strlen( tmp_file ), sizeof( char ) );
    strcpy( get_ls, "ls " );
    strcat( get_ls, path );
	strcat( get_ls, " > " );
	strcat( get_ls, tmp_file );
    
	char* add_sl = calloc( strlen(tmp_file) + 20, sizeof( char ) );
	strcpy( add_sl, "sed -i 's/^/\\//' " );
	strcat( add_sl, tmp_file );
	
    // directory is empty, need to do it!
    FILE *f = popen( get_ls, "r" );
    if( f == NULL ){
    }else{
       //??
    }
    pclose(f);
	
	FILE *f2 = popen( add_sl, "r" );
    if( f2 == NULL ){
    }else{
       //??
    }
    pclose(f2);

    return get_file(tmp_file, data);
}

////////////////
////////////////Functions to deal with parsing requests
////////////////


//get the full content of a request from the socket
char * get_request( int socket ){
  char * request = calloc( 1001, sizeof( char ) );
  char * request_ptr = request;

  char c = ' ';
  int n = 0;
  int count;

  while( request_ptr >= request && 
         ( request_ptr - 4 < request || 
           strncmp( request_ptr - 4, "\r\n\r\n", 4 ) ) )
  {
    n = recv( socket, request_ptr, 1, 0 );

    if( n > 0 ){

      count = count + 1;
      request_ptr = request_ptr + 1;
    }else{
      return (char *) -1;
    }
  }
  request_ptr = '\0';

  return request;
}


//checks a request
//returns an absolute path to the filename 
//if the request is incorrectly formatted returns -1
char* parse_request(char* full_request){
  char* request_ptr = full_request;
  char* method = strtok(request_ptr, " ");
  char* name = strtok(NULL, " ");

  int path_len = 1 + strlen(root_directory) + strlen(name);
  char* path = calloc(path_len + 20, sizeof(char));
  snprintf(path, path_len, "%s%s", root_directory, name);
  printf("<%d> REQUEST %s\n", id, path);

  if(strcmp(method, "GET") == 0) //should be more checking here
      return path;
  else
      return -1;
}

////////////////
////////////////Top level functions to deal with fulfilling requests
////////////////


/**
 * This function will serve the request of one client on the socket that's
 * passed in.  The socket is already contructed, you just need to read/write
 * from it to serve the request, close it, and then return from the 
 * function.
 */
void serve_client(int cl_sock) {
  id = rand(); //set a random id for this connection
  printf("<%d> CONNECT\n", id);

  // serve request
  printf("initiating shenanigans\n");
 
  //set default values
  int code = 200;
  char* mime_type = "text/html";
  int data_length = 0;
  char* data;

  ////////
  //get full request
  char* full_request = get_request(cl_sock);
  //printf( "full request: \n%s\n", full_request );
  char* path = parse_request(full_request);

  //bad request
  if (path == -1){
      send_packet(cl_sock, 500, 0, "text/html", calloc(1, sizeof(char)));
      close(cl_sock);
      printf("<%d> CLOSE\n", id);
      exit(0);
  }

  //deal with directory requests
  if( path[strlen(path) - 1] == '/' ){
      data_length = check_for_file_named("index.html", path, &data);
      if (data_length == -1){
          data_length = check_for_file_named("index.htm", path, &data);
          if (data_length == -1){
              //return directory 
              data_length = get_directory(path, &data);
          }
      }
  //deal with regular files
  }else{ 
      //get the mime type
      mime_type = get_mime_type(path);  
      //get the file data
      data_length = get_file(path, &data);
  }

  if( data_length == -1 ){
      code = 404;
      printf( "<%d> NOTFOUND\n", id );
      data = calloc(1, sizeof(char)); //make sure this can be freed later.
  }

  ////////
  //create request
  send_packet(cl_sock, code, data_length, mime_type, data);
  // and clean up
  close(cl_sock);
  printf("<%d> CLOSE\n", id);
}






int main(int argc, char *argv[]) {
  ///////////
  // process the arguments

  int port = 8080; //default port
  //root_directory is a global
  root_directory = argv[1];

  if ( strcmp(argv[1], "-p") == 0) {  //if there's a port flag
      port = atoi(argv[2]); //change port
      root_directory = argv[3];
  }

  printf("port: %d, root: %s \n", port, root_directory);

  // first, open a UDP socket  
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    // handle error
    printf("Can't open socket\n");
    return -1;
  }
  printf("socket num: %d", sock);

  ///////////
  // next, construct the destination address
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    // handle error
    printf("Can't bind socket %d \n", port);
    return -1;
  }

  if (listen(sock, 10) < 0) {
    // handle error
    printf("Can't listen on socket %d \n", port);
    return -1;
  }
  
  ///////////
  // now loop, accepting incoming connections and serving them on other threads
  while (1) {
    struct sockaddr_in cl_addr;
    int cl_addr_len = sizeof(cl_addr);
    int cl_sock = accept(sock, &cl_addr, &cl_addr_len);

    printf(".\n");

    if ( cl_sock > 0) {
      pthread_t thread;
      if (pthread_create(&thread, NULL, serve_client, cl_sock) != 0) {
        // handle error
        printf("Can't create thread\n");
      } 
    } else {
      // handle error
      printf("Didn't accept socket: %d \n", cl_sock);
    }
  }
  
  return 0;
}
