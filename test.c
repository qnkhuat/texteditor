#include <stdlib.h>
#include <termios.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <string.h>

enum editorKey {
	ARROW_LEFT  = 'a';
	ARROW_RIGHT = 'd';
	ARROW_UP    = 'w';
	ARROW_DOWN  = 's';
}

int main(){
	char c;
	std::cout << ARROW_LEFT << std::endl;
	
	while (1){
		printf("%zd", read(STDIN_FILENO, &c, 1));
		printf("%c", c);
		printf("\n");
	}


}
