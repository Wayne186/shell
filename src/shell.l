
/*
 *
 * CS-252 Fall 2013
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <regex.h>
#include <fcntl.h>
#include "y.tab.h"
#include "command.h"

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
    unput(c);
}

extern "C" char * read_line();

int mygetc(FILE * f) {
	static char *p;
  char ch;

  if (!isatty(0))
  {
	return getc(f);
  }

  if (p==NULL || *p == 0)
  {
    char * s = read_line();
    p = s;
  }

  ch = *p;
  p++;

  return ch;
}

#undef getc
#define getc(f) mygetc(f)

%}

%%

\n 	{
		return NEWLINE;
	}

[ \t] 	{
		/* Discard spaces and tabs */
     	}	

">" 	{
		return GREAT;
	}

">>" {
	return GREATGREAT;
}

">&" { 
	return GREATAMPERSAND;
}

">>&" {
	return GREATGREATAMPERSAND;
}

"<" {
	return LESS;
}

"|" {
	return PIPE;
}

"&" {
	return AMPERSAND;
}

"exit" {
	printf("Good bye!!\n");
	exit(-1);
}

[^ \t\n]*(\\[^\s])[^ \t\n]*	{
	/*escape characters*/
		
	char * escape = (char*)malloc(strlen(yytext)+1);
	int flag = 0, y = 0, i = 0;
	while(i < strlen(yytext)){
		if( yytext[i] == '\\' && flag == 0 )
			flag = 1;
		else if(flag == 1){
			escape[y++] = yytext[i];
			flag = 0;
		}else
			escape[y++] = yytext[i];
		i++;
	}
	escape[y] = '\0';
	strcpy(yytext, escape);
	free(escape);

	yylval.string_val = strdup(yytext);
	return WORD;
}

\"[^\n\"]*\"	{
		yytext = (char *)(yytext+1);
		yytext[strlen(yytext)-1] = '\0';
		yylval.string_val = strdup(yytext);	
		return WORD;
}

[^ \t\n][^ \t\n]*  {
		/* Assume that file names have only alpha chars */
		yylval.string_val = strdup(yytext);
		return WORD;
}
