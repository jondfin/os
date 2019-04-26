#include <stdio.h>
#include <stdlib.h>
unsigned long int **page_table;



unsigned long int
translate(unsigned long int *pagedir, unsigned long int *va){
	printf("Virtual address: 0x%lx\n", va);
	unsigned long int offset = va & 0xFFF;
	printf("Offset: 0x%lx\n", offset);
	va = va >> 12;
	unsigned long int pte = va & 0xF;
	printf("PTE: 0x%lx\n", pte);
	va = va >> 4; //remaining bits are for page directory


	printf("0x%05lx\n", page_table[0][pte]);
	printf("0x%05lx\n", pagedir[pte]);
	return offset + (pagedir[pte]<<12); 
}

int main(int argc, char** argv){
	int hex = 0xFEED;
	printf("hex: %d\n", hex);
	int seg1 = hex >> 4; //offset
	printf("seg1: %d\n", seg1);
	hex = hex >> 4;
	printf("hex: %d\n", hex);
	int seg2 = hex >> 4; //pte
	printf("seg2: %d\n", seg2);
	hex = hex >> 4; //pde
	printf("hex: %d\n", hex);

	page_table = (unsigned long int**)malloc(16 * sizeof(unsigned long int*));
	int i, j;
	for(i = 0; i < 16; i++){
		page_table[i] = (unsigned long int*)malloc(16*sizeof(unsigned long int));
	}
	for(i = 0; i < 16; i++){
		for(j = 0; j < 16; j++){
			page_table[i][j] = (unsigned long int)malloc(sizeof(unsigned long int));
		}
	}
	
		
	/*Using example from slides*/
	unsigned long int *va = 0x01ABC;
	
	page_table[0][0] = 0x10;
	page_table[0][1] = 0x23;
	page_table[0][4] = 0x80;
	page_table[0][5] = 0x59;

	unsigned long int *pa = translate(page_table[0], va);
	printf("Corresponding physical address: 0x%05lx\n", pa);
	va = 0x00000;
	unsigned long int *pa2 = translate(page_table[0], va);
	printf("Corresponding physical address: 0x%05lx\n", pa2);

	for(i = 0; i < 16; i++){
		free(page_table[i]);
	}
	free(page_table);
	return 1;
}
