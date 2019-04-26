#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <math.h>
#include <errno.h>

void setbit(char *bitmap, int bit){
//	printf("setting bit: %d\n", bit);

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

	char *physBM = malloc(1 << 28); //2^31 / 2^3
	memset(physBM, 0, 1 << 28);

	unsigned long int PHYSMEMSIZE = 2u*1024*1024*1024; //allocate 2GB
	unsigned long *PHYS_MEM = mmap(0, PHYSMEMSIZE,
					PROT_READ|PROT_WRITE,
					MAP_PRIVATE|MAP_ANONYMOUS,
					-1, 0);
	if(PHYS_MEM == MAP_FAILED){
		printf("Error: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	unsigned long int OCC1SIZE = 4096; //reserve 4KB of memory
	unsigned long *occupied = mmap(PHYS_MEM, OCC1SIZE,
					PROT_READ|PROT_WRITE,
					MAP_PRIVATE|MAP_ANONYMOUS,
					-1, 0);
	if(occupied == MAP_FAILED){
		printf("Error(2): %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/*Mark OCC1SIZE bits of physical bitmap as occupied*/
	int i = 0;
	for(i = 0; i < OCC1SIZE; i++){
		setbit(physBM, i);
	}
	
	//Clear the n/2 + 50 bits
	for(i = OCC1SIZE/2; i < OCC1SIZE/2+50; i++){
		clearbit(physBM, i);
	}


	//Print bitmap just in case
	for(i = 0; i < OCC1SIZE; i++){
		if(getbit(physBM, i) == 0){
			printf("No bit @ %d\n", i);
		}else{
			printf("%d\n", i);
		}
	 }


	

	if(munmap(occupied, OCC1SIZE) < 0) printf("Failed\n");
	if(munmap(PHYS_MEM, PHYSMEMSIZE) < 0) printf("Failed\n");
	free(physBM);

	return 0;
}
