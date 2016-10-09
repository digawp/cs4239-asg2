#include "bar.h"

char* nuber() {
	return "abc";
}

char* nub() {
	char c = 'e';
	char* ptr = &c;
	char* wut = nuber();
	return ptr;
}

void hello(){
}
