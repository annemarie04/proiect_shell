#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
///include <readline>
//#include<readline/readline.h>
//#include<readline/history.h>

void create_shell(){
    ///clrscr();
    printf("\n--------------------------Proiect Sisteme de Operare - Grupa 251-------------------------\n");
    printf("\n----------Alpetri Iulita, Messner Annemarie-Beatrix si Patranjel David-George------------\n");
    char* username = getenv("USER");
    printf("USER is: @%s\n", username);
    sleep(1);
}

int parsingSpace(char *buf, char **argv){
    int i = 0;
    int capacity = 4;
    char *delimiters = " \t\r\n";
    char *aux = strtok(buf, delimiters);

    while (aux != NULL) {
        argv[i] = aux;
        i++;

        if (i >= capacity) {
            capacity = (int) (capacity * 1.5);
            argv = realloc(argv, sizeof(char*) * capacity);
        }

        aux = strtok(NULL, delimiters);
    }
    argv[i] = NULL;
    return i;
}

int main()
{
    char comm_line[400];
    create_shell();
    while(1){
        ///citirea comenzii
        char* buf = NULL;
        size_t buflen = 0;
        char** argv = malloc(sizeof(char*)*4);
        printf("> ");
        getline(&buf, &buflen, stdin);
        printf("> %s", buf);
        if(strcmp(buf, "stop\n") == 0){
            exit(0);
        }

        int argc = parsingSpace(buf, argv);
        for(int i = 0; i < argc; ++i){
            printf("> arg %d = %s\n", i, argv[i]);
        }
        
    
        pid_t pid;
        pid = fork();
        if(pid < 0){
            return -1;
        }
        if(pid == 0){
            //child
            if(strcmp(argv[0], "ls") == 0){
                execve("/usr/bin/ls", argv, NULL);
            }
        }
        else{
            wait(NULL);
            ///father
        }

        free(argv);
        free(buf);
    }
    return 0;
}