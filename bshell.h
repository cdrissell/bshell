/*
  header file for shell.  Adapted from Operating Systems, Nutt.
*/
//#define DEBUG
#define TRUE 1
#define FALSE 0
#define LINE_LEN 80

#define MAX_ARGS 64
#define MAX_ARG_LEN 16

#define MAX_JOBS 10

#define MAX_PATHS 8 //changed from 3
#define MAX_PATH_LEN 96

#define SEP " \t\n"
#define WHITESPACE " .,\t\n"
#define DELIM ":"

#define PROMPT "Hello User: "

#define MAG "\x1B[35m"
#define CYN "\x1B[36m"
#define RESET "\x1B[0m"

/* Store commands in this structure */
typedef struct {
	int argc; // number of args
	char *argv[MAX_ARGS]; // args.  arg[0] is command
} Command;

/* Store job info in this structure */
typedef struct {
	int job_id;
	int pid;
	char name[MAX_ARGS];
} Job;