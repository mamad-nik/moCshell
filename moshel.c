#include <stddef.h>
#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#define ARG_MAX 4096
  	
char buff[ARG_MAX];
char *cmd;
char *arg[ARG_MAX];
char *home;
char *user;
char host[255];
char *cwd;
int cmdlength;
int bg;
char *built_in[] = {"echo","pwd","cd"};

int pwd(int p) {
	cwd = getcwd(NULL,0);
	if (p == 1) printf("%s\n",cwd);
	return 0;
}


void checkbg() {
	if (!strcmp("&", arg[cmdlength-1])) {
		bg = 1;
		arg[cmdlength-1] = NULL;
	} else {
		bg = 0;
	}
}

int parse_simple_cmd() {
	char *ptr = strtok(buff, " ");
	cmd = ptr;
	arg[0] = ptr;
	int i = 1;
	while(ptr != NULL) {
		ptr = strtok(NULL, " ");
		arg[i] = ptr;
		i++;
	}
	cmdlength = i-1;
	checkbg();
	return cmdlength;
}

int echo() {
	for (int i = 1;i < cmdlength; i++) {
		printf("%s ",arg[i]);
	}
    printf("\n");
	return 0;	
}
int cd() {
	if(arg[1] == NULL) {
		arg[1] = home;
		cwd = home;
	}
	if (chdir(arg[1])) {
		printf("error changing directory: %d\n",errno);
		return errno;
	}
	pwd(0);
	return 0;
}
int get_input() {
	if (NULL == fgets(buff, ARG_MAX, stdin)){
		printf("error fgets");
		exit(EXIT_FAILURE);
	}
	if (buff[0] == '\n') {
		return 1;
	}
	if ((strlen(buff) > 0) && (buff[strlen (buff) - 1] == '\n'))
		buff[strlen (buff) - 1] = '\0';
	printf("your order was: %s\n",buff);
	return 0;
		
}

void ps1() {
	printf("%s@%s: %s$ ",user,host,cwd);
}
int preparation() {
	home = getenv("HOME");
	user = getenv("USER");
	if (0 != gethostname(host, sizeof(host))) {
		printf("error getting hostname");
		exit(EXIT_FAILURE);
	}
	cd();
	return 0;
}

int exec_external() {
	pid_t pid;
	int status;
	pid = fork();

	if (pid < 0) {
		fprintf(stderr, "error making a new process, errno = %d",errno);
	} else if (pid == 0) {
		if (-1 == execvp(cmd, arg)){
			printf("error executing command: %d\n",errno);
			exit(EXIT_FAILURE);
		}

	} else {
		if (bg){
			printf("[%d]\n",pid);
		} else {
			waitpid(pid,&status,0);
		}
	}
	
	return 0;
}

int check_cmd() {
	int i;
	for (i = 0; i < sizeof(built_in)/sizeof(char *); i++) {
		if (!strcmp(built_in[i], cmd)) {
			break;
		}
	}
	switch (i) {
			case 0:
				echo();
				break;
			case 1:
				pwd(1);
				break;
			case 2:
				cd();
				break;
			default:
				exec_external();
				break;
		}
	return 0;	
}

int main() {
	preparation();
	while(1){
		ps1();
		if (1 == get_input()) {
			continue;
		}
		int s = parse_simple_cmd();		
		check_cmd();
		
	}
}
