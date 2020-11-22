#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <execinfo.h> // backtrace backtrace_symbols
#include <string.h>

#define SIZE 100

void dump()
{
	int j, nptrs;
	void *buffer[SIZE];
	char **strings;

	nptrs = backtrace(buffer, SIZE);
	printf("backtrace() return %d adressess\n", nptrs);

	strings = backtrace_symbols(buffer, nptrs);
	if (strings == NULL) {
		perror("backtrace_symbol");
		return;
	}

	for (j = 0; j<nptrs; j++) {
		printf(" [%02d] %s\n", j, strings[j]);
	}

	free(strings);
}


void handler(int signo)
{
	printf("\n============>>> catch signal %d(%s) <<<===========\n",
		signo, (char *)strsignal(signo));
	printf("Dump stack start...\n");
	dump();
	printf("Dump stack end...\n");

	signal(signo, SIG_DFL);
	raise(signo);
}

void printSigno(int signo)
{
	static int i = 0;
	printf("\n==========>>> catch signal %d (%s) i = %s <<<============\n", 
		signo, (char*)strsignal(signo), i++);
	printf("Dump stack start...\n");

	dump();

	printf("Dump stack end...\n");
}

void myFunc3(void)
{
	signal(SIGINT, handler);
	signal(SIGSEGV, handler);
	signal(SIGUSR1, printSigno);


	printf("Current function calls list is:\n");
	printf("------------------\n");
	dump();
	printf("------------------\n");

	while(1) {

	}
}

void myFunc2(void)
{
	myFunc3();
}

void myFunc1(int ncalls)
{
	if (ncalls > 1)
		myFunc1(ncalls - 1);
	else
		myFunc2();
}

/**
 * 编译指令：gcc -g -rdynamic backtrace.c -o backtrace
 *
 *
 * 
 */
int main(int argc, char const *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "%s num-calls\n", argv[0]);
		return -1;
	}

	myFunc1(atoi(argv[1]));

	return 0;
}
/**
Current function calls list is:
------------------
backtrace() return 11 adressess
 [00] ./bt(dump+0x1f) [0x400bdf]
 [01] ./bt(myFunc3+0x4f) [0x400d88]
 [02] ./bt(myFunc2+0x9) [0x400d9d]
 [03] ./bt(myFunc1+0x25) [0x400dc4]
 [04] ./bt(myFunc1+0x1e) [0x400dbd]
 [05] ./bt(myFunc1+0x1e) [0x400dbd]
 [06] ./bt(myFunc1+0x1e) [0x400dbd]
 [07] ./bt(myFunc1+0x1e) [0x400dbd]
 [08] ./bt(main+0x56) [0x400e1c]
 [09] /lib64/libc.so.6(__libc_start_main+0xf5) [0x7faab2c14555]
 [10] ./bt() [0x400af9]
------------------
^C
============>>> catch signal 2(Interrupt) <<<===========
Dump stack start...
backtrace() return 13 adressess
 [00] ./bt(dump+0x1f) [0x400bdf]
 [01] ./bt(handler+0x40) [0x400caf]
 [02] /lib64/libc.so.6(+0x36400) [0x7faab2c28400]
 [03] ./bt(myFunc3+0x59) [0x400d92]
 [04] ./bt(myFunc2+0x9) [0x400d9d]
 [05] ./bt(myFunc1+0x25) [0x400dc4]
 [06] ./bt(myFunc1+0x1e) [0x400dbd]
 [07] ./bt(myFunc1+0x1e) [0x400dbd]
 [08] ./bt(myFunc1+0x1e) [0x400dbd]
 [09] ./bt(myFunc1+0x1e) [0x400dbd]
 [10] ./bt(main+0x56) [0x400e1c]
 [11] /lib64/libc.so.6(__libc_start_main+0xf5) [0x7faab2c14555]
 [12] ./bt() [0x400af9]
Dump stack end...
 */


