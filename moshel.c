#include <stddef.h>
#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>


#define ARG_MAX 4096

// Global variables
char buff[ARG_MAX];
char *cmd;
char *arg[ARG_MAX];
char *home;
char *user;
char host[255];
char *cwd;
int cmdlength;
int bg;
char *built_in[] = {"echo","pwd","cd","quit"};
int redirect;
char *filename;
int stdout_fd;
int filefd;


// some self-implemented functions
int pwd(int p) {
	cwd = getcwd(NULL,0);
	if (p == 1) printf("%s\n",cwd);
	return 0;
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

int quit() {
	printf("are you sure you want to quit? [Y/n] ");
	int c = getchar();
	if (c == 'Y' || c == 'y') {
		exit(EXIT_SUCCESS);
	} else {
		return 0;
	}
}

int redirect_s() {
	filefd = open(filename,  O_WRONLY | O_CREAT | O_TRUNC, 0644); 
	if (filefd == -1) {
		printf("error creating file: errno = %d\n",errno);
		return 1;
	}
	stdout_fd = dup(STDOUT_FILENO);

	if(dup2(filefd, STDOUT_FILENO) == -1) {
		printf("error writing to file: errno = %d\n",errno);
		return 1;
	}
	return 0;

}
int redirect_f() {
	if (-1 == dup2(stdout_fd,STDOUT_FILENO)) {
		printf("error restoring STDOUT_FILENO: errno = %d",errno);
	}
	close(filefd);
	return 0;
}

// parsing command

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

void checkbg() {
	if (!strcmp("&", arg[cmdlength-1])) {
		bg = 1;
		arg[cmdlength-1] = NULL;
	} else {
		bg = 0;
	}
}

int parse_simple_cmd() {
	redirect = 0;
	char *ptr = strtok(buff, " ");
	cmd = ptr;
	arg[0] = ptr;
	int redlo;
	int i = 1;
	while(ptr != NULL) {
		if (!strcmp(ptr,">")){
			redirect = 1;
			redlo = i - 1;
		}
		ptr = strtok(NULL, " ");
		arg[i] = ptr;
		i++;
	}
	if (redirect == 1){
		filename = arg[redlo+1];
		arg[redlo]= NULL;
	}
	cmdlength = i-1;
	checkbg();
	return cmdlength;
}

void ps1() {
	printf("%s@%s: %s$ ",user,host,cwd);
}

// initializing and setting some environment varables 
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

// running commands that are not implemented
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
			case 3:
				quit();
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

		if (redirect) redirect_s();
		check_cmd();
		if (redirect) redirect_f();
	}
}
