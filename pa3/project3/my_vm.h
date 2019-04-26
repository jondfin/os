#ifndef MY_VM_H_INCLUDED
#define MY_VM_H_INCLUDED
#include <stdbool.h>
#include <stdlib.h>

//Assume the address space is 32 bits, so the max memory size is 4GB
//Page size is 4KB

//Add any important includes here which you may need
#include <sys/mman.h>
#include <math.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define PGSIZE 4096

// Maximum size of your memory
#define MAX_MEMSIZE 4u*1024u*1024u*1024u

#define MEMSIZE 1024u*1024u*1024u

// Represents a page table entry
typedef unsigned long pte_t;

// Represents a page directory entry
typedef unsigned long pde_t;

//#define TLB_SIZE 

//Structure to represents TLB
struct tlb {

    //Assume your TLB is a direct mapped TLB of TBL_SIZE (entries)
    // You must also define wth TBL_SIZE in this file.
    //Assume each bucket to be 4 bytes
};
struct tlb tlb_store;


void set_physical_mem();
unsigned long* translate(unsigned long **pgdir, void *va);
int page_map(unsigned long **pgdir, void *va, void* pa);
bool check_in_tlb(void *va);
void put_in_tlb(void *va, void *pa);
void *a_malloc(unsigned int num_bytes);
void a_free(void *va, int size);
void put_value(void *va, void *val, int size);
void get_value(void *va, void *val, int size);
void mat_mult(void *mat1, void *mat2, int size, void *answer);

void setbit(unsigned long *bitmap, int bit);
int getbit(unsigned long *bitmap, int bit);
void clearbit(unsigned long *bitmap, int bit);

void *findPhys(unsigned int numBytes);
#endif
