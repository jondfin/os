#include "my_vm.h"

unsigned long *PHYS_MEM;
pde_t *pageDir;

unsigned long virtEntries; //total number of entries used by the page directory and page tables
unsigned long numPDEntries; //total number of page directory entries, same as number of page table entries
unsigned long numPhysEntries; //total number of slots in physical memory

int numVirtPages = MAX_MEMSIZE / PGSIZE;
int numPhysPages = MEMSIZE / PGSIZE;

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
	printf("\nNumber of virtual entries: %d\n", virtEntries);
	printf("Number of physical entries: %d\n", numPhysEntries);
	printf("Number of directory/pt entries: %d\n", numPDEntries);
	printf("Number of bits for offset: %d\n", offsetBits);
	printf("Number of bits for pte: %d\n", pteBits);
	printf("Number of bits for pde: %d\n", pdeBits);

	printf("Address of Physical Memory: 0x%lx\n\n", PHYS_MEM);
	

	/*Put page directory into physical memory*/
	pageDir = mmap(PHYS_MEM, numPDEntries, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);
	
	printf("Address of page dir: 0x%lx\n", pageDir);
	printf("End address of page dir: 0x%lx\n\n", pageDir+numPDEntries);

	/*Create bitmaps*/
	virtBM = malloc(virtEntries);
	physBM = malloc(numPhysEntries);
	memset(virtBM, 0, virtEntries);
	memset(physBM, 0, numPhysEntries);

	/*Mark the bits in physical memory used by the page directory*/
	int i;
	for(i = 0; i <= numPDEntries; i++){
		setbit(physBM, i);
	}
}



/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/
pte_t * translate(pde_t *pgdir, void *va) {
    //HINT: Get the Page directory index (1st level) Then get the
    //2nd-level-page table index using the virtual address.  Using the page
    //directory index and page table index get the physical address

	printf("Translating: 0x%lx\n", va);

	unsigned long address = (unsigned long)va;
	unsigned long offset = address & ((1 << offsetBits)-1);
	address = address >> offsetBits;
	pte_t pte = address & ((1 << pteBits) - 1);
	printf("PTE: 0x%lx\n", pte);
	pde_t pde = address >> pteBits;
	printf("PDE: 0x%lx\n", pde);

	printf("0x%lx\n", pgdir+pte);
	unsigned long *page = pgdir + pde;
	printf("0x%lx\n", *page+pte);
	unsigned long *pa = (unsigned long *)(*page+pte);
	printf("Translated address: 0x%lx\n", pa);
	return pa;
//	return (pte_t*)((*pa << offsetBits) | offset);
}


/*
The function takes a page directory address, virtual address, physical address
as an argument, and sets a page table entry. This function will walk the page
directory to see if there is an existing mapping for a virtual address. If the
virtual address is not present, then a new entry will be added
*/
int
page_map(pde_t *pgdir, void *va, void *pa)
{

    /*HINT: Similar to translate(), find the page directory (1st level)
    and page table (2nd-level) indices. If no mapping exists, set the
    virtual to physical mapping */

	unsigned long virt = (unsigned long)va;
	printf("Provided virtual address: 0x%lx\n", va);
	unsigned long offset = virt & ((1 << offsetBits) - 1);
	printf("Extracted offset: 0x%lx\n", offset);
	virt = virt >> offsetBits;
	pte_t pte = virt & ((1 << pteBits) - 1);
	printf("Exracted pte: 0x%lx\n", pte);
	pde_t pde = virt >> pteBits;
	printf("Extraced pde: 0x%lx\n", pde);

//	unsigned long bit = pde*numPDEntries + pte;

	/*Check the virtual bit map*/
	int i;
	for(i = 0; i < virtEntries; i++){
		if(checkDir(virtBM, i)){
			/*Page directory is valid, find available slots*/
			int j;
			for(j = 0; j < numPDEntries; j++){
				unsigned long start = pde * numPDEntries;
				if(getbit(virtBM, start+j) == 0){
					/*Mapping doesn't exist, create it*/
					pde_t *dirEntry = pgdir + pde;
					pte_t *page = (pte_t*)(*dirEntry + j);
					*page = offset;
					setbit(virtBM, start+j);
					return 1; 
				}else return 0; 
			}
			return -1; //page table is full, probably received bad virtual address
		}else{
			/*Page directory is invalid, create page table in physical memory*/
			pte_t *newPageTable = (pte_t*)findPhys(numPDEntries);
			printf("address of new page table: 0x%lx\n", newPageTable);
			pte_t *check = mmap(newPageTable, numPDEntries, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);
			printf("address of new page table: 0x%lx\n", check);
			if(check == MAP_FAILED){
				printf("page_map error: %s\n", strerror(errno));
				return -1;	
			}
			
//			setbit(virtBM, pde*numPDEntries + pte);			
//			printf("Set virtual bit: %d\n", pde*numPDEntries + pte);
//			setbit(virtBM, (unsigned long)pgdir + pde);

			/*Set bits in physical bitmap*/
			int j;
			for(j = 0; j < numPhysEntries; j++){
				if(PHYS_MEM + j == check){
					printf("setting physical bits\n");
					printf("0x%lx\n", PHYS_MEM+j);
					printf("0x%lx\n", check);
					int k;
					for(k = 0; k < numPDEntries; k++){
						setbit(physBM, j+k);
					}
					break;
				}
			}
			unsigned long offset = (unsigned long)check & ((1 << offsetBits - 1));
			printf("Offset: 0x%lx\n", offset);
			unsigned long *dirEntry = pgdir + pde;
			printf("DirEntry: 0x%lx\n", dirEntry);
			*dirEntry = (unsigned long)check;
			unsigned long *pageEntry = (unsigned long*)(*dirEntry) + pte;
			printf("Page: 0x%lx\n", pageEntry);
			*pageEntry = offset;
			printf("Value: 0x%lx\n", *pageEntry);
			setbit(virtBM, pde * numPDEntries);
//			unsigned long phys = (unsigned long)check >> offsetBits;
//			*(pgdir+pde) = phys & ((1 << pteBits) - 1); //store page table entry
			
			printf("Set virtual bit: %d\n", pde * numPDEntries);

			return 1; //shouldn't ever get here
		}
	}		

    return -1;
}


/*Function that gets the next available page
*/
void *get_next_avail(int num_pages) {
 
    //Use virtual address bitmap to find the next free page
    	int i, count = 0;
	pte_t *page;
	for(i = 0; i < virtEntries; i++){
		if(getbit(virtBM, i) == 0){
			count++;
		}else count = 0;
		if(count == num_pages){
			pde_t pde = (unsigned long)i/numPDEntries;	
			pte_t pte = (unsigned long)i - (pte_t)pde;

			pde = pde << pdeBits;
			pde |= pte;
			pde = pde << offsetBits;
			
			return (pde_t*)pde;
		}
	}
	return NULL;
}


/* Function responsible for allocating pages
and used by the benchmark
*/
void *a_malloc(unsigned int num_bytes) {

    //HINT: If the physical memory is not yet initialized, then allocate and initialize.

	printf("Number of bytes requested: %d\n", num_bytes);
	if(!init){
		set_physical_mem();
		init = true;
	}

   /* HINT: If the page directory is not initialized, then initialize the
   page directory. Next, using get_next_avail(), check if there are free pages. If
   free pages are available, set the bitmaps and map a new page. Note, you will 
   have to mark which physical pages are used. */

	unsigned long *pa = (unsigned long *)findPhys(num_bytes);
	if(pa == NULL) return NULL;
	/*Mark this portion of physical memory for the allocation*/
	int i;
	for(i = 0; i <= numPhysEntries; i++){
		if(PHYS_MEM + i == pa){	
			int j;
			for(j = 0; j < num_bytes; j++){
				setbit(physBM, i + j);
			}
			break;
		}
	}


	printf("Physical address: 0x%lx\n", pa);
	unsigned long offset = (unsigned long)pa & ((1 << offsetBits) - 1);
	printf("Offset: 0x%lx\n", offset);
	unsigned long va = (unsigned long)get_next_avail(1); //is allowed to return 0x0
	printf("Virtual address: 0x%lx\n", va);
	va |= offset;
	if(page_map(pageDir, (unsigned long*)va, pa) < 0) return NULL;


	printf("Malloc completed\n");

	pte_t *addr = translate(pageDir, (unsigned long*)va);
	printf("VA: 0x%lx PA: 0x%lx\n\n", va, addr);
	return (unsigned long *)va;
}

/* Responsible for releasing one or more memory pages using virtual address (va)
*/
void a_free(void *va, int size) {

    //Free the page table entries starting from this virtual address (va)
    // Also mark the pages free in the bitmap
    //Only free if the memory from "va" to va+size is valid

	unsigned long pa = (unsigned long*)translate(pageDir, va);
	int i;
	for(i = 0; i < numPhysEntries; i++){
		if(PHYS_MEM + i == pa){
			int j;
			for(j = 0; j < size; j++){
				if(getbit(physBM, i + j) == 0);
				clearbit(physBM, i + j);
			}
			return;
		}
	}
}


/* The function copies data pointed by "val" to physical
 * memory pages using virtual address (va)
*/
void put_value(void *va, void *val, int size) {

    /* HINT: Using the virtual address and translate(), find the physical page. Copy
       the contents of "val" to a physical page. NOTE: The "size" value can be larger
       than one page. Therefore, you may have to find multiple pages using translate()
       function.*/
}


/*Given a virtual address, this function copies the contents of the page to val*/
void get_value(void *va, void *val, int size) {

    /* HINT: put the values pointed to by "va" inside the physical memory at given
    "val" address. Assume you can access "val" directly by derefencing them.
    If you are implementing TLB,  always check first the presence of translation
    in TLB before proceeding forward */


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


void *findPhys(int numBytes){
	int i, count = 0;
	unsigned long *start = PHYS_MEM+numPDEntries;

	printf("Starting at: 0x%lx\n", start);
	//printBitmap(physBM, numPhysEntries);

	for(i = 0; i < numPhysEntries; i++){
		if(getbit(physBM, i) == 0) count++;
		else{
			count = 0;
			start = PHYS_MEM+numPDEntries+i;
		}
		if(count == numBytes){
			printf("count: %d\n", count);
			printf("start: 0x%lx\n", start);
			return start;
		}
	}
	return NULL;
}
