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

// struct for signal handling
struct sigaction SIGINT_action = {{ 0 }};
struct sigaction SIGTSTP_action = {{ 0 }};

/*
EXIT STATUS FUNCTION
gets terminated status for child process
*/

void shStatus(int status) {
	if (WIFEXITED(status)) {
		printf("exit value %d\n", WEXITSTATUS(status));
		fflush(stdout);
	} else {
		printf("terminated by signal %d\n", WTERMSIG(status));
		fflush(stdout);
	}
}

/*
SIGNAL HANDLER FUNCTION FOR SIGTSTP 
Source: https://canvas.oregonstate.edu/courses/1830250/pages/exploration-signal-handling-api?module_item_id=21468881
*/
void handleSIGTSTP(int signo) {
	// enter into fg mode, write message than flip bool flag
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
	char* prompt = ": ";
    write(STDOUT_FILENO, prompt, 3);
    fflush(stdout);
}


/*
EXECUTE FUNCTION 
handles execute of commands and I/O
Sources: https://canvas.oregonstate.edu/courses/1830250/pages/exploration-processes-and-i-slash-o?module_item_id=21468882
https://canvas.oregonstate.edu/courses/1830250/pages/exploration-process-api-creating-and-terminating-processes?module_item_id=21468872

*/

void shExecute(char* arr[], char inputFileName[], char outputFileName[]) {
		
    //Spawn the child process
	int result;
	pid_t spawnpid = -5;

	// If fork is successful, the value of spawnpid will be 0 in the child, the child's pid in the parent
    spawnpid = fork();

    switch (spawnpid) {
			case -1:
				// Code in this branch will be exected by the parent when fork() fails and the creation of child process fails as well
				perror("fork() failed!");
				exit(1);
				break;
			case 0:	
			// spawnpid is 0. This means the child will execute the code in this branch
			// set Crtl C back to default
			
			SIGINT_action.sa_handler = SIG_DFL;
            sigaction(SIGINT, &SIGINT_action, NULL);
			fflush(stdout);



			//use /dev/null for output only when output redirection is not specified in the command.
			if(backgroundRunning == 1) {
				int targetFD = open("/dev/null", O_WRONLY);
				if (targetFD == -1) {
					fprintf(stderr, "cannot access /dev/null\n");
					fflush(stderr);
					exit(1);
				}
			}
			// An input and output file redirected via stdin should be opened for reading/writing only; 
			//if your shell cannot open the file for reading, print an error message and set the exit status to 1 
			if (strcmp(inputFileName, "")) {
				// Open source file
				int sourceFD = open(inputFileName, O_RDONLY);
				if (sourceFD == -1) {
					perror("cannot open input file");
					exit(1);
				}
				// Redirect stdin to source file
				result = dup2(sourceFD, 0);
				if (result == -1) {
					perror("cannot open input file");
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
					perror("cannot open output file");
					exit(1);
			}
			// Redirect stdout to target file
			result = dup2(targetFD, 1);
			if (result == -1) {
				perror("cannot open output file"); 
				exit(1);
			}
			fcntl(targetFD, F_SETFD, FD_CLOEXEC);
			}
			
			if (execvp(arr[0], (char* const*)arr)) {
				printf("%s: no such file or directory\n", arr[0]);
				fflush(stdout);
				exit(1);
			}
			break;
		default:	
		
			if (background) {
					waitpid(spawnpid, &status, WNOHANG);
					printf("background pid is %d\n", spawnpid);
					fflush(stdout);
				}
				
				else {
					waitpid(spawnpid, &status, 0);
					if WIFSIGNALED(status){
					shStatus(status);
					}
					fflush(stdout);
				}
			}
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
void shRead(char* arr[], char inputName[], char outputName[]) {

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
		arr[0] = strdup("");
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
				arr[i] = strdup(token); 
				
				
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
					printf("cd failed: no such file or directory found: %s\n", args[1]);
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
		// clear array, and file names before restarting loop
		background = 0;
		inputFile[0] = '\0';
		outputFile[0] = '\0';
		for (int i = 0; args[i]; i++) { 
			args[i] = NULL; }
	}

}

/*
MAIN FUNCTION
Sources: Signal Handling: Module 5 Exploration 3
https://canvas.oregonstate.edu/courses/1830250/pages/exploration-signal-handling-api?module_item_id=21468881
Signals SIGCHLD: Module 5 Exploration 4 
https://canvas.oregonstate.edu/courses/1830250/pages/exploration-signals-concepts-and-types?module_item_id=21468880
*/
int main() {
	
  // SIGNT  - shell must ignore SIGINT
  //A CTRL-C command from the keyboard sends a SIGINT signal to the parent process and all children at the same time
  SIGINT_action.sa_handler = SIG_IGN;       // signal type is ignored
  sigfillset(&SIGINT_action.sa_mask);       // block all signals
  SIGINT_action.sa_flags = 0;      // automatic restart of the interrupted system call 
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





