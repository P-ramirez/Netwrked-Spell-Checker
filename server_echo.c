#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#define portNumber 8080
#define BUFF_MAX 20

FILE* dictionary;

/*FUNCTION TO COMPARE WORD WITH THE WORDS IN THE DICTIONARY*/
int dic_check(char* phrase){
	int retval = 0;//creates return values
	char* buf = (char*)malloc(sizeof(char)*1024);//creates buffer for word in dictionary
    int cmp;
	while((fgets(buf, 1024, dictionary) != NULL)&&(retval != 1)){
            /*
        printf("buf lenght= %d\nphrase length= %d\n", strlen(buf), strlen(phrase));
        printf("phrase = %sbuf = %s\n", phrase,buf);
        */
	    cmp = strncmp(phrase, buf, strlen(buf)-2);
        printf("cmp = %d\n", cmp);
		if(cmp == 0){
			retval = 1;
		}
	}
	rewind(dictionary);
	return retval;
}
/*MAIN THREAD-> NETWORK CONNECTION, PRODUCER*/
int main(int argc, char *argv[]){//if argc > 1 then argv[1] hould be a dictionary that the user has
    int valread, cmp, c;//declares vlaread and cmp
    char* ret_buffer = (char*)malloc(sizeof(char)*1024);

    char* correct = "->correct\n";
    char* incorrect = "->incorrect\n";
	//Load in the dictionary through a fopen
	dictionary = fopen("dic.txt", "r");

	//SETTING UP THE NETWORK
	struct sockaddr_in client, server;
	int server_fd, client_socket_fd;

	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){//sets upt the server file decsriptor
		puts("Error in creating socket!");
	}
	//setting up the server socket
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(portNumber);//must define portNumber mysef
	//binding the server socket address (server) to server_fd

	if((bind(server_fd, (struct sockaddr*)&server, sizeof(server)))<0)
        puts("COULD NOT BIND SOCKET");

	//set the server to listen for the clients
	if((listen(server_fd, 3))<0)
        puts("ERROR IN LUSTENING TO SOCKET");

	//START THE LOOP TO ACCEPT NEW SOCKETS AND PROCESS THEM -> PRODUCER
	while(1){
		//STILL IN INFINITE LOOP - BEGINS PRODUCING
		if((client_socket_fd = accept(server_fd, (struct sockaddr*)&client, (socklen_t*)&c)) > 0){//if the accept value returns a non negative number, the client file descriptor
			//WORKS INSIDE OF THE CLIENT BUFFER -> CRITICAL SECTION
			//while(1){
            while((valread = recv(client_socket_fd, ret_buffer, 1024, 0))){//reads a word into the phrase buffer while the client gives words
                cmp = dic_check(ret_buffer);//compare the word to the dictionary
                ret_buffer = strtok(ret_buffer, "\n");//strip the newline character form the string
                if(cmp == 1){//if the word is found in the dictionary
                    strcat(ret_buffer, correct);
                }
                else{
                    strcat(ret_buffer, incorrect);
                }

                send(client_socket_fd, ret_buffer, strlen(ret_buffer), 0);
                free(ret_buffer);
                ret_buffer = (char*)malloc(sizeof(char)*1024);
                //WORKS ON SIGNALING THAT CONSUMERS CAN BEGIN, AND RELEASES THE LOCK ON THE CLIENT BUFFER
                }
			//}
        }

        else{
            puts("ERROR ERROR ERROR ERROR");
            exit(1);
        }
    }
}
