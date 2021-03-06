/* Author: Jaclyn Sabo
Course: CS 344 Operating Systems
Date: October 30,2021
Description: Assignment 3, Portfolio Project: smallsh

Sources: Processes: https://www.youtube.com/watch?v=1R9h-H2UnLs
Used sources to help organize code to create a shell and understand the development of the shell:
  https://www.cs.purdue.edu/homes/grr/SystemsProgrammingBook/Book/Chapter5-WritingYourOwnShell.pdf
 https://brennan.io/2015/01/16/write-a-shell-in-c/ 
 https://indradhanush.github.io/blog/writing-a-unix-shell-part-1/
 https://indradhanush.github.io/blog/writing-a-unix-shell-part-2/
*/ 

#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>  
#include <signal.h> 


//command lines with a maximum length of 2048 characters, and a maximum of 512 arguments.
#define MAX_INPUT 2048								
#define MAX_ARGS 512


// functions declared
void shStart();
void shRead();
void shExecute();
void shStatus();
void handleSIGTSTP();

// track background processes
int backgroundRunning = 1;
int background = 0;
int status = 0;

// struct for signal handling, inialized to empty; used {{}} to avoid warrning errors
struct sigaction SIGINT_action = {{ 0 }};
struct sigaction SIGTSTP_action = {{ 0 }};

/*
EXIT STATUS FUNCTION
gets terminated status for child process
Source: Interpreting the Termination Status
WIFEXITED() and WIFSIGNALED() are the only two flags that show how a child process terminates
https://canvas.oregonstate.edu/courses/1830250/pages/exploration-process-api-monitoring-child-processes?module_item_id=21468873
3.1 Processes by Benjamin Brewster: https://www.youtube.com/watch?v=1R9h-H2UnLs 
*/

void shStatus(int status) {
	// WIFEXITED returns true if the child was terminated normally
	if (WIFEXITED(status)) {
		printf("exit value %d\n", WEXITSTATUS(status));
		fflush(stdout);
	} else {
		// process was terminated by a signal, WIFSIGNALED returns true; WTERMSIG gets terminating signal
		printf("terminated by signal %d\n", WTERMSIG(status));
		fflush(stdout);
	}
}

/*
SIGNAL HANDLER FUNCTION FOR SIGTSTP 
Source: Signal Handling https://canvas.oregonstate.edu/courses/1830250/pages/exploration-signal-handling-api?module_item_id=21468881
3.3 Signals by Benjamin Brewster: https://www.youtube.com/watch?v=VwS3dx3uyiQ
*/
void handleSIGTSTP(int signo) {
	// int signo: signal number, prints message when user enters ^Z to enter/exit foreground mode
  char* message = "\nEntering foreground-only mode (& is now ignored)\n";
  char* exitMessage = "\nExiting foreground-only mode.\n";
	if (backgroundRunning == 1) {
    backgroundRunning = 0;
		write(STDOUT_FILENO, message, 52); 
		fflush(stdout);
  }
	else {
    backgroundRunning = 1;
		write (STDOUT_FILENO, exitMessage, 32); 
		fflush(stdout);
	}
	// reprompt
	char* prompt = ": ";
    write(STDOUT_FILENO, prompt, 3);
    fflush(stdout);
}


/*
EXECUTE FUNCTION 
handles execute of commands and I/O
Sources: Redirecting Input and Output & fork, exec and File Descriptor Inheritance
https://canvas.oregonstate.edu/courses/1830250/pages/exploration-processes-and-i-slash-o?module_item_id=21468882
3.1 Processes by Benjamin Brewster: https://www.youtube.com/watch?v=1R9h-H2UnLs 
*/
void shExecute(char* args[], char inputFileName[], char outputFileName[]) {
    // spawnpid set to a bogus value so it doesn't get confused with actual meaningful values
	pid_t spawnpid = -5;

	//Spawn the child process
	// If fork is successful, the value of spawnpid will be 0 in the child, the child's pid in the parent
    spawnpid = fork();

    switch (spawnpid) {
			case -1:
				// If something went wrong, fork() returns -1 to the parent process; no child process was created
				perror("fork() failed!");
				exit(1);
				break;
			case 0:	
			// child process, child will execute the code in this branch

			// set Crtl C back to default 
			SIGINT_action.sa_handler = SIG_DFL;
            sigaction(SIGINT, &SIGINT_action, NULL);
			fflush(stdout);

			// An input and output file redirected via stdin should be opened for reading/writing only; 
			//if your shell cannot open the file for reading, print an error message and set the exit status to 1 
			//a background command should use /dev/null for input/output only when input/ouput redirection is not specified in the command.
			if (strcmp(inputFileName, "")) {
				// Open source file
				int sourceFD = open(inputFileName, O_RDONLY);
				if (sourceFD == -1) {
					fprintf(stderr, "cannot open %s for input\n", inputFileName);
					exit(1);
				} else{
				int sourceFD = open("/dev/null", O_RDONLY);
				if (sourceFD == -1) {
					fprintf(stderr, "cannot access /dev/null\n");
					fflush(stderr);
					exit(1);
				}
			}
				// Redirect stdin to source file
				int result = dup2(sourceFD, 0);
				if (result == -1) {
					fprintf(stderr, "cannot open %s for input\n", inputFileName);
					exit(1);
				}
				// closes file
				fcntl(sourceFD, F_SETFD, FD_CLOEXEC);
			}

			// output file
			if (strcmp(outputFileName, "")) {
				// Open target file
				int targetFD = open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, 0666); 
				if (targetFD == -1) {
					fprintf(stderr, "cannot open %s for output\n", outputFileName);
					exit(1);
			} else {
				int targetFD = open("/dev/null", O_WRONLY);
				if (targetFD == -1) {
					fprintf(stderr, "cannot access /dev/null\n");
					fflush(stderr);
					exit(1);
			}
			}
			// Redirect stdout to target file
			int result = dup2(targetFD, 1);
			if (result == -1) {
				fprintf(stderr, "cannot open %s for output\n", outputFileName);
				exit(1);
			}
			fcntl(targetFD, F_SETFD, FD_CLOEXEC);
			}
			// execvp replaces the currently running with a new specified program and executes the commands, does not create a new process
			// p in execvp stands for path and searches your PATH enviroment variable for the given path
			if (execvp(args[0], (char* const*)args)) {
				perror(args[0]);
				fflush(stdout);
				exit(1);
			}
			break;
		default:
			// parent process (any other value), fork() returns the process id of the child process that was just created
			if (background) {
				// waitpid(spawnpid, &status, WNOHANG): check if the specified process has completed, return with 0 if it hasn't
					waitpid(spawnpid, &status, WNOHANG);
					printf("background pid is %d\n", spawnpid);
					fflush(stdout);
				}
				else {
					// waitpid(spawnpid, &status, 0): blocks the parent until the specified child process terminates
					waitpid(spawnpid, &status, 0);
					// returns true if the child was terminated abnormally
					if WIFSIGNALED(status){
					shStatus(status);
					}
					fflush(stdout);
				}
			}			// waitpid(-1, &status, WNOHANG)) checks if ANY process has completed, and returns with 0 if none have
				while ((spawnpid = waitpid(-1, &status, WNOHANG)) > 0) {
				printf("background pid %d is done: ", spawnpid);
				fflush(stdout);
				shStatus(status);
			}
		}

/*
READ/PARSE LINE FUNCTION
reads input and parses line
Sources: CS344 Assignments 1 and 2 Code
reading and parsing the line - https://www.cs.utah.edu/~mflatt/past-courses/cs5460/hw2.pdf
strtok - https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm
parsing the line - https://brennan.io/2015/01/16/write-a-shell-in-c/ 
strdup - https://www.geeksforgeeks.org/strdup-strdndup-functions-c/
*/
void shRead(char* args[], char inputName[], char outputName[]) {


     char input[MAX_INPUT]; 
	 background = 0;
    // print prompt 
    printf(": ");
    fflush(stdout);
    // read line
    fgets(input, MAX_INPUT, stdin); 
    
    // get rid of new line
    input[strcspn(input, "\n")] = 0;

	char extInput[MAX_INPUT]; // String to store the extended user input
	pid_t pid = getpid();

    // checks for $$ to expand line
    for(int i = 0; i < strlen(input); i++) {
    if((input[i] == '$') && (input[i+1] == '$')) {       
    char * sign = strstr(input, "$$"); 
    sign[0] = '%'; sign[1] = 'd';
    // Extended string is copied into line to be parsed, $$ is replaced with pid
    sprintf(extInput, input, (int)pid);
    strcpy(input, extInput);
    }
}
	// if no input, return empty string to reprompt
	if (!strcmp(input, "")) {
		args[0] = strdup("");
	}
    // parse the input
    char *token = strtok(input, " "); 

    for (int i = 0; token; i++) {
        // check for & to change background process
			
			if (strcmp(token, "&") == 0 && strrchr(token, '\0')[-1]) { 
				if (backgroundRunning){
				background = 1;
				}
			}
			// input
			else if (strcmp(token, "<") == 0) {
				token = strtok(NULL, " "); 
				strcpy(inputName, token);
			}
			// output
			else if (strcmp(token, ">") == 0) {
				// printf("output redirection here \n");
				token = strtok(NULL, " "); 
				strcpy(outputName, token);
			}
			else {
				// copy into array to check for other commands
				args[i] = strdup(token); 
				
				
		}
    // get next token
		token = strtok(NULL, " "); 
    }
}
/*
START SHELL FUNCTION
while active, iterates to get, read, and parse input, as well as checks for built-in commands
*/
void shStart(){
	int active = 1; // flag for loop

  	// arrays to store commands as well as file names
  	char* args[MAX_ARGS];
	char inputFile[MAX_INPUT] = "";
	char outputFile[MAX_INPUT] = "";
	//char* userInput = malloc(sizeof(char) * 2048);

	while(active) {
    // function to prompt user and read lines
		shRead(args, inputFile, outputFile);

		// checks for comment or blank line, ignore, do nothing
		if (args[0][0] == '\0' || args[0][0] == '#'){
		}
		// check for built in commands
		// "exit" : exit shell and kill backgroound
		else if (strcmp(args[0], "exit") == 0) {
			active = 0;
		}
		// "cd": By itself - with no arguments - it changes to the HOME directory
		// it can also take run argument: a path of the directory to change to; if no directory available, print error
		else if (strcmp(args[0], "cd") == 0) {
			if (args[1]) {
				if (chdir(args[1]) == -1) {
					fprintf(stderr,"%s failed to open: no such directory\n", args[1]);
					fflush(stdout);
				}
			} else {
				chdir(getenv("HOME"));
			}
		}
		// "status": prints out either the exit status or the terminating signal of the last foreground process
		else if (strcmp(args[0], "status") == 0) {
			shStatus(status);
		} else {
			// if not built-in, call execute function to launch any other commands
			shExecute(args, inputFile, outputFile);
		}
		// clear array, and file names before restarting loop, reset background
		background = 0;
		inputFile[0] = '\0';
		outputFile[0] = '\0';
		for (int i = 0; args[i]; i++) { 
			args[i] = NULL; 
			}
		}
	}

/*
MAIN FUNCTION
Sources: Signal Handling: Module 5 Exploration 3
https://canvas.oregonstate.edu/courses/1830250/pages/exploration-signal-handling-api?module_item_id=21468881
Signals SIGCHLD: Module 5 Exploration 4 
https://canvas.oregonstate.edu/courses/1830250/pages/exploration-signals-concepts-and-types?module_item_id=21468880
3.3 Signals by Benjamin Brewster: https://www.youtube.com/watch?v=VwS3dx3uyiQ
*/
int main() {
	
  // SIGINT:parent process and any children running as background process must ignore SIGINT; a child running as a foreground process
  // must terminate itself when it recieves SIGINT
  //A CTRL-C command from the keyboard sends a SIGINT signal to the parent process and all children at the same time
  SIGINT_action.sa_handler = SIG_IGN;       // signal type is ignored
  sigfillset(&SIGINT_action.sa_mask);       // blocks all signals
  SIGINT_action.sa_flags = 0;      			// no flags
  sigaction(SIGINT, &SIGINT_action, NULL);

  // SIGTSTP: 
  //A CTRL-Z command from the keyboard sends a SIGTSTP signal to your parent shell process and all children at the same time
  SIGTSTP_action.sa_handler = handleSIGTSTP;   // Register handle_SIGTSTP as the signal handler
 //sigfillset(&SIGTSTP_action.sa_mask);
  SIGTSTP_action.sa_flags = SA_RESTART;
  sigaction(SIGTSTP, &SIGTSTP_action, NULL);

// start shell
shStart();

	return 0;
}



