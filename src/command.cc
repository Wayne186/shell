
/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <pwd.h>
#include <fcntl.h>
#define MAX 2048

#include "command.h"
char * path = (char*)malloc(MAX);

extern char **environ;

SimpleCommand::SimpleCommand()
{
	// Create available space for 5 arguments
	_numOfAvailableArguments = 5;
	_numOfArguments = 0;
	_arguments = (char **) malloc( _numOfAvailableArguments * sizeof( char * ) );
}

void
SimpleCommand::insertArgument( char * argument )
{
	if ( _numOfAvailableArguments == _numOfArguments  + 1 ) {
		// Double the available space
		_numOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numOfAvailableArguments * sizeof( char * ) );
	}
	
	//environment
	if (strchr(argument, '$')) {
    	int len = strlen(argument);
    	char * str = (char*)malloc(MAX);
        for (int i = 0; argument[i] != '\0'; i++) {
            if (argument[i] != '$') {
            	char * temp = (char*)malloc(len);
                for(int j = 0; argument[j] != '\0' && argument[i] != '$'; j++)
                    temp[j] = argument[i++];
                strcat(str, temp);
                free(temp);
                i--;
            } else {
				char * temp = (char*)malloc(len);
                i = i + 2;
				int k;
                for (int j = 0; argument[i] != '}'; j++){
                    temp[j] = argument[i++];
					k = j + 1;
                }
                temp[k] = '\0';
                strcat(str, getenv(temp));
                free(temp);
            }
        }
        argument = strdup(str);
	}

	//tilde
	if (argument[0] == '~') {
		if (strlen(argument) != 1)
			argument = strdup(getpwnam(argument+1)->pw_dir);
		else
			argument = strdup(getenv("HOME"));
	}

	_arguments[ _numOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numOfArguments + 1] = NULL;
	
	_numOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
	_err = 0;
	_ambiguous = 0;
}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numOfAvailableSimpleCommands == _numOfSimpleCommands ) {
		_numOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numOfSimpleCommands ] = simpleCommand;
	_numOfSimpleCommands++;
}

void
Command:: clear()
{
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inFile ) {
		free( _inFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inFile?_inFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void
Command::execute()
{
	// Don't do anything if there are no simple commands
	if ( _numOfSimpleCommands == 0 ) {
		prompt();
		return;
	}

	// Print contents of Command data structure
	//print();

	// Add execution here
	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec

	if(_ambiguous) {
		printf("Ambiguous output redirect\n");
		exit(1);
	}
	
	int tempin = dup(0);
	int tempout = dup(1);
	int temperr = dup(2);

	int in, err;

	if (_inFile)
		in = open(_inFile, O_RDONLY);
	else
		in = dup(tempin);

	if (_err != 2)
		err = dup(temperr);
	else {
		if(_append)
			err = open(_outFile, O_WRONLY|O_APPEND|O_CREAT, 0664);
		else
			err = open(_outFile, O_CREAT|O_WRONLY|O_TRUNC, 0664);
	}


	int out, ret;
	int fdpipe[2];
	pid_t pid;

	int i = 0;
	while (i < _numOfSimpleCommands) {
		dup2(in, 0);
		close(tempin);
		
		if (i + 1 < _numOfSimpleCommands) {
			pipe(fdpipe);
			in = fdpipe[0];
			out = fdpipe[1];
		} else {
			if(_outFile == 0) 
				out = dup(tempout);
			else {
				if(_append)
					out = open(_outFile, O_WRONLY|O_APPEND|O_CREAT, 0664);
				else
					out = open(_outFile, O_CREAT|O_WRONLY|O_TRUNC, 0664);
			}
		}
		dup2(out, 1);

		if (out)
			dup2(err, 2);

		close(out);
		ret = 0;

		if( !(strcmp("setenv", _simpleCommands[i]->_arguments[0]))){
			ret = setenv(_simpleCommands[0]->_arguments[1], _simpleCommands[0]->_arguments[2], 1);
			if(ret != 0) 
				perror("setenv");
		} 
		else if( !(strcmp("unsetenv", _simpleCommands[i]->_arguments[0]))){
			ret = unsetenv(_simpleCommands[0]->_arguments[1]);
			if(ret != 0) 
				perror("unsetenv");
		} 
		else if( !(strcmp("cd", _simpleCommands[i]->_arguments[0]))) {
			char ** str = environ;
			while (*str != NULL) {
				if (!strncmp(*str++, "HOME", 4)) {
					str--;
					break;
				}
			}
			int len = strlen(*str) - 5;
			char * temp = (char *)malloc(len);
			strcpy(temp, *str+5);
			if (_simpleCommands[i]->_numOfArguments > 1)
				ret = chdir(_simpleCommands[i]->_arguments[1]);
			else
				ret = chdir(temp);
			if (ret != 0)
				fprintf(stderr, "No such file or directory\n");
		}
		else {
			pid = fork();
			if (pid < 0) {
				perror("fork\n");
				exit(2);
			} 

			if(pid == 0){
				close(in);
				close(out);
				close(tempin);
				close(tempout);
				close(temperr);

				execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
				perror("execvp");
				_exit(1);
			}

			if(!_background){
				waitpid(ret, NULL, 0);
			}
		}
		i++;
	}


	// Print contents of Command data structure
	dup2(tempin, 0);
	dup2(tempout, 1);
	dup2(temperr, 2);
	close(tempin);
	close(tempout);
	close(temperr);



	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
}

// Shell implementation

void
Command::prompt()
{
	if(isatty(fileno(stdin))) {
		printf("shell $ ");
		fflush(stdout);
	}
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;


int yyparse(void);


main(int argc, char ** argv)
{
	Command::_currentCommand.prompt();
	yyparse();
}

