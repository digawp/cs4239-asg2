#include <stdio.h>
#include "bar.h"

int hehe();

int lol(int x) {
	if (x == 0) {
		return 1;
	}
	return lol(x-1) - 1;
}

int hehe() {
	return 2;
}

int main(int argc, char const *argv[])
{
	hello();
	int (*ret_num)(int) = &lol;
	if (ret_num(2)) {
		int a = 0;
	} else {
		int b = 3122;
	}
	// char* k = nub();
	// printf("%s\n", k);
	return 0;
}
