#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#define portNumber 8084
#define BUFF_MAX 20

/*DATA STRUCTURE FOR THE CLIENT QUEUE*/
typedef struct fd_queue {
	int *fd_buff[BUFF_MAX];//holds the file descriptors
	int size;//keeps track of the size of the buffer

	pthread_mutex_t mutex;//mutex for the buffer
	pthread_cond_t open;//condition variable for when buffer is not full -> can produce not consume
	pthread_cond_t closed;//condition variable for when buffer is full ->can consume not produce
}fd_queue;

/*DATA STRUCTURE FOR THE PHRASE QUEUE*/
typedef struct ph_queue {
	char *ph_buff[BUFF_MAX];//holds the file descriptors
	int size;//keeps track of the size of the buffer

	pthread_mutex_t mutex;//mutex for the buffer
	pthread_cond_t open;//condition variable for when buffer is not full -> can produce not consume
	pthread_cond_t closed;//condition variable for when buffer is full ->can consume not produce
}ph_queue;

fd_queue* client_buf;
ph_queue* phrase_buf;
FILE* dictionary;

/*REMOVES A FILE DESCRIPTOR FROM THE CLIENT BUFFER*/
int remove_fd(fd_queue *buff){
	int retval;
	if(buff->fd_buff[0] == NULL)//if the buff is empty
		retval = -1;//returns -1
	else{
		retval = *(buff->fd_buff[0]);
		buff->fd_buff[0] = NULL;
		int x;
		for(x = 1; buff->fd_buff[x] != NULL; x++){//continuously go through the buffer until end of values
			buff->fd_buff[x-1] = buff->fd_buff[x];//places value of fd_buff[x] at previous element
			buff->fd_buff[x] = NULL;//replaces element x as NULL, only stays NULL for the last element
		}
	}
	return retval;//returns the fd if buffer is not empty and returns -1 if buffer is empty
}

/*REMOVES A PHRASE FROM THE PHRASE BUFFER*/
char* remove_ph(ph_queue *buff){
    if(buff->size == 0){
        printf("buffer is empty\n");
        return NULL;
    }
    char* retval = buff->ph_buff[0];
    buff->ph_buff[0] = NULL;
    int x;
		for(x = 1; buff->ph_buff[x] != NULL; x++){//continuously go through the buffer until end of values
			buff->ph_buff[x-1] = buff->ph_buff[x];//places value of fd_buff[x] at previous element
			buff->ph_buff[x] = NULL;//replaces element x as NULL, only stays NULL for the last element
		}
    return retval;
}

/*FUNCTION TO COMPARE WORD WITH THE WORDS IN THE DICTIONARY*/
int dic_check(char* phrase){
    puts("TESTD");
	int retval = 0;//creates return values
	char* buf = (char*)malloc(sizeof(char)*1024);//creates buffer for word in dictionary
    int cmp;
	while((fgets(buf, 1024, dictionary) != NULL)&&(retval != 1)){
	    cmp = strncmp(phrase, buf, strlen(buf)-2);//compares a certain length of buffer
		if(cmp == 0){
			retval = 1;
		}
	}
	rewind(dictionary);
	return retval;
}

/*WORKER THREAD FUNCTION -> CONSUMER, WRITER*/
//void spell_check(fd_queue* client_buf, ph_queue* phrase_buf){
void spell_check(void* p){
	int sd;//ckient socket descriptor
	int valread = 0;//used for read words from the client
	int cmp;//used for the comparison of the client word and the dictionary
	char* correct = "->correct\n";//to be appended to correctly spelled words
	char* incorrect = "->incorrect\n";//to be appended to incorrectly spelled words
	char* buffer = (char*)malloc(sizeof(char)*1024);//stores the word from the client, will be appended to and added to the phrase buffer

	while(1){//continuous loop
        puts("TEST Z1");
		pthread_mutex_lock(&client_buf->mutex);//locks the use of the client

		while(client_buf->size == 0){//checks to make sure the client buff isn't empty
            puts("TEST Z2");
			pthread_cond_wait(&client_buf->closed, &client_buf->mutex);//if the client buffer is empty then wait on the client closed condition and unlock the client mutex
		}
		puts("TEST Z3");
		sd = remove_fd(client_buf);//gets the socket descriptor from the client buffer
        puts("TEST Z4");
		--client_buf->size;//decrement buffer size

		//WORKER THREAD WRITE TO PHRASE BUFFER
        puts("TEST A1");
            puts("TEST A");
            while((valread = recv(sd, buffer, 1024, 0))){//reads a word into the phrase buffer while the client gives words
                puts("TEST A2");
                cmp = dic_check(buffer);//compares word to dictionary

                buffer = strtok(buffer, "\n");//strips the newline character

                if(cmp == 1){//if the word is found in the dicionary
                    strcat(buffer, correct);
                }
                else{
                    strcat(buffer, incorrect);
                }
                send(sd, buffer, strlen(buffer), 0);//send the word + correctness to the client
                free(buffer);
                buffer = (char*)malloc(sizeof(char)*1024);
            }
    puts("TEST1");
    pthread_cond_signal(&client_buf->open);//signals that there is an open slot in the client buffer
    pthread_mutex_unlock(&client_buf->mutex);//unlock the client buffer
    }
}


/*LOG WRITER THREAD FUNCTION -> READER*/
//void write_log(fd_queue* client_buf, ph_queue* phrase_buf){
void write_log(void* p){
	char* buf = (char*)malloc(sizeof(char)*1024);
	FILE* fp = fopen("log_file.txt", "w");
	while(1){
		pthread_mutex_lock(&phrase_buf->mutex);//lock the phrase mutex
		while(phrase_buf->size == 0){//while the phrase buffer is empty
			pthread_cond_wait(&phrase_buf->closed, &phrase_buf->mutex);//wait on closed and unlock the mutex on phrase buf
		}
		buf = remove_ph(phrase_buf);//remove phrase from buffer and place in buff
		phrase_buf->size--;//decrement the size of phrase buffer
		fputs(buf, fp);//writes the phrase to the log file
		pthread_mutex_unlock(&phrase_buf->mutex);//unlock the phrase buffer
		pthread_cond_signal(&phrase_buf->open);//signals anyone waiting on the open variable
	}
}

/*MAIN THREAD-> NETWORK CONNECTION, PRODUCER*/
int main(int argc, char *argv[]){//if argc > 1 then argv[1] hould be a dictionary that the user has

    client_buf = (fd_queue*)malloc(sizeof(fd_queue));
    client_buf->size = 0;
    phrase_buf = (ph_queue*)malloc(sizeof(ph_queue));
    phrase_buf->size = 0;

	//Load in the dictionary through a fopen
	dictionary = fopen("dictionary.txt", "r");

	//CREATE POOL OF WORKER THREADS - pretty sure that spell_check threads will be created here and will all wait until we start producing
	pthread_t *thread_pool = (pthread_t*)malloc(sizeof(pthread_t)*10);
	int pool_num;
	for(pool_num = 0; pool_num<2; pool_num++)
		pthread_create(&thread_pool[pool_num], NULL, (void*)spell_check, NULL);


	//SETTING UP THE NETWORK
	struct sockaddr_in client, server;
	int socket_desc, new_socket, c;

	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_desc == -1){//creates socket descriptor
		puts("Error in creating socket");
	}
	//setting up the server socket
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(portNumber);//must define portNumber myself - 8080

	//binding the server socket address (server) to socket_desc
    if(bind(socket_desc, (struct sockaddr*)&server, sizeof(server)) < 0){
        puts("BINDING FAILED");
        return 1;
    }

	//set the server to listen for the clients
	listen(socket_desc, 3);

	//START THE LOOP TO ACCEPT NEW SOCKETS AND PROCESS THEM -> PRODUCER
	while(1){
        puts("TEST M1");
		pthread_mutex_lock(&client_buf->mutex);//LOCKS ANY OTHER THREAD FROM WORKING INSIDE THE CIENT BUFFER
        puts("TEST M2");
		while(client_buf->size == BUFF_MAX){//WHILE THE CLIENT BUFFER IS FULL
			pthread_cond_wait(&client_buf->open, &client_buf->mutex);//if the client buffer is full then wait the main thread for consumption, release mutex on the client buffer
		}//when the client buffer is open for more connections -> start producing more connections in the client buffer
        puts("TEST M3");
		//STILL IN INFINITE LOOP - BEGINS PRODUCING
		c = sizeof(struct sockaddr_in);
		puts("TEST M4");
		new_socket = accept(socket_desc, (struct sockaddr*)&client, (socklen_t*)&c);
		puts("TEST M5");
		if(new_socket < 0){//if the accept value returns a negative number, the client file descriptor accept failed
			puts("ERROR: ACCEPT FAILED");
		}
			//WORKS INSIDE OF THE CLIENT BUFFER -> CRITICAL SECTION
        puts("TEST M6");
        client_buf->fd_buff[client_buf->size-1] = (int*)malloc(sizeof(int));
        *client_buf->fd_buff[client_buf->size-1] = new_socket;//places the client socket descriptor into the client buffer

        ++client_buf->size;//increment the size of the client buffer since we added a new item

			//WORKS ON SIGNALING THAT CONSUMERS CAN BEGIN, AND RELEASES THE LOCK ON THE CLIENT BUFFER

		//INFINITE LOOP BEGINS AGAIN LOCKING THE CLIENT BUFFER, CHECKING THE SIZE OF THE BUFFER, WAITING FOR CONSUMERS IF THE BUFFER IS FULL, ACCEPTING NEW CLIENTS IF IT IS NOT FULL
		//MUTEX LOCKS THE CLIENT BUFFER IN THE BEGINNING SO IT CAN WORK, UNLOCKS IF THERE IS NOT ROOM TO PRODUCE, OR UNLOCKS WHEN CLIENTS ARE ADDED TO BUFFER
		//CONDITION WAITS WHEN CLIENT BUFFER CANNOT PRODUCE, CONDITION SIGNALS WHEN CLIENT BUFFER CAN BE CONCUMED
		pthread_cond_signal(&client_buf->closed);//signals that client buffer is ready for consumption
        pthread_mutex_unlock(&client_buf->mutex);//releases the lock on the cient buffer OTHER THREADS MAY NOW DO WORK IN THE CLIENT BUFFER
        puts("TEST M7");
        printf("client_buf size = %d\n", client_buf->size);
	}
}
