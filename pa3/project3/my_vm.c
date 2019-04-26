#include "my_vm.h"

unsigned long *PHYS_MEM; //physical memory
unsigned long **pageDir; //multilevel page table

unsigned long numPDEntries; //total number of page directory entries, same as number of page table entries
unsigned long numPhysEntries; //total number of slots in physical memory

unsigned long numVirtPages = (MAX_MEMSIZE / PGSIZE)/32; //total number of virtual pages

bool init = false;

/*Bitmaps*/
unsigned long *virtBM;
unsigned long *physBM;

int offsetBits;
int pteBits;
int pdeBits;

pthread_mutex_t m1;

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

	numPDEntries = 1 << pdeBits;
	numPhysEntries = MEMSIZE/32; //4bytes for unsigned long, 8 bits per byte = 32 bits per unsigned long

	/*Initialize mutex*/
	if(pthread_mutex_init(&m1, NULL) != 0){
		printf("Error initializing mutex: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	pthread_mutex_lock(&m1);

	/*Allocate physical address space*/
	PHYS_MEM = mmap(0, MEMSIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if(PHYS_MEM == MAP_FAILED){
		printf("Error creating physical memory: %s\n", strerror(errno));
		pthread_mutex_unlock(&m1);
		exit(EXIT_FAILURE);
	}

	/*Only allocate enough space for page directory, will allocate page tables when needed*/
	pageDir = (unsigned long **)malloc(numPDEntries * sizeof(unsigned long*));
	
	/*Create bitmaps*/
	virtBM = malloc(numVirtPages);	
	physBM = malloc(numPhysEntries);
	memset(virtBM, 0, numVirtPages);
	memset(physBM, 0, numPhysEntries);

	pthread_mutex_unlock(&m1);
}



/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/
unsigned long * translate(unsigned long **pgdir, void *va) {
    //HINT: Get the Page directory index (1st level) Then get the
    //2nd-level-page table index using the virtual address.  Using the page
    //directory index and page table index get the physical address

	/*Extract pte and pde from virtual address*/
	unsigned long address = (unsigned long)va;
	unsigned long offset = address & ((1 << offsetBits)-1);
	address = address >> offsetBits;
	pte_t pte = address & ((1 << pteBits) - 1);
	pde_t pde = address >> pteBits;

	pthread_mutex_lock(&m1);

	if(pgdir[pde] == NULL){
		pthread_mutex_unlock(&m1);	
		return NULL;
	}
	/*Find offset from page table and combine to make physical address*/
	unsigned long pa = (unsigned long)pgdir[pte][pde];
	pthread_mutex_unlock(&m1);

	pa = pa << offsetBits;
	pa |= offset;
	
	return (unsigned long *)pa;
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
	unsigned long offset = virt & ((1 << offsetBits) - 1);
	virt = virt >> offsetBits;
	pte_t pte = virt & ((1 << pteBits) - 1);
	pde_t pde = virt >> pteBits;

	pthread_mutex_lock(&m1);

	/*Map entry*/
	if(getbit(virtBM, pde*numPDEntries + pte) == 0){
		pgdir[pde] = (unsigned long*)malloc(numPDEntries*sizeof(unsigned long));
	}
	if(pgdir[pde] == NULL){
		pthread_mutex_unlock(&m1);
		return -1;
	}
	pgdir[pde][pte] = (unsigned long)pa >> offsetBits;
	
	/*Set bits*/
	unsigned long bit = pde*numPDEntries + pte;
	setbit(virtBM, bit);

	pthread_mutex_unlock(&m1);
	return 1;
}


/*Function that gets the next available page
*/
void *get_next_avail(int num_pages) {
	int i, count = 0;
	unsigned long addr = 0;
	
	pthread_mutex_lock(&m1);

	/*Check virtual bitmap for available pages*/
	for(i = 0; i < numVirtPages; i++){
		if(getbit(virtBM, i) == 0) count++;
		else{
			count = 0;
			addr = i*PGSIZE;
		}
		if(count == num_pages){
			pthread_mutex_unlock(&m1);
			return (unsigned long*)addr;
		}
	}

	pthread_mutex_unlock(&m1);
	return NULL;
}

/*Find available space in physical memory*/
void *findPhys(unsigned int numBytes){
	int i, count = 0;
	unsigned long *start = PHYS_MEM;

	pthread_mutex_lock(&m1);

	/*Check physical bitmap for available slots*/
	for(i = 0; i < numPhysEntries; i++){
		if(getbit(physBM, i) == 0) count++;
		else{
			count = 0;
			start = PHYS_MEM+i;
		}
		if(count == numBytes){
			pthread_mutex_unlock(&m1);
			return start;
		}
	}

	pthread_mutex_unlock(&m1);
	return NULL;
}

/* Function responsible for allocating pages
and used by the benchmark
*/
void *a_malloc(unsigned int num_bytes) {

    //HINT: If the physical memory is not yet initialized, then allocate and initialize.
	/*Check whether or not to call set_physical_mem*/
	if(!init){
		set_physical_mem();
		init = true;
	}

   /* HINT: If the page directory is not initialized, then initialize the
   page directory. Next, using get_next_avail(), check if there are free pages. If
   free pages are available, set the bitmaps and map a new page. Note, you will 
   have to mark which physical pages are used. */

	/*Find next available physical address*/
	unsigned long *pa = (unsigned long *)findPhys(num_bytes);
	if(pa == NULL) return NULL;
	/*Mark this portion of physical memory for the allocation*/

	pthread_mutex_lock(&m1);

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

	pthread_mutex_unlock(&m1);	

	/*Extract offset from physical address*/
	unsigned long offset = (unsigned long)pa & ((1 << offsetBits) - 1);

	int numPages = (int)ceil((double)num_bytes/(double)PGSIZE);

	/*Get virtual address and separate into pte and pde*/
	unsigned long va = (unsigned long)get_next_avail(numPages);
	unsigned long pte = va & ((1 << pteBits) - 1);
	unsigned long pde = va >> pteBits;

	/*Create virtual address and map it*/
	va = va << (pteBits + offsetBits);
	va |= offset;
	if(page_map(pageDir, (unsigned long*)va, pa) < 0) return NULL;

	return (unsigned long *)va;
}

/* Responsible for releasing one or more memory pages using virtual address (va)
*/
void a_free(void *va, int size) {

    //Free the page table entries starting from this virtual address (va)
    // Also mark the pages free in the bitmap
    //Only free if the memory from "va" to va+size is valid

	/*Calculate physical address*/
	unsigned long pa = (unsigned long)translate(pageDir, va);

	pthread_mutex_lock(&m1);

	int i;
	for(i = 0; i < numPhysEntries; i++){
		if((unsigned long)PHYS_MEM + i == pa){
			int j;
			for(j = 0; j < size; j++){
				/*Unmark the slots in physical memory*/
				if(getbit(physBM, i + j)) clearbit(physBM, i + j);
			}
		}
	}
	/*Check the virtual bitmap if the page table is completely empty*/
	unsigned long pde = (unsigned long)va >> (offsetBits + pteBits);
	for(i = 0; i < numPDEntries; i++){
		if(getbit(virtBM, pde*numPDEntries + i) == 1){
			pthread_mutex_unlock(&m1);
			return;
		}
	}
	/*Page table is completely empty, can invalidate the page directory entry*/
	free(pageDir[pde]);

	pthread_mutex_unlock(&m1);
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
	int m1 = 0, m2 = 0, ans = 0;
	int a, b;
	int i, j, k;
	for(i = 0; i < size; i++ ){
		for(j = 0; j < size; j++){
			sum = 0;
			for(k = 0; k < size; k++){
				/*Get address of next value in matrices*/
				m1 = (unsigned int)mat1 + ((i * size * sizeof(int)) + (k * sizeof(int)));
				m2 = (unsigned int)mat2 + ((k * size * sizeof(int)) + (j * sizeof(int))); 
				/*Get value from address in matrices*/
				get_value((void*)m1, &a, sizeof(int));
				get_value((void*)m2, &b, sizeof(int));
				
				sum += a * b;
			}
			/*Get address of answer matrix*/
			ans = (unsigned int)answer + ((i * size * sizeof(int)) + (j * sizeof(int)));
			/*Store sum in answer matrix*/
			put_value((void*)ans, &sum, sizeof(int));
		}
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

