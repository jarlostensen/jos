#include <stdint.h>

struct gdt_entry_struct
{
   uint16_t limit_low;           // The lower 16 bits of the limit.
   uint16_t base_low;            // The lower 16 bits of the base.
   uint8_t  base_middle;         // The next 8 bits of the base.
   uint8_t  access;              // Access flags, determine what ring this segment can be used in.
   uint8_t  granularity;
   uint8_t  base_high;           // The last 8 bits of the base.
} __attribute__((packed));
typedef struct gdt_entry_struct gdt_entry_t;

struct gdt32_descriptor
{
    uint16_t size;
    uint32_t address;
};
typedef struct gdt32_descriptor gdt32_descriptor_t;

// =======================================================

// boot.asm
extern void _bochs_debugbreak();
// boot.asm
extern void _lgdt(gdt32_descriptor_t*);

static gdt_entry_t _gdt[3] = {
    // null
    { .limit_low = 0, .base_low = 0, .base_middle = 0, .access = 0, .granularity = 0, .base_high = 0 },
    // code
    { .limit_low = 0xffff, .base_low = 0, .base_middle = 0, .access = 0x9a, .granularity = 0xcf, .base_high = 0 },
    // data
    { .limit_low = 0xffff, .base_low = 0, .base_middle = 0, .access = 0x92, .granularity = 0xcf, .base_high = 0 },
};

static const uint32_t kVgaMemBase = 0xb8000;
static const uint32_t kVgaMemWords = 80*25;
static const uint32_t kVgaMemEnd = kVgaMemBase + kVgaMemWords*2;

void clear_vga(char attr)
{
    char* vga_ptr = (char*)(kVgaMemBase);
    for(int c = 0; c < kVgaMemWords<<1; ++c)
    {
        vga_ptr[0] = 0x20;
        vga_ptr[1] = attr;
        vga_ptr+=2;
    }
}

void write_to_vga(char* str, char attr, char x, char y)
{
    char* vga_ptr = (char*)(kVgaMemBase + x + y*(80*2));    
    while(*str &&  (uint32_t)(vga_ptr)<kVgaMemEnd)
    {
        vga_ptr[0] = *str++;
        vga_ptr[1] = attr;
        vga_ptr+=2;
    }
}

void _kmain(void* mboot)
{
    //_bochs_debugbreak();
    clear_vga(0x1c);
    write_to_vga("hello kernel world!", 0x2a, 4,4);
}