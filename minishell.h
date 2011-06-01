#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <malloc.h>
#include <fcntl.h>
#include <wait.h>
#include <stdlib.h>

typedef struct exec_list{
	char *cmd;
	struct exec_list *next;
} exec_list;

typedef struct proc_list{
	char **argv;
	char *redir;
	int redir_type;
	int run_background;
	struct proc_list *next;
} proc_list;

void trim(char *buf);
char *get_prompt();
int strmatch(char *a,char *b,int n);
exec_list *parse_wildcards(char *cmd);
proc_list *parse_proc(char *cmd);
void free_proc(proc_list *x);
void exec_proc(proc_list *proclist);
void execute(char *cmd);
int shell_main();
