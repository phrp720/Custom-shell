/*
 *Author : Filippos Rafail Papadakis
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <unistd.h>
#include <sys/wait.h>
#include <termios.h>
int gl_pid = 0;
int signal_flag = 0;
int pipe__ = 0;
int counter = 0;
/*Struct for the list of commands*/
typedef struct command
{
    char **cmd;
    size_t size;
    size_t max;
} * command_t;
/*Struct for the list of processes*/
typedef struct node
{
    int pid;
    struct node *next;
} my_list;

my_list *list = NULL;
/*Adds process to list*/
void addprocess()
{
    my_list *tmp = malloc(sizeof(my_list));
    tmp->next = list;
    list = tmp;
}
/*Removes process from process list*/
void removeProcess()
{
    if (list == NULL)
        return;

    my_list *temp = list;
    list = list->next;

    free(temp);
}
/*If we catch Ctrl+Z then stop process with pid list->pid*/
void signal_handler(int signo)
{
    if (signo == SIGTSTP)
    {
        list->pid = gl_pid;
        signal_flag = 1;
        kill(list->pid, SIGSTOP);
    }
    return;
}
/*Changes the characters with wildcard $*/
void getValue(command_t cmd, size_t first)
{
    for (size_t i = first; i < cmd->size; i++)
    {
        if (strchr(cmd->cmd[i], '$'))
        {
            char *ch2 = strchr(cmd->cmd[i], '$');
            char *var = getenv(ch2 + 1);
            free(cmd->cmd[i]);
            cmd->cmd[i] = var;
        }
    }
}
void addCommand(char *str, command_t command)
{
    if (command->size + 1 == command->max)
    {
        command->cmd = realloc(command->cmd, 2 * command->size);
        command->size *= 2;
    }

    command->cmd[command->size] = strdup(str);
    command->size++;
    command->cmd[command->size] = NULL;
}

void prompt_name()
{
    // POSIX method for getting file path metadata.
    char *path_name = malloc(1000);
    getcwd(path_name, pathconf(path_name, _PC_PATH_MAX));
    char *login = getlogin();
    printf("%s@cs345sh/%s$ ", login, path_name);
}
/*Execution if our commandline has pipes*/
void execute_pipes(command_t *command, size_t size, size_t prev)
{
    int fd[2];
    pid_t pid;
    int fd_input = 0;
    size_t i = 0;

    while (i < size)
    {
        pipe(fd);
        pid = fork();
        if (pid == 0)
        {
            dup2(fd_input, 0);
            if (i + 1 < size)
                dup2(fd[STDOUT_FILENO], 1);
            close(fd[STDIN_FILENO]);
            execvp(command[i]->cmd[prev], &command[i]->cmd[prev]);
            exit(0);
        }
        else
        {
            wait(NULL);
            close(fd[STDOUT_FILENO]);
            fd_input = fd[STDIN_FILENO]; 
            i++;
        }
    }
}
/*Classic execution if our command line has only one command*/
void execute(command_t cmd, size_t first)
{

    if (strchr(cmd->cmd[first], '=') && strchr(cmd->cmd[first], '=') != &cmd->cmd[first][0])
    {
        char *ch = strchr(cmd->cmd[first], '=');
        *ch = 0;
        printf("setting var %s = %s\n", cmd->cmd[first], ch + 1);
        setenv(cmd->cmd[first], ch + 1, 1);
        return;
    }
    if (!strcmp(cmd->cmd[first], "fg"))
    {
        if (list->next != NULL)
        {
            kill(list->next->pid, SIGCONT);
            waitpid(list->next->pid, NULL, WUNTRACED);
            removeProcess();
        }
        else
        {
            printf("fg: current: no such job\n");
        }
        return;
    }
    if (!strcmp(cmd->cmd[first], "exit"))
    {
        my_list *current = list;
        while (current)
        {
            if(current->pid!=0){
            kill(current->pid, SIGKILL);
            }
            current = current->next;
        }

        exit(0);
    }
    else if (!strcmp(cmd->cmd[first], "cd"))
    {
        if (chdir(cmd->cmd[first + 1]))
        {
            perror("cd: ");
        }
    }
    else
    {
        pid_t  pid = fork();
        if (pid == 0)
        {
            getValue(cmd, first);
            execvp(cmd->cmd[first], &cmd->cmd[first]);
            printf("Command %s not found\n", cmd->cmd[first]);
            exit(1);
        }
        else if (pid > 0)
        { // parent
            gl_pid = pid;
            waitpid(gl_pid, NULL, WUNTRACED);
        }
        else
        {
            perror("pid");
            return;
        }
    }
}
/*Used for debug*/
void printCommands(command_t *cmd, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        for (size_t j = 0; j < cmd[i]->size; j++) {
            puts(cmd[i]->cmd[j]);
        }
    }
}
/*Initialization of struct array of commands*/
void command_create(command_t command)
{
    command->cmd = malloc(500 * sizeof(char *));
    command->size = 0;
    command->max = 500;
    return;
}
int main(void)
{
    command_t *command = malloc(500 * sizeof(struct command));
    for (int i = 0; i < 500; i++)
    {
        command[i] = malloc(sizeof(struct command));
        command_create(command[i]);
    }
    struct termios artermios;
    /*Ctrl+Z handler*/
    signal(SIGTSTP, signal_handler);
    /*Ctrl+S && Ctrl+Q handler*/
    tcgetattr(STDIN_FILENO, &artermios);
    artermios.c_iflag |= (IXON);
    artermios.c_iflag |= (IXOFF);
    tcsetattr(STDIN_FILENO, TCSANOW, &artermios);
    size_t prev = 0;
    addprocess();
    while (1)
    {
        if (signal_flag == 1)
        {
            addprocess();
            signal_flag = 0;
        }
        prompt_name();

        char line[500];
        scanf(" %499[^\n]", line);
        char delim[] = "|";
        if (line[0] == delim[0])
        {
            printf("bash: syntax error near unexpected token `|'\n");
            continue;
        }
        /*If we got pipes then do this parse otherwise use the other parser*/
        if (strchr(line, '|'))
        {
            char *ptr = strtok(line, delim);

            pipe__ = 1;
            while (ptr)
            {
                char *save;
                char *com = strtok_r(ptr, " ", &save);

                while (com != NULL)
                {
                    addCommand(com, command[counter]);
                    com = strtok_r(NULL, " ", &save);
                }

                ptr = strtok(0, "|");
                counter++;
            }
            execute_pipes(command, counter,0);
            prev = command[counter]->size;
        }
        else
        {
            char *ptr_2 = strtok(line, " ");
            while (ptr_2)
            {
                addCommand(ptr_2, command[counter]);
                ptr_2 = strtok(NULL, " ");
            }

            execute(command[counter],0);
            counter++;
        
        }
        
    }

    return 0;
}
