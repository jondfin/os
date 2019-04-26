#include "my_vm.h"

unsigned long *PHYS_MEM;
unsigned long **pageDir;

unsigned long virtEntries; //total number of entries used by the page directory and page tables
unsigned long numPDEntries; //total number of page directory entries, same as number of page table entries
unsigned long numPhysEntries; //total number of slots in physical memory

unsigned long numVirtPages = (MAX_MEMSIZE / PGSIZE)/32;
//unsigned long numPhysPages = (MEMSIZE / PGSIZE)/32;

bool init = false;

unsigned long *virtBM;
unsigned long *physBM;

int offsetBits;
int pteBits;
int pdeBits;

void printBitmap(unsigned long *bitmap, int size){
	int i;
	for(i = 0; i < size; i++){
		if(getbit(bitmap, i)) printf("%d\n", i);
	}
}


/*
Function responsible for allocating and setting your physical memory 
*/
void set_physical_mem() {

    //Allocate physical memory using mmap or malloc; this is the total size of
    //your memory you are simulating

    
    //HINT: Also calculate the number of physical and virtual pages and allocate
    //virtual and physical bitmaps and initialize them
	
	offsetBits = (int)(log(PGSIZE)/log(2));
	pteBits = (32 - offsetBits) / 2;
	pdeBits = 32 - pteBits - offsetBits;

	virtEntries = 1 << (32 - offsetBits);
	numPDEntries = 1 << pdeBits;
	numPhysEntries = MEMSIZE/32; //4bytes for unsigned long, 8 bits per byte = 32 bits per unsigned long

	/*Allocate physical address space*/
	PHYS_MEM = mmap(0, MEMSIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if(PHYS_MEM == MAP_FAILED){
		printf("Error creating physical memory\n");
		exit(EXIT_FAILURE);
	}
/*
	printf("\nNumber of virtual entries: %d\n", virtEntries);
	printf("Number of physical entries: %d\n", numPhysEntries);
	printf("Number of directory/pt entries: %d\n", numPDEntries);
	printf("Number of bits for offset: %d\n", offsetBits);
	printf("Number of bits for pte: %d\n", pteBits);
	printf("Number of bits for pde: %d\n", pdeBits);

	printf("Address of Physical Memory: 0x%lx\n\n", PHYS_MEM);
*/	
	pageDir = (unsigned long **)malloc(numPDEntries * sizeof(unsigned long*));
	

	/*Create bitmaps*/
	virtBM = malloc(numVirtPages);	
//	virtBM = malloc(virtEntries);
	physBM = malloc(numPhysEntries);
//	physBM = malloc(numPhysEntries);
	memset(virtBM, 0, numVirtPages);
	memset(physBM, 0, numPhysEntries);

}



/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/
unsigned long * translate(unsigned long **pgdir, void *va) {
    //HINT: Get the Page directory index (1st level) Then get the
    //2nd-level-page table index using the virtual address.  Using the page
    //directory index and page table index get the physical address

//	printf("Translating: 0x%lx\n", va);

	unsigned long address = (unsigned long)va;
	unsigned long offset = address & ((1 << offsetBits)-1);
	address = address >> offsetBits;
	pte_t pte = address & ((1 << pteBits) - 1);
//	printf("PTE: 0x%lx\n", pte);
	pde_t pde = address >> pteBits;
//	printf("PDE: 0x%lx\n", pde);

//	unsigned long pa = pde << pteBits;
//	printf("PA: 0x%lx\n", pa);
//	pa |= pte;
//	printf("PA: 0x%lx\n", pa);
//	pa = pa << offsetBits;
	if(pgdir[pde] == NULL) return NULL;
	unsigned long pa = (unsigned long)pgdir[pte][pde];
	pa = pa << offsetBits;
	pa |= offset;
//	printf("Translated: 0x%lx\n", pa);
	
	return (unsigned long *)pa;
//	return (pte_t*)((*pa << offsetBits) | offset);
}


/*
The function takes a page directory address, virtual address, physical address
as an argument, and sets a page table entry. This function will walk the page
directory to see if there is an existing mapping for a virtual address. If the
virtual address is not present, then a new entry will be added
*/
int
page_map(unsigned long **pgdir, void *va, void *pa)
{

    /*HINT: Similar to translate(), find the page directory (1st level)
    and page table (2nd-level) indices. If no mapping exists, set the
    virtual to physical mapping */

	unsigned long virt = (unsigned long)va;
//	printf("Provided virtual address: 0x%lx\n", va);
	unsigned long offset = virt & ((1 << offsetBits) - 1);
//	printf("Extracted offset: 0x%lx\n", offset);
	virt = virt >> offsetBits;
	pte_t pte = virt & ((1 << pteBits) - 1);
//	printf("Exracted pte: 0x%lx\n", pte);
	pde_t pde = virt >> pteBits;
//	printf("Extraced pde: 0x%lx\n", pde);

	/*Map entry*/
	if(getbit(virtBM, pde*numPDEntries + pte) == 0){
//		printf("Empty BITMAP\n");
		pgdir[pde] = (unsigned long*)malloc(numPDEntries*sizeof(unsigned long));
	}
	if(pgdir[pde] == NULL) return -1;
	pgdir[pde][pte] = (unsigned long)pa >> offsetBits;
//	printf("Stored PFN: 0x%lx\n", pgdir[pde][pte]);

	unsigned long bit = pde*numPDEntries + pte;
	/*Set bits*/
	setbit(virtBM, bit);
	return 1;
}


/*Function that gets the next available page
*/
void *get_next_avail(int num_pages) {
//	printf("----Get_next_avail----\n");
	int i, count = 0;
	unsigned long addr = 0;
	for(i = 0; i < numVirtPages; i++){
		if(getbit(virtBM, i) == 0) count++;
		else{
			count = 0;
			addr = i*PGSIZE;
		}
		if(count == num_pages){
//			printf("return address: 0x%lx\n", addr);
			return (unsigned long*)addr;
		}
	}
//	printf("\nDidn't find an address\n");
	return NULL;
}

/*Find available space in physical memory*/
void *findPhys(unsigned int numBytes){
	int i, count = 0;
	unsigned long *start = PHYS_MEM;
//	printf("----FindPhys----\n");
//	printf("Starting at: 0x%lx\n", start);
	//printBitmap(physBM, numPhysEntries);

	for(i = 0; i < numPhysEntries; i++){
		if(getbit(physBM, i) == 0) count++;
		else{
			count = 0;
			start = PHYS_MEM+i;
		}
		if(count == numBytes){
/*			printf("%d\n", i);
			printf("count: %d\n", count);
			printf("start: 0x%lx\n", start);
*/			return start;
		}
	}
	return NULL;
}

/* Function responsible for allocating pages
and used by the benchmark
*/
void *a_malloc(unsigned int num_bytes) {

    //HINT: If the physical memory is not yet initialized, then allocate and initialize.

	if(!init){
		set_physical_mem();
		init = true;
	}

   /* HINT: If the page directory is not initialized, then initialize the
   page directory. Next, using get_next_avail(), check if there are free pages. If
   free pages are available, set the bitmaps and map a new page. Note, you will 
   have to mark which physical pages are used. */
//	printf("---A_Malloc----\n");
//	printf("Number of bytes requested: %d\n", num_bytes);	
	unsigned long *pa = (unsigned long *)findPhys(num_bytes);
	if(pa == NULL) return NULL;
	/*Mark this portion of physical memory for the allocation*/
	int i;
	for(i = 0; i <= numPhysEntries; i++){
		if(PHYS_MEM + i == pa){	
			int j;
			for(j = 0; j <= num_bytes; j++){
				setbit(physBM, i + j);
			}
			break;
		}
	}
//	printf("Physical address: 0x%lx\n", pa);

	unsigned long offset = (unsigned long)pa & ((1 << offsetBits) - 1);
//	printf("Offset: 0x%lx\n", offset);

	int numPages = (int)ceil((double)num_bytes/(double)PGSIZE);
//	printf("Looking for %d page \n", numPages);


	unsigned long va = (unsigned long)get_next_avail(numPages);
//	printf("VIRTUAL ADDRESS: 0x%lx\n", va);
//	printf("---A_malloc---\n");
	unsigned long pte = va & ((1 << pteBits) - 1);
//	printf("pte: 0x%lx\n", pte);
	unsigned long pde = va >> pteBits;
	printf("pde: 0x%lx\n", pde);


	va = va << (pteBits + offsetBits);
	va |= offset;
//	printf("Virtual address: 0x%lx\n", va);
	if(page_map(pageDir, (unsigned long*)va, pa) < 0) return NULL;


//	printf("Malloc completed\n");

//	pte_t *addr = translate(pageDir, (unsigned long*)va);
//	printf("VA: 0x%lx PA: 0x%lx\n\n", va, addr);
	return (unsigned long *)va;
}

/* Responsible for releasing one or more memory pages using virtual address (va)
*/
void a_free(void *va, int size) {

    //Free the page table entries starting from this virtual address (va)
    // Also mark the pages free in the bitmap
    //Only free if the memory from "va" to va+size is valid

	unsigned long pa = (unsigned long)translate(pageDir, va);
//	printf("Freeing 0x%lx | 0x%lx\n", va, pa);
	int i;
	for(i = 0; i < numPhysEntries; i++){
		if((unsigned long)PHYS_MEM + i == pa){
			int j;
			for(j = 0; j < size; j++){
				if(getbit(physBM, i + j) == 0);
				clearbit(physBM, i + j);
			}
//			printf("Successfully freed 0x%lx\n", va);
		}
	}
	/*Check the virtual bitmap if the page table is completely empty*/
	unsigned long pde = (unsigned long)va >> (offsetBits + pteBits);
	for(i = 0; i < numPDEntries; i++){
		if(getbit(virtBM, pde*numPDEntries + i) == 1) return;
	}
	free(pageDir[pde]);
}


/* The function copies data pointed by "val" to physical
 * memory pages using virtual address (va)
*/
void put_value(void *va, void *val, int size) {

    /* HINT: Using the virtual address and translate(), find the physical page. Copy
       the contents of "val" to a physical page. NOTE: The "size" value can be larger
       than one page. Therefore, you may have to find multiple pages using translate()
       function.*/
	unsigned long *pa = translate(pageDir, va);
//	printf("Putting %d in va: 0x%lx | pa: 0x%lx\n", val, va, pa);
	memcpy(pa, val, size);

}


/*Given a virtual address, this function copies the contents of the page to val*/
void get_value(void *va, void *val, int size) {

    /* HINT: put the values pointed to by "va" inside the physical memory at given
    "val" address. Assume you can access "val" directly by derefencing them.
    If you are implementing TLB,  always check first the presence of translation
    in TLB before proceeding forward */
	unsigned long *pa = translate(pageDir, va);
	memcpy(val, pa, size);
}



/*
This function receives two matrices mat1 and mat2 as an argument with size
argument representing the number of rows and columns. After performing matrix
multiplication, copy the result to answer.
*/
void mat_mult(void *mat1, void *mat2, int size, void *answer) {

    /* Hint: You will index as [i * size + j] where  "i, j" are the indices of the
    matrix accessed. Similar to the code in test.c, you will use get_value() to
    load each element and perform multiplication. Take a look at test.c! In addition to 
    getting the values from two matrices, you will perform multiplication and 
    store the result to the "answer array"*/

	int sum = 0;
	int m1 = 0, m2 = 0, ans;
	int a, b;
	int i, j;
	for(i = 0; i < size; i++ ){
		sum = 0;
		for(j = 0; j < size; j++){
			/*Get address of next value in matrices*/
			m1 = (unsigned int)mat1 + ((i * size * sizeof(int)) + (j * sizeof(int)));
			m2 = (unsigned int)mat2 + ((j * size * sizeof(int)) + (i * sizeof(int))); 
			/*Get value from address in matrices*/
			get_value((void*)m1, &a, sizeof(int));
			get_value((void*)m2, &b, sizeof(int));
			
			sum += a * b;
		}
		ans = (unsigned int)answer + ((i * size * sizeof(int)) + (j * sizeof(int)));



		printf("sum: %d\n", sum);
		printf("\n");
	}

       
}

/*Helper functions*/

void setbit(unsigned long *bitmap, int bit){
	int offset = bit%32;
	int frame = bit/32;

	bitmap[frame] |= 1 << offset;
	return;
}
int getbit(unsigned long *bitmap, int bit){
	int offset = bit%32;
	int frame = bit/32;

	return bitmap[frame] & (1 << offset);
}
void clearbit(unsigned long *bitmap, int bit){
	int offset = bit%32;
	int frame = bit/32;
	
	bitmap[frame] &= ~(1 << offset);
	return;
}

/*Checks if a page directory has been set*/
int checkDir(unsigned long *bitmap, int dir){
	int start = dir * numPDEntries;

	int i;
	for(i = 0; i < numPDEntries; i++){
		if(getbit(virtBM, start+i)) return 1; //dir is valid
	} 
	return 0; //dir is invalid
}


