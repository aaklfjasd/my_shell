/*****************************************************************
 *                      Bill Daws, tuf57013                      *
 *                  Operating Systems Project 2                  *
 *                   Professor Eugene Kwatny                     *
 *****************************************************************/
#include "myshell.h"

/***********************************************************************************
 *                        SKELETON FUNCTIONS                                       *
 *                           a.k.a. main shell logic functions                     *
 ***********************************************************************************/


int main(int argc, char ** argv) {
  my_set_env("SHELL");
  char * batchfile = NULL; //container for batchfile name
  if(argv[1]) { //if argv[1] then the user must be trying to use
                //a batchfile.
    batchfile = argv[1];
    //thus, we pass the name to the batchfile loop.
    begin_loop_batchfile(argv[1]);
  }

  //if we don't wish to execute a bashfile,
  //we just begin the normal loop.
  begin_loop();

  //I am actually pretty sure this line is never reached,
  //but I fear what might happen if I remove it.
  return EXIT_SUCCESS;
}

void begin_loop_batchfile(char * batchfile) {
  FILE * bfp = fopen(batchfile, "r"); //open the batchfile for reading
  char temp[128] = ""; //arbitrarily large buffer
  char ** command = NULL; //container for the parsed command.

  FILE * in = NULL; //containers for input and output
  FILE * out = NULL; //redirection.

  int in_cp  = dup(STDIN_FILENO);  //here I save the original stdin/out/err
  int out_cp = dup(STDOUT_FILENO); //so that I can restore them later.
  int err_cp = dup(STDERR_FILENO);
  
  if(bfp == NULL) {
    perror("error opening batchfile");
  }else {
    fgets(temp, 128 * sizeof(char), bfp);
    while(!feof(bfp)) {
      /**
       * In this loop I read the batchfile until it ends
       * and execute commands as I go.
       */

      //replace newline chars with null chars to make
      //commands easier to parse.
      replace_newline(temp);

      //tokenize temp into commands (standard shell stuff)
      command = parse(temp);
      
      //redirect stdin/out/err if necessary.
      in = redirect_in(command);
      out = redirect_out(command);

      //if we redirected, dup2 to replace the default stdin/out/err
      //with the user-specified stdin/out/err.
      if(in) {
	dup2(fileno(in), STDIN_FILENO);
      }

      if(out) {
	dup2(fileno(out), STDOUT_FILENO);
	dup2(fileno(out), STDERR_FILENO);
      }

      //the meat of the project.
      //execute the command.
      execute(command);

      //after execution, reset stdin/out/err to the defaults.
      reset_IO(in, out, in_cp, out_cp, err_cp);

      //free command so that we can reuse the memory and not
      //worry about getting something that's gibberish for a command.
      free(command);

      //here we basically do the same thing as free(command) but for temp.
      strcpy(temp, "");

      //get another line.
      fgets(temp, sizeof(temp), bfp);
    }
    //just in case.
    strcpy(temp, "");

    //close the pointer when we finish.
    fclose(bfp);
  }

  //leave the function so we can go back to business as usual.
  exit(EXIT_SUCCESS);
}


/**
 * this loop is very similar to the batchfile loop. I wrote this loop first,
 * so some of the comments here might be redundant.
 */
void begin_loop() {
  char * input; //container for user input.
  char ** command; //container for parsed commands.
  /* int state = 1; //I intended to use this variable for something more meaningful, */
  /*                //but I am pretty sure it could be a const and make no difference. */
  /*                // I'm just gonna leave it here as a precaution. */


  //containers for user IO
  FILE * in = NULL;
  FILE * out = NULL;

  /**
   * Here I am saving std[in/out/err] so that I can restore them
   * as the shell loop runs.
   */
  int in_cp  = dup(STDIN_FILENO);
  int out_cp = dup(STDOUT_FILENO);
  int err_cp = dup(STDERR_FILENO);

  //  while(state) {
  while(1) {
    //print a prompt for the user
    prompt();

    //get their input from the keyboard/stdin
    input = input_read();
    
    if(strcmp(input, "\n") == 0) {
      //if empty input, do nothing, print new prompt
      continue;
    }

    //parse input for command execution
    command = parse(input);

    //redirect in/out if the user wants to
    in = redirect_in(command);
    out = redirect_out(command);

    if(in) {
      dup2(fileno(in), STDIN_FILENO);
    }

    if(out) {
      dup2(fileno(out), STDOUT_FILENO);
      dup2(fileno(out), STDERR_FILENO);
    }

    //execute the tokenized command.
    execute(command);

    //reset IO and free memory so we don't have a memory leak
    // or have undesirable IO
    reset_IO(in, out, in_cp, out_cp, err_cp);
    free(command);
    free(input);
  }
}

/***********************************************************************************
 *                        END OF SKELETON FUNCTIONS                                *
 ***********************************************************************************/








/***********************************************************************************
 *                        PROCESSING FUNCTIONS                                     *
 ***********************************************************************************/

//take the user input and send it to a buffer
char * input_read(void) {
  char * ret = NULL;
  ssize_t buf = 0;
  getline(&ret, &buf, stdin);
  return ret;
}


//tokenize the user input
char ** parse(char * input) {
  int bufsize = BUF, index = 0;
  char ** ret = malloc(bufsize * sizeof(char *));
  char * temp;
  
  if(!ret) {
    fprintf(stderr, "issue with malloc in str_tokenize\n");
    exit(EXIT_FAILURE); //crash on failure.
  }

  //here I use strok to delimit the user input by whitespace characters
  // that I defined in my header file.
  temp = strtok(input, WSPACE);
  while(temp != NULL) {
    ret[index] = temp; //make parts of ret point to temp.
    index++;

    if(index >= bufsize) {
      //if we need to, allocate more space to ret.
      bufsize += BUF;
      ret = realloc(ret, bufsize * sizeof(char*));
      if(!ret) {
	fprintf(stderr, "issue reallocing in str_tokenize\n");
	exit(EXIT_FAILURE);
      }
    }

    /**
     * from the strtok() man page:
     *  In each subsequent call that should parse the same string, 
     *  str should be NULL. (where str is the first arg)
     */
    temp = strtok(NULL, WSPACE);
  }
  
  ret[index] = NULL; //null terminate ret
  return ret;
}



/**
 * This function is the centerpiece of my program. It accepts a
 * char double pointer called "command" and executes internal or
 * external commands based of the contents of the variable command.
 * while this function technically returns an int, I don't think it has to.
 */
int execute(char ** command) {
  /**
   * It doesn't make sense to run cd or quit in a forked process
   * because the whole point of those functions is to affect
   * the root process. For example, if I were to fork() and
   * have the child process call quit, this would be the same
   * thing as never calling fork() in the first place.
   */
  if(strcmp(command[0], "quit") == 0) {
    my_quit(command);
  }else if(strcmp(command[0], "cd") == 0) {
    my_cd(command);
  }else if(strcmp(command[0], "myshell") == 0) {
    /**
     * this section allows the user to execute batchfiles
     * while in myshell rather than strictly when invoking myshell.
     * 
     * I do this by getting the parent directory of the executable file
     * (so that the user doesn't have to reference myshell by path)
     * and then using execvp to pass the other arguments to that invokation
     * of myshell.
     */
    char path[50] = ""; //container for path of myshell
    pid_t shellchild;

    strcat(path, getenv("PWD")); //get PWD of executable myshell
    strcat(path, "/myshell"); //append executable name to PWD
    
    if((shellchild = fork()) == 0) {
      //child process, run the shell invokation
      execvp(path, command);
    }else {
      //wait for the batchfile to finish if it's not in the background.
      //I have no idea why one would want to do this, but they can.
      //I can't say that I advise it, though, as it will make the screen
      //likely very messy.
      if(!background(command)) {
	waitpid(shellchild, NULL, 0);
      }
    }
  }else {
    pid_t pid;
    //here I am declaring a pointer to a function that
    // takes a char double pointer and returns void.
    void (*func)(char **) = NULL;
    func = get_func(command); //get the address corresponding to
                              //the function specified in command.
    
    int pb;
    //find a pipe, if it exists, get its index and enter
    //a version of execute() that can handle piping,
    // called pipe_fork_and_exec(). More on this function later.
    if((pb = pipe_bool(command)) != 0) {
      pipe_fork_and_exec(func, command, pb);
    }else if((pid = fork()) == 0) { //child
      if(func) {
	//if func is internal, we execute it and then kill the child
	// process. If func is not internal, func will be NULL so
	// this section will not be reached.
	(*func)(command);
	exit(EXIT_SUCCESS);
      }else if(execvp(command[0], command) == -1) {
	//here we execute externals and kill the child process
	// if the exec fails.
	perror("execute"); //exec fails
	exit(EXIT_FAILURE); //so kill child
      }
    }else
      if(pid < 0) {
	perror("forking");
      }else {
	if(!background(command)){
	  //if it's not in the background we wait for the child process
	  // to finish whatever it's doing.
	  waitpid(pid, NULL, 0);
	}
      }    
    return 0;
  }
  return 0;
}


/**
 * this function is similar to execute, except it is capable of piping.
 * I figured it would be easier to understand if the piping logic
 * was contained in a second function. I wasn't sure whether to put
 * this in the processing section or the redirection section but I don't
 * think it makes that big of a difference.
 */
void pipe_fork_and_exec(void func(char **), char ** command, int pb) {
  int pfd[2];
  pipe(pfd);
  int pid, pid2;
  char ** command2 = command + pb; //this is the command that comes after
                                   // the pipe.
  void (*func2)(char **) = NULL;   //func2 holds an internal function if
  func2 = get_func(command2);      // command2 contains an internal function.

  if((pid2 = fork()) == 0) {
    //child process
    //redirect stdin per user request
    dup2(pfd[PIPE_READ], STDIN_FILENO);

    if((pid = fork()) == 0) {
      //child process (again)
      //redirect stdout per user request
      dup2(pfd[PIPE_WRITE], STDOUT_FILENO);

      //close the pipe, don't need it in this
      //process anymore.
      close(pfd[PIPE_READ]);
      close(pfd[PIPE_WRITE]);

      //do the first function or exec the first command
      if(func) {
	(*func)(command);
	//kill this process when func finishes.
	exit(EXIT_SUCCESS);
      }else {
	execvp(command[0], command);
      }
    }

    //close the pipe, don't need it in this
    //process anymore.
    close(pfd[PIPE_WRITE]);
    close(pfd[PIPE_READ]);

    //wait for the child of this process.
    waitpid(pid, NULL, 0);

    //do the second internal function if it exists,
    // else exec the second command.
    if(func2) {
      (*func2)(command2);
      //kill this process when func2 finishes.
      exit(EXIT_SUCCESS);
    }else {
      execvp(command2[0], command2);
    }
  }
  //close the pipe, don't need it in the
  // parent process anymore.
  close(pfd[PIPE_WRITE]);
  close(pfd[PIPE_READ]);
  //wait for the child process to finish.
  waitpid(pid2, NULL, 0);
}


/***********************************************************************************
 *                         END OF PROCESSING FUNCTIONS                             *
 ***********************************************************************************/







/***********************************************************************************
 *                                 BUILT IN FUNCTIONS                              *
 ***********************************************************************************/

void my_cd(char ** command) {
  /**
   * cd:
   *   forking to do this would be ineffective.
   *   it is not deisrable to change the working directory of a child process.
   */
  if(!command[1] || strcmp(command[1], " ") == 0) {
    //if the user didn't specify a target directory, then
    chdir(getenv("HOME")); //change current dir to home dir
  } else {
    //if the user did specify a target directory, then
    chdir(command[1]); //change current dir to that dir.
  }
}

void my_clr(char ** command) {
  /**
   * clr or clear:
   *   here I use ANSI escape codes to reset the screen and cursor position.
   *     J with param 2 clears the entire screen.
   *     H with no params defaults to 1;1, which moves the cursor to position 1;1.
   *   I rewind and truncate stdout so that what was written to the screen before
   *   the clear statement does not interfere with what is written to the screen
   *   after the clear statement.
   */
  printf("\033[2J\033[H");
}

void my_dir(char ** command) {
  char * target = command[1];
  DIR *d;
  struct dirent *dir;
  char path[512]; //arbitrarily long path buffer for pathname
  /**
   * some of this code may seem familiar if you have TA'd CIS2107 for Professor Fiore.
   * this function contains code he provided in his directory statistics assignment.
   * seeing as it wasn't mentioned that I can't re-use this code, I have taken the
   * liberty of doing so.
   */

  //here an error message is printed if the directory cannot be opened.
  if((d = opendir(target)) == NULL) {
    fprintf(stderr, "opendir %s  %s\n", path, strerror(errno));
    return;
  }
  if(d != NULL) {
    //if the directory exists...
    while((dir = readdir(d))) {
      //here we skip printing "." and ".."
      if(strcmp(".", dir->d_name) == 0 ||
	 strcmp("..", dir->d_name) == 0)
	continue;

      //print everything else.
      printf("%s\n", dir->d_name);
    }
    //close the directory pointer.
    closedir(d);
  }
}

void my_disp_environ(char ** command) {
  //just go through the array of environment strings
  // and print them.
  int i = 0;
  while(environ[i]) {
    printf("%s\n", environ[i]);
    i++;
  }
}

void my_echo(char ** command) {
  int i = 1;
  while(command[i]) {
    if((strcmp(command[i], ">")  == 0) ||
       (strcmp(command[i], ">>") == 0)) {
      break;
      /**
       * this is so that when we redirect output with
       * echo we don't wind up writing "> filename" to filename.
       */
    }
    //because we can reduce multiple spaces or other WSPACE chars
    // to one space, I figured this would be okay to do.
    printf("%s ", command[i]);
    i++;
  }
  printf("\n");
}



void my_pause(char ** command) {
  //busy-wait until user presses Enter
  while(getchar() != '\n') {}
}


void my_set_env(char * command) {
  char this_dir[50];

  //get the current working directory. this is executed at
  //the start of the program.
  if(!getcwd(this_dir, sizeof(this_dir))) {
    perror("error getting dir");
    return;
  }

  //append "/myshell" to the path
  strcat(this_dir, "/myshell");

  //set an environment string accordingly.
  if(command) {
    setenv(command, this_dir, 1);
  }
}

void my_help(char ** command) {
  char current[50] = "";
  pid_t pid;  

  //get parent directory of executable file--
  //the same directory that contains the readme file
  strcat(current, getenv("PWD"));

  //append /readme to the path so that we can execute
  // more on the correct file
  strcat(current, "/readme");

  if((pid = fork()) == 0) {
    //child process executes more with the readme path
    // as an argument.
    execlp("more", "more", current, NULL);

    //if the exec fails, kill the child
    perror("my_help exec");
    exit(EXIT_FAILURE);
  }else {
    //wait for the child.
    waitpid(pid, NULL, 0);
  }
}

void my_quit(char ** command) {
  //kill the invoking process.
  exit(EXIT_SUCCESS);
}

/***********************************************************************************
 *                         END OF BUILT IN FUNCTIONS                               *
 ***********************************************************************************/







/***********************************************************************************
 *                           REDIRECTION FUNCTIONS                                 *
 ***********************************************************************************/

FILE * redirect_in(char ** command) {
  FILE * fp = NULL;
  int i = 1; //can skip command[0], it will never need
             // to be examined for redirection.
  int arrow; //contains index of arrow command
  
  while(command[i]) {
    //if redirecting in, command[i] will only be '<'
    // in other words, redirection chars are delimited by
    // whitespace.
    //this assumption is justified in the assignment description.
    if(command[i][0] == '<') {
      arrow = i;
      if(command[i++]) {
	//when we find the arrow, open the file specified after
	// the arrow for reading.
	fp = fopen(command[i], "r");
	open(command[i], O_RDONLY);
	if(fp == NULL) {
	  perror("error open input redirect");
	}
	//this function moves the pointers in command so that
	// the redirection chars and redirection target doesn't
	// interfere with the execution of the process we are
	// trying to redirect.
	move_pointers(command, arrow);
	return fp;
      }
    }
    i++;
  }

  //if the user didn't want to redirect, we just return NULL.
  return fp;
}


//very similar to redirect_in
FILE * redirect_out(char ** command) {
  FILE * fp = NULL;
  int i = 1;
  char mode[] = ""; //open file for truncating or appending?
  int arrow; //contains the index of the redirection char
  while(command[i]) {
    /**
     * For single arrow, we truncate.
     * For double arrow, we append.
     */
    if(strcmp(command[i], ">") == 0) {
      mode[0] = 'w';
      arrow = i;
    }else if(strcmp(command[i], ">>") == 0) {
      mode[0] = 'a';
      arrow = i;
    }

    //this comparison will not equal zero
    // if we encountered some output redirection char.
    if(strcmp(mode, "") != 0) {
      if(command[i++]) {
	//in which case, we open the next part of command
	// as a file since the proper syntax for such a
	// command lets us assume the next token is a filename.
	fp = fopen(command[i], mode);
	if(fp == NULL) {
	  perror("error open output redirect");
	}

	//we do not need to move pointers after this because
	// output redirection comes at the end of a command,
	// as opposed to input redirection which comes at the beginning.
	command[arrow] = NULL;
	return fp;
      }
    }
    i++;
  }
  
  //if the user didn't want to redirect out, we return NULL.
  return fp;
}

int pipe_bool(char ** command) {
  int i = 0;
  while(command[i]) {
    if(strcmp(command[i], "|") == 0) {
      /**
       * set command[i], which contains "|", to NULL
       * so that it makes it easier to parse the pipe command
       * after this function executes. This prevents the pipe
       * from being interpreted as a command or a function.
       */
      command[i] = NULL;
      i++;
      
      return i; /**
		 * better to return the index of
		 * the command after pipe rather than 1 or 0,
		 * as I would for a standard boolean search.
		 *
		 * this is why I set command2 = command + pb.
		 */
    }
    i++;
  }

  /**
   * I thought about returning -1 if the pipe wasn't found
   * because that's how I would normally return the index
   * of something I was looking for but couldn't find (say,
   * for example, searching for 'x' in the string "hello")
   * but I figured this would be better to use for the logic
   * that you can find in my pipe handling function.
   */
  return 0;
}

void reset_IO(FILE * in, FILE * out, int in_cp, int out_cp, int err_cp) {
  /**
   * here I am just cleaning up open file descriptors as well as
   * resetting stdin/out/err to what they should be by default.
   * I do this after every command.
   */
  dup2(in_cp, STDIN_FILENO);

  if(in) {
    fclose(in);
  }
  if(out) {
    fclose(out);
  }

  dup2(out_cp, STDOUT_FILENO);
  dup2(err_cp, STDERR_FILENO);
}

/***********************************************************************************
 *                    END OF REDIRECTION FUNCTIONS                                 *
 ***********************************************************************************/






/***********************************************************************************
 *                             MISC. FUNCTIONS                                     *
 ***********************************************************************************/

/**
 * here I use built-in system calls to get the name of the
 * user's machine and the current directory to print out a useful
 * prompt that resembles most standard shells.
 */
void prompt() {
  char this_dir[1024], machine[1024];
  if(!gethostname(machine, sizeof(machine))) {
    printf("%s: ", machine);
  }else {
    perror("error getting hostname");
  }

  if(getcwd(this_dir, sizeof(this_dir))) {
    printf("%s> ", this_dir);
  }else {
    perror("error getting dir");
  }
}

/**
 * this is a fairly basic function, it takes a char * and replaces
 * all instances of newline chars with null chars.
 * as mentioned before, this makes it easier to parse commands from
 * user-specified batchfiles.
 */
void replace_newline(char * temp) {
  int i = 0;
  while(temp[i] != '\0') {
    if(temp[i] == '\n') {
      temp[i] = '\0';
    }
    i++;
  }
}

/**
 * just getting a boolean to detect whether or not the process
 * should run in the background by detecting whether or not
 * ampersand (&) is at the end of the command
 */
int background(char ** command) {
  int i = 0;
  while(command[i]) {
    i++; //go to the end of the command
  }
  if(strcmp(command[i-1], "&") == 0) {
    return 1; //if command ends in &, return true
  }else {
    return 0; //else return false
  }
}

/**
 * this is a nasty function declaration.
 * it's just a function that takes in a char ** and
 * returns a pointer to a function that returns void
 * and takes a char ** as an argument.
 * in other words:
 *  get_func takes char **
 *  returns a pointer (called func) to
 *  a function that takes char **
 *    and returns nothing.
 * 
 * get_func determines the correct address to return
 * based on a sequence of string comparisons.
 * cd and quit are omitted because they are handled in execute().
 */
void (*get_func(char ** command))(char ** command) {
  void (*func)(char **) = NULL;

  if(strcmp(command[0], "clr") == 0 || strcmp(command[0], "clear") == 0) {
    func = &my_clr;
  }else if(strcmp(command[0], "dir") == 0) {
    func = &my_dir;
  }else if(strcmp(command[0], "environ") == 0) {
    func = &my_disp_environ;
  }else if(strcmp(command[0], "echo") == 0) {
    func = &my_echo;
  }else if(strcmp(command[0], "pause") == 0) {
    func = &my_pause;
  }else if(strcmp(command[0], "help") == 0) {
    func = &my_help;
  }
  
  return func;
}



void move_pointers(char ** command, int arrow) {
  /**
   * basically we just replace each part of command
   * with the one after it, kind of like when you remove part
   * of a linked list.
   *
   * this is so that nothing gets printed to files/stdout when
   * it shouldn't be. for example, before implementing this,
   * commands like cat < text.txt > text1.txt
   * would print error messages from cat trying to find "<" to
   * text1.txt.
   */
  int i = arrow;
  while(command[i+1]) {
    command[i] = command[i+1];
    i++;
  }
}


/***********************************************************************************
 *                      END OF MISC. FUNCTIONS                                     *
 ***********************************************************************************/


//The end!
