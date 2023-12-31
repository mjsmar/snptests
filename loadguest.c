# include <stdio.h>
# include <unistd.h>
# include <math.h>
# include <float.h>
# include <limits.h>
# include <sys/time.h>
# include <sys/mman.h>
# include <stdlib.h>
# include <time.h>
# include <errno.h>
# include <string.h>
# include <stdbool.h>

#define ONE_K 1024
#define ONE_M (ONE_K * ONE_K)
#define ONE_G (1000 * ONE_M)

/*
 * Loads the guest with size specified in MBs, that will cause KVM
 * page faults to populate NPTs. 
 */

int main(int argc, char *argv[])
{

    if (argc != 2) {
        printf("usage: %s [size] in M\n", argv[0]);
	exit(0);
    }
  
    int flags = MAP_PRIVATE | MAP_ANONYMOUS; // | MAP_LOCKED;
    unsigned long size = strtoul(argv[1], NULL, 10);
    size *= ONE_M;
    unsigned long i, len = size/sizeof(unsigned long);

    fprintf(stderr, "size = %lu i = %lu\n", size, len);
    unsigned long *ptr;
    if ((ptr = (void *) mmap((void *) NULL, size, PROT_READ|PROT_WRITE, flags,0, 0)) == MAP_FAILED) {
        fprintf(stderr, "failed to map() a size=%lu errno=%d\n", size, errno);
	exit(0);
    } else
        fprintf(stderr, "allocated %lu bytes for 'a'\n", size);

    for (i=0; i < len; i++) {
       ptr[i] = i;
    }


    ptr[len/2] = 0xdeaddead;

    mprotect((void *) ptr, size, PROT_READ);
        
    for (i=0; i < len; i++) {
       if(ptr[i] != i) 
	       printf("for i ptr[i=%d]=%lx\n", i, ptr[i]);
    }
}
