
/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%token	<string_val> WORD


%token 	NOTOKEN GREAT NEWLINE AMPERSAND GREATAMPERSAND GREATGREAT GREATGREATAMPERSAND LESS PIPE

%union	{
		char   *string_val;
	}

%{
//#define yylex yylex
#include "command.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>
#include <dirent.h>
#define MAX 1024

void yyerror(const char * s);
int yylex();

void expandWildcard(char * prefix, char * suffix, int prefixMatch);
int compare(const void* s1,const void* s2){
	return strcmp(*(char**)s1,*(char**)s2);
}

int max = 20;
int entries = 0;
char** list;
int flag;


%}

%%

goal:	
	command
	;

command: simple_command
		| command simple_command
        ;
        
simple_command:	
	pipe_list iomodifier_list back_opt NEWLINE {
		Command::_currentCommand.execute();
	}
	| NEWLINE {
		Command::_currentCommand.prompt();
	}
	| error NEWLINE { yyerrok; }
	;

argument_list:
	argument_list argument 
	| /* can be empty */
	;

argument:
	WORD {
		char * arg = $1;
		if (strchr(arg, '*') || strchr(arg, '?')) {
			list = (char**)malloc(max*sizeof(char*));
			list[0] = NULL;

			expandWildcard((char*)"", arg, 0);

			if(list[0] == NULL)
				Command::_currentSimpleCommand->insertArgument(arg);
			qsort(list, entries, sizeof(char*), compare);
			int i = 0;
			while (i<entries)
				Command::_currentSimpleCommand->insertArgument(list[i++]);
			free(list);
			max = 20;
			entries = 0;
			list = NULL;
		}
  		else 
			Command::_currentSimpleCommand->insertArgument(arg);
	}
	;

command_word:
	WORD {
	       Command::_currentSimpleCommand = new SimpleCommand();
	       Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

pipe_list:
	pipe_list PIPE command_and_args
	| command_and_args
	;

command_and_args:
	command_word argument_list {
		Command::_currentCommand.insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;

iomodifier_list:
	iomodifier_list iomodifier_opt
	| /* can be empty*/
	;

iomodifier_opt:
	GREAT WORD {
		if(Command::_currentCommand._outFile) 
			Command::_currentCommand._ambiguous = 2;
		Command::_currentCommand._outFile = $2;
	}
	| GREATGREAT WORD {
		Command::_currentCommand._append = 2;
		if(Command::_currentCommand._outFile) 
			Command::_currentCommand._ambiguous = 2;
		Command::_currentCommand._outFile = $2;
	}
	| GREATGREATAMPERSAND WORD {
		Command::_currentCommand._append = 2;
		Command::_currentCommand._err = 2;
		if(Command::_currentCommand._outFile) 
			Command::_currentCommand._ambiguous = 2;
		Command::_currentCommand._outFile = $2;
	}
	| GREATAMPERSAND WORD {
		Command::_currentCommand._err = 2;
		if(Command::_currentCommand._outFile) 
			Command::_currentCommand._ambiguous = 2;
		Command::_currentCommand._outFile = $2;
	}
	| LESS WORD {
		if(Command::_currentCommand._inFile) 
			Command::_currentCommand._ambiguous = 2;
		Command::_currentCommand._inFile = $2;
	}	
	;

back_opt:
	AMPERSAND{
		Command::_currentCommand._background = 1;
    }
	| /*can be empty*/
	;
%%





void expandWildcard(char * prefix, char * suffix, int prefixMatch) {
	if (list == NULL)
		list = (char**) malloc(max * sizeof(char*));

	if (suffix[0] == 0) {
		// suffix is empty. Put prefix in argument.
		if(entries == max) {
			max *= 2;
			list =(char**) realloc(list, sizeof(char*) * max);
		}
		list[entries++] = strdup(prefix);
		flag = 1;
		return;
	} else {
		char * s = strchr(suffix, '/');
		char component[MAX];
		flag = 1;
		/*if (s!= NULL) { 
			if(!(s-suffix))
				strcpy(component,"");
			else
				strncpy(component,suffix, s-suffix);
		suffix = s + 1;
		}*/
	}
	char * s = strchr(suffix, '/');
	char component[MAX];
	if (s == NULL) { 
		strcpy(component, suffix);
		suffix = suffix + strlen(suffix);
	} else {
		if(s - suffix)
			strncpy(component,suffix, s - suffix);
		else
			strcpy(component,"");
		suffix = s + flag;
	}

	if(strstr(prefix, "//") && flag == 1) {
		char * str = strstr(prefix, "//");
		char * temp = (char*)malloc(strlen(prefix) - flag);
		memcpy(temp, prefix, prefix - str);
		prefix = str + flag;
		strcat(temp, prefix);
		prefix = temp;
	}

	char newPrefix[MAX];
	if (!strchr(component, '*') && !strchr(component, '?')) {
		sprintf(newPrefix, "%s/%s", prefix, component);
		expandWildcard(newPrefix, suffix, 0);
		if (flag == 1)
			return;
	}

	char * reg = (char*)malloc(2 * strlen(component) + 10); 
	char * a = component;
	char * r = reg;
	*r++ = '^';
	while (*a) {
		if (*a == '*') { 
			*r++ = '.'; 
			*r++ ='*';  
		}
		else if (*a == '?') 
			*r++='.';
		else if (*a == '.') { 
			*r++ = '\\';
			*r++ = '.';
		}
		else 
			*r++ = *a;
		a++;
	}
	*r++ = '$'; 
	*r = 0;
	
	regex_t expbuf;
	if (regcomp( &expbuf, reg, REG_EXTENDED|REG_NOSUB)) {
		perror("compile");
		return;
	}
	char * dir = prefix;
	// If prefix is empty then list current directory
	if (prefix[0] == 0) 
		dir = (char*)".";

	DIR * d = opendir(dir);

	if (d == NULL) {
		if(prefixMatch == 0 && flag != 0){
			// suffix is empty. Put prefix in argument.
			flag = max * 2;
			if(entries == max)
				list =(char**)realloc(list, sizeof(char*) * flag);
			list[entries] = strdup(prefix);
			entries ++;
			flag++;
		}
		return;
	}
	
	struct dirent * ent;
	while ( (ent = readdir(d))!= NULL && flag != 0) {
		// Check if name matches
		if (!regexec( &expbuf , ent->d_name, 0, 0, 0) && 
					( (component[0] == '.' && ent->d_name[0] == '.' ) || 
					(component[0] != '.' && ent->d_name[0] != '.'))) {
			if(strlen(prefix) != 0 && a != NULL)
				sprintf(newPrefix, "%s/%s", prefix, ent->d_name);
			else
				sprintf(newPrefix, "%s", ent->d_name);
			expandWildcard(newPrefix, suffix, 1);
		}
	}
	closedir(d);
	regfree(&expbuf);
}

void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}

#if 0
main()
{
	yyparse();
}
#endif
