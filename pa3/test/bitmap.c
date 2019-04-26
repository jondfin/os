#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int setbit(char *bitmap, int bit_to_set){
	int offset = bit_to_set%8; 
	int frame = bit_to_set/8;

	bitmap[frame] |= 1 << offset;

	return 1;

}

int getbit(char *bitmap, int bit_to_get){
	int offset = bit_to_get%8;
	int frame = bit_to_get/8;

	printf("Getting bit at %d | 0x%lx\n", bit_to_get, (unsigned long int)bit_to_get);

	return bitmap[frame] & (1 << offset);

}

int clearbit(char *bitmap, int bit_to_clear){
	int offset = bit_to_clear%8;
	int frame = bit_to_clear/8;

	bitmap[frame] &= ~(1 << offset);
	return 1;
}

int main(int argc, char **argv){
	char *bitmap = malloc(128);
	memset(bitmap, 0, 128);

	char *physBM = malloc(1 << 20);
	if(physBM == NULL){
		printf("Error\n");
		exit(EXIT_FAILURE);
	}
	memset(physBM, 0, 1 << 20); //1 << 10 entries in page directory

/*
	setbit(bitmap, 30);	
	printf("Bit @ 30: %d\n", getbit(bitmap, 30));
	clearbit(bitmap, 30);
	printf("bit @ 30: %d\n", getbit(bitmap, 30));

	setbit(bitmap, 127);
	printf("bit @ 127: %d\n", getbit(bitmap, 127));
	clearbit(bitmap, 127);
	printf("bit @ 127: %d\n", getbit(bitmap, 127));	
*/
	void *address = (void*)0x01ABC;
	unsigned long int addr = (unsigned long int)address;
	unsigned long int offset = addr & 0xFFF;
	addr = addr >> 12;
	unsigned long int PTE = addr & 0x3FF;
	addr = addr >> 10;

	printf("0x%lx\n", offset);
	printf("0x%lx\n", PTE);
	printf("0x%lx\n", addr);

	setbit(bitmap, addr);
	printf("%d\n", getbit(bitmap, addr));
		
	void *pa = (unsigned long int*)0x23ABC;
	unsigned long int paddr = (unsigned long int)pa;
//	paddr = paddr >> 12;

	printf("Physical address: 0x%lx\n", paddr);
	setbit(physBM, paddr);
	
	printf("%d\n", getbit(physBM, paddr));

	unsigned long size = 1 << 26;
	unsigned long *real = malloc(size);
	printf("%lu\n", size);

	printf("%lu\n", 1 << 31);	
	printf("%d\n", sizeof(unsigned long));
	printf("%lu\n", size * sizeof(unsigned long)* 8);


	free(bitmap);
	free(physBM);
	free(real);
	return 0;
}
