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
int error, nr;
char path[1024];
char *output;
char **words;

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

//afisarea unui mesaj specific unei erori 
void error_msg(int error_code)
{
    if(error_code == 1) printf("No such file or directory\n");

    if(error_code == 2) printf("Unable to locate current path\n");

    if(error_code == 3) printf("Unable to open source file\n");

    if(error_code == 4) printf("Unable to open destination file\n");

    if(error_code == 5) printf("Unable to create folder\n");

    if(error_code == 6) printf("Unable to delete folder\n");

    if(error_code == 7) printf("Unable to create file\n");

    if(error_code == 8) printf("Unable to delete file\n");

    if(error_code == 9) printf("Invalid number of operands\n");

    if(error_code == 10) printf("Command '%s' not found\n", words[0]);

}



//help- prints the command manual 
void help()
{
    printf("\n -----Welcome to my shell-----\n");
    printf("\n----------Commands:-------------\n");
    printf("\n\n");
     printf("history   Prints the command history of the current session. \n");
	printf("clear      Clears the terminal. \n");// facut
	printf("cd         Changes the working directory. \n");// facut
	printf("pwd        Prints the path of the current directory. \n");// facut
	printf("ls         Lists all files and directories in the current directory. \n");//facut
    printf("touch      Creates a new empty file. \n");// facut
    printf("rm         Deletes a particular file. \n");//facut
	printf("cp         Copies the content of a file to another file. \n");//facut
	printf("makedir    Creates a new directory. \n");// facut
	printf("removedir  Deletes an already existing directory. \n");// facut
    printf("echo       Displays a string that is passed as an argument.\n");
    printf("quit       Exits the shell. \n");

   
}
  

//// history command
//void hist()
//{
//    printf("%s", history);
//}



  void clear()
{
    
	write(1, "\33[H\33[2J", 7);// ANSI escape code, \33[H- moves the cursor to the top left corner of the screen, 
                                //33[2J- clears the part of the screen from the cursor to the end of the screen.
}



void cd(char* folder)
{
    //chdir command is a system function which is used to change the current working directory
    //returns 0 for success, -1 otherwise
    if (chdir(folder))
    {
        error = 1;
    }
}



void pwd()
{
    // getcwd(buffer)- obtine current path-ul si il pune in buffer
    //If successful, getcwd() returns a pointer to the buffer.
    //otherwise, getcwd() returns NULL

    if (getcwd(path, sizeof(path))) 
    {
        strcat(output, path);
        printf("%s\n", output);
    } 
    else 
    {
        error = 2;
    }
}



void ls()
{
    // se creeaza un proces nou pentru executarea functiei ls din bin
    pid_t pid = fork ();
    if (pid == 0)
    {
        char *arguments[] = {"ls", NULL};
        execve ("/bin/ls", arguments , NULL);
        kill(getpid(), 0);// opreste procesul
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);// asteapta sa termine copilul cu pidul pid executia
        // status stores the child status
    }
}


void touch(char* file)
{
   
    // cu fopen se creeaza un fisier in care putem scrie
    FILE *aux;
    aux = fopen(file, "w");
    if (aux)
    {
        strcat(output, "File created succesfully");
        printf("%s\n", output);
    }
    else
    {
        error = 7;
    }
    
    fclose(aux);
}


void rm (char* filename)
{
    // se obtine current path-ul
    char file_path[1024];
    if (getcwd(file_path, sizeof(file_path)) == NULL) 
    {
        error = 2;
    }

    // adaugam numele fisierul ce trebuie sters
    // la current path pentru a il sterge
    strcat(file_path, "/");
    strcat(file_path, filename);

    if(!remove(file_path))// remove deletes the given filename, on success, returns 0, else -1
	{
        strcat(output, "File deleted succesfully");
        printf("%s\n", output);
	}
	else
	{
		error = 8;
	}
}



void cp(char* file1, char* file2)
{
	char caract;
	FILE *f1, *f2;
	
	if ((f1 = fopen(file1, "r")) == 0)// deschidem fisierul sursa cu permisiunea de citire
	{
		error = 3;
        return;
	}

    
    if ((f2 = fopen(file2, "w")) == 0)// deschidem fisierul destinatie cu permisiunea de scriere
	{
		error = 4;
        return;
	}

	caract = fgetc(f1);// citeste continutul fisierului sursa caravcter cu caracter

	while(caract != EOF)// cat timp caract!= end of file
	{
		
		fputc(caract, f2);// scrie in fis destinatie caract cu caract
		caract = fgetc(f1);
	}

    strcat(output, "File copied succesfully");
    printf("%s\n", output);

	fclose(f1);
	fclose(f2);
}



void makedir(char* folder)
{
   
    if (getcwd(path, sizeof(path)) == NULL) //obtinem calea
    {
        error = 2;
        return;
    }

    strcat(path, "/");
    strcat(path, folder);


    if (mkdir(path, 0777) == -1)// mkdir- creeaza un director , 0777= permisiuni de scriere, citire si executie
	{
        error = 5;
    }
    else
    {
        strcat(output, "Folder created succesfully");
        printf("%s\n", output);
    }
}



void removedir(char* folder)
{
    
    if (getcwd(path, sizeof(path)) == NULL) // obtinem calea
    {
        error = 2;
        return;
    }

    strcat(path, "/");
    strcat(path, folder);

    if(rmdir(path) == -1)
	{
        error = 6;
    }
    else
    {
        strcat(output, "Folder deleted succesfully");
        printf("%s\n", output);
    }
}



void echo()
{
    // afisam toate cuvintele scrise dupa comanda echo
    for (int i = 1; i < nr; i ++)
    {
        printf("%s ", words[i]);
        strcat(output, words[i]);
        strcat(output, " ");
    }

    printf("\n");
}



void exec(char **arg, int nr_args){
    // verificam pt fiecare comanda daca nr de arg este corect

    if(!strcmp(arg[0], "help")){
        if(nr_args!=1){
            error= 9;
            return;
        }
        help();
    }
    else if(!strcmp(arg[0], "History")){
        return;// de continuat aici-------------------------------------------
    }
    else if(!strcmp(arg[0],"clear")){
        
        if(nr_args!=1){
            error=9;
            return;
        }
        clear();
    }
    else if(!strcmp(arg[0], "cd")){

        if(nr_args> 2){
            error=9; 
            return;
        }
        else if(nr_args==2) cd(arg[1]);
        else cd("..");
    } 
    else if(!strcmp(arg[0], "pwd")){
        if(nr_args!=1){
            error=9; 
            return;
        }
        pwd();
    }
    else if(!strcmp(arg[0], "ls")){
        if(nr_args!=1){
            error=9;
            return;
        }
        ls();
    }
    else if (!strcmp(arg[0], "touch"))
    {
        if(nr_args != 2)
        {
           error = 9;
            return;
        }
        touch(arg[1]);
    }
    else if (!strcmp(arg[0], "cp"))
    {
        if (nr_args != 3)
        {
            error = 9;
            return;
        }
        cp(arg[1], arg[2]);
    }
    else if (!strcmp(arg[0], "makedir"))
    {
        if(nr_args != 2)
        {
            error = 9;
            return;
        }
        makedir(arg[1]);
    }
    else if (!strcmp(arg[0], "removedir"))
    {
        if(nr_args != 2)
        {
            error = 9;
            return;
        }
       removedir(arg[1]);
    }
    else if (!strcmp(arg[0], "rm"))
    {
        if(nr_args != 2)
        {
            error = 9;
            return;
        }
        rm(arg[1]);
    }

    else if (!strcmp(arg[0], "echo"))
    {
        echo();
    }

    else if (!strcmp(arg[0], "quit"))
    {
        if (nr_args != 1)
        {
            error = 9;
            return;
        }
        exit(0);
    }
    else
    {
        error = 10;
        return;
    }
    


}











// mai am de fct in main apelarea fctiei pt msj de eroare + && si ||

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
            exec(argv, argc );
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