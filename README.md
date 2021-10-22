# Dynamic-Memory-Allocator

Overview  

Created an allocator for the x86-64 architecture with the following features:  


* Free lists segregated by size class, using first-fit policy within each size class.
* Immediate coalescing of blocks on free with adjacent free blocks.
Boundary tags to support efficient coalescing, with footer optimization that allows
footers to be omitted from allocated blocks.
* Block splitting without creating splinters.
* Allocated blocks aligned to (64-byte) boundaries.
* Free lists maintained using last in first out (LIFO) discipline.
* Use of a prologue and epilogue to achieve required alignment and avoid edge cases
at the end of the heap.
* Implemented own versions of the malloc, realloc, and free functions.
* Tested Program using Criterion unit tests
