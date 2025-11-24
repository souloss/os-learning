#include "vga.h"
#include "vmm.h"
#include "kheap.h"
#include "yieldlock.h"
#include "rand.h"

static kheap_t kheap;
static yieldlock_t kheap_lock;

#define HEADER_SIZE (sizeof(kheap_block_header_t))
#define FOOTER_SIZE (sizeof(kheap_block_footer_t))
#define BLOCK_META_SIZE (sizeof(kheap_block_header_t) + sizeof(kheap_block_footer_t))

#define IS_HOLE 1
#define NOT_HOLE 0

static uint32_t align_to_page(uint32_t num)
{
    if ((num & 0xFFF) != 0)
    {
        return (num & 0xFFFFF000) + PAGE_SIZE;
    }
    return num;
}

static uint32_t kheap_expand(kheap_t *this, uint32_t expand_size)
{
    vga_printf("kheap expand size = %d, end_address = %p, max_address = %p \n", expand_size, this->end_address, this->max_address);
    expand_size = align_to_page(expand_size);
    if (expand_size <= 0)
    {
        return 0;
    }

    uint32_t new_end = this->end_address + expand_size;
    ASSERT(new_end <= this->max_address);
    this->end_address = new_end;
    this->size = this->size + expand_size;
    return expand_size;
}

__attribute__((unused)) static uint32_t kheap_contract(kheap_t *this, uint32_t contract_size)
{
    contract_size = align_to_page(contract_size);
    if (contract_size <= 0)
    {
        return 0;
    }

    uint32_t new_size = this->size - contract_size;
    if (new_size < KHEAP_MIN_SIZE)
    {
        new_size = KHEAP_MIN_SIZE;
    }
    this->size = new_size;

    this->end_address = this->start_address + new_size;
    return new_size;
}

static kheap_block_header_t *make_block(uint32_t start, uint32_t size, uint8_t is_hole)
{
    ASSERT(size > 0);
    uint32_t end = start + size + BLOCK_META_SIZE;

    // vga_printf("start = %x\n", start);
    kheap_block_header_t *block_header = (kheap_block_header_t *)start;
    block_header->magic = KHEAP_MAGIC;
    block_header->size = size;
    block_header->is_hole = is_hole;

    // vga_printf("end = %x\n", end);
    kheap_block_footer_t *block_footer = (kheap_block_footer_t *)(end - FOOTER_SIZE);
    block_footer->magic = KHEAP_MAGIC;
    block_footer->header = block_header;

    return block_header;
}

static int32_t kheap_block_comparator(type_t x, type_t y)
{
    uint32_t size1 = ((kheap_block_header_t *)x)->size;
    uint32_t size2 = ((kheap_block_header_t *)y)->size;
    return size1 < size2 ? -1 : (size1 == size2 ? 0 : 1);
}

kheap_t create_kheap(uint32_t start, uint32_t end, uint32_t max, uint8_t supervisor, uint8_t readonly)
{
    ASSERT((start & 0xFFF) == 0);
    ASSERT((end & 0xFFF) == 0);

    kheap_t kheap;

    // Initialize the index array.
    kheap.index = ordered_array_create((type_t *)start, KHEAP_INDEX_NUM, &kheap_block_comparator);

    // Shift the start address forward to resemble where we can start putting data.
    start += ((sizeof(type_t) * KHEAP_INDEX_NUM));

    // Write the start, end and max addresses into the heap structure.
    kheap.start_address = start;
    kheap.end_address = end;
    kheap.size = end - start;
    kheap.max_address = max;
    kheap.supervisor = supervisor;
    kheap.readonly = readonly;

    // Start off with one large hole in the index.
    make_block(start, end - start - BLOCK_META_SIZE, IS_HOLE);
    ordered_array_insert(&kheap.index, (type_t)start);

    return kheap;
}

// Find the smallest hole that fits requested size.
// Note if page_align is required,
static int32_t find_hole(kheap_t *this, uint32_t size, uint8_t page_align, uint32_t *alloc_pos)
{
    for (int i = 0; i < this->index.size; i++)
    {
        kheap_block_header_t *header = (kheap_block_header_t *)ordered_array_get(&this->index, i);
        uint32_t start = (uint32_t)header + HEADER_SIZE;
        if (page_align)
        {
            // Align the starting point.
            // |..................|..................|..................|  page align
            //      |h| data  |f|h| data |f|
            uint32_t end = start + header->size;
            uint32_t next_page_align = align_to_page(start);
            while (next_page_align + size <= end)
            {
                if (next_page_align - start > BLOCK_META_SIZE)
                {
                    *alloc_pos = next_page_align;
                    return i;
                }
                next_page_align += PAGE_SIZE;
            }
        }
        else if (header->size >= size)
        {
            *alloc_pos = start;
            return i;
        }
    }

    return -1;
}

void *alloc(kheap_t *this, uint32_t size, uint8_t page_align)
{
    ASSERT(size > 0);

    uint32_t alloc_pos;
    int32_t iterator = find_hole(this, size, page_align, &alloc_pos);
    if (iterator < 0)
    {
        // No free hole fits, we need to expand the heap.
        uint32_t old_end_address = this->end_address;
        uint32_t extended_size = kheap_expand(this, size + BLOCK_META_SIZE);

        kheap_block_footer_t *last_footer = (kheap_block_footer_t *)(old_end_address - FOOTER_SIZE);
        kheap_block_header_t *last_header = last_footer->header;
        if (last_header->is_hole)
        {
            // Extend the last hole. Note after extension, it needs to be taken out and re-inserted
            // into the index ordered array since its size has been changed.
            make_block((uint32_t)last_header, last_header->size + extended_size, IS_HOLE);
            int32_t remove = ordered_array_remove_element(&this->index, last_header);
            ASSERT(remove);
            ordered_array_insert(&this->index, last_header);
        }
        else
        {
            // Append a new hole to the end.
            kheap_block_header_t *new_last_header =
                make_block(old_end_address, extended_size - BLOCK_META_SIZE, IS_HOLE);
            ordered_array_insert(&this->index, (type_t)new_last_header);
        }

        // Now try alloc again.
        return alloc(this, size, page_align);
    }

    kheap_block_header_t *header = (kheap_block_header_t *)ordered_array_get(&this->index, iterator);
    ASSERT(header->magic == KHEAP_MAGIC);
    uint32_t block_size = header->size;

    ordered_array_remove(&this->index, iterator);
    // If page-align is required, there may be space in the front that can make a new hole.
    if (page_align)
    {
        kheap_block_header_t *alloc_block_header = (kheap_block_header_t *)(alloc_pos - HEADER_SIZE);
        if (alloc_block_header > header)
        {
            uint32_t cut_block_size = (uint32_t)alloc_block_header - (uint32_t)header;
            ASSERT(cut_block_size > BLOCK_META_SIZE);
            make_block((uint32_t)header, cut_block_size - BLOCK_META_SIZE, IS_HOLE);
            ordered_array_insert(&this->index, (type_t)header);
            block_size -= cut_block_size;
            header = alloc_block_header;
        }
    }

    // Use this block.
    ASSERT(block_size >= size);
    uint32_t remain_size = block_size - size;
    if (remain_size <= BLOCK_META_SIZE)
    {
        size = block_size;
        remain_size = 0;
    }
    kheap_block_header_t *selected_hole_header = make_block((uint32_t)header, size, NOT_HOLE);
    (void)selected_hole_header;

    // If there is remaining size after, cut a new hole.
    if (remain_size > 0)
    {
        ASSERT(remain_size > BLOCK_META_SIZE);
        kheap_block_header_t *remain_hole_header = make_block(
            (uint32_t)header + BLOCK_META_SIZE + size, remain_size - BLOCK_META_SIZE, IS_HOLE);
        ordered_array_insert(&this->index, (type_t)remain_hole_header);
    }

    // done
    return (void *)(alloc_pos);
}

void free(kheap_t *this, void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }

    kheap_block_header_t *header = (kheap_block_header_t *)((uint32_t)ptr - HEADER_SIZE);
    kheap_block_footer_t *footer = (kheap_block_footer_t *)((uint32_t)ptr + header->size);
    ASSERT(header->magic == KHEAP_MAGIC);
    ASSERT(footer->magic == KHEAP_MAGIC);

    // Make us a hole.
    header->is_hole = 1;
    kheap_block_header_t *new_hole = header;

    // Merge with right.
    kheap_block_header_t *right_header = (kheap_block_header_t *)((uint32_t)footer + FOOTER_SIZE);
    if (right_header->magic == KHEAP_MAGIC && right_header->is_hole)
    {
        make_block((uint32_t)header, header->size + right_header->size + BLOCK_META_SIZE, IS_HOLE);
        int32_t remove = ordered_array_remove_element(&this->index, right_header);
        ASSERT(remove);
    }

    // Merge with left.
    kheap_block_footer_t *left_footer = (kheap_block_footer_t *)((uint32_t)header - FOOTER_SIZE);
    if (left_footer->magic == KHEAP_MAGIC && left_footer->header->is_hole == 1)
    {
        kheap_block_header_t *left_header = left_footer->header;
        make_block((uint32_t)left_header, left_header->size + header->size + BLOCK_META_SIZE, IS_HOLE);
        int32_t remove = ordered_array_remove_element(&this->index, left_header);
        ASSERT(remove);
        new_hole = left_header;
    }

    ordered_array_insert(&this->index, (type_t)new_hole);
}

// ****************************************************************************
uint32_t kheap_validate_print(uint8_t print)
{
    if (print)
    {
        vga_printf("*************************** kheap *****************************\n");
    }
    uint32_t start = kheap.start_address;
    uint32_t hole_num = 0;
    uint32_t alloc_num = 0;
    while (start < kheap.end_address)
    {
        kheap_block_header_t *header = (kheap_block_header_t *)(start);
        ASSERT(header->magic == KHEAP_MAGIC);
        if (header->is_hole)
        {
            ASSERT(ordered_array_find_element(&kheap.index, (type_t)header) < kheap.index.size);
            if (print)
            {
                vga_printf("[]--- start:%x end:%x size: %d\n",
                               header, (uint32_t)header + header->size + BLOCK_META_SIZE, header->size);
            }
            hole_num++;
        }
        else
        {
            if (print)
            {
                vga_printf("      start:%x end:%x size: %d\n",
                               header, (uint32_t)header + header->size + BLOCK_META_SIZE, header->size);
            }
            alloc_num++;
        }
        start += (header->size + BLOCK_META_SIZE);
        ASSERT(start <= kheap.end_address);
    }
    if (print)
    {
        vga_printf("***************************************************************\n");
    }
    ASSERT(hole_num == kheap.index.size);
    return alloc_num;
}

void init_kheap()
{
    yieldlock_init(&kheap_lock);
    kheap = create_kheap(KHEAP_START, KHEAP_START + KHEAP_MIN_SIZE, KHEAP_MAX, 0, 0);
}

static void *kmalloc_impl(uint32_t size, uint8_t align)
{
    if (size == 0)
    {
        return 0;
    }
    void *ptr = alloc(&kheap, size, (uint8_t)align);
    if (ptr == NULL)
    {
        PANIC();
    }
    return ptr;
}

void *kmalloc(uint32_t size)
{
    yieldlock_lock(&kheap_lock);
    void *ptr = kmalloc_impl(size, 0);
    yieldlock_unlock(&kheap_lock);
    return ptr;
}

void *kmalloc_aligned(uint32_t size)
{
    yieldlock_lock(&kheap_lock);
    void *ptr = kmalloc_impl(size, 1);
    yieldlock_unlock(&kheap_lock);
    return ptr;
}

void kfree(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }
    yieldlock_lock(&kheap_lock);
    free(&kheap, ptr);
    yieldlock_unlock(&kheap_lock);
}

// ******************************** unit tests **********************************
void kheap_test()
{
    uint8_t *ptr = (uint8_t *)kmalloc(32);
    *ptr = 100;
    uint8_t *ptr1 = (uint8_t *)kmalloc(200);
    *ptr1 = 101;

    uint8_t *ptr2 = (uint8_t *)kmalloc_aligned(4096 * 2);
    *ptr2 = 200;

    kfree(ptr);
    ptr = (uint8_t *)kmalloc(14);
    *ptr = 100;

    uint8_t *ptr3 = (uint8_t *)kmalloc(1);
    *ptr3 = 5;

    uint8_t *ptr4 = (uint8_t *)kmalloc_aligned(4096 * 10);
    *ptr4 = 200;

    kfree(ptr);
    kfree(ptr1);
    kfree(ptr2);
    kfree(ptr3);
    kfree(ptr4);

    // Test kheap expand.
    ptr = (uint8_t *)kmalloc(32);
    ptr1 = (uint8_t *)kmalloc(2621374);
    ptr2 = (uint8_t *)kmalloc(2);
    ptr3 = (uint8_t *)kmalloc(1);
    ptr4 = (uint8_t *)kmalloc(10);

    kheap_validate_print(/* print = */ 1);
}

void kheap_killer()
{
    uint32_t size = 200;
    rand_seed(5);

    vga_printf("kheap stress test ... ");
    for (uint32_t loop = 0; loop < 5; loop++)
    {
        // alloc
        uint8_t *ptrs[size * 2];
        for (int i = 0; i < size; i++)
        {
            uint32_t random = rand_range(1, 1000);
            if (i % 5 == 1)
            {
                ptrs[i] = (uint8_t *)kmalloc_aligned(random);
            }
            else
            {
                ptrs[i] = (uint8_t *)kmalloc(random);
            }
            ASSERT(kheap_validate_print(0) == (i + 1));
        }

        // free half
        for (int i = 0; i < size / 2; i++)
        {
            kfree(ptrs[i * 2]);
            ASSERT(kheap_validate_print(0) == (size - (i + 1)));
        }

        // alloc again
        for (int i = 0; i < size; i++)
        {
            uint32_t random = rand_range(1, 1000);
            if (i % 5 >= 2)
            {
                ptrs[i + size] = (uint8_t *)kmalloc_aligned(random);
            }
            else
            {
                ptrs[i + size] = (uint8_t *)kmalloc(random);
            }
            ASSERT(kheap_validate_print(0) == size / 2 + (i + 1));
        }

        // free all
        for (int i = 0; i < size / 2; i++)
        {
            kfree(ptrs[size + i * 2 + 1]);
            ASSERT(kheap_validate_print(0) == (size / 2 * 3 - (i + 1)));
        }
        for (int i = 0; i < size / 2; i++)
        {
            kfree(ptrs[i * 2 + 1]);
            ASSERT(kheap_validate_print(0) == (size - (i + 1)));
        }
        for (int i = 0; i < size / 2; i++)
        {
            kfree(ptrs[size + i * 2]);
            ASSERT(kheap_validate_print(0) == (size / 2 - (i + 1)));
        }

        ASSERT(kheap_validate_print(0) == 0);

        // new seed for rand
        rand_seed_with_time();
    }

    vga_printf("OK\n");
    ASSERT(kheap_validate_print(1) == 0);
}