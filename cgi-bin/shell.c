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

void trim(char *buf){
	char *x;
	int i,len;
	x = buf;
	while(*x == ' ' || *x == '\t' || *x == '\n'){
		if(*x == 0){*buf = 0; return;}
		x++;
	}
	len = strlen(x);
	for(i=0;i<len;i++)
		buf[i]=*(x+i);
	i--;
	while(i>=0 && buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\n')
		i--;
	buf[i+1]=0;
}
char *get_prompt(){
	char *buf = (char *)malloc(sizeof(char)*1024);
	printf("%s $ ",get_current_dir_name());
	fgets(buf,1024,stdin);
	trim(buf);
	return buf;
}

int strmatch(char *a,char *b,int n){
	int i;
	for(i=0;i<n;i++)
		if(*(a+i) != *(b+i)) return 0;
	return 1;
}

exec_list *parse_wildcards(char *cmd){
	exec_list *ret,*ptr;
	DIR *dp;
	struct dirent *item;
	ret = NULL;
	if(strchr(cmd,'?')){
		int len1,len2;
		char *st,*ed,*p;

		p = strchr(cmd,'?');

		ed = p;
		while(*ed != '\n' && *ed != '\t' && *ed != 0 && *ed != '<' && *ed != '>' && *ed != ' ' && *ed != '|' )
			ed++;
		len2 = ed-p-1;

		st = p;
		while( st != cmd && *st != '\n' && *st != '\t' && *st != 0 && *st != '<' && *st != '>' && *st != ' ' && *st != '|' )
			st--;
		len1 = p-st-1;
		if( st == cmd ) len1++;
		else st++;

		dp = opendir(get_current_dir_name());

		if(dp != NULL){
			while(1){
				item = readdir(dp);
				if(item == NULL) break;
				if(*st != '.' && item->d_name[0] == '.') continue;
				if(ed-st == strlen(item->d_name) && strmatch(st,item->d_name,len1) && strmatch(p+1,item->d_name+len1+1,len2)){
					if( ret == NULL ){
						ptr = (exec_list *)malloc(sizeof(exec_list));
						ret = ptr;
					}else{
						ptr->next = (exec_list *)malloc(sizeof(exec_list));
						ptr = ptr->next;
					}
					int newlen = strlen(cmd)-(ed-st)+strlen(item->d_name)+1;
					ptr->cmd = (char *)malloc(sizeof(char)*newlen);
					strncpy(ptr->cmd,cmd,st-cmd);
					strncpy(ptr->cmd+(st-cmd),item->d_name,strlen(item->d_name));
					strcpy(ptr->cmd+(st-cmd)+strlen(item->d_name),ed);
					ptr->next = NULL;
				}
			}
		}

	}else if(strchr(cmd,'*') != NULL){
		int len1,len2;
		char *st,*ed,*p;

		p = strchr(cmd,'*');

		ed = p;
		while(*ed != '\n' && *ed != 0 && *ed != '<' && *ed != '>' && *ed != ' ' && *ed != '|' )
			ed++;
		len2 = ed-p-1;

		st = p;
		while( st != cmd && *st != '\n' && *st != 0 && *st != '<' && *st != '>' && *st != ' ' && *st != '|' )
			st--;
		len1 = p-st-1;
		if( st == cmd ) len1++;
		else st++;

		dp = opendir(get_current_dir_name());

		if(dp != NULL){
			while(1){
				item = readdir(dp);
				if(item == NULL) break;
				if(*st != '.' && item->d_name[0] == '.') continue;
				if(ed-st-1 <= strlen(item->d_name) && strmatch(st,item->d_name,len1) && strmatch(p+1,item->d_name+strlen(item->d_name)-len2,len2)){
					if( ret == NULL ){
						ptr = (exec_list *)malloc(sizeof(exec_list));
						ret = ptr;
					}else{
						ptr->next = (exec_list *)malloc(sizeof(exec_list));
						ptr = ptr->next;
					}
					int newlen = strlen(cmd)-(ed-st)+strlen(item->d_name)+1;
					ptr->cmd = (char *)malloc(sizeof(char)*newlen);
					strncpy(ptr->cmd,cmd,st-cmd);
					strncpy(ptr->cmd+(st-cmd),item->d_name,strlen(item->d_name));
					strcpy(ptr->cmd+(st-cmd)+strlen(item->d_name),ed);
					ptr->next = NULL;
				}
			}
		}

	}else{
		ret = (exec_list *)malloc(sizeof(exec_list));
		ret->cmd = (char *)malloc(sizeof(char)*(strlen(cmd)+1));
		strcpy(ret->cmd,cmd);
		ret->next = NULL;
	}
	return ret;
}

char **str2argv(char *cmd){
	int i,cnt;
	int argc;
	char **argv,**ptr;
	trim(cmd);
	for(i=0,cnt=0,argc=0;i<strlen(cmd);i++){
		if(*(cmd+i) == ' ' || *(cmd+i) == '\t' || *(cmd+i) == '\n' || *(cmd+i) == 0){
			if(cnt>0){
				argc++;
				cnt=0;
			}
		}else{
			cnt++;
		}
	}
	if(cnt>0) argc++;
	argv = (char **)malloc(sizeof(char *)*(argc+1));
	for(i=0,cnt=0,ptr=argv;i<strlen(cmd);i++){
		if(*(cmd+i) == ' ' || *(cmd+i) == '\t' || *(cmd+i) == '\n' || *(cmd+i) == 0){
			if(cnt>0){
				*ptr = (char *)malloc(sizeof(char *)*(cnt+1));
				strncpy(*ptr,cmd+i-cnt,cnt);
				*((*ptr)+cnt) = 0;
				ptr++;
				cnt=0;
			}
		}else{
			cnt++;
		}
	}
	if(cnt>0){
		*ptr = (char *)malloc(sizeof(char *)*(cnt+1));
		strncpy(*ptr,cmd+i-cnt,cnt);
		*(*(ptr)+cnt) = 0;
		ptr++;
	}
	*ptr = 0;
	return argv;
}

proc_list *parse_proc(char *cmd){
	char *str = cmd,*p,*pt,*st,*ed,*ptrcmd;
	proc_list *ret,*ptr;
	ret = NULL;
	while(strchr(str,'|') != NULL){
		if(ret == NULL){
			ret = (proc_list *)malloc(sizeof(proc_list));
			ptr = ret;
		}else{
			ptr->next = (proc_list *)malloc(sizeof(proc_list));
			ptr = ptr->next;
		}
		p = strchr(str,'|');

		ptrcmd = (char *)malloc(sizeof(char)*(p-str+1));
		strncpy(ptrcmd,str,p-str);
		*(ptrcmd+(p-str)) = 0;
		trim(ptrcmd);
		if(strlen(ptrcmd) == 0) {ptr->redir_type =-1; ptr->next = NULL; free(ptrcmd); str = p+1; continue;}
		if(ptrcmd[strlen(ptrcmd)-1] == '&'){
			ptrcmd[strlen(ptrcmd)-1] = 0;
			ptr->run_background = 1;
		}else{
			ptr->run_background = 0;
		}
		if((pt=strstr(ptrcmd,">>") != NULL)){
			ptr->redir_type = 3;
		}else if((pt=strchr(ptrcmd,'>')) != NULL){
			ptr->redir_type = 2;
		}else if((pt=strchr(ptrcmd,'<')) != NULL){
			ptr->redir_type = 1;
		}else{
			ptr->redir_type = 0;
		}
		if(ptr->redir_type != 0){
			st = pt;
			while(*st == ' ' || *st == '\t' || *st == '>' || *st == '<')
				st++;
			ed=st;
			while(!(*ed == ' ' || *ed == '\t' || *ed == '>' || *ed == '<' || *ed == '|' || *ed == 0 || *ed == '\n'))
				ed++;
			if(st==ed) ptr->redir_type = -1;
			else{
				ptr->redir = (char *)malloc(sizeof(char)*(ed-st+1));
				strncpy(ptr->redir,st,ed-st);
				*(ptr->redir+(ed-st)) = 0;
				strcpy(pt,ed);
			}
		}else{
			ptr->redir = NULL;
		}
		if(ptr->redir_type != -1) ptr->argv = str2argv(ptrcmd);
		str = p+1;
		ptr->next = NULL;
		free(ptrcmd);
	}
	if(ret == NULL){
		ret = (proc_list *)malloc(sizeof(proc_list));
		ptr = ret;
	}else{
		ptr->next = (proc_list *)malloc(sizeof(proc_list));
		ptr = ptr->next;
	}
	ptrcmd = (char *)malloc(sizeof(char)*(strlen(str)+1));
	strcpy(ptrcmd,str);
	trim(ptrcmd);
	if(strlen(ptrcmd) == 0) { ptr->redir_type = -1; ptr->next = NULL; free(ptrcmd); return ret;}
	if(ptrcmd[strlen(ptrcmd)-1] == '&'){
		ptrcmd[strlen(ptrcmd)-1] = 0;
		ptr->run_background = 1;
	}else{
		ptr->run_background = 0;
	}
	if((pt=strstr(ptrcmd,">>")) != NULL){
		ptr->redir_type = 3;
	}else if((pt=strchr(ptrcmd,'>')) != NULL){
		ptr->redir_type = 2;
	}else if((pt=strchr(ptrcmd,'<')) != NULL){
		ptr->redir_type = 1;
	}else{
		ptr->redir_type = 0;
	}
	if(ptr->redir_type != 0){
		st = pt;
		while(*st == ' ' || *st == '\t' || *st == '>' || *st == '<')
			st++;
		ed=st;
		while(!(*ed == ' ' || *ed == '\t' || *ed == '>' || *ed == '<' || *ed == '|' || *ed == 0 || *ed == '\n'))
			ed++;
		if(st == ed) ptr->redir_type = -1;
		else{
			ptr->redir = (char *)malloc(sizeof(char)*(ed-st+1));
			strncpy(ptr->redir,st,ed-st);
			*(ptr->redir+(ed-st)) = 0;
			strcpy(pt,ed);
		}
	}else{
		ptr->redir = NULL;
	}
	if(ptr->redir_type != -1) ptr->argv = str2argv(ptrcmd);
	ptr->next = NULL;
	free(ptrcmd);
	return ret;
}

void free_proc(proc_list *x){
	if(!x) return;
	int i;
	if(x->redir_type != -1){
		for(i=0;x->argv[i] != NULL;i++)
			free(x->argv[i]);
		free(x->argv);
		if(x->redir_type != 0)
			free(x->redir);
		if(x->next) free_proc(x->next);
	}
	free(x);
}

void exec_proc(proc_list *proclist){
	int fd[2];
	int ifile,ofile,pid,status;
	proc_list *chk;
	ifile = STDIN_FILENO;
	chk = proclist;
	while(chk){
		if(chk->redir_type <0){fprintf(stderr,"Parse Error\n"); return;}
		chk = chk->next;
	}
	while(proclist){
		if(proclist->next){
			if(pipe(fd) < 0){
				fprintf(stderr,"Pipe Creation Failed\n");
				exit(1);
			}
			ofile = fd[1];
		}else{
			ofile = STDOUT_FILENO;
		}
		if(proclist->redir_type !=0){
			if(proclist->redir_type == 1){
				ifile = open(proclist->redir,O_RDONLY);
			}else if(proclist->redir_type == 2){
				ofile = open(proclist->redir,O_WRONLY | O_CREAT, 0644);
			}else if(proclist->redir_type == 3){
				ofile = open(proclist->redir,O_WRONLY | O_APPEND | O_CREAT, 0644);
			}
		}
		pid = fork();
		if(pid < 0){
			fprintf(stderr,"Fork Failed\n");
			exit(1);
		}else if(pid == 0){ //Child
			signal (SIGINT,SIG_DFL);
			signal (SIGTSTP,SIG_DFL);
			if(ifile != STDIN_FILENO){
				dup2(ifile,STDIN_FILENO);
				close(ifile);
			}
	 		if(ofile != STDOUT_FILENO){
				dup2(ofile,STDOUT_FILENO);
				close(ofile);
			}
			if(execvp(proclist->argv[0],proclist->argv)<0)
				fprintf(stderr,"Command Not Found\n");
			exit(1);
		}else{ //Parent
			if(proclist->run_background == 0){
				waitpid(pid,&status,0);
			}else{
				waitpid(pid,&status,WNOHANG);
			}
		}
		if(ifile != STDIN_FILENO) close(ifile);
		if(ofile != STDOUT_FILENO) close(ofile);
		ifile = fd[0];
		proclist = proclist->next;
	}
}

void execute(char *cmd){
	proc_list *proclist = parse_proc(cmd);
	exec_proc(proclist);
	free_proc(proclist);
}

int main(int argc,char**argv){
	char *cmd;
	exec_list *execlist,*ptr;
	int status;
	signal (SIGTSTP,SIG_IGN);
	signal (SIGINT,SIG_IGN);
	printf("Content-length: \r\n");
	printf("Content-type: text/html\r\n\r\n");
	fflush(stdout);
	if((cmd = getenv("QUERY_STRING")) != NULL){
		fprintf(stderr,"Shell Executed by %d\n",STDOUT_FILENO);
		if(strcmp(cmd,"exit") == 0){ return 0;}
		if(strlen(cmd) != 0){
			execlist = parse_wildcards(cmd);
			if(execlist == NULL) fprintf(stderr,"No Matching Wildcards\n");
			else{
				while(execlist != NULL){
					execute(execlist->cmd);
					free(execlist->cmd);
					ptr = execlist->next;
					free(execlist);
					execlist = ptr;
				}
			}
			waitpid(-1,&status,WNOHANG);
		}	
		waitpid(-1,&status,WNOHANG);
	}
	fflush(stdout);
	return 0;
}

