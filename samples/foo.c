#include <stdio.h>
#include "bar.h"

int lol() {
	return 1;
}

int main(int argc, char const *argv[])
{
	hello();
	if (lol()) {
		int a = 0;
	} else {
		int b = 3122;
	}
	// char* k = nub();
	// printf("%s\n", k);
	return 0;
}
