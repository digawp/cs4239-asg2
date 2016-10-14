#include <stdlib.h>

int magic() {
	return 1;
}

char* generate_cp() {
	char* hip = malloc(10);
	if(!magic()) {
		return hip;
	} else {
		char a = 'a';
		char arr[3];
		char* ptr = &a;
		return ptr;
	}
}

char* init_array(void) {
	char array[10];
	char* p = array;
	if (magic()) {
		char a = 'b';
		char* q = &a;
		return q;
	} else {
		char* r = p;
	return r;
	}
}

char init_primitive(void) {
	char p = 'a';
	char q = p;
	char r = q;
	return r;
}

int main(int argc, char const *argv[]) {
	char* perlu_cp = generate_cp();
	return 0;
}