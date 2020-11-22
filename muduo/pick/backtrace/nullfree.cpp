#include <iostream>
#include <stdlib.h>

int main(int argc, char const *argv[])
{
	char *p = NULL;

	free(p);
	free(p);
	delete p;
	delete p;

	std::cout << "..." << std::endl;
	return 0;
}