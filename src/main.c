#include <stdio.h>
#include "sfmm.h"
#include <errno.h>
extern void* heap_start;
extern void* heap_end;
extern int first_call;
extern void* prologue;
extern void* epilogue;
extern size_t list_sizes[9];

int main(int argc, char const *argv[]) {
	sf_malloc(32624);
	return EXIT_SUCCESS;
}


