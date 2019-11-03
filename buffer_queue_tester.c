#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUFF_MAX 50

typedef struct queue {
	int *fd_buff[BUFF_MAX];//holds the file descriptors
	int empty;//1 for empty 0 for not empty
}queue;

typedef struct ph_queue {
	char *ph_buff[BUFF_MAX];//holds the file descriptors
	int empty;//1 for empty 0 for not empty
	int size;//keeps track of the size of the buffer
}ph_queue;

char* remove_ph(ph_queue *buff){
    if(buff->empty == 1 || buff->ph_buff[0] == NULL){
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

int remove_fd(queue *buff){
	int retval;
	if(buff->fd_buff[0] == NULL)//if the buff is empty
		retval = -1;//returns -1
	else{
		retval = buff->fd_buff[0];
		buff->fd_buff[0] = NULL;
		int x;
		for(x = 1; buff->fd_buff[x] != NULL; x++){//continuously go through the buffer until end of values
			buff->fd_buff[x-1] = buff->fd_buff[x];//places value of fd_buff[x] at previous element
			buff->fd_buff[x] = NULL;//replaces element x as NULL, only stays NULL for the last element
		}
	}
	return retval;//returns the fd if buffer is not empty and returns -1 if buffer is empty
}


int main(int argc, char* argv[]){
    ph_queue *buffer = (queue*)malloc(sizeof(queue)*BUFF_MAX);

    char test[1024];

    buffer->ph_buff[0] = "hello";
    buffer->ph_buff[1] = "world";
    buffer->ph_buff[2] = "!";

    printf("1. retval = %s\n", remove_ph(buffer));
    printf("2. retval = %s\n", remove_ph(buffer));
    printf("3. retval = %s\n", remove_ph(buffer));
    printf("4. retval = %s\n", remove_ph(buffer));
    printf("5. retval = %s\n", remove_ph(buffer));
}
