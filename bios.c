typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#define SCREEN_W 800
#define SCREEN_H 600

// --- ПОРТЫ С ЗАЩИТОЙ ОТ ОПТИМИЗАЦИИ GCC ---
static inline void outb(u16 port, u8 val) { __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port) : "memory"); }
static inline u8 inb(u16 port) { u8 ret; __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port) : "memory"); return ret; }
static inline void outw(u16 port, u16 val) { __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port) : "memory"); }
static inline u16 inw(u16 port) { u16 ret; __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port) : "memory"); return ret; }
static inline void outl(u16 port, u32 val) { __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port) : "memory"); }
static inline u32 inl(u16 port) { u32 ret; __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port) : "memory"); return ret; }

// ЖЕСТКИЙ АДРЕС ВИДЕОПАМЯТИ: Компилятор не сможет это сломать
#define LFB_ADDR 0xE0000000
#define LFB ((volatile u8*)LFB_ADDR)

// --- ИНИЦИАЛИЗАЦИЯ НАСТОЯЩЕГО BIOS ---
void init_vga() {
    // 1. ВЫКЛЮЧАЕМ видеокарту перед настройкой (Command Reg = 0)
    outl(0xCF8, 0x80001004);
    outl(0xCFC, 0x00000000);

    // 2. ВПИСЫВАЕМ безопасный адрес 0xE0000000 в BAR0
    outl(0xCF8, 0x80001010);
    outl(0xCFC, LFB_ADDR);

    // 3. ВКЛЮЧАЕМ доступ к памяти и управление шиной (Command Reg = 0x07)
    outl(0xCF8, 0x80001004);
    outl(0xCFC, 0x00000007);

    // 4. Палитра
    outb(0x3C8, 0); 
    for (int i = 0; i < 256; i++) { outb(0x3C9, i / 4); outb(0x3C9, i / 4); outb(0x3C9, i / 4); }
    outb(0x3C8, 63); outb(0x3C9, 63); outb(0x3C9, 63); outb(0x3C9, 63); // 63 - Белый
    outb(0x3C8, 0);  outb(0x3C9, 0);  outb(0x3C9, 0);  outb(0x3C9, 0);  // 0 - Черный
    outb(0x3C8, 48); outb(0x3C9, 63); outb(0x3C9, 63); outb(0x3C9, 0);  // 48 - Желтый
    outb(0x3C8, 32); outb(0x3C9, 0);  outb(0x3C9, 63); outb(0x3C9, 0);  // 32 - Зеленый
    outb(0x3C8, 40); outb(0x3C9, 63); outb(0x3C9, 32); outb(0x3C9, 32); // 40 - Красный

    // 5. Настраиваем экран QEMU (Bochs VBE)
    outw(0x01CE, 0); outw(0x01CF, 0xB0C5); // Требуем LFB
    outw(0x01CE, 4); outw(0x01CF, 0);      // Отключаем
    
    outw(0x01CE, 1); outw(0x01CF, SCREEN_W); 
    outw(0x01CE, 2); outw(0x01CF, SCREEN_H); 
    outw(0x01CE, 6); outw(0x01CF, SCREEN_W); 
    outw(0x01CE, 7); outw(0x01CF, SCREEN_H); 
    outw(0x01CE, 3); outw(0x01CF, 8);        
    
    // Включаем + LFB (0x41)
    outw(0x01CE, 4); outw(0x01CF, 0x41); 
    inb(0x3DA); outb(0x3C0, 0x20);
}

// --- РИСОВАЛКИ ---
static void put_pixel(int x, int y, u8 color) {
    // Пишем прямо в железо!
    LFB[y * SCREEN_W + x] = color;
}

static void fill_rect(int x, int y, int w, int h, u8 color) {
    for(int j = y; j < y + h; j++) for(int i = x; i < x + w; i++) put_pixel(i, j, color);
}

static const u8 font[128][8] = {
    ['A']={0x3C,0x66,0x66,0x7E,0x66,0x66,0x66,0}, ['B']={0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0},
    ['C']={0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0}, ['D']={0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0},
    ['E']={0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0}, ['F']={0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0},
    ['G']={0x3C,0x66,0x60,0x6E,0x66,0x66,0x3E,0}, ['H']={0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0},
    ['I']={0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0}, ['J']={0x06,0x06,0x06,0x06,0x66,0x66,0x3C,0},
    ['K']={0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0}, ['L']={0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0},
    ['M']={0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0}, ['N']={0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0},
    ['O']={0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0}, ['P']={0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0},
    ['Q']={0x3C,0x66,0x66,0x66,0x6A,0x64,0x3A,0}, ['R']={0x7C,0x66,0x66,0x7C,0x6C,0x66,0x66,0},
    ['S']={0x3E,0x60,0x60,0x3C,0x06,0x06,0x7C,0}, ['T']={0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0},
    ['U']={0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0}, ['V']={0x66,0x66,0x66,0x66,0x3C,0x3C,0x18,0},
    ['W']={0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0}, ['X']={0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0},
    ['Y']={0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0}, ['Z']={0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0},
    ['0']={0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0}, ['1']={0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0},
    ['2']={0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0}, ['3']={0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0},
    ['4']={0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0}, ['5']={0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0},
    ['6']={0x3C,0x60,0x7C,0x66,0x66,0x66,0x3C,0}, ['7']={0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0},
    ['8']={0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0}, ['9']={0x3C,0x66,0x66,0x66,0x3E,0x06,0x3C,0},
    ['-']={0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0}, ['.']={0x00,0x00,0x00,0x00,0x00,0x18,0x18,0},
    [' ']={0,0,0,0,0,0,0,0}
};

static void draw_char(int x, int y, unsigned char c, u8 color) {
    if (c >= 128) return;
    for(int row = 0; row < 8; row++) {
        for(int col = 0; col < 8; col++) {
            if(font[(int)c][row] & (0x80 >> col)) {
                put_pixel(x + col*2, y + row*2, color);
                put_pixel(x + col*2 + 1, y + row*2, color);
                put_pixel(x + col*2, y + row*2 + 1, color);
                put_pixel(x + col*2 + 1, y + row*2 + 1, color);
            }
        }
    }
}
static void draw_string(int x, int y, const char* str, u8 color) {
    while(*str) { draw_char(x, y, *str, color); x += 16; str++; }
}

// --- ЧТЕНИЕ ДИСКА ---
void read_sector_0(u8* buffer) {
    while((inb(0x1F7) & 0xC0) != 0x40);
    outb(0x1F2, 1); outb(0x1F3, 0); outb(0x1F4, 0); outb(0x1F5, 0);
    outb(0x1F6, 0xE0); outb(0x1F7, 0x20); 
    while((inb(0x1F7) & 0x08) == 0);
    u16* ptr = (u16*)buffer;
    for(int i = 0; i < 256; i++) ptr[i] = inw(0x1F0);
}

void search_os() {
    draw_string(288, 350, "READING HDD...", 63);
    
    u8* boot_sector = (u8*)0x7C00; 
    read_sector_0(boot_sector);

    if(boot_sector[510] == 0x55 && boot_sector[511] == 0xAA) {
        draw_string(232, 370, "BOOT SIGNATURE FOUND!", 32); 
        draw_string(296, 390, "BOOTING OS...", 63);
        
        for(volatile int i = 0; i < 50000000; i++); 
        
        void (*boot)(void) = (void*)0x7C00;
        boot();
    } else {
        draw_string(240, 370, "NO BOOTABLE OS FOUND", 40);
    }
}

// --- ГЛАВНАЯ ФУНКЦИЯ ---
void bios_main() {
    init_vga();

    for(int y = 0; y < SCREEN_H; y++) {
        u8 color = (y * 63) / SCREEN_H;
        for(int x = 0; x < SCREEN_W; x++) put_pixel(x, y, color);
    }
    
    fill_rect(0, 0, SCREEN_W, 2, 63);        
    fill_rect(0, SCREEN_H-2, SCREEN_W, 2, 63); 
    fill_rect(0, 0, 2, SCREEN_H, 63);        
    fill_rect(SCREEN_W-2, 0, 2, SCREEN_H, 63); 
    
    draw_string(332, 204, "VIBEBIOS", 0);  
    draw_string(328, 200, "VIBEBIOS", 63); 
    
    draw_string(280, 250, "VGA INITIALIZED", 48);
    draw_string(312, 270, "32-BIT MODE", 32);

    search_os();

    int color = 0;
    while(1) {
        color = (color + 1) % 64;
        if(color == 0) color = 1; 
        
        draw_string(296, 450, "SYSTEM HALTED", color);
        for(volatile int i = 0; i < 1000000; i++); 
    }
}