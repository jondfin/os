#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

#define MAX_MEMSIZE 3u*1024u*1024u*1024u

int main(int argc, char** argv){
	void *address = (void*)0x01ABC;

	unsigned long int addr = (unsigned long int)address;
	unsigned long int offset = addr & 0xFFF;
	printf("0x%05lx\n", offset);

//	unsigned long int mem = 1 * 1024 * 1024 * 1024;

	char *ptr = mmap(0, MAX_MEMSIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	if(ptr == MAP_FAILED){
		printf("Error: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	ptr = ptr;
	printf("success\n");

	return 0;
}
