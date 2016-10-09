#include <stdlib.h>

char* generate_cp() {
	char* hip = malloc(10);
	char a = 'a';
	char arr[3];
	char* ptr = &a;
	return &a;
}

int main(int argc, char const *argv[]) {
	char* perlu_cp = generate_cp();
	return 0;
}