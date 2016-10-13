char* init_array(void) {
	char array[10];
	char* p = array;
	return p;
}

char* escape_local(void) {
	char local_char = 'a';
	char* p = &local_char;
	return p;
}

int main(int argc, char const *argv[])
{
	char* ptr = init_array();
	return 0;
}
