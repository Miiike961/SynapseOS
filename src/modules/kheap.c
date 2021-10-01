//

#include "../include/kheap.h"
bool kheap_is_init; // was kheap manager initialised or not.         TODO: is it unused?
//virtual_addr kheap_last_alloc_addr; // address of last allocation
virtual_addr kheap_begin; // address where kheap begins
virtual_addr kheap_end; // address where kheap ends
uint32_t kheap_memory_used; // how many memory was used
int kheap_allocs_num; // how many allocations now


void kheap_init() {
	kheap_begin = KHEAP_START_VADDR;
	kheap_end = 0;

	kheap_allocs_num = 0;
	kheap_memory_used = 0;
}


// increase kernel heap by some amount, this will be rounded up by the page size 
void *kheap_morecore(uint32_t size) {
	// calculate how many pages we will need
	int pages = (size / PAGE_SIZE) + 1;
	// when kheap_end == 0 we must create the initial heap
	if (kheap_end == 0)
		kheap_end = kheap_begin;
	// set the address to return
	void *prev_kheap_end = &kheap_end;
	// create the pages
	for (; pages-- > 0; kheap_end += PAGE_SIZE) {
		vmm_alloc_page(kheap_end);
		memset(&kheap_end, 0x00, PAGE_SIZE);
	}
	// return the start address of the memory we allocated to the heap
	//tty_printf("(prev_kheap_end) = %x\n", prev_kheap_end);
	return prev_kheap_end;
}


// free a previously allocated item from the kernel heap
void kheap_free(void *address) {
	kheap_item *tmp_item, *item;
	// sanity check
	if (address == 0) return;
	// set the item to remove
	item = (kheap_item*)((uint32_t)address - (uint32_t)sizeof(kheap_item));
	// find it
	for (tmp_item = kheap_begin; tmp_item != 0; tmp_item = tmp_item->next) {
		//tty_printf("tmp_item = %x\n", tmp_item);
		if (tmp_item == item) {
			// free it
			tmp_item->used = FALSE;
			kheap_memory_used -= tmp_item->size;
			kheap_allocs_num--;
			// coalesce any adjacent free items
			for (tmp_item = kheap_begin; tmp_item != 0; tmp_item = tmp_item->next)
			{
				while (!tmp_item->used && tmp_item->next != 0 && !tmp_item->next->used)
				{
					tmp_item->size += sizeof(kheap_item) + tmp_item->next->size;
					tmp_item->next = tmp_item->next->next;
				}
			}
			// break and return as we are finished
			break;
		}
	}
}


// allocates an arbiturary size of memory (via first fit) from the kernel heap
void *kheap_malloc(uint32_t size) {
	kheap_item *new_item = 0, *tmp_item;
	uint32_t total_size;
	// sanity check
	if (size == 0) return 0;

	// round up by 8 bytes and add header size
	total_size = ( ( size + 7 ) & ~7 ) + sizeof(kheap_item);

	kheap_item *last_item;
	// if the heap exists
	if (kheap_end != 0) {
		// search for first fit

		for (new_item = kheap_begin; new_item != 0; new_item = new_item->next) {
			if (new_item->next == 0) last_item = new_item;

			if (!new_item->used && (total_size <= new_item->size))
				break;
		}
	}
	// if we found one
	if (new_item != 0) {
		tmp_item = (kheap_item*)((uint32_t)new_item + total_size);
		tmp_item->size = new_item->size - total_size;
		tmp_item->used = FALSE;
		tmp_item->next = new_item->next;
	} else {
		// didnt find a fit so we must increase the heap to fit
		new_item = kheap_morecore(total_size);
		if (new_item == 0) {
			// return 0 as we are out of physical memory!
			return 0;
		}
		// create an empty item for the extra space kheap_morecore() gave us
		// we can calculate the size because morecore() allocates space that is page aligned
		tmp_item = (kheap_item*)((uint32_t)new_item + total_size);
		tmp_item->size = PAGE_SIZE - (total_size%PAGE_SIZE ? total_size%PAGE_SIZE : total_size) - sizeof(kheap_item);
		tmp_item->used = FALSE;
		tmp_item->next = 0;

		//tty_printf("last_item = %x", last_item);why commenting this causes exception??? ANSWER IS BECAUSE OF FUCKING OPTIMIZATION -O1. i disabled it and it works now witout this line
		last_item->next = new_item;
	}

	// create the new item
	new_item->size = size;
	new_item->used = TRUE;
	new_item->next = tmp_item;

	kheap_allocs_num++;
	kheap_memory_used += total_size;
	// return the newly allocated memory location
	return (void *)((uint32_t)new_item + (uint32_t)sizeof(kheap_item));//old (int)... + ...
}

void kheap_print_stat() {
	tty_printf("\nallocs number = %d", kheap_allocs_num);
	tty_printf("\nmemory used = %d bytes\n", kheap_memory_used);
}

void kheap_test() {

	uint32_t sz = 1024*768*4;
	uint8_t *mas = kheap_malloc(sz);
    //mas[0x003FFFFF] = 17;
    memset(mas, 5, sz);
    tty_printf("mas[sz-1] = %d\n", mas[sz - 1]);
    tty_printf("mas_addr = %x\n", mas);

    kheap_print_stat();


	int cnt = 12;
    int *arr = (int*)kheap_malloc(cnt*sizeof(int));
    int i;
    for (i = 0; i < cnt; i++) {
    	arr[i] = i*2;
    }
    //tty_printf("\n");
    /*for (i = 0; i < cnt; i++) {
    	tty_printf("%d ", arr[i]);
    }*/
    tty_printf("arr = %x", arr);
    kheap_print_stat();
    //kheap_free(arr);
    //tty_printf("\narr[0] = %d ", arr[1]);
    //kheap_print_stat();

    int *arr2 = (int*)kheap_malloc(24*sizeof(int));
    for (i = 0; i < 24; i++) {
    	arr2[i] = i*3;
    }
    tty_printf("\n");
    /*for (i = 0; i < 24; i++) {
    	tty_printf("%d ", arr2[i]);
    }*/
    tty_printf("arr2 = %x", arr2);
    kheap_print_stat();
    kheap_free(arr2);
    kheap_print_stat();

    char *arr3 = (char*)kheap_malloc(5*sizeof(char));
    tty_printf("arr3 = %x", arr3);

    int *arr4 = (int*)kheap_malloc(8200*sizeof(int));
    for (i = 0; i < 8200; i++) {
    	arr4[i] = i*2;
    }
    tty_printf("\n");
    /*for (i = 0; i < 8200; i++) {
    	tty_printf("%d ", arr4[i]);
    }*/
    tty_printf("(arr4) = %x\n", arr4);
    tty_printf("(arr4-hdr) = %x   kheap_end = %x\n", (uint32_t)arr4 - (uint32_t)sizeof(kheap_item), kheap_end);//if without uint32_t will be icorrect result
    kheap_print_stat();
    kheap_free(arr4);
    kheap_print_stat();
}