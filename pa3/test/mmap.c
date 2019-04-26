#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

void setbit(char *bitmap, int bit){
	int offset = bit%8;
	int frame = bit/8;

	bitmap[frame] |= 1 << offset;
	return;
}

int getbit(char *bitmap, int bit){
	int offset = bit%8;
	int frame = bit/8;

	return bitmap[frame] & (1 << offset);
}

void clearbit(char *bitmap, int bit){
	int offset = bit%8;
	int frame = bit/8;

	bitmap[frame] &= ~(1 << offset);
	return;
}



int main(int argc, char **argv){

	char *physBM = malloc(1 << 28);

	unsigned long int addrsize = 2u*1024u*1024u*1024u;
	printf("Size of physical address %lu\n", addrsize);
	char *phys = mmap(0, addrsize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if(phys == MAP_FAILED){
		printf("Error 1\n");
		exit(0);
	}
	printf("address of physical memory: %p\n", phys);
	printf("last address of physical memory: %p\n", phys+addrsize);
	
	unsigned long int dirsize = 1 << 10;
	printf("dirsize: %d\n", dirsize);


	unsigned long int *page_dir = mmap(phys, dirsize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);
	if(page_dir == MAP_FAILED){
		printf("Error pagedir\n");
		exit(0);
	}
	printf("address of page_dir: %p\n", page_dir);

	unsigned long int i;
	for(i = 0; i <= dirsize; i++){
	//	printf("%p\n", page_dir + i);
	}
	unsigned long int *pde = page_dir + i;
	unsigned long int *pte = page_dir+i-0x4;
	*pde = (unsigned long int)pte;
	*pte = 0x500;
	
	printf("Page directory: %p\n", pde);
	printf("Page table entry: 0x%lx\n", *pde);
	printf("Page table entry: 0x%lx\n", pte);
	printf("Corresponding physical address: 0x%lx\n", *pte);

	unsigned long int *end = page_dir + dirsize;
	printf("end: %p\n", end);
	*end = 0x69;
	printf("end value: 0x%lx\n", *end);

	unsigned long int *m1 = mmap(end, 400, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);
	if(m1 == MAP_FAILED){
		printf("Error %s\n", strerror(errno));
		exit(0);
	}
	printf("address of m1: %p\n", m1); 
	printf("value: %lx\n", *m1);
	printf("end allocation of m1: 0x%lx\n", m1+400);
	printf("end address of m1: 0x%lx\n", m1+4096);
	
	unsigned long int *second = m1 + 4096;


	
	unsigned long int *m2 = mmap(second, 400, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);
	if(m2 == MAP_FAILED){
		printf("Error %s\n", strerror(errno));
		exit(0);
	}
	printf("address of m2: %p\n", m2); 
	printf("value: %lx\n", *m2);
	printf("end address of m2: 0x%lx\n", m2+400);

	/*Search for the next available page*/
	int i, startIndex = 0;
	int numFrames = addrsize/4096;
	unsigned long int *startpage;
	for(i = 0; i < numPhysEntries; i++){
		if(getbit(phys

	
	if(munmap(m1, 400) < 0) printf("Fail\n");
	if(munmap(m2, 400) < 0) printf("Fail\n");
	if(munmap(phys, 2u*1024u*1024u*1024u) < 0) printf("Fail\n");

	return 0;
}
