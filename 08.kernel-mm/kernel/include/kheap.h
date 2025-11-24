#ifndef KHEAP_H
#define KHEAP_H

#include "types.h"
#include "ordered_array.h"

#define KHEAP_START 0xC0C00000
#define KHEAP_MIN_SIZE 0x300000
#define KHEAP_MAX 0xE0000000

#define KHEAP_INDEX_NUM 0x20000
#define KHEAP_MAGIC 0x12345678

// 9 bytes
struct kheap_block_header
{
    uint32_t magic;
    uint8_t is_hole;
    uint32_t size;
} __attribute__((packed));
typedef struct kheap_block_header kheap_block_header_t;

// 8 bytes
struct kheap_block_footer
{
    uint32_t magic;
    kheap_block_header_t *header;
} __attribute__((packed));
typedef struct kheap_block_footer kheap_block_footer_t;

typedef struct kernel_heap
{
    ordered_array_t index;
    uint32_t start_address;
    uint32_t end_address;
    uint32_t size;
    uint32_t max_address;
    uint8_t supervisor;
    uint8_t readonly;
} kheap_t;

// ****************************************************************************
void init_kheap();

kheap_t create_kheap(uint32_t start, uint32_t end, uint32_t max, uint8_t supervisor, uint8_t readonly);

void *kmalloc(uint32_t size);

void *kmalloc_aligned(uint32_t size);

void kfree(void *p);

uint32_t kheap_validate_print(uint8_t print);

// ******************************** unit tests **********************************
void kheap_test();
void kheap_killer();
#endif
