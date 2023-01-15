///run this-> sudo apt-get install libncurses5-dev libncursesw5-dev
///compile-> gcc shell.c -o shell -lncurses
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
#include <ncurses.h>
#define MAX_HIST 50

int error= 0, nr, hist_count = 0, hist_found_comm;
bool flag_has_input = false;
char path[1024];
char *output, *cuv;
char *output_pipe;
char **argv, **comanda;
char comm_line[400];
char **history;
int does_pipe = 0;

void create_shell();
int parsingSpace(char *buf, char **argv);
void error_msg(int error_code, const char *comm);
void help();
void hist(const char* command, bool wr, bool save);
void hcm();
void myclear();
void cd(char* folder);
void pwd();
void ls();
void touch(char* file);
void rm (char* filename);
void cp(char* file1, char* file2);
void makedir(char* folder);
void removedir(char* folder);
void myecho();
void exec(char **arg, int nr_args, char *raw_com);
void colors(int color);
void grep(char* string, char* file);
void grep_pipe_echo(char* string);
void grep_pipe(char* file, char* string);
void cat(char* file);

int main()
{
    output = malloc(1024 * sizeof(char));
    output_pipe = malloc(2048 * sizeof(char));
    history = malloc(sizeof(char*)*MAX_HIST);
    create_shell();

    while(1){
        error=0;
        //verificam calea 
        if (getcwd(path, sizeof(path)) != NULL){
            colors(3);
            printf("SHELL: %s$ ", path);
            colors(-1);
        } 
        else{
            error = 2;
        }

        char* buf = NULL;
        char *aux_buf = NULL;
        size_t buflen = 0;
        char** argv = malloc(sizeof(char*)*8);// pt despartirea in cuvinte
        char** comanda = malloc(sizeof(char*)*8);
        int nr=0;// nr_cuvinte dintr- o comanda

        if(!flag_has_input){
            printf("> ");
            getline(&buf, &buflen, stdin); // citim comanda
        }else{
            flag_has_input = false;
            buf = strdup(history[hist_found_comm]);
        }
        hist(buf, false, true);
        printf("> %s", buf);
        // despartim comanda in cuvinte 
        int argc = parsingSpace(buf, argv);
        
        if(!strcmp(argv[0], ""))
        {
            printf("\n");
            continue;
        }

        for(int i = 0; i < argc; ++i){
            does_pipe = 0;
            printf("> arg %d = %s\n", i, argv[i]);
            
            if (!strcmp(argv[i], "||"))
            {
                free(output);
                output = malloc(1024 * sizeof(char));
                exec(comanda, nr, buf);

                if(error)
                {
                    // daca a intampinat o eroare o va ignora
                    // deoarece doar prima comanda corecta va rula
                    error = 0;
                    nr = 0;// o luam de la capat pt urmatoarea comanda
                    
                    //continue;-----------------------------------------------------------------------------------------------
                }
                else
                {
                    // cand gaseste prima comanda care nu da eroare le va ignora pe restul
                   nr = 0;
                   for (int j= i+ 1; j<argc; ++j)
                    {
                        
                        if(!strcmp(argv[j], "&&"))
                        {
                            break;
                        }
                        
                    }
                    if (argv[argc-1] == NULL) error = -1;
                }
            }
            else if (!strcmp(argv[i], "&&"))
            {
                free(output);
                output = malloc(1024 * sizeof(char));
                exec(comanda, nr, buf);
                
                // la prima eroare intalnita va opri executia
                if (error != 0)
                {
                    break;
                }

                nr = 0;
            }
            else if (!strcmp(argv[i], "|"))
            {
                does_pipe = 1;
                if(!strcmp(argv[0], "cat")){
                    continue;
                }
                free(output);
                output = malloc(1024 * sizeof(char));
                exec(comanda, nr, buf);
                
                // la prima eroare intalnita va opri executia
                if (error != 0)
                {
                    break;
                }

                nr = 0;
            }
            else comanda[nr++]= argv[i];

        }
        does_pipe = 0;
       
        // daca nu am avut o eroare
        // va executa comanda curenta
        if (error == 0){
            free(output);
            output = malloc(1024 * sizeof(char));
            exec(comanda, nr, buf);
        }

        if(error){
            error_msg(error, argv[0]);
            error = 0;
        }

        free(argv);
        free(buf);
        free(comanda);
    } 
    
    return 0;
}


void create_shell(){
    colors(2);
    printf("\n--------------------------Proiect Sisteme de Operare - Grupa 251-------------------------\n");
    printf("\n----------Alpetri Iulita, Messner Annemarie-Beatrix si Patranjel David-George------------\n");
    colors(-1);
    colors(1);
    char* username = getenv("USER");
    printf("USER is: @%s\n", username);
    colors(-1);
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
void error_msg(int error_code, const char *comm)
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

    if(error_code == 10) printf("Command '%s' not found\n", comm);

}
//help- prints the command manual 
void help()
{
    printf("\n-------Welcome to my shell------\n");
    printf("\n----------Commands:-------------\n");
    printf("\n\n");
    printf("history    Prints the command history of the current session. \n"); ///facut
    printf("hcm        Use old command by using the arrow keys. \n"); ///facut
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

void hist(const char* command, bool wr, bool save)
{
    if(wr){
        if(!does_pipe)
            printf("\n---------Command History--------\n");
        if(hist_count == 0){
            if(!does_pipe)
                printf("No commands yet!\n");
        }else{
            for(int i = hist_count - 1; i >= 0; --i){
                strcat(output_pipe, history[i]);
                if(!does_pipe)
                    printf("%s", history[i]);
            }
        }
    }
    
    if(save){
        bool move_flag = hist_count >= MAX_HIST ? true : false;
        if(move_flag){
            free(history[0]);
            for(int i = 0; i < MAX_HIST - 1; ++i){
                history[i] = history[i + 1];
            }
            hist_count--;
        }

        history[hist_count++] = strdup(command);
    }
    return;
}
void hcm()
{
    hist_found_comm = hist_count;
    while(1){
        initscr();
        int ch = getch();        
        if (ch == '\033') { // if the first value is esc
            getch(); // skip the [
            switch(getch()) { // the real value
                case 'A':
                    // code for arrow up
                    clear();
                    if(hist_found_comm == 0){
                        addstr(history[0]);
                    }
                    else if(hist_found_comm > 0){
                        addstr(history[--hist_found_comm]);
                    }
                    ///printf("Up arrow\n");
                    break;
                case 'B':
                    // code for arrow down
                    clear();
                    if(hist_found_comm == hist_count - 1){
                        addstr(history[hist_count - 1]);
                    }
                    else if(hist_found_comm < hist_count - 1){
                        addstr(history[++hist_found_comm]);
                    }
                    ///printf("Down arrow\n");
                    break;
                default:
                    clear();
                    endwin();
                    break;
            }
        }else if(ch == '\n'){
            clear();
            endwin();
            printf("Done\n");
            printf("Urmatoarea comanda este %s", history[hist_found_comm]);
            flag_has_input = true;
            break;
        }
    }
    return;
}
void myclear()
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
void myecho()
{
    // afisam toate cuvintele scrise dupa comanda echo
    for (int i = 1; i < nr; i ++)
    {
        printf("%s ", argv[i]);
        strcat(output, argv[i]);
        strcat(output, " ");
    }

    printf("\n");
}


void cat(char* file)
{
    pid_t pid = fork ();
    if (pid == 0)
    {
        char *arguments[] = {"cat", file, NULL};
        execve ("/bin/cat", arguments , NULL);
        kill(getpid(), 0);// opreste procesul
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);// asteapta sa termine copilul cu pidul pid executia
        // status stores the child status
    }
    return;
}

void grep(char* string, char* file)
{
    pid_t pid = fork ();
    if (pid == 0)
    {
        char *arguments[] = {"grep", string, file, NULL};
        execve ("/bin/grep", arguments , NULL);
        kill(getpid(), 0);// opreste procesul
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);// asteapta sa termine copilul cu pidul pid executia
        // status stores the child status
    }
    return;
}

void grep_pipe_echo(char* string)
{
    pid_t pid_1, pid_2;
    int fd[2];
    int status;
    char *mass_1[] = {"echo", output_pipe, NULL};
    char *mass_2[] = {"grep", "-a", string, NULL};
    pipe(fd);
      if(pipe(fd) == -1)
    {
        perror(NULL);
        return;
    }
    pid_1 = fork();
    if (pid_1 == 0)
    {
        dup2(fd[1], 1);
        close(fd[0]);
        execvp(mass_1[0], mass_1);
        exit(1);
    }
    pid_2 = fork();
    if (pid_2 == 0)
    {
        dup2(fd[0], 0);
        close(fd[1]);
        execvp(mass_2[0], mass_2);
        exit(1);
    }
    
    close(fd[0]);
    close(fd[1]);
    waitpid(pid_1, &status, WUNTRACED);
    waitpid(pid_2, &status, WUNTRACED);

    memset(output_pipe, 0, sizeof output_pipe);

    return;
}


void grep_pipe(char* file, char* string)
{
    pid_t pid_1, pid_2;
    int fd[2];
    int status;
    char *mass_1[] = {"cat", file, NULL};
    char *mass_2[] = {"grep", string, NULL};
    pipe(fd);
      if(pipe(fd) == -1)
    {
        perror(NULL);
        return;
    }
    pid_1 = fork();
    if (pid_1 == 0)
    {
        dup2(fd[1], 1);
        close(fd[0]);
        execvp(mass_1[0], mass_1);
        exit(1);
    }
    pid_2 = fork();
    if (pid_2 == 0)
    {
        dup2(fd[0], 0);
        close(fd[1]);
        execvp(mass_2[0], mass_2);
        exit(1);
    }
    
    close(fd[0]);
    close(fd[1]);
    waitpid(pid_1, &status, WUNTRACED);
    waitpid(pid_2, &status, WUNTRACED);

    memset(output_pipe, 0, sizeof output_pipe);

    return;
}


void exec(char **arg, int nr_args, char *raw_com)
{
    // verificam pt fiecare comanda daca nr de arg este corect
    if(!strcmp(arg[0], "help")){
        if(nr_args!=1){
            error= 9;
            return;
        }
        help();
    }
    else if(!strcmp(arg[0], "history")){
        if(nr_args!=1){
            error=9;
            return;
        }
        hist(raw_com, true, false);
        return;
    }
    else if(!strcmp(arg[0], "hcm")){
        if(nr_args!=1){
            error=9;
            return;
        }
        hcm();
        return;
    }
    else if(!strcmp(arg[0],"clear")){
        if(nr_args!=1){
            error=9;
            return;
        }
        myclear();
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
        myecho();
    }

    else if (!strcmp(arg[0], "grep"))
    {

        if(nr_args == 3)
            grep(arg[1], arg[2]);
        if(nr_args == 2){
            grep_pipe_echo(arg[1]);
        }
        
    }
    else if(!strcmp(arg[0], "cat"))
    {

        if(arg[2] != NULL && (!strcmp(arg[2], "grep"))) 
        {
            grep_pipe(arg[1], arg[3]);
            return;
        }
        if(nr_args != 2)
        {
            error = 9;
            return;
        }
        cat(arg[1]);
    }

    else if (!strcmp(arg[0], "quit"))
    {
        if (nr_args != 1)
        {
            error = 9;
            return;
        }
        free(argv);
        free(comanda);
        free(history);
        free(output_pipe);
        ///endwin();
        exit(0);
    }
    else
    {
        error = 10;
        return;
    }
}

void colors(int color){
    ///Colorarea textului folosind ANSI
    switch(color){
        case 1:
            ///Red
            printf("\033[1;31m");
            break;
        case 2:
            ///Yellow
              printf("\033[1;33m");

            break;
        case 3:
            ///Green
            printf("\033[0;32m");
            break;
        case 4:
            ///Blue
            printf("\033[0;34m");
            break;
        default:
            ///reset
            printf("\033[0m");
    }
}
