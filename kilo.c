#include <stdlib.h>
#include <termios.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <string.h>


/*** defines ***/
#define KILO_VERSION "0.0.0"

#define CTRL_KEY(k) ((k) & 0x1f) // & in this line is bitwise-AND operator

/*** data ***/


struct editorConfig { 
	int cx, cy;
	int screenrows;
	int screencols;
	struct termios orig_termios;
};


struct editorConfig E;



/*** terminal ***/

void clearScreen() {
	write(STDOUT_FILENO, "\x1b[2J", 4); //4 means write 4 bytes out to terminal
															// \x1b ~ 27 ~ esc
															// esc + [ => means escape sequences
															// J is the erase command
															// 2 is the argument for J. and it means clear the entire screen
	write(STDOUT_FILENO, "\x1b[H", 3); // position cursor
}

void die(const char *s){
	// Exit program and with a message
	clearScreen();
	perror(s);
	exit(1);
}

void disableRawMode(){
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1 ) die("tcsetattr");
}

void enableRawMode(){
	if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");

	tcgetattr(STDIN_FILENO, &E.orig_termios);
	atexit(disableRawMode); // register to call this function after exit program

	struct termios raw = E.orig_termios;

	// c_iflag : input flags
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); // IXON: prevent effect of Ctrl-S,-Q
																	// ICRNL: fix Ctrl-M
	raw.c_cflag |= (CS8);
	
	// c_oflag : output flags
	raw.c_oflag &= ~(OPOST); // OPOST: turn off rendering things like \n as new line
	
	// c_flag : local flags
	raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN); // ECHO turn off echo when we type 
																					// ICANON : turn of CANONICAL mode
																					// ISIG : prevent Ctrl-C, -Z to send sign
																					// IEXTEN : prevent effect of Ctrl-V, -O
	raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;																					

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}


char editorReadKey() {
	// Wait for user input
	int nread=0;
	char c;
	while ((nread == read(STDIN_FILENO, &c, 1)) != 1){
		if (nread == -1 && errno != EAGAIN) die("read");
	}
	return c;
}

int getCursorPosition(int *rows, int*cols){
	char buf[32];
	unsigned int i = 0;

	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1; // send command to get the current cursor position
																													// with n command parsed with a paramenter : 6 (6n)

	while(i < sizeof(buf) -1 ) { // after sending command. This is how we read the response
															 // The response will have format : rows;colsR
			if(read(STDIN_FILENO, &buf[i], 1) != 1) break;
			if (buf[i] == 'R') break; // break until we read the R char
			i++;
	}
	if (buf[0] != '\x1b' || buf[1] != '[') return -1;
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

	return 0;
}

int getWindowSize(int *rows, int *cols){
	struct winsize ws;
	
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0 ){
		// move cursor 999 forward (C command), move cursor 999 down (B command)
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12 ) return -1;
		return getCursorPosition(rows, cols);
	} else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}

/*** append buffer ***/
struct abuf {
	char *b;
	int len;
};

// acts as constructor for the abuf type
#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *c, int len){
	char *new = realloc(ab->b, ab->len + len);
	if (new == NULL) return;
	memcpy(&new[ab->len], c, len);
	ab->b = new;
	ab->len += len;
}

void abFree(struct abuf *ab){
	free(ab->b);
}


/*** input ***/

void editorMoveCursor(char key){
	switch(key){
		case 'a':
			E.cx--;
			break;
		case 'd':
			E.cx++;
			break;
		case 'w':
			E.cy--;
			break;
		case 's':
			E.cy++;
			break;
		case 'A':
			E.cx-=10;
			break;
		case 'D':
			E.cx+=10;
			break;
		case 'W':
			E.cy-=10;
			break;
		case 'S':
			E.cy+=10;
			break;
	}
}

void editorProcessKeypress(){
	char c = editorReadKey();

	switch(c){
		case CTRL_KEY('q'):
			clearScreen();
			exit(0);
			break;
		case 'w':
		case 'a':
		case 's':
		case 'd':
		case 'W':
		case 'A':
		case 'S':
		case 'D':
			editorMoveCursor(c);
			break;
	}
	
}

/*** output ***/
void editorDrawRows(struct abuf *ab){
	for(int y=0; y< E.screenrows; y++){
		if (y==E.screenrows / 3){
			char welcome[80];
			int welcomelen = snprintf(welcome, sizeof(welcome),
        "Welcome to my VIM  -- version %s", KILO_VERSION);

			if (welcomelen > E.screencols) welcomelen = E.screencols;

			int padding = (E.screencols - welcomelen) / 2;
			if(padding) abAppend(ab, "~", 1);
			while (padding --) abAppend(ab, " ", 1);
			
			abAppend(ab, welcome, welcomelen); // say welcome to users
		}
		else{
			abAppend(ab, "~", 1);
		}
		abAppend(ab, "\x1b[K", 3); // clear the current line
		if (y < E.screenrows -1) abAppend(ab, "\r\n", 2);
	}
}


void editorRefreshScreen() {
  struct abuf ab = ABUF_INIT;
	
	// clear screen
	abAppend(&ab, "\x1b[?25l", 6); // Turn off cursor before refresh 
  abAppend(&ab, "\x1b[H", 3);

  editorDrawRows(&ab);
	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
	abAppend(&ab, buf, strlen(buf)); // position cursor at user current position
	
	abAppend(&ab, "\x1b[?25h", 6); // Turn on cursor

  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

/*** init ***/

void initEditor(){
	E.cx = 0;
	E.cy = 0;
	if (getWindowSize(&E.screenrows, &E.screencols) == -1 ) die("WindowSize");
}

int main(){
	enableRawMode();
	initEditor();

	while(1){
		editorRefreshScreen();
		editorProcessKeypress();
	}

	return 0;
}
