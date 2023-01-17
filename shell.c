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
///ncurses is used for opening a new window and to get key input
///used for hcm - history commands with arrow kay selection
#include <ncurses.h>
///this is the maximum number of commands
///saved in the history array
#define MAX_HIST 50

int error= 0, nr; ///error - error number, nr - nr of args in a command
int hist_count = 0, hist_found_comm; ///nr of commands in history and the command chosen from the hcm funciton
bool flag_has_input = false; ///boolean value that is true if a command was chosen from hisotry
char path[1024]; ///current absolute path
char *output; ///output - printed text
char *output_pipe; ///output from a funciton to be used by the next command
char **argv, **comanda; ///list of commands
char **history; ///history command array
int does_pipe = 0; ///pipe flag

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
void grep_pipe_echo(char* string);
void exec(char **arg, int nr_args, char *raw_com);
void colors(int color);
void grep(char* string, char* file);
void grep_pipe(char* file, char* string);
void grep_pipe_ls(char* string);
void cat(char* file);

int main()
{
    output = malloc(1024 * sizeof(char));
    output_pipe = malloc(2048 * sizeof(char));
    history = malloc(sizeof(char*)*MAX_HIST);
    create_shell();

    while(1){
        error=0;
        ///char *getcwd(char *buf, size_t size)
        ///gets the absolute path of the working dir into the buffer 
        if (getcwd(path, sizeof(path)) != NULL){
            colors(3); ///green
            printf("SHELL: %s$ ", path);
            colors(-1); ////reset color
        } 
        else{
            error = 2;
        }

        char* buf = NULL; ///input buffer
        char *aux_buf = NULL;
        size_t buflen = 0;
        char** argv = malloc(sizeof(char*)*8); ///list of all arguments after parsing the input
        char** comanda = malloc(sizeof(char*)*8); ///we save the commands
        int nr=0; ///no argument of each command

        if(!flag_has_input){  ///has input flag is raised if we have a command selected from hcm
            printf("> ");
            getline(&buf, &buflen, stdin); //read input
        }else{
            ///if we have a command from hcm then we reset the flag and duplicate the
            ///command from the list of commands to the input buffer
            flag_has_input = false; 
            buf = strdup(history[hist_found_comm]);
        }
        ///we save the input in the history list
        hist(buf, false, true);
        //printf("> %s", buf);

	    ///parsing the input and finding all the arguments
	    ///we return the number of arguments found
        int argc = parsingSpace(buf, argv);
        
        if(argv[0] == NULL)
        {
            ///for null command
            continue;
        }
        bool flag_or = true; ///flag for 'or' operator
        for(int i = 0; i < argc; ++i){
            does_pipe = 0;
            //printf("> arg %d = %s\n", i, argv[i]);
            
            if (!strcmp(argv[i], "||")) ///when we meet 'or' log.op.
            {
                free(output);
                output = malloc(1024 * sizeof(char));
                exec(comanda, nr, buf);

                if(error)
                {
                    //if we found an error we print it and we reset the error
                    error_msg(error, comanda[0]);
                    error = 0;
                    nr = 0;                 
                }
                else 
                {
                    //after the first working command found we skip to the first && or to the end
                    nr = 0;
                    while(strcmp(argv[i], "&&")){
                        ++i;
                        if(argv[i] == NULL) {
                            flag_or = false;
                            break;  
                        }
                    }
                }
            }
            else if (!strcmp(argv[i], "&&")) ///when we meet 'and' log.op.
            {
                free(output);
                output = malloc(1024 * sizeof(char));
                exec(comanda, nr, buf);
                
                //after the first error found we stop
                if (error != 0)
                {
                    break;
                }

                nr = 0;
            }
            else if(!strcmp(argv[i], "|")) { ///when we meet 'pipe' operator
                does_pipe = 1; ///we raise the doing pipe flag
                // if there is a pipe keep reading the command
                if(!strcmp(argv[0], "cat")) {
                    continue;
                }
                if(!strcmp(argv[0], "ls")){
                    continue;
                }
                free(output);
                output = malloc(1024 * sizeof(char));
                exec(comanda, nr, buf);
                
                // we stop at the first error
                if (error != 0)
                {
                    break;
                }

                nr = 0;
            }
            else {
                comanda[nr++]= argv[i];
            } 

        }
        does_pipe = 0;
       
        // if we don't have errors we execute the command
        if (error == 0 && flag_or){
            free(output);
            output = malloc(1024 * sizeof(char));
            exec(comanda, nr, buf);
        }

        // if we found errors we print them
        if(error){
            error_msg(error, argv[0]);
            error = 0;
        }
    
        
        free(argv);
        free(buf);
        free(aux_buf);
        free(comanda);
    } 
    
    return 0;
}

// START SHELL
void create_shell(){
    colors(2); ///yellow
    printf("\n--------------------------Proiect Sisteme de Operare - Grupa 251-------------------------\n");
    printf("\n----------Alpetri Iulita, Messner Annemarie-Beatrix si Patranjel David-George------------\n");
    colors(-1); ///reset color
    colors(1); ///red
    char* username = getenv("USER");
    printf("USER is: @%s\n", username);
    colors(-1); ///reset color
    sleep(1);
}

// parsing the input line
int parsingSpace(char *buf, char **argv){
    int i = 0;  ///no. args found
    int capacity = 4; ///first we consider that we have max 4 args
    char *delimiters = " \t\r\n"; ///delimiters: sapce, tab, carriage return, newline
    char *aux = strtok(buf, delimiters); ///we parse with strtok

    while (aux != NULL) {
        argv[i] = aux;
        i++;

        if (i >= capacity) {
            ///if needed, we multiply the max capacity by 1.5
            capacity = (int) (capacity * 1.5);
            argv = realloc(argv, sizeof(char*) * capacity);
        }

        aux = strtok(NULL, delimiters);
    }
    argv[i] = NULL; ///last arg is null
    return i;
}

// error_msg - printing a specific message for every error code 
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

//help - prints the command manual 
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
    printf("%s---", output);
    return;
}

//history - prints the command history of the current session
void hist(const char* command, bool wr, bool save)
{
    ///wr bool is true only if the command is used to print the history (via command)
    if(wr){
    	///we print the list of saved commands from newest to oldest (if we are not piping)
        if(!does_pipe)
             printf("\n---------Command History--------\n");
        if(hist_count == 0) {
            if(!does_pipe)
                printf("No commands yet!\n");
        }
        else {
                for(int i = hist_count - 1; i >= 0; --i) {
                strcat(output_pipe, history[i]);
                if(!does_pipe)
                    printf("%s", history[i]);
            }
        }
            
    }
    
    ///save is true only when we want to save the command, we usualy dont print it
    if(save){
    	///if we reached the maximum no commands, we delete the oldest one
        bool move_flag = hist_count >= MAX_HIST ? true : false;
        if(move_flag){
            free(history[0]);
            for(int i = 0; i < MAX_HIST - 1; ++i){
                history[i] = history[i + 1];
            }
            hist_count--;
        }
	    ///strdup - it duplicates the buffer
        history[hist_count++] = strdup(command);
    }
    return;
}

//hcm - use old command by using the arrow keys 
void hcm()
{
    ///when we start, we create a screen and we go from the last command to the first one
    hist_found_comm = hist_count;
    while(1){
        initscr();
        int ch = getch();        
        if (ch == '\033') { // if the first value is esc
            getch(); // skip the [
            switch(getch()) { // the real value
                case 'A':
                    // code for arrow up
                    clear(); ///clear the screen
                    if(hist_found_comm == 0){ ///the first command
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
                    if(hist_found_comm == hist_count - 1){ ///the last command
                        addstr(history[hist_count - 1]);
                    }
                    else if(hist_found_comm < hist_count - 1){
                        addstr(history[++hist_found_comm]);
                    }
                    ///printf("Down arrow\n");
                    break;
                default:
                    clear();
                    endwin(); ///we exit the window and we dont select a command
                    break;
            }
        }else if(ch == '\n'){ 
            ///if enter was pressed, then we know that the selected command selected
            ///is where the hist_found_comm stopped stopped changing
            clear();
            endwin();
            printf("Done\n");
            printf("Next command is %s", history[hist_found_comm]);
            flag_has_input = true;
            break;
        }
    }
    return;
}

//clear - clears the terminal
void myclear()
{
	write(1, "\33[H\33[2J", 7);
	// ANSI escape code, \33[H- moves the cursor to the top left corner of the screen, 
        //33[2J- clears the part of the screen from the cursor to the end of the screen.
        return;
}

//cd - changes the working directory
void cd(char* folder)
{
    //chdir command is a system function which is used to change the current working directory
    //returns 0 for success, -1 otherwise
    if (chdir(folder))
    {
        error = 1;
    }
    return;
}

//pwd - prints the path of the current directory
void pwd()
{
    ///char *getcwd(char *buf, size_t size)
    ///gets the absolute path of the working dir into the buffer 

    if (getcwd(path, sizeof(path))) 
    {
        strcat(output, path);
        printf("%s\n", output);
    } 
    else 
    {
        error = 2;
    }
    return;
}

//ls - lists all files and directories in the current directory
void ls()
{
    //we create a new child procces that calls ls syscall
    pid_t pid = fork ();
    if (pid == 0)
    {
    	///ls overrides the current process
        char *arguments[] = {"ls", NULL};
        execve ("/bin/ls", arguments , NULL);
    }
    else
    {
        wait(NULL);
        // waiting for the child process
    }
    return;
}

//touch - creates a new empty file
void touch(char* file)
{
   
    // with fopen, we create a writable file
    FILE *aux; ///return is a file pointer
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
    return;
}

//rm - deletes a file
void rm (char* filename)
{
    ///char *getcwd(char *buf, size_t size)
    ///gets the absolute path of the working dir into the buffer 
    char file_path[1024];
    if (getcwd(file_path, sizeof(file_path)) == NULL) 
    {
        error = 2;
    }

    ///we get the absolute path of the given file
    strcat(file_path, "/");
    strcat(file_path, filename);

    if(!remove(file_path))// remove - deletes the given filename, on success, returns 0, else -1
	{
        strcat(output, "File deleted succesfully");
        printf("%s\n", output);
	}
	else
	{
		error = 8;
	}
	return;
}

//cp - copies the contents of a file to another file
void cp(char* file1, char* file2)
{
	char caract;
	FILE *f1, *f2;
	
	if ((f1 = fopen(file1, "r")) == 0) ///open the source file with read acces rigths
	{
		error = 3;
        return;
	}

    
    if ((f2 = fopen(file2, "w")) == 0) ///open the destination file with write acces rigths
	{
		error = 4;
        return;
	}

	caract = fgetc(f1); ///reads character by character from source file

	while(caract != EOF) ///while not end of file
	{
		
		fputc(caract, f2); ///write character by character in the destination file
		caract = fgetc(f1);
	}

    strcat(output, "File copied succesfully");
    printf("%s\n", output);
	
	fclose(f1);
	fclose(f2);
	return;
}
	
//makedir - creates a new directory
void makedir(char* folder)
{
    ///char *getcwd(char *buf, size_t size)
    ///gets the absolute path of the working dir into the buffer 
    if (getcwd(path, sizeof(path)) == NULL)
    {
        error = 2;
        return;
    }

    ///we get the absolute path of the given directory
    strcat(path, "/");
    strcat(path, folder);

    ///int mkdir(const char* pathname, mode_t mode);
    ///attempts to create a directory named pathname
    ///mode = 0777 (write, read, execution)
    ///return = 0 succes, -1 error
    if (mkdir(path, 0777) == -1)
	{
        error = 5;
    }
    else
    {
        strcat(output, "Folder created succesfully");
        printf("%s\n", output);
    }
    return;
}

//removedir - deletes a directory
void removedir(char* folder)
{
    ///char *getcwd(char *buf, size_t size)
    ///gets the absolute path of the working dir into the buffer 
    
    if (getcwd(path, sizeof(path)) == NULL) // obtinem calea
    {
        error = 2;
        return;
    }
    
    
    ///we get the absolute path of the given directory
    strcat(path, "/");
    strcat(path, folder);
	
    ///int rmdir(const char* pathname);
    ///attempts to delete a directory named pathname
    ///return = 0 succes, -1 error
    if(rmdir(path) == -1)
    {
        error = 6;
    }
    else
    {
        strcat(output, "Folder deleted succesfully");
        printf("%s\n", output);
    }
    return;
}

//echo - prints the given arguments
void myecho(char** arg, int nr)
{
    // afisam toate cuvintele scrise dupa comanda echo
    for (int i = 1; i < nr; i ++)
    {
        printf("%s ", arg[i]);
    }
    printf("\n");
}

// cat - prints the contents of a file
void cat(char* file) {
    //we create a new child procces that calls cat syscall
    pid_t pid = fork();
    
    if(pid == 0) {
	    ///cat overrides the current process
        char* arguments[] = {"cat", file, NULL};
        execve("/bin/cat", arguments, NULL);
    } else {
    	///parent waits for the child process
        wait(NULL);
    }
    return;
}

//grep - searches for a pattern in a file
void grep(char* string, char* file) {
    //we create a new child procces that calls grep syscall
    pid_t pid = fork();
    
    if(pid == 0) {
	    ///grep overrides the current process
        char* arguments[] = {"grep", string, file, NULL};
        execve("/bin/grep", arguments, NULL);
    } else {
        ///parent waits for the child process
        wait(NULL);
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
// grep cat pipe
void grep_pipe(char* file, char* string) {
    pid_t pid1, pid2;
    int fd[2];
    int status;
    char* mass_1[] = {"cat", file, NULL};
    char* mass_2[] = {"grep", string,  NULL};

    ///int pipe(int pipefd[2])
    ///creates an unidirectional pipe
    ///pipefd[0] - read from pipe end, pipefd[1] - write to pipe end
    ///returns -1 on error
    pipe(fd);
    if(pipe(fd) == -1) {
        perror(NULL);
        return;
    }
    pid1 = fork();
    if(pid1 == 0) {
        ///int dup2(int oldfd, int newfd)
        ///makes newfd be a copy of oldfd
        ///we do this so that the write fd is the standard one
        dup2(fd[1], 1);
        ///we close the read end
        close(fd[0]);
        ///int execvp(const char *file, char *const argv[]);
        ///just like execve, but without the absolute path
        execvp(mass_1[0], mass_1);
    }

    pid2 = fork();
    if(pid2 == 0) {
        ///int dup2(int oldfd, int newfd)
        ///makes newfd be a copy of oldfd
        ///we do this so that the read fd is the standard one
        dup2(fd[0], 0);
        ///we close the write end
        close(fd[1]);
        ///int execvp(const char *file, char *const argv[]);
        ///just like execve, but without the absolute path
        execvp(mass_2[0], mass_2);
    }

    close(fd[0]);
    close(fd[1]);
    waitpid(pid1, &status, WUNTRACED);
    waitpid(pid2, &status, WUNTRACED);

    return;
}

// ls grep pipe
void grep_pipe_ls(char* string) {
    pid_t pid_1, pid_2;
    int fd[2];
    int status;
    char *mass_1[] = {"ls", NULL};
    char *mass_2[] = {"grep", "-a", string, NULL};

    ///int pipe(int pipefd[2])
    ///creates an unidirectional pipe
    ///pipefd[0] - read from pipe end, pipefd[1] - write to pipe end
    ///returns -1 on error
    pipe(fd);
      if(pipe(fd) == -1)
    {
        perror(NULL);
        return;
    }
    pid_1 = fork();
    if (pid_1 == 0)
    {
        ///int dup2(int oldfd, int newfd)
        ///makes newfd be a copy of oldfd
        ///we do this so that the write fd is the standard one
        dup2(fd[1], 1);
        ///we close the write end
        close(fd[0]);
        ///int execvp(const char *file, char *const argv[]);
        ///just like execve, but without the absolute path
        execvp(mass_1[0], mass_1);
        exit(1);
    }
    pid_2 = fork();
    if (pid_2 == 0)
    {
        ///int dup2(int oldfd, int newfd)
        ///makes newfd be a copy of oldfd
        ///we do this so that the read fd is the standard one
        dup2(fd[0], 0);
        ///we close the read end
        close(fd[1]);
        ///int execvp(const char *file, char *const argv[]);
        ///just like execve, but without the absolute path
        execvp(mass_2[0], mass_2);
        exit(1);
    }
    
    close(fd[0]);
    close(fd[1]);
    waitpid(pid_1, &status, WUNTRACED);
    waitpid(pid_2, &status, WUNTRACED);

    return;
}

// exec - command execution
void exec(char **arg, int nr_args, char *raw_com)
{
    // for each command we check if the no of args is correct
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
        if(arg[1] != NULL && (!strcmp(arg[1], "grep"))) // verify if the second argument is grep
        {
            if(nr_args != 3) // verify if the number of arguments is correct
            {
                error = 9;
                return;
            }
            grep_pipe_ls(arg[2]); // call the function grep_pipe_ls
            return;
        }
        if(nr_args != 1){
            error = 9;
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
        myecho(arg, nr_args);
    }

    else if(!strcmp(arg[0], "grep")) {
        if(nr_args == 3) 
            grep(arg[1], arg[2]);
        if(nr_args == 2){
            grep_pipe_echo(arg[1]);
        }
    }

    else if(!strcmp(arg[0], "cat")) {
        if(arg[2] != NULL && (!strcmp(arg[2], "grep"))) {
            grep_pipe(arg[1], arg[3]);
            return;
        }
        if(nr_args != 2) {
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
        for(int i = 0; i < hist_count; i++){
                free(history[i]);
        }
        free(history);
        free(output);
        free(output_pipe);
        exit(0);
    }
    else
    {
        error = 10;
        return;
    }
}

// colors - gives colors to text in terminal
void colors(int color){
    ///We use ANSI  escape codes (com. with the terminal)
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