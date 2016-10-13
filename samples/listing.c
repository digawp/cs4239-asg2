char* init_array(void) {
	char array[10];
	char* p = array;
	return p;
}

char* escape_local(void) {
	char local_char = 'a';
	char* ptr = &local_char;
	return ptr;
}

int main(int argc, char const *argv[])
{
	char* ptr = init_array();
	return 0;
}
