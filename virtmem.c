/**
 * virtmem.c
 * Written by Michael Ballantyne
 */

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define TLB_SIZE 16
#define PAGES 256
#define PAGE_MASK 255

#define PAGE_SIZE 256
#define OFFSET_BITS 8
#define OFFSET_MASK 255

#define MEMORY_SIZE PAGES * PAGE_SIZE
#define BUFFER_SIZE 10

struct tlbentry {
    unsigned char logical;
    unsigned char physical;
};
struct tlbentry tlb[TLB_SIZE];
int tlbindex = 0;
int pagetable[PAGES];

signed char main_memory[MEMORY_SIZE];
signed char* backing;

int main(int argc, const char* argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage ./virtmem backingstore input\n");
        exit(1);
    }

    const char* backing_filename = argv[1];
    int backing_fd = open(backing_filename, O_RDONLY);
    backing = mmap(0, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0);

    const char* input_filename = argv[2];
    FILE* input_fp = fopen(input_filename, "r");

    // Fill page table entries with -1 for initially empty table.
    int i;
    for (i = 0; i < PAGES; i++)
    {
        pagetable[i] = -1;
    }

    // Character buffer for reading lines of input file.
    char buffer[BUFFER_SIZE];

    // Data we need to keep track of to compute stats at end.
    int total_addresses = 0;
    int tlb_hits = 0;
    int page_faults = 0;

    // Number of the next unallocated physical page in main memory
    unsigned char free_page = 0;
    signed char value;

    // help variables
    int  physical_address = 0,logicalPage,PhysicalPage;
    struct tlbentry* currentTlb;
    struct tlbentry* changeTlb;

    while (fgets(buffer, BUFFER_SIZE, input_fp) != NULL)
    {
        int logical_address = atoi(buffer);
        total_addresses++; //increment the total addresses
        logicalPage = PAGE_MASK & (logical_address >> OFFSET_BITS); // calc the logical page by shifting the 8 bits
        PhysicalPage = -1; // we init the PhysicalPage=-1

        if(tlbindex-TLB_SIZE>0) // if tlbindex-tlbSize >0 this means we are in the range of the array
        {
           i=tlbindex-TLB_SIZE;
        }
        else // if not we are out so we have to back to 0
        {
            i=0;
        }

        while(i<tlbindex) // checking the tlb
        {
           currentTlb = &tlb[i % TLB_SIZE]; // circular
            if (currentTlb->logical == logicalPage) // checking if the current-> logical equal the logical page needed
            {
                PhysicalPage = currentTlb->physical;
            }
            i++;
        }

        if (PhysicalPage != -1) // if the PhysicalPage is not -1 this means we got the PhysicalPage from the tlb
        {
            tlb_hits++; //
        }

        else {
            PhysicalPage = pagetable[logicalPage]; // if not we have to update the PhysicalPage from the pageTable
            if (PhysicalPage == -1) { // if also the pageTable in the logicalPage -1 we have to update it
                PhysicalPage = free_page++;
                page_faults++;
                  memcpy(PAGE_SIZE * PhysicalPage + main_memory, PAGE_SIZE * logicalPage + backing,PAGE_SIZE);
                pagetable[logicalPage] = PhysicalPage; // copying the data from the right logical page in backing  to main memory
            }


            changeTlb = &tlb[tlbindex % TLB_SIZE]; // update the tlb
            tlbindex++;
            changeTlb->logical = logicalPage;
            changeTlb->physical = PhysicalPage;
        }
         physical_address = (PhysicalPage << OFFSET_BITS) | (logical_address & OFFSET_MASK); //calc the physical_address
         value=main_memory[physical_address]; // getting the value of the physical_address from  main_memory
         printf("Virtual address: %d Physical address: %d Value: %d\n", logical_address, physical_address,value);
    }

    printf("Number of Translated Addresses = %d\n", total_addresses);
    printf("Page Faults = %d\n", page_faults);
    printf("Page Fault Rate = %.3f\n", page_faults / (1. * total_addresses));
    printf("TLB Hits = %d\n", tlb_hits);
    printf("TLB Hit Rate = %.3f\n", tlb_hits / (1. * total_addresses));
    return 0;
}
