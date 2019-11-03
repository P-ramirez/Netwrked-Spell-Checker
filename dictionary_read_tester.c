#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


int dic_check(char* phrase){
    FILE* fp = fopen("dictionary.txt", "r");
	int retval = 0;//creates return values
	char* buf = (char*)malloc(sizeof(char)*1024);//creates buffer for word in
    int cmp;
	while((fgets(buf, 1024, fp) != NULL)&&(retval == 0)){
        buf = strtok(buf, "\n");
	    cmp = strcmp(phrase, buf);
		if(cmp == 0){
			retval = 1;
		}
	}
	return retval;
}

int main(){
    char *str = "repeatasdf";
    int val = dic_check(str);
    if(val == 1)
        puts("WORD FOUND");
    else
        puts("WORD NOT FOUND");
}
