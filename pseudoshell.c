#include <myshell.h> //include what I need

#define EXIT_SUCCESS 1

/**
 * array of function pointers here
 * might be a little difficult to understand, 
 * but I would rather try to do an array of 
 * function pointers than an elaborate
 * switch statement, so that's probably worth
 * implementing
 */

int main(int argc, char **argv) {
  if(argv[2] == some_batchfile) do_batchfile();
  /**
   * Simply, do_batchfile should just replace stdin with the batchfile, which can be done with dup2.
   * if pipe is detected in read or parse, then pipe() and dup2() will need to be used.
   */
  begin_loop();
  return EXIT_SUCCESS;
}

void begin_loop() {
  char * input;
  char ** args;
  int state = 1; //this will be 0 when exit is entered
                 //or something fails
  while(state) {
    //print some prompt statement, likely including
    // the current directory
    input = read();
    command = parse(input);
    execute(command);
  }
}

char * read(void) {
  char * ret = NULL;
  ssize_t buf = 0;
  getline(&ret, &buf, stdin);
  /**
   * As far as I understand, piping or arrow directives just change stdin
   * so this should still work for either of those cases. If it turns
   * out that this is not the case, I will update it.
   */
  return ret;
}

char ** parse(char * input) {
  /**
   * take the output of read() and parse it into some usable syntax
   *  most commands will take the following form:
   *    [command] [argument]
   *    e.g. ls ..
   * 
   * so really, we just take what we read from read(), tokenize it,
   * and maybe we check that it's valid syntax. That could also be done in execute().
   * 
   * something like this:
   *   char ** tokens = tokenize(input, delimiting_char); //where delimiting_char can be any whitespace
   *   if(!tokens) printf("error\n");
   *   tokens[last] = NULL;
   * 
   * buffer sizes will need to be checked, of course.
   */
}


int execute(char ** command) {
  /**
   * make child process, child process executes the syntax
   * retrieved from parse().
   *
   * 
   * pid_t pid = fork();
   * //also will include some error checking
   * if(child_process) {
   *   exec(the function dictated by command); // e.g. if (command == ls . ) exec(ls);
   * }else{
   *   wait(...);
   * }
   *
   * if( & is included, that is, to run the command in the background) {
   *   fork();
   *   if(child process) {
   *     fork() again;
   *     if(child process) {
   *	   exec(the appropriate function);
   *     }
   *     Doing this additional fork will allow the shell to continue executing in the background, or at least it should?
   *   }
   * }
   *
   * At this point, after parsing, the presence of a pipe or redirection command will be passed as an argument to the functions executed.
   * in other words, if the user enters ls | file.txt, then "|" and "file.txt" will be passed to ls() so that ls "knows" to write
   * its output to the file in question rather than stdout. Although, this can be done with dup2. So while the concept is really just
   * to replace the path of stdout with the path of the file specified, there are builtin functions to support this.
   */
}
