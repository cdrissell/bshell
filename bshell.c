/*
  Implements a minimal shell.  The shell simply finds executables by
  searching the directories in the PATH environment variable.
  Specified executable are run in a child process.

  AUTHOR: Cullen Drissell
*/

#include "bshell.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>

int parsePath(char *dirs[]);
char *lookupPath(char *fname, char **dir,int num);
int parseCmd(char *cmdLine, Command *cmd);
char *thePath;

/*
  Read PATH environment var and create an array of dirs specified by PATH.

  PRE: dirs allocated to hold MAX_ARGS pointers.
  POST: dirs contains null-terminated list of directories.
  RETURN: number of directories.

  NOTE: Caller must free dirs[0] when no longer needed.

*/
int parsePath(char *dirs[]) {
	int i;
	int numDirs = 0;
	char *pathEnv;
	char *token;
	
	for (i = 0; i < MAX_PATHS; i++) dirs[i] = NULL;
	
	pathEnv = (char *) getenv("PATH");
	
	/* Allocates space for thePath in memory */
	thePath = malloc(strlen(pathEnv)*sizeof(char));
	
	if (pathEnv == NULL) return 0; /* No path var. That's ok.*/
	
	/* for safety copy from pathEnv into thePath */
	strcpy(thePath,pathEnv);

#ifdef DEBUG
	printf("Path: %s\n",thePath);
#endif

	/* 
		Find all substrings delimited by DELIM.  Make a dir element
		point to each substring.
		TODO: approx a dozen lines.
	*/

	/* Breaks up the path into tokens */
	token = strtok(thePath, DELIM);
	
	/* Adds the tokens into dirs[] */
	while (token != NULL){
		dirs[numDirs] = token;
		numDirs += 1;
		token = strtok(NULL, DELIM);
	}

	/* Print all dirs */
#ifdef DEBUG
	for (i = 0; i < numDirs; i++) {
		printf("%s\n",dirs[i]);
	}
#endif

	return numDirs;
}


/*
	Search directories in dir to see if fname appears there.  This
	procedure is correct!

	PRE dir is valid array of directories
	PARAMS
	fname: file name
	dir: array of directories
	num: number of directories.  Must be >= 0.

	RETURNS full path to file name if found.  Otherwise, return NULL.

	NOTE: Caller must free returned pointer.
*/

char *lookupPath(char *fname, char **dir,int num) {
	char *fullName; // resultant name
	int maxlen; // max length copied or concatenated.
	int i;

	fullName = (char *) malloc(MAX_PATH_LEN);
	/* Check whether filename is an absolute path.*/
	if (fname[0] == '/') {
		strncpy(fullName,fname,MAX_PATH_LEN-1);
		if (access(fullName, F_OK) == 0) {
			return fullName;
		}
		else printf("%s: directory not found\n", fname);
	}

  /* Look in directories of PATH.  Use access() to find file there. */
	else {
		for (i = 0; i < num; i++) {
			// create fullName
			maxlen = MAX_PATH_LEN - 1;
			strncpy(fullName,dir[i],maxlen);
			maxlen -= strlen(dir[i]);
			strncat(fullName,"/",maxlen);
			maxlen -= 1;
			strncat(fullName,fname,maxlen);
			// OK, file found; return its full name.
			if (access(fullName, F_OK) == 0) {
				return fullName;
			}
		}
	}
	fprintf(stderr,"%s: command not found\n",fname);
	free(fullName);
	return NULL;
}

/*
  Parse command line and fill the cmd structure.

  PRE 
   cmdLine contains valid string to parse.
   cmd points to valid struct.
  PST 
   cmd filled, null terminated.
  RETURNS arg count

  Note: caller must free cmd->argv[0..argc]

*/
int parseCmd(char *cmdLine, Command *cmd) {
	int argc = 0; // arg count
	char* token;
	int i = 0;

	token = strtok(cmdLine, SEP);
	while (token != NULL && argc < MAX_ARGS){    
		cmd->argv[argc] = strdup(token);
		token = strtok(NULL, SEP);
		argc++;
	}

	cmd->argv[argc] = NULL;  
	cmd->argc = argc;
	
#ifdef DEBUG
	printf("CMDS (%d): ", cmd->argc);
	for (i = 0; i < argc; i++)
		printf(" CMD: %s ",cmd->argv[i]);
	printf("\n");
#endif
	
	return argc;
}

/*
  Runs simple shell.
*/
int main(int argc, char *argv[]) {
	
	char *prompt = malloc(MAX_PATH_LEN); // memory for prompt
	char *dirs[MAX_PATHS]; // list of dirs in environment
	int numPaths; // number of paths in dirs
	char cmdline[LINE_LEN]; // holds command line args
	int argCount; // number of args
	Command *command = malloc(sizeof(Command)); // memory for command
	char *fullpath; // points to path where command lives
	int status; // status for waitpid()
	Job *jobList = malloc(MAX_JOBS*sizeof(Job)); // memory for list of running jobs
	int numJobs = 0; // keeps track of the number of running jobs
	int f; // holds return value of fork() for both parent and child
	int marker = 0; // used to mark if user has reached the job limit
	
	system("clear"); //clears screen for new shell
	chdir("/home/pi"); //changes directory to '/home/pi'
	
	while(TRUE){
		
		// forces shell to ignore 'ctrl-c' input 
		signal(SIGINT, SIG_IGN);
		
		
		// parses the path
		numPaths = parsePath(dirs);
		
		
		// prints working directory to user
		printf("%sbshell:~%s", CYN, RESET);
		printf("%s%s: %s", MAG, getcwd(prompt, MAX_PATH_LEN), RESET); 
		
		
		// gets input from user
		char *input = fgets(cmdline, LINE_LEN, stdin); 


		// Checks if user leaves whitespace as the first argument and resets the shell if so
		if (strcmp(input, "\n") == 0) continue;
		else if (strcmp(input, " \n") == 0) continue;
		else if (strcmp(input, "\t\n") == 0) continue;
				
		
		// parses command
		else argCount = parseCmd(input, command);
		
		
		// Checks if user exits the shell by entering 'exit' or 'Exit'
		if ((strcmp(command->argv[0], "Exit") == 0) || (strcmp(command->argv[0], "exit") == 0)) {
			
			// free memory
			free(thePath);
			free(prompt);
			
			for (int i = 0; i < command->argc; i++){
				free(command->argv[i]);
			}
			
			free(command);
			free(jobList);
			
			system("clear"); //clears screen and returns to original shell
			
			// if any jobs are running, kills them before exiting
			if (jobList != NULL){
				for (int i = 0; i < numJobs; i++){
					kill(jobList[i].pid, SIGKILL);
				}
			}
			
			return FALSE;
		}
		
		
		// implements 'jobs' command. lists all current running jobs
		else if ((strcmp(command->argv[0], "jobs") == 0) || (strcmp(command->argv[0], "Jobs") == 0)){
			if (jobList != NULL){
				if ((command->argv[1] != NULL) && (strcmp(command->argv[1], "-l") == 0)){
					for (int i = 0; i < numJobs; i++){
						printf("[%d]\t%d\t%s\n", jobList[i].job_id, jobList[i].pid, jobList[i].name);
					}
				}
				else {
					for (int i = 0; i < numJobs; i++){
						printf("[%d]\t%s\n", jobList[i].job_id, jobList[i].name);
					}
				}
			}
		}
		
		
		// implements 'kill' command. User needs to input job id of process they wish to kill as an argument of kill
		else if (((strcmp(command->argv[0], "kill") == 0) || (strcmp(command->argv[0], "Kill") == 0)) && command->argv[1] != NULL){
			int num = -1;
			int checker = atoi(command->argv[1]);

			// free memory
			for (int i = 0; i < command->argc; i++){
				free(command->argv[i]);
			}

			for (int i = 0; i < numJobs; i++){
					if (checker == jobList[i].job_id){
						num = jobList[i].job_id;
						kill(jobList[i].pid, SIGKILL);
					}
			}
			if (num == -1){
				printf("Job ID not found. Enter a valid Job ID.\n");
				continue;
			}	
			for (int j = num-1; j <= numJobs-2; j++){
				jobList[j] = jobList[j+1];
			}
			for (int j = num-1; j <= numJobs-2; j++){
				jobList[j].job_id -= 1;
			}
			free(thePath);
			numJobs--;
		}
		
		
		// resets shell if user inputs 'r'
		else if ((strcmp(command->argv[0], "r") == 0) && (command->argv[1] == NULL)) {
			for (int i = 0; i < command->argc; i++){
				free(command->argv[i]);
			}
			free(thePath);
			system("clear"); 
		}
		
		
		// implements 'cd' command
		else if ((strcmp(command->argv[0], "cd") == 0) && (command->argv[1] != NULL)) {
			free(thePath);
			
			chdir(command->argv[1]);
			
			for (int i = 0; i < command->argc; i++){
				free(command->argv[i]);
			}
		}
		
		
		// if user inputs 'cd' with no args then user is taken back to '/home/pi'
		else if ((strcmp(command->argv[0], "cd") == 0) && (command->argv[1] == NULL)) {
			free(thePath);
			
			chdir("/home/pi");
			
			for (int i = 0; i < command->argc; i++){
				free(command->argv[i]);
			}
		}
		
		
		else {
			
			// checks if last arg is '&'. parent doesn't wait. child runs in background keeps a list of running jobs
			if ((strcmp(command->argv[argCount-1], "&") == 0) && (numJobs < MAX_JOBS)){
				
				// remove '&' from command args
				command->argv[argCount-1] = NULL;
				command->argc = command->argc-1;
				free(command->argv[argCount-1]);
			
				fullpath = lookupPath(command->argv[0], dirs, numPaths);
		
				if (fullpath != NULL){
			
					f = fork();
		
					if (f == 0) { //child process
						execv(fullpath,command->argv);
					}	
					else if (f > 0) { // parent process
						waitpid(0, &status, WNOHANG);
						
						// add job to the list of jobs
						numJobs++;
						int num = numJobs;
						jobList[numJobs-1].job_id = num;
						jobList[numJobs-1].pid = f;
						for (int i = 0; i < command->argc; i++){
							strcat(jobList[numJobs-1].name, command->argv[i]);
							strcat(jobList[numJobs-1].name, " ");
						}
						printf("[%d]  %d\n", jobList[numJobs-1].job_id, jobList[numJobs-1].pid);

						// free memory
						free(fullpath);
						free(thePath);
						for (int j = 0; j < command->argc; j++){
							free(command->argv[j]);
						}
					}
					else { // fork fails, exit
						fprintf(stderr, "fork failed\n");

						// free memory
						free(fullpath);
						free(thePath);
						for (int j = 0; j < command->argc; j++){
							free(command->argv[j]);
						}

						exit(1);
					}
				}
			} 
			else { // normal: parent waits for child
				
				// accounts for if user runs over the job limit
				if ((strcmp(command->argv[argCount-1], "&") == 0) && (numJobs == MAX_JOBS)){
					command->argv[argCount-1] = NULL;
					command->argc = command->argc-1;
					marker = -1;
				}
				
				fullpath = lookupPath(command->argv[0], dirs, numPaths);
		
				if (fullpath != NULL){
			
					f = fork();
					
					if (f == 0) { // child process
						if (marker == -1){
							printf("Max number of background jobs attained. Job running in foreground...\n");
						}
						execv(fullpath,command->argv);
					}
					else if (f > 0) { // parent process	
						waitpid(0, &status, 0);
						
						// free memory
						free(fullpath);
						free(thePath);
						for (int j = 0; j < command->argc; j++){
							free(command->argv[j]);
						}
						
						marker = 0;
					}
					else { // fork fails, exits
						fprintf(stderr, "fork failed\n");

						// free memory
						free(fullpath);
						free(thePath);
						for (int j = 0; j < command->argc; j++){
							free(command->argv[j]);
						}
						
						exit(1);
					}
				}
			}
		}
	}		
}