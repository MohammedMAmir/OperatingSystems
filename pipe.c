#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

struct exit_status{
    int status;
    int first_recorded;
};

struct exit_status exits = {.status = 0,  .first_recorded = 0};

static void check_error(int ret, const char* message){
    if(ret != -1){
        return;    
    }
    
    int err = errno;
    if(exits.first_recorded == 0){
    	exits.status = err;
    	exits.first_recorded =1;
    }
}

/*static void parentfunc(int in_pipe[2], int out_pipe[2], pid_t childPid){    
    check_error(close(in_pipe[0]), "close");
    check_error(close(out_pipe[1]), "close");
    
     Output Pipe 
    const char* message = "Testing\n";
    int bytes_written = write(in_pipe[1], message, strlen(message));
    check_error(bytes_written, "write");
    check_error(close(in_pipe[1]), "close");
    
    int wstatus;
    check_error(waitpid(childPid, &wstatus, 0), "waitpid");
    assert(WIFEXITED(wstatus));
    int exit_status = WEXITSTATUS(wstatus);
    if(exit_status != 0 && exits.first_recorded == 0){
        exits.status = exit_status;
    }
    
     Input Pipe
    char buff[4096];
    
    int bytes_read = read(out_pipe[0], buff, sizeof(buff));
    check_error(bytes_read, "read");
    printf("%.*s\n", bytes_read, buff);
}

static void childfunc(int in_pipe[2], int out_pipe[2], const char* program){
    Input Pipe 
    check_error(dup2(in_pipe[0], STDIN_FILENO), "dup2");
    // close unused fds
    check_error(close(in_pipe [0]), "close");
    check_error(close(in_pipe[1]), "close"); 
    
     Output Pipe 
    check_error(dup2(out_pipe[1], STDOUT_FILENO), "dup2");
    
    // close unused fds
    check_error(close(out_pipe [0]), "close");
    check_error(close(out_pipe[1]), "close");
    
    check_error(execlp(program, program, NULL), "execlp");
    
}
*/


int main(int argc, char *argv[]) {
    // checks that arguments of main are valid
    if (argc < 2){
        return EINVAL;
    }
    
    pid_t poo_table[argc];
    exits.first_recorded = 0;
    exits.status = 0;
    int in_pipe[2] = {0};
    int out_pipe[2] = {0};
    
    if(argc == 2){
    	check_error(pipe(in_pipe), "pipe");
    	check_error(pipe(out_pipe), "pipe" );
        pid_t pid = fork();

        if(pid > 0){
            check_error(close(in_pipe[1]), "close");
            check_error(close(out_pipe [0]), "close");
            check_error(close(out_pipe[1]), "close");
            check_error(close(in_pipe[0]), "close");
            
            int wstatus;
            check_error(waitpid(pid, &wstatus, 0), "waitpid");
            //assert(WIFEXITED(wstatus));
            int exit_status = WEXITSTATUS(wstatus);
            if(exit_status != 0 && exits.first_recorded == 0){
               exits.status = exit_status;
               exits.first_recorded = 1;
    	    }

        } else {
            check_error (close(in_pipe[1]), "close");
            check_error(close(out_pipe [0]), "close");
            check_error(close(out_pipe[1]), "close");
            check_error(close(in_pipe[0]), "close");
            
            check_error(execlp(argv[1], argv[1], NULL), "execlp");
        }
    }else{
    	for(int i = 1; i < argc; i++){
    		pid_t poo = fork();
    		if(poo > 0){
    			poo_table[i-1] = poo;
    		}else{
    			check_error(execlp(argv[i], argv[i], NULL), "execlp");
    		}
    	}
    	
    	for(int i = 1; i < argc; i++){
    		int wstatus;
    		check_error(waitpid(poo_table[i - 1], &wstatus, 0), "waitpid");
    		//assert(WIFEXITED(wstatus));
    		//if(WIFSIGNALED(wstatus) == 1){
    		//	exits.status = 1;
		//	exits.first_recorded = 1;
    		//}
    		int exit_status = WEXITSTATUS(wstatus);
    		if(exit_status != 0 && exits.first_recorded == 0){
			exits.status = exit_status;
			exits.first_recorded = 1;
   		}
    	}
        // Now that the output of first process has been written to in_pipe, we fork and finish each process and read
        // its output back into the in_pipe
        /*char buff[4096];
        for(int i = 1; i < argc; i++){ 
           check_error(pipe(in_pipe), "pipe");
	   check_error(pipe(out_pipe), "pipe" );

           pid_t pid = fork();
	   
           if(pid > 0){
           	if(i != 1 && i != (argc - 1)){
           		check_error(close(in_pipe[0]), "close");
	   		check_error(close(out_pipe[1]), "close");
               
       			int bytes_written = write(in_pipe[1], buff, strlen(buff));
    			check_error(bytes_written, "write");
			check_error(close(in_pipe[1]), "close");
			
			int wstatus;
	    		check_error(waitpid(pid, &wstatus, 0), "waitpid");
	    		//assert(WIFEXITED(wstatus));
	    		if(WIFSIGNALED(wstatus) == 1){
	    			exits.status = 1;
				exits.first_recorded =1;
	    		}
	    		int exit_status = WEXITSTATUS(wstatus);
	    		if(exit_status != 0 && exits.first_recorded == 0){
				exits.status = exit_status;
				exits.first_recorded = 1;
           		}
           		
           		for(int i = 0; i < 4096; i++){
           			buff[i] = 0;
           		}
           		memset(buff, 0, strlen(buff));
           		int bytes_read = read(out_pipe[0], buff, sizeof(buff));
			check_error(bytes_read, "read");
			check_error(close(out_pipe[0]), "close");		
           	}else if(i == (argc - 1)){
           		check_error(close(in_pipe[0]), "close");
	   		check_error(close(out_pipe[1]), "close");
               
       			int bytes_written = write(in_pipe[1], buff, strlen(buff));
    			check_error(bytes_written, "write");
			check_error(close(in_pipe[1]), "close");
			
			int wstatus;
	    		check_error(waitpid(pid, &wstatus, 0), "waitpid");
	    		//assert(WIFEXITED(wstatus));
	    		if(WIFSIGNALED(wstatus) == 1){
	    			exits.status = 1;
				exits.first_recorded =1;
	    		}
	    		int exit_status = WEXITSTATUS(wstatus);
	    		if(exit_status != 0 && exits.first_recorded == 0){
				exits.status = exit_status;
				exits.first_recorded =1;
           		}
           		
           		memset(buff, 0, strlen(buff));
           		check_error(close(out_pipe[0]), "close");	
           	}else{
           		check_error(close(in_pipe[0]), "close");
           		check_error(close(in_pipe[1]), "close");
           		check_error(close(out_pipe[1]), "close");
           		
           		int wstatus;
	    		check_error(waitpid(pid, &wstatus, 0), "waitpid");
	    		//assert(WIFEXITED(wstatus));
	    		if(WIFSIGNALED(wstatus) == 1){
	    			exits.status = 1;
				exits.first_recorded =1;
	    		}
	    		int exit_status = WEXITSTATUS(wstatus);
	    		if(exit_status != 0 && exits.first_recorded == 0){
				exits.status = exit_status;
				exits.first_recorded =1;
           		}
           	
           		int bytes_read = read(out_pipe[0], buff, sizeof(buff));
			check_error(bytes_read, "read");
			check_error(close(out_pipe[0]), "close");
           	}         
           }else{ 	
               if(i == 1){
                    check_error(close(out_pipe [0]), "close");
                    check_error(close(in_pipe[0]), "close");
                    check_error(close(in_pipe[1]), "close");
			//printf("hello from child %d\n", i);
                    check_error(dup2(out_pipe[1], STDOUT_FILENO), "dup2");
                    check_error(close(out_pipe[1]), "close");
		
			
                    check_error(execlp(argv[1], argv[1], NULL), "execlp");
               }else if(i != (argc-1)){
               	   check_error(dup2(in_pipe[0], STDIN_FILENO), "dup2");
    		// close unused fds
    		   check_error(close(in_pipe [0]), "close");
    		   check_error(close(in_pipe[1]), "close"); 
    		   check_error(close(out_pipe [0]), "close");
    		   //printf("hello from child %d\n", i);
    		   check_error(dup2(out_pipe[1], STDOUT_FILENO), "dup2");
    
		// close unused fds
    		   
    		   check_error(close(out_pipe[1]), "close");
    
    			
    		   check_error(execlp(argv[i], argv[i], NULL), "execlp");
                // output of previous process is in in_pipe, so we'll shunt output of this process into out_pipe
               }else{
		        check_error(close(out_pipe [0]), "close");
		        check_error(close(out_pipe[1]), "close");
		        check_error(close(in_pipe[1]), "close");
		        //printf("hello from child %d\n", i);
		        check_error(dup2(in_pipe[0], STDIN_FILENO), "dup2");
		        check_error(close(in_pipe[0]), "close");
			
				        
		        check_error(execlp(argv[argc - 1], argv[argc - 1], NULL), "execlp");
               }
            }
          }*/     
    }
    exit(exits.status);    
}
