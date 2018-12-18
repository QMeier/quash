/**
 * @file execute.c
 *
 * @brief Implements interface functions between Quash and the environment and
 * functions that interpret an execute commands.
 *
 * @note As you add things to this file you may want to change the method signature
 */

#include "execute.h"
#include "command.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>
#include "quash.h"
#include <sys/types.h>
#include <sys/wait.h>
#include "deque.h"


//pid_t waitpid(pid_t pid,int *status, int options);
char* get_current_dir_name();
char *getenv(const char *env_var);
pid_t pid;



/*#define IMPLEMENT_DEQUE_STRUCT(Jobber, CommandHolder);
#define IMPLEMENT_DEQUE(Jobber, CommandHolder);
Jobber jb = new_Jobber(1);
Jobber* deq = &jb; */

/***************************************************************************
 * Interface Functions
 ***************************************************************************/

// Return a string containing the current working directory.
char* get_current_directory(bool* should_free) {
  char* wd = get_current_dir_name();
  // Change this to true if necessary
  *should_free = true;
  return wd;
}

// Returns the value of an environment variable env_var
const char* lookup_env(const char* env_var) {
  char *env = getenv(env_var);
  return env;
}

// Check the status of background jobs
void check_jobs_bg_status() {
  // TODO: Check on the statuses of all processes belonging to all background
  // jobs. This function should remove jobs from the jobs queue once all
  // processes belonging to a job have completed.


  // TODO: Once jobs are implemented, uncomment and fill the following line
  // print_job_bg_complete(job_id, pid, cmd);
}

// Prints the job id number, the process id of the first process belonging to
// the Job, and the command string associated with this job
void print_job(int job_id, pid_t pid, const char* cmd) {
  printf("[%d]\t%8d\t%s\n", job_id, pid, cmd);
  fflush(stdout);
}

// Prints a start up message for background processes
void print_job_bg_start(int job_id, pid_t pid, const char* cmd) {
  printf("Background job started: ");
  print_job(job_id, pid, cmd);
}

// Prints a completion message followed by the print job
void print_job_bg_complete(int job_id, pid_t pid, const char* cmd) {
  printf("Completed: \t");
  print_job(job_id, pid, cmd);
}

/***************************************************************************
 * Functions to process commands
 ***************************************************************************/
// Run a program reachable by the path environment variable, relative path, or
// absolute path
void run_generic(GenericCommand cmd) {
  // Execute a program with a list of arguments. The `args` array is a NULL
  // terminated (last string is always NULL) list of strings. The first element
  // in the array is the executable
  char* exec = cmd.args[0];
  char** args = cmd.args;

  if(execvp(exec,args)<0){
    perror("ERROR: Failed to execute program");
  }
}

// Print strings
void run_echo(EchoCommand cmd) {
  // Print an array of strings. The args array is a NULL terminated (last
  // string is always NULL) list of strings.
  char** str = cmd.args;
  int w = 1;

  while(cmd.args[w-1] != NULL){
    printf("%*s%s ",0, "", *str);
    *str = cmd.args[w];
    w++;
  }
  printf("\n");
  // Flush the buffer before returning
  fflush(stdout);
}

// Sets an environment variable
void run_export(ExportCommand cmd) {
  // Write an environment variable
  const char* env_var = cmd.env_var;
  const char* val = cmd.val;

  if (setenv(env_var, val, 1) == -1){
    perror("ERROR: Could not set env");
  }
  //printf("Environment variable %s set to %s.\n",env_var,val);

}

// Changes the current working directory
void run_cd(CDCommand cmd) {
  // Get the directory name
  const char* dir = cmd.dir;
  // Check if the directory is valid
  if (dir == NULL) {
    perror("ERROR: Failed to resolve path");
    return;
  }
  char buf[PATH_MAX]; /* not sure about the "+ 1" */
  const char *wd = realpath(dir, buf);
  chdir(wd);
  // Update the PWD environment variable to be the new current working directory
  setenv("PWD", wd, 1);
}

// Sends a signal to all processes contained in a job
void run_kill(KillCommand cmd) {
  int signal = cmd.sig;
  int job_id = cmd.job;

  // TODO: Kill all processes associated with a background job
  kill(job_id, signal);
}


// Prints the current working directory to stdout
void run_pwd() {
  char *wd = get_current_dir_name();
  printf("%s\n",wd);
  // Flush the buffer before returning
  fflush(stdout);
  free(wd);
}

// Prints all background jobs currently in the job list to stdout
void run_jobs() {
  // TODO: Print background jobs
  /*if(is_empty_Jobber(deq)){}
  else{
    int x = length_Jobber(deq);
    CommandHolder* jbs = as_array_Jobber(deq,x);

    for(int i = 0; i<x; i++){
      printf("%s",jbs[i]);
      //push_back_Jobber(deq, jbs[i]);
      //int job_id = length_Jobber(deq);
      //char* cmd = get_command_string();
    }
  }*/
  // Flush the buffer before returning
  fflush(stdout);
}

/***************************************************************************
 * Functions for command resolution and process setup
 ***************************************************************************/

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for child processes.
 *
 * This version of the function is tailored to commands that should be run in
 * the child process of a fork.
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void child_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);

  switch (type) {
  case GENERIC:
    run_generic(cmd.generic);
    break;

  case ECHO:
    run_echo(cmd.echo);
    break;

  case PWD:
    run_pwd();
    break;

  case JOBS:
    run_jobs();
    break;

  case EXPORT:
  case CD:
  case KILL:
  case EXIT:
  case EOC:
    break;

  default:
    fprintf(stderr, "Unknown command type: %d\n", type);
  }
}

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for the quash process.
 *
 * This version of the function is tailored to commands that should be run in
 * the parent process (quash).
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void parent_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);

  switch (type) {
  case EXPORT:
    run_export(cmd.export);
    break;

  case CD:
    run_cd(cmd.cd);
    break;

  case KILL:
    run_kill(cmd.kill);
    break;

  case GENERIC:
  case ECHO:
  case PWD:
  case JOBS:
  case EXIT:
  case EOC:
    break;

  default:
    fprintf(stderr, "Unknown command type: %d\n", type);
  }
}

/**
 * @brief Creates one new process centered around the @a Command in the @a
 * CommandHolder setting up redirects and pipes where needed
 *
 * @note Processes are not the same as jobs. A single job can have multiple
 * processes running under it. This function creates a process that is part of a
 * larger job.
 *
 * @note Not all commands should be run in the child process. A few need to
 * change the quash process in some way
 *
 * @param holder The CommandHolder to try to run
 *
 * @sa Command CommandHolder
 */
void create_process(CommandHolder holder) {
  // Read the flags field from the parser
  bool p_in  = holder.flags & PIPE_IN;
  bool p_out = holder.flags & PIPE_OUT;
  bool r_in  = holder.flags & REDIRECT_IN;
  bool r_out = holder.flags & REDIRECT_OUT;
  bool r_app = holder.flags & REDIRECT_APPEND; // This can only be true if r_out
                                               // is true

  // TODO: Remove warning silencers
  (void) p_in;  // Silence unused variable warning
  (void) p_out; // Silence unused variable warning
  (void) r_in;  // Silence unused variable warning
  (void) r_out; // Silence unused variable warning
  (void) r_app; // Silence unused variable warning

  // TODO: Setup pipes, redirects, and new process

  pid = fork();

  if (pid < 0){
     perror("ERROR:Fork Failed\n");
  }
  else if (pid == 0 && holder.flags == false){
     child_run_command(holder.cmd);
     exit(0);
  }
  else{
     wait(0);
     parent_run_command(holder.cmd);
  }
}
// Run a list of commands
void run_script(CommandHolder* holders) {
  if (holders == NULL)
    return;
  check_jobs_bg_status();

  if (get_command_holder_type(holders[0]) == EXIT &&
      get_command_holder_type(holders[1]) == EOC) {
    end_main_loop();
    return;
  }
  CommandType type;

  // Run all commands in the `holder` array
  for (int i = 0; (type = get_command_holder_type(holders[i])) != EOC; ++i)
    create_process(holders[i]);
  if (!(holders[0].flags & BACKGROUND)) {
    // Not a background Job
    // Wait for all processes under the job to complete
    wait(0);
  }
  else {
    // A background job.
    // TODO: Push the new job to the job queue
    /*push_back_Jobber(deq, holders);
    int job_id = length_Jobber(deq);
    char* cmd = get_command_string();*/

    // TODO: Once jobs are implemented, uncomment and fill the following line
    /*print_job_bg_start(job_id, pid, cmd);
    free(cmd);

  }
  if (length_Jobber(deq)==0)
  {
     destroy_Jobber(deq);*/
  }

}
