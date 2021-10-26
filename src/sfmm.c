/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "debug.h"
#include "sfmm.h"

void* heap_start;
void* heap_end;
int first_call;
sf_block* prologue;
sf_block* epilogue;
size_t list_sizes[9];

void validate_pointer(void* pp) {
    if(!pp) {
        abort();
    }

    if((pp - sf_mem_start()) % 64 != 0) {
        abort();
    }

    sf_block* cur_block = (sf_block*)(pp - 16);
    size_t header = cur_block->header;

    if(pp < sf_mem_start()+128){
        abort();
    }

    if((pp >= sf_mem_end())){
        abort();
    }
    if((void*)(cur_block) + cur_block->header > sf_mem_end()){
        abort();
    }

    if(!(header & THIS_BLOCK_ALLOCATED)) {
        abort();
    }
}


void initialize_free_lists(){
    int fib1=0;
    int fib2 = 1;
    int fib = 1;
    for(int i=0; i<8; i++){
        list_sizes[i] = fib*64;
        fib1 = fib2;
        fib2 = fib;
        fib = fib1+fib2;
    }
    first_call = 0;
    for(int i=0; i<9; i++){
        sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
        sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
    }
}

void set_epilogue(sf_block* block, bool prev_alloc) {
    size_t head = THIS_BLOCK_ALLOCATED;
    if(prev_alloc) {
        head = head | PREV_BLOCK_ALLOCATED;
    }
    block->header = head;
}

void delete_from_list(sf_block* block) {
    sf_block* prev = block->body.links.prev;
    sf_block* next = block->body.links.next;
    prev->body.links.next = next;
    next->body.links.prev = prev;
}


void insert_to_free_list(void *location, size_t size, int i){
    struct sf_block* node = (sf_block*) location;
    node->body.links.next = NULL;
    node->body.links.prev = NULL;
    node->header = size;
    node->header |= PREV_BLOCK_ALLOCATED;
    node->body.links.next = sf_free_list_heads[i].body.links.next;
    node->body.links.prev = &sf_free_list_heads[i];
    node->body.links.next->body.links.prev = node;
    sf_free_list_heads[i].body.links.next = node;

    void* next = location + size;
    if(next + 16 <= sf_mem_end()) {
        sf_block* next_block = next;
        next_block->prev_footer = node->header;
        next_block->header = (next_block->header & ~0x3f) | (next_block->header & THIS_BLOCK_ALLOCATED);
    }

}

void insert_to_free_lists(void *location, size_t size){
    int flag = 0;
    for(int i=0; i<8; i++){
        if(list_sizes[i] >= size){
            flag = 1;
            insert_to_free_list(location, size, i);
            break;
        }
    }
    if(flag==0){
        insert_to_free_list(location, size, 8);
    }
}


sf_block* allocate_memory(size_t size){
    struct sf_block* head;
    struct sf_block* cur;
    int flag=0;
    for(int i=0; i<8; i++){
        if(list_sizes[i] >= size){
            head = &sf_free_list_heads[i];
            cur = head->body.links.next;
            while(cur != &sf_free_list_heads[i]){
                size_t true_size = cur->header & ~0x3f;
                if(true_size >= size){
                    flag = 1;
                    size_t diff = true_size - size;
                    cur->body.links.next->body.links.prev = cur->body.links.prev;
                    cur->body.links.prev->body.links.next = cur->body.links.next;
                    int prev_alloc_status = cur->header & PREV_BLOCK_ALLOCATED;
                    cur->header = size;
                    if(prev_alloc_status){
                        cur->header |= PREV_BLOCK_ALLOCATED;
                    }
                    cur->header |= THIS_BLOCK_ALLOCATED;
                    if(diff > 0){
                        insert_to_free_lists((void*)cur+size, diff);
                    }
                    else{
                        void* cur_next = (void*)cur + true_size;
                        sf_block* cur_next_next = (sf_block*)(cur_next +  16);
                        if((void*)cur_next_next <= sf_mem_end()){
                            sf_block* cur_next1 = (sf_block*)cur_next;
                            cur_next1->header = cur_next1->header | PREV_BLOCK_ALLOCATED;
                        }
                    }
                    return cur;
                    //break;
                }
/*                else{
                    void* cur_next = (void*)cur + true_size;
                    sf_block* cur_next_next = (sf_block*)(cur_next +  16);
                    if((void*)cur_next_next <= sf_mem_end()){
                        sf_block* cur_next1 = (sf_block*)cur_next;
                        cur_next1->header = cur_next1->header | PREV_BLOCK_ALLOCATED;
                    }
                    return cur;
                }*/
                cur = cur->body.links.next;
            }
        }
    }

    if(flag==0){
        head = &sf_free_list_heads[8];
        cur = head->body.links.next;
        while(cur != &sf_free_list_heads[8]){
            size_t true_size = cur->header & ~0x3f;
            if(true_size >= size){
                flag = 1;
                size_t diff = true_size - size;
                //struct sf_block* p1 = cur->body.links.next;
                //struct sf_block *p2 = p1->body.links.prev;
                cur->body.links.next->body.links.prev = cur->body.links.prev;
                cur->body.links.prev->body.links.next = cur->body.links.next;
                cur->body.links.next = NULL;
                cur->body.links.prev = NULL;
                int prev_alloc_status = cur->header & PREV_BLOCK_ALLOCATED;
                cur->header = size;
                if(prev_alloc_status){
                    cur->header |= PREV_BLOCK_ALLOCATED;
                }
                cur->header |= THIS_BLOCK_ALLOCATED;
                if(diff > 0){
                    insert_to_free_lists((void*)cur+size, diff);
                }
                else{
                    void* cur_next = (void*)cur + true_size;
                    sf_block* cur_next_next = (sf_block*)(cur_next +  16);
                    if((void*)cur_next_next <= sf_mem_end()){
                        sf_block* cur_next1 = (sf_block*)cur_next;
                        cur_next1->header = cur_next1->header | PREV_BLOCK_ALLOCATED;
                    }
                }
                return cur;
                break;
            }
/*            else{
                void* cur_next = (void*)cur + true_size;
                sf_block* cur_next_next = (sf_block*)(cur_next +  16);
                if((void*)cur_next_next <= sf_mem_end()){
                    sf_block* cur_next1 = (sf_block*)cur_next;
                    cur_next1->header = cur_next1->header | PREV_BLOCK_ALLOCATED;
                }
                return cur;
            }*/
            cur = cur->body.links.next;
        }
    }

    return NULL;
}


void initialize_prologue_epilogue(){
    prologue = heap_start + 48;
    epilogue = (sf_block*)(sf_mem_end() - 16);
    prologue->header = 65;
    epilogue->header = 1;
}

size_t get_size(size_t size){
    size = size + 8;
    size_t m, m1;

    double t = size/64.0;
    size_t x = t;
    if(t-x > 0){
        m1 = (size/64) + 1;
    }
    else{
        m1 = size/64;
    }
    size_t num_bytes = 64*m1;


    if(num_bytes - 64 < 24){
        num_bytes += 24 - (num_bytes - 64);
    }

    size_t x1 = num_bytes/64.0;
    if(t-x1 > 0){
        m = (num_bytes/64) + 1;
    }
    else{
        m = num_bytes/64;
    }

    return m;
}

void *sf_malloc(size_t size) {

    if(first_call == 0){
        initialize_free_lists();
        first_call = 1;
        heap_start = sf_mem_grow();
        heap_start = sf_mem_start();
        heap_end = sf_mem_end();
        initialize_prologue_epilogue();
        insert_to_free_lists(heap_start+112, 8064);
    }

    size_t m = get_size(size);
    size_t bytes = m*64;

    void *block_pointer = (void*)allocate_memory(bytes);
    if(block_pointer){
        //sf_show_heap();
        return block_pointer+16;
    }

    int i = 0;
    //sf_block* end = (void*)heap_end - 16;
    sf_block* end = (void*)sf_mem_end() - 16;

    size_t prev_size = 0;
    int prev = end->header & PREV_BLOCK_ALLOCATED;

    if(!(prev)) {
        prev_size = (*((size_t*)((void*)end)) & ~0x3f);
        end = (void*)end - prev_size;
    }

    while(bytes > prev_size + i*PAGE_SZ) {
        if(sf_mem_grow() == NULL) {
            return NULL;
        }
        i++;
        if(!(end->header & THIS_BLOCK_ALLOCATED)){
            delete_from_list(end);
        }
        set_epilogue((void*)sf_mem_end()-16, 0);
        insert_to_free_lists(end, prev_size + i*PAGE_SZ);
    }

    void *ans = (void*) allocate_memory(bytes)+16;
    //sf_show_heap();
    return ans;
}

void sf_free(void *pp) {

    // Check for invalid pointer
    validate_pointer(pp);

    pp = pp - 16;
    sf_block* pp1 = pp;
    int prev_alloc = pp1->header & PREV_BLOCK_ALLOCATED;
    void* next = pp + (pp1->header & ~0x3f);
    int next_alloc;

    if(next+16 <= sf_mem_end()){
        sf_block* next_block = next;
        next_alloc = next_block->header & THIS_BLOCK_ALLOCATED;
    }
    else{
        next_alloc = 1;
    }

    if(prev_alloc && next_alloc){
        size_t size = pp1->header & ~0x3f;
        insert_to_free_lists(pp1, size);
    }
    else if(prev_alloc && !next_alloc){
        //sf_block* pp1 = pp;
        size_t size = pp1->header & ~0x3f;
        void* next = pp + size;
        sf_block* next_block = next;
        size_t next_block_size = next_block->header & ~0x3f;
        size_t total_size = size + next_block_size;
        delete_from_list(next);
        insert_to_free_lists(pp1, total_size);

    }
    else if(!prev_alloc && next_alloc){
        size_t size = pp1->header & ~0x3f;
        size_t prev_block_size = pp1->prev_footer & ~0x3f;
        size_t total_size = size + prev_block_size;
        void* to_be_deleted = pp-prev_block_size;
        delete_from_list((sf_block*)to_be_deleted);
        insert_to_free_lists((sf_block*)to_be_deleted, total_size);
    }
    else{
        size_t size = pp1->header & ~0x3f;
        size_t prev_block_size = pp1->prev_footer & ~0x3f;
        void* next = pp + size;
        sf_block* next_block = next;
        size_t next_block_size = next_block->header & ~0x3f;
        size_t total_size = size + prev_block_size + next_block_size;
        void* to_be_deleted_1 = pp-prev_block_size;
        void* to_be_deleted_2 = pp+size;
        delete_from_list((sf_block*)to_be_deleted_1);
        delete_from_list((sf_block*)to_be_deleted_2);
        insert_to_free_lists((sf_block*)to_be_deleted_1, total_size);
    }

    //sf_show_heap();


    return;
}

void *sf_realloc(void *pp, size_t rsize) {

    validate_pointer(pp);

    if(rsize==0){
        sf_free(pp);
        return NULL;
    }

    pp = pp - 16;
    sf_block* pp1 = (sf_block*)pp;
    size_t header = pp1->header;
    size_t block_size = pp1->header & ~0x3f;
    size_t new_size = get_size(rsize)*64;

    if(block_size < new_size){
        void* ptr = sf_malloc(rsize);
        memcpy(ptr, pp+16, (header & ~0x3f) - 8);
        sf_free(pp+16);
        return ptr;
    }
    else if(block_size > new_size){
        void* next = pp + (pp1->header & ~0x3f);
        int prev_status = pp1->header & PREV_BLOCK_ALLOCATED;
        pp1->header = new_size;
        pp1->header |= THIS_BLOCK_ALLOCATED;
        pp1->header |= prev_status;
        int next_alloc;

        if(next+16 <= sf_mem_end()){
            sf_block* next_block = next;
            next_alloc = next_block->header & THIS_BLOCK_ALLOCATED;
        }
        else{
            next_alloc = 1;
        }

        if(next_alloc){
            insert_to_free_lists(pp1+new_size, block_size-new_size);
            if(next+16 <= sf_mem_end()){
                sf_block* next_block = next;
                next_block->header = ((next_block->header & ~0x3f) | THIS_BLOCK_ALLOCATED);
            }
        }
        else{
            void* next = pp + block_size;
            sf_block* next_block = next;
            size_t next_block_size = next_block->header & ~0x3f;
            size_t total_size = block_size - new_size + next_block_size;
            delete_from_list(next);
            insert_to_free_lists(pp+new_size, total_size);
/*            void* next_to_next = (void*)pp1 + new_size + total_size;
            if(next_to_next+16 <= sf_mem_end()){
                sf_block* next_to_next1 = (sf_block*)next_to_next;
                next_to_next1->header =
            }*/
        }

/*        if(next+16 <= sf_mem_end()){
            sf_block* next_p = (sf_block*)next;
            //sf_block *pointer = (void*)pp1 + block_size;
            next_p->header = next_p->header & ~0x3f;
            next_p->header = next_p->header | THIS_BLOCK_ALLOCATED;
            //pointer->header = pointer->header & ~0x3f;
            //pointer->header = pointer->header | THIS_BLOCK_ALLOCATED;
        }*/
        return pp+16;

    }
    return pp+16;
}
