/*****************************************************************
 *                      Bill Daws, tuf57013                      *
 *                  Operating Systems Project 2                  *
 *                   Professor Eugene Kwatny                     *
 *****************************************************************/

/**
 * this file contains prototype declarations and other typical stuff you'd find in
 * a header file, as well as brief summaries for some aspects that might require it.
 * for example, what WSPACE is and how it is used, etc.
 */

#ifndef MYSHELL_H_
#define MYSHELL_H_

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>

#define BUF 64 //just a predefined buffer size max
#define WSPACE " \t\r\n\a" //whitespace characters used as delimiters

const int PIPE_READ = 0; //just stuff to make piping easier
const int PIPE_WRITE = 1;//to read and work with

extern char **environ; //so we can print environment strings


/***********************************************************************************
 *                        SKELETON FUNCTIONS                                       *
 ***********************************************************************************/

//the following two functions contain the logic to execute commands if:
// * they are coming from a batchfile
// * or they are coming from user input (e.g. keyboard)
void begin_loop_batchfile(char * batchfile);
void begin_loop();


/***********************************************************************************
 *                        PROCESSING FUNCTIONS                                     *
 ***********************************************************************************/

char * input_read(void); //simple reading function, get input
char ** parse(char * input); //simple input tokenizer

int execute(char ** command); //determines what to do with tokenized input,
                              // including determining whether or not to pipe.

void pipe_fork_and_exec(void func(char **), char ** command, int pb);
              //this last function handles piping if necessary.




/***********************************************************************************
 *                                 BUILT IN FUNCTIONS                              *
 ***********************************************************************************/

void my_cd(char ** command);             //change directory
void my_clr(char ** command);            //clear the screen
void my_dir(char ** command);            //display contents of target directory
void my_disp_environ(char ** command);   //display environment strings
void my_set_env(char * command);         //set environment string
void my_echo(char ** command);           //repeat input back to stdout
void my_pause(char ** command);          //busy-wait
void my_help(char ** command);           //display readme with more filter
void my_quit(char ** command);           //quit!


/***********************************************************************************
 *                           REDIRECTION FUNCTIONS                                 *
 ***********************************************************************************/

FILE * redirect_in(char ** command);  //redirect stdin at user request
FILE * redirect_out(char ** command); //redirect stdout/err at user request
int pipe_bool(char ** command);       //determine if the command contains a pipe,
                                      // if so return its index
void reset_IO(FILE * in, FILE * out, int in_cp, int out_cp, int err_cp);
                                      //clean up (that is, reset to default) stdin,
                                      // out, and err




/***********************************************************************************
 *                             MISC. FUNCTIONS                                     *
 ***********************************************************************************/

void prompt();
  //prints a prompt with machine and host names and cwd.

int background(char ** command);
  //determines if a process should be executed in the background or not.

void (*get_func(char ** command))(char ** command);
  //returns a function pointer to a built in function based
  //off of the contents of command.

void replace_newline(char * temp);
  //replaces newline characters in a batchfile to make it easier to parse.

void move_pointers(char ** command, int arrow);
  //moves pointers in command to prevent stdin redirection arrow
  // from being interpreted as part of the command to execute.




//The end!
#endif
