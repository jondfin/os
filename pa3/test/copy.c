#include "my_vm.h"
pte_t *PHYS_MEM;
pde_t *pageDir;


//TODO compute bits to account for different page sizes
unsigned long numEntries = 1 << 20; //virtual bitmap
unsigned long numPDEntries = 1 << 10; //same number of page table entries per page dir
unsigned long numPhysEntries = 1 << 17; //number of bitmap entries


bool init = false;//boolean to determine when to call set_physical_mem()

pde_t **pageTable;
char *virtBM; //virtual bitmap
char *physBM; //physical bitmap

/*
Function responsible for allocating and setting your physical memory 
*/
void set_physical_mem() {

    //Allocate physical memory using mmap or malloc; this is the total size of
    //your memory you are simulating

    
    //HINT: Also calculate the number of physical and virtual pages and allocate
    //virtual and physical bitmaps and initialize them

	/*Allocate address space*/
	PHYS_MEM = mmap(0, MEMSIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if(PHYS_MEM == MAP_FAILED){
		printf("Error(1)\n");
		exit(EXIT_FAILURE);
	}

	/*Place page directory in beginning of physical space*/
	pageDir = mmap(PHYS_MEM, numPDEntries, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);
		
//	pageTable = (pde_t**)malloc(numPDEntries * sizeof(pde_t*)); //only initialize the pageDir, initialize page table when necessary

	/*Create bitmaps*/
	virtBM = (char*)malloc(numEntries); 
	physBM = (char*)malloc(numPhysEntries); 
	memset(virtBM, 0, numEntries);
	memset(physBM, 0, numPhysEntries);

	/*Set the bits used by the page directory in physical memory*/
	physBM &= numPDEntries - 1;
}



/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/
pte_t * translate(pde_t *pgdir, void *va) {
    //HINT: Get the Page directory index (1st level) Then get the
    //2nd-level-page table index using the virtual address.  Using the page
    //directory index and page table index get the physical address

	pte_t address = (pte_t)va;
//TODO
	/*Split virtual address into offset and VPN*/
	pte_t offset = address & 0xFFF;
	address = address >> 12; //VPN remains
	/*Split VPN into page directory entry and page table entry*/
	pte_t PTE = address & 0x3FF;
	address = address >> 10; //page dir entry remains

	return (pte_t*)( (address << 12) + offset);

    //If translation not successfull
    //return NULL; 
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

	pde_t address = (pde_t)va;
	
	/*Split virtual address into offset and VPN*/
	pde_t offset = address & 0xFFF;
	address = address >> 12; //VPN remains
	/*Split VPN into page directory entry and page table entry*/
	pte_t PTE = address & 0x3FF;
	address = address >> 10; //page dir entry remains

	/*If the physical address was already marked valid then the physical address should already be mapped*/
	if(getbit(physBM, (pde_t)pa == 0)){
		/*Check if page directory entry is valid*/
		if(checkDir(virtBM, address) == 0){
//		if(getbit(pgdirBM, address) == 0){
			/*Allocate space for a page table*/
			pte_t *newPage = get_next_avail(1);
			pte_t *check = mmap(newPage, numPTEntries, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);
			if(check == MAP_FAILED){
				printf("Error creating new page\n");
				return -1;
			}
			/*Map address of page table to page directory*/
			*(pgdir+address) = check;
			/*Set page directory bit*/
			//setbit(pgdirBM, pgdir+address);
		}
		/*Map the entry in the page table*/	
		pte_t *pte = *(pdgir+address);
		*pte = offset;
		
		/*Set the bits*/
		//setbit(pgdirBM, address);
		int i;
		for(i = 0; i <= numPDEntries; i++){
			setbit(physBM, (address*numPDEntries)+i);
		}
		return 1;
//		else if(getbit(pgdirBM, address) == 1){
//			/*Page directory was marked as valid, check if mapping exists*/
//			if(getbit(physBM, (unsigned long int)pa == 0)){
				/*Create mapping from PTE to physical*/
				//unsigned long int *pte = *pgdir+address;
				//*pte = offset;
//				(*pgdir+address) = offset;
//			}
		
	}

    return -1;
}

/*Finds next available page and returns the VPN*/
void *get_next_avail(int num_pages) {
 
    //Use virtual address bitmap to find the next free page
	int i, count = 0, pgdirIndex = 0, pteIndex = 0;
	for(i = 0; i < numEntries; i++){
		if(checkDir(virtBM, pgdirIndex)){
			if(count == 0) pteIndex = i;
			if(getbit(virtBM, i) == 0){
				count++;	
			}else count = 0;
			if(count == numPages){
				pte_t *entry = pgdirIndex << 10;
				entry |= i;
				return entry;
			}
			if(i%numPDEntries == 0) pgdirIndex++; 
		}else{
			pgdirIndex++;
			if(pgdirIndex == numEntries) return NULL;
			i = pgdirIndex*numPDEntries;
		}

	}				
	
	return NULL;
}
/*Finds a suitable physical page and returns its address*/
void *find_phys(int num_pages){
	int i, startIndex = 0; count = 0;
	for(i = 0; i < numPhysEntries; i++){
		



	}
	return NULL;
}

/* Function responsible for allocating pages
and used by the benchmark
*/
void *a_malloc(unsigned int num_bytes) {

    //HINT: If the physical memory is not yet initialized, then allocate and initialize.

	if(!init) {
		set_physical_mem();
		init = true;
	}

   /* HINT: If the page directory is not initialized, then initialize the
   page directory. Next, using get_next_avail(), check if there are free pages. If
   free pages are available, set the bitmaps and map a new page. Note, you will 
   have to mark which physical pages are used. */

	int numPages = (int)ceil(num_bytes / PGSIZE);
	
	/*Find next virtual address*/
	void *vpn = get_next_avail(numPages);
	if(!vpn){
		printf("Error allocating memory\n");
		return NULL;
	}
	
		
	
	
	char *pa = mmap(PHYS_MEM, num_bytes, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);
	if(map == MAP_FAILED){
		printf("Error mapping\n");
		return NULL;
	}


	if(page_map(pageTable, va, pa) > 0){
		printf("Something went wrong\n");
		return NULL;
	}

	return va << 12;
}

/* Responsible for releasing one or more memory pages using virtual address (va)
*/
void a_free(void *va, int size) {

    //Free the page table entries starting from this virtual address (va)
    // Also mark the pages free in the bitmap
    //Only free if the memory from "va" to va+size is valid
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

int checkDir(char *bitmap, int dir){
	return ( (bitmap << (dir * numPDEntries) |= (1 << 10)-1));
}
