A simple shell in C that implements a subset of features of well-known shells, such as bash. 

The program will:
1. Provide a prompt for running commands
2. Handle blank lines and comments, which are lines beginning with the # character
3. Provide expansion for the variable $$
4. Execute 3 commands exit, cd, and status via code built into the shell
5. Execute other commands by creating new processes using a function from the exec family of functions
6. Support input and output redirection
7. Support running commands in foreground and background processes
8. Implement custom handlers for 2 signals, SIGINT and SIGTSTP

To complie and run:
1. Run Makefile by typing the "make" command into the terminal. It will run the compiler.
2. Open the excutable by typing "smallsh" on the command line to run the program.

