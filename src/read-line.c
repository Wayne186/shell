/*
 * CS354: Operating Systems. 
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_BUFFER_LINE 2048

// Buffer where line is stored
int line_length = 0;
char line_buffer[MAX_BUFFER_LINE];
int line = 0;

// Simple history array
// This history does not change. 
// Yours have to be updated.
int history_index = 0;
char * history [];
int history_length;


void read_line_print_usage() {
	char * usage = "\n"
    		" ctrl-?       Print usage\n"
    		" Backspace    Deletes last character\n"
    		" up arrow     See last command in the history\n";
	write(1, usage, strlen(usage));
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {

	// Set terminal in raw mode
	tty_raw_mode();

	line_length = 0;

	// Read one line until enter is typed
	for (line = 0; line < MAX_BUFFER_LINE; line++) {

	    // Read one character in raw mode.
	    char ch;
	    read(0, &ch, 1);

	    if (ch > 31 && ch < 127) {
	        
		}
	}

  	// Add eol and null char at the end of string
  	line_buffer[line_length] = 10;
  	line_length++;
  	line_buffer[line_length] = 0;

  	return line_buffer;
}

