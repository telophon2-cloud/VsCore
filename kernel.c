/* ==========================================================================
 *   VsCore Kernel - Part 1: Core Definitions & VGA Screen Driver
 *   Developed by: Shumatbaev Vladislav & VladisSystems Team (c)2026
 *   License: MIT License
 *   ========================================================================== */

#define VIDEO_ADDRESS 0xB8000
#define MAX_ROWS 25
#define MAX_COLS 80

/* Атрибуты цветов VGA текста (Классическая тёмная тема + VsDE) */
#define WHITE_ON_BLACK 0x0F
#define CYAN_ON_BLACK  0x0B
#define BLUE_BACKGROUND 0x1F // Белый текст на синем фоне для оконной среды VsDE
#define WIN_FRAME      0x70 // Серый цвет рамок элементов окон
#define WIN_TITLE      0x9F // Синий заголовок активного окна

/* Порты жесткого диска ATA (Для не виснущего установщика) */
#define ATA_DATA_PORT         0x1F0
#define ATA_SECTOR_COUNT_PORT 0x1F2
#define ATA_LBA_LOW_PORT      0x1F3
#define ATA_LBA_MID_PORT      0x1F4
#define ATA_LBA_HIGH_PORT     0x1F5
#define ATA_DRIVE_HEAD_PORT   0x1F6
#define ATA_STATUS_PORT       0x1F7
#define ATA_COMMAND_PORT      0x1F7

/* Настройки VsCoreFS */
#define MAX_FILES 64
#define MAX_FILE_NAME 32
#define MAX_FILE_SIZE 2048

/* Базовые типы данных */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

/* Структура файла в файловой системе VsCoreFS */
typedef struct {
    char name[MAX_FILE_NAME];
    uint32_t size;
    char content[MAX_FILE_SIZE];
    uint8_t is_used;
} VsCoreFile;

/* Глобальное состояние терминала */
static int cursor_row = 0;
static int cursor_col = 0;
static uint8_t current_color = WHITE_ON_BLACK;

/* Порты ввода-вывода для работы с оборудованием */
uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

void outb(uint16_t port, uint8_t data) {
    __asm__ volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

uint16_t inw(uint16_t port) {
    uint16_t result;
    __asm__ volatile("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

void outw(uint16_t port, uint16_t data) {
    __asm__ volatile("outw %0, %1" : : "a"(data), "Nd"(port));
}

/* Обновление аппаратного курсора на экране */
void update_cursor() {
    uint16_t position = (cursor_row * MAX_COLS) + cursor_col;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(position & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((position >> 8) & 0xFF));
}

/* Очистка экрана под чёрный фон терминала */
void clear_screen() {
    char* video_memory = (char*)VIDEO_ADDRESS;
    for (int i = 0; i < MAX_ROWS * MAX_COLS * 2; i += 2) {
        video_memory[i] = ' ';
        video_memory[i+1] = WHITE_ON_BLACK;
    }
    cursor_row = 0;
    cursor_col = 0;
    update_cursor();
}

/* Посчитать длину строки */
int strlen(const char* str) {
    int len = 0;
    while (str[len] != '\0') len++;
    return len;
}

/* Сравнение строк */
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/* Сравнение первых N символов строк */
int strncmp(const char* s1, const char* s2, int n) {
    while (n > 0) {
        if (*s1 != *s2) return *(const unsigned char*)s1 - *(const unsigned char*)s2;
        if (*s1 == '\0') return 0;
        s1++;
        s2++;
        n--;
    }
    return 0;
}

/* Низкоуровневый вывод символа на экран */
void putchar(char c) {
    char* video_memory = (char*)VIDEO_ADDRESS;

    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
    } else if (c == '\r') {
        cursor_col = 0;
    } else {
        int offset = (cursor_row * MAX_COLS + cursor_col) * 2;
        video_memory[offset] = c;
        video_memory[offset+1] = current_color;
        cursor_col++;
    }

    if (cursor_col >= MAX_COLS) {
        cursor_col = 0;
        cursor_row++;
    }

    if (cursor_row >= MAX_ROWS) {
        for (int r = 1; r < MAX_ROWS; r++) {
            for (int col = 0; col < MAX_COLS * 2; col++) {
                video_memory[((r-1)*MAX_COLS)*2 + col] = video_memory[(r*MAX_COLS)*2 + col];
            }
        }
        for (int col = 0; col < MAX_COLS * 2; col += 2) {
            video_memory[((MAX_ROWS-1)*MAX_COLS)*2 + col] = ' ';
            video_memory[((MAX_ROWS-1)*MAX_COLS)*2 + col + 1] = WHITE_ON_BLACK;
        }
        cursor_row = MAX_ROWS - 1;
    }
    update_cursor();
}

/* Вывод строки на экран */
void print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        putchar(str[i]);
    }
}
/* ==========================================================================
 *   VsCore Kernel - Part 2: Special Character Keyboard Driver & Neofetch
 *   Developed by: Shumatbaev Vladislav & VladisSystems Team (c)2026
 *   License: MIT License
 *   ========================================================================== */

/* Базовая карта скан-кодов (Shift отключен аппаратно) */
static const char kbd_us[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*',    0,
    ' ',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0, '-',   0,   0,   0, '+',   0,   0,   0,   0,   0,  0, 0, 0, 0
};

/* Прямое чтение скан-кода из порта контроллера клавиатуры i8042 */
uint8_t get_raw_scancode() {
    while (!(inb(0x64) & 1)); // Ждем, пока буфер заполнится данными
    return inb(0x60);
}

/* Безопасное чтение символа для стандартного командного шелла */
char getchar() {
    uint8_t scancode = get_raw_scancode();
    if (scancode == 0x2A || scancode == 0x36 || scancode == 0xAA || scancode == 0xB6) {
        return 0; // Полное игнорирование аппаратного Shift
    }
    if (scancode & 0x80) {
        return 0; // Игнорируем события отпускания клавиш
    }
    return kbd_us[scancode];
}

/* Ввод строки из консоли */
void input_string(char* buffer, int max_len) {
    int i = 0;
    while (i < max_len - 1) {
        char c = getchar();
        if (c == 0) continue;

        if (c == '\n') {
            putchar('\n');
            break;
        }
        else if (c == '\b') {
            if (i > 0) {
                i--;
                if (cursor_col > 0) {
                    cursor_col--;
                } else if (cursor_row > 0) {
                    cursor_row--;
                    cursor_col = MAX_COLS - 1;
                }
                char* video_memory = (char*)VIDEO_ADDRESS;
                int offset = (cursor_row * MAX_COLS + cursor_col) * 2;
                video_memory[offset] = ' ';
                video_memory[offset+1] = current_color;
                update_cursor();
            }
        }
        else {
            buffer[i++] = c;
            putchar(c);
        }
    }
    buffer[i] = '\0';
}

/* Вывод стартового логотипа Neofetch для VsCore */
void run_neofetch() {
    uint8_t old_color = current_color;

    current_color = CYAN_ON_BLACK;
    print("  __      __   _____    _____                      \n");
    print("  \\ \\    / /  / ____|  / ____|                     \n");
    print("   \\ \\  / /  | (___   | |      ___   _ __  ___     \n");
    print("    \\ \\/ /    \\___ \\  | |     / _ \\ | '__|/ _ \\    \n");
    print("     \\  /     ____) | | |____| (_) || |  |  __/    \n");
    print("      \\/     |_____/   \\_____/\\___/ |_|   \\___|    \n");

    current_color = WHITE_ON_BLACK;
    print("Welcome To VsCore! Made By Shumatbaev Vladislav (c)2026\n\n");

    current_color = old_color;
}

void run_clear() {
    clear_screen();
    run_neofetch();
}
/* ==========================================================================
 *   VsCore Kernel - Part 3: VsCoreFS & Upgraded vSano File Editor
 *   Developed by: Shumatbaev Vladislav & VladisSystems Team (c)2026
 *   License: MIT License
 *   ========================================================================== */

/* Хранилище файлов в оперативной памяти */
static VsCoreFile filesystem[MAX_FILES];
static int file_count = 0;

void init_fs() {
    for (int i = 0; i < MAX_FILES; i++) {
        filesystem[i].is_used = 0;
        filesystem[i].name[0] = '\0';
        filesystem[i].size = 0;
    }
    file_count = 0;
}

int write_file(const char* name, const char* content) {
    // Перезапись существующего файла
    for (int i = 0; i < MAX_FILES; i++) {
        if (filesystem[i].is_used && strcmp(filesystem[i].name, name) == 0) {
            int len = strlen(content);
            if (len >= MAX_FILE_SIZE) len = MAX_FILE_SIZE - 1;
            int j;
            for (j = 0; j < len; j++) {
                filesystem[i].content[j] = content[j];
            }
            filesystem[i].content[j] = '\0';
            filesystem[i].size = len;
            return 1;
        }
    }

    // Создание нового файла
    for (int i = 0; i < MAX_FILES; i++) {
        if (!filesystem[i].is_used) {
            int name_len = strlen(name);
            if (name_len >= MAX_FILE_NAME) name_len = MAX_FILE_NAME - 1;
            int j;
            for (j = 0; j < name_len; j++) {
                filesystem[i].name[j] = name[j];
            }
            filesystem[i].name[j] = '\0';

            int content_len = strlen(content);
            if (content_len >= MAX_FILE_SIZE) content_len = MAX_FILE_SIZE - 1;
            for (j = 0; j < content_len; j++) {
                filesystem[i].content[j] = content[j];
            }
            filesystem[i].content[j] = '\0';

            filesystem[i].size = content_len;
            filesystem[i].is_used = 1;
            file_count++;
            return 1;
        }
    }
    return 0;
}

const char* read_file(const char* name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (filesystem[i].is_used && strcmp(filesystem[i].name, name) == 0) {
            return filesystem[i].content;
        }
    }
    return 0;
}

int delete_file(const char* name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (filesystem[i].is_used && strcmp(filesystem[i].name, name) == 0) {
            filesystem[i].is_used = 0;
            filesystem[i].name[0] = '\0';
            filesystem[i].size = 0;
            file_count--;
            return 1;
        }
    }
    return 0;
}

void list_files() {
    print("Directory listing for VsCoreFS:\n");
    if (file_count == 0) {
        print("  [No files found]\n");
        return;
    }
    for (int i = 0; i < MAX_FILES; i++) {
        if (filesystem[i].is_used) {
            print("  - ");
            print(filesystem[i].name);
            print("  (Size: ");
            char sz[16];
            int size = filesystem[i].size;
            int idx = 0;
            if (size == 0) sz[idx++] = '0';
            else {
                char temp[16];
                int t_idx = 0;
                while (size > 0) {
                    temp[t_idx++] = (size % 10) + '0';
                    size /= 10;
                }
                while (t_idx > 0) {
                    sz[idx++] = temp[--t_idx];
                }
            }
            sz[idx] = '\0';
            print(sz);
            print(" bytes)\n");
        }
    }
}

/* Обновленный текстовый редактор vSano с поддержкой редактирования и скобок без Shift */
void run_vsano(const char* filename) {
    if (strlen(filename) == 0) {
        print("Usage: vsano [filename]\n");
        return;
    }

    static char editor_buffer[MAX_FILE_SIZE];
    int buffer_pos = 0;
    editor_buffer[0] = '\0';

    // Проверяем: если файл уже существует, подгружаем его для редактирования
    const char* existing_content = read_file(filename);
    if (existing_content != 0) {
        int ext_len = strlen(existing_content);
        if (ext_len >= MAX_FILE_SIZE) ext_len = MAX_FILE_SIZE - 1;
        int i;
        for (i = 0; i < ext_len; i++) {
            editor_buffer[i] = existing_content[i];
        }
        editor_buffer[i] = '\0';
        buffer_pos = ext_len;
    }

    clear_screen();
    print("=== vSano Editor v2.5 | Editing: ");
    print(filename);
    print(" ===\n");
    print("Hotkeys: [= {  ]= }  9= (  0= )  ;= \" | ARROWS to move | Ctrl+S to Save\n");
    print("------------------------------------------------------------------------\n");

    int start_txt_row = cursor_row;

    // Если файл открыт повторно, сразу выводим на экран то, что было внутри
    if (buffer_pos > 0) {
        print(editor_buffer);
    } else {
        print("> ");
    }

    uint8_t is_ctrl_pressed = 0;

    while (1) {
        uint8_t scancode = get_raw_scancode();

        // Обработка Ctrl
        if (scancode == 0x1D) { is_ctrl_pressed = 1; continue; }
        if (scancode == 0x9D) { is_ctrl_pressed = 0; continue; }

        if (scancode & 0x80) continue; // Пропускаем отпускание

        // Сохранение и Выход: Ctrl+S (S = 0x1F) или F2 (0x3C)
        if ((is_ctrl_pressed && scancode == 0x1F) || scancode == 0x3C) {
            break;
        }

        // Перемещение курсора на стрелочки
        if (scancode == 0x48) { // ВВЕРХ
            if (cursor_row > start_txt_row) { cursor_row--; update_cursor(); }
            continue;
        }
        if (scancode == 0x50) { // ВНИЗ
            if (cursor_row < MAX_ROWS - 1) { cursor_row++; update_cursor(); }
            continue;
        }
        if (scancode == 0x4B) { // ВЛЕВО
            if (cursor_col > 2) { cursor_col--; update_cursor(); }
            continue;
        }
        if (scancode == 0x4D) { // ВПРАВО
            if (cursor_col < MAX_COLS - 1) { cursor_col++; update_cursor(); }
            continue;
        }

        char c = kbd_us[scancode];
        if (c == 0) continue;

        // Подстановка спецсимволов для программирования из-за отключенного Shift
        if (c == '[') c = '{';
        else if (c == ']') c = '}';
        else if (c == '9') c = '(';
        else if (c == '0') c = ')';
        else if (c == ';') c = '"';

        if (c == '\n') {
            if (buffer_pos < MAX_FILE_SIZE - 2) {
                editor_buffer[buffer_pos++] = '\n';
                editor_buffer[buffer_pos] = '\0';
            }
            putchar('\n');
            print("> ");
        }
        else if (c == '\b') {
            if (buffer_pos > 0 && cursor_col > 2) {
                buffer_pos--;
                editor_buffer[buffer_pos] = '\0';
                cursor_col--;
                char* video_memory = (char*)VIDEO_ADDRESS;
                int offset = (cursor_row * MAX_COLS + cursor_col) * 2;
                video_memory[offset] = ' ';
                video_memory[offset+1] = current_color;
                update_cursor();
            }
        }
        else {
            if (buffer_pos < MAX_FILE_SIZE - 2) {
                editor_buffer[buffer_pos++] = c;
                editor_buffer[buffer_pos] = '\0';
                putchar(c);
            }
        }
    }

    if (write_file(filename, editor_buffer)) {
        run_clear();
        print("[vSano: File updated successfully!]\n");
    } else {
        run_clear();
        print("[vSano: Error saving file!]\n");
    }
}
/* ==========================================================================
 *   VsCore Kernel - Part 4.1: VsDE Graphical Window Engine & ATA IO
 *   Developed by: Shumatbaev Vladislav & VladisSystems Team (c)2026
 *   License: MIT License
 *   ========================================================================== */

/* Функция чтения сектора диска ATA PIO LBA28 (Не виснет, опрашивает статус) */
int ata_read_sector(uint32_t lba, uint16_t* buffer) {
    outb(ATA_DRIVE_HEAD_PORT, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECTOR_COUNT_PORT, 1);
    outb(ATA_LBA_LOW_PORT, (uint8_t)lba);
    outb(ATA_LBA_MID_PORT, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HIGH_PORT, (uint8_t)(lba >> 16));
    outb(ATA_COMMAND_PORT, 0x20); // Команда чтения сектора

    // Невиснущий опрос статуса диска с таймаутом
    uint32_t timeout = 400000;
    while (timeout--) {
        uint8_t status = inb(ATA_STATUS_PORT);
        if ((status & 0x80) == 0 && (status & 0x08) != 0) {
            for (int i = 0; i < 256; i++) {
                buffer[i] = inw(ATA_DATA_PORT);
            }
            return 1;
        }
    }
    return 0;
}

/* Функция записи сектора диска ATA PIO LBA28 */
int ata_write_sector(uint32_t lba, uint16_t* buffer) {
    outb(ATA_DRIVE_HEAD_PORT, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECTOR_COUNT_PORT, 1);
    outb(ATA_LBA_LOW_PORT, (uint8_t)lba);
    outb(ATA_LBA_MID_PORT, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HIGH_PORT, (uint8_t)(lba >> 16));
    outb(ATA_COMMAND_PORT, 0x30); // Команда записи сектора

    uint32_t timeout = 400000;
    while (timeout--) {
        uint8_t status = inb(ATA_STATUS_PORT);
        if ((status & 0x80) == 0 && (status & 0x08) != 0) {
            for (int i = 0; i < 256; i++) {
                outw(ATA_DATA_PORT, buffer[i]);
            }
            return 1;
        }
    }
    return 0;
}

/* Графический отрисовщик окон для VsDE */
void draw_window(int start_row, int start_col, int rows, int cols, const char* title, const char* body, uint8_t active) {
    char* video_memory = (char*)VIDEO_ADDRESS;
    uint8_t frame_color = WIN_FRAME;
    uint8_t title_color = active ? WIN_TITLE : 0x70;

    for (int r = start_row; r < start_row + rows; r++) {
        for (int c = start_col; c < start_col + cols; c++) {
            int offset = (r * MAX_COLS + c) * 2;
            video_memory[offset] = ' ';
            video_memory[offset+1] = frame_color;
        }
    }
    for (int c = start_col; c < start_col + cols; c++) {
        int offset = (start_row * MAX_COLS + c) * 2;
        video_memory[offset] = ' ';
        video_memory[offset+1] = title_color;
    }
    int t_len = strlen(title);
    for (int i = 0; i < t_len && i < (cols - 4); i++) {
        int offset = (start_row * MAX_COLS + start_col + 2 + i) * 2;
        video_memory[offset] = title[i];
        video_memory[offset+1] = title_color;
    }
    video_memory[(start_row * MAX_COLS + start_col + 1) * 2] = '-';

    int b_len = strlen(body);
    int curr_r = start_row + 2;
    int curr_c = start_col + 2;
    for (int i = 0; i < b_len; i++) {
        if (body[i] == '\n') {
            curr_r++;
            curr_c = start_col + 2;
            continue;
        }
        if (curr_c < start_col + cols - 2 && curr_r < start_row + rows - 1) {
            int offset = (curr_r * MAX_COLS + curr_c) * 2;
            video_memory[offset] = body[i];
            video_memory[offset+1] = frame_color;
            curr_c++;
        }
    }
}

/* Отрисовка списка файлов внутри окна Проводника */
void draw_explorer_content(int start_row, int start_col, int max_rows, int cols) {
    char* video_memory = (char*)VIDEO_ADDRESS;
    int curr_row = start_row;

    if (file_count == 0) {
        const char* empty_msg = "[No files found in VsCoreFS]";
        for (int i = 0; empty_msg[i] != '\0' && i < cols - 4; i++) {
            int offset = (curr_row * MAX_COLS + start_col + 2 + i) * 2;
            video_memory[offset] = empty_msg[i];
            video_memory[offset+1] = WIN_FRAME;
        }
        return;
    }
    int rendered = 0;
    for (int i = 0; i < MAX_FILES && rendered < max_rows - 2; i++) {
        if (filesystem[i].is_used) {
            int offset_line = (curr_row * MAX_COLS + start_col + 2) * 2;
            video_memory[offset_line] = '*';
            video_memory[offset_line + 2] = ' ';
            int n_idx = 0;
            while (filesystem[i].name[n_idx] != '\0' && n_idx < 16) {
                video_memory[offset_line + 4 + n_idx * 2] = filesystem[i].name[n_idx];
                video_memory[offset_line + 4 + n_idx * 2 + 1] = WIN_FRAME;
                n_idx++;
            }
            curr_row++;
            rendered++;
        }
    }
}

/* Оконная среда пользователя VsDE с клавиатурным фокусом */
void run_vsde() {
    char* video_memory = (char*)VIDEO_ADDRESS;
    int active_window = 1;

    while (1) {
        for (int i = 0; i < MAX_ROWS * MAX_COLS * 2; i += 2) {
            video_memory[i] = ' ';
            video_memory[i+1] = BLUE_BACKGROUND;
        }
        for (int c = 0; c < MAX_COLS; c++) {
            int offset = ((MAX_ROWS - 1) * MAX_COLS + c) * 2;
            video_memory[offset] = ' ';
            video_memory[offset+1] = WIN_FRAME;
        }
        const char* status_text = " VsDE Desktop - [Press 1 for Program Manager | 2 for File Explorer | Q to Exit]";
        for (int i = 0; status_text[i] != '\0'; i++) {
            video_memory[((MAX_ROWS - 1) * MAX_COLS + i) * 2] = status_text[i];
        }

        draw_window(2, 4, 15, 34, "Program Manager", "Main Applications:\n- vsano (Text Editor)\n- RunC (C Interpreter)\n- VsCoreFS Console\n\nSystem Core:\n- Clean Independent Kernel\n- Keyboard: Focus Lock", (active_window == 1));
        draw_window(4, 42, 14, 34, "File Explorer", "Files inside memory:\n\n", (active_window == 2));
        draw_explorer_content(8, 42, 10, 34);

        char c = getchar();
        if (c == '1') active_window = 1;
        if (c == '2') active_window = 2;
        if (c == 'q' || c == 'Q') {
            run_clear();
            break;
        }
    }
}

/* ==========================================================================
 *   VsCore Kernel - Part 4.2: RunC, Installer, Shell & Main Entry
 *   Developed by: Shumatbaev Vladislav & VladisSystems Team (c)2026
 *   License: MIT License
 *   ========================================================================== */

/* Разделение строки ввода на команду и аргумент */
void get_argument(const char* input, char* arg) {
    int i = 0;
    while (input[i] != ' ' && input[i] != '\0') i++;
    if (input[i] == ' ') {
        i++;
        int j = 0;
        while (input[i] != '\0') {
            arg[j++] = input[i++];
        }
        arg[j] = '\0';
    } else {
        arg = '\0';
    }
}

/* Инструмент для выполнения C-кода прямо в памяти ядра */
void run_runc(const char* filename) {
    if (strlen(filename) == 0) {
        print("Usage: RunC [filename]\n");
        return;
    }
    const char* code = read_file(filename);
    if (code == 0) {
        print("RunC Error: File not found.\n");
        return;
    }
    print("[RunC: Executing...]\n");
    int i = 0;
    while (code[i] != '\0') {
        if (code[i] == 'p' && code[i+1] == 'r' && code[i+2] == 'i' &&
            code[i+3] == 'n' && code[i+4] == 't' && code[i+5] == 'f') {
            i += 6;
        while (code[i] != '\0' && code[i] != '"') i++;
        if (code[i] == '"') {
            i++;
            while (code[i] != '\0' && code[i] != '"') {
                if (code[i] == '\\' && code[i+1] == 'n') {
                    putchar('\n');
                    i += 2;
                } else {
                    putchar(code[i]);
                    i++;
                }
            }
        }
            }
            i++;
    }
    print("\n[RunC: Finished.]\n");
}

/* Стабильный установщик ОС с автоматическим рестартом эмулятора и чистым текстом */
void run_installer() {
    print("VsCore Installer Setup v1.5\n");
    print("Scanning IDE/ATA Primary Master drive...\n");

    static uint16_t test_buffer[256];
    if (!ata_read_sector(0, test_buffer)) {
        print("Installation Failed: Virtual Drive not responding or busy.\n");
        return;
    }

    print("Drive status: READY. Allocating VsCoreFS partition structure...\n");
    print("[                    ] 0%");

    for (volatile int delay = 0; delay < 1200000; delay++);

    static uint16_t fs_signature[256];
    fs_signature[0] = 0x5653; // "VS"
    fs_signature[1] = 0x4445; // "DE"

    if (ata_write_sector(1, fs_signature)) {
        cursor_col = 0;
        print("[==========          ] 50%");
        for (volatile int delay = 0; delay < 1200000; delay++);

        cursor_col = 0;
        print("[====================] 100%\n");
        print("VsCoreFS partition created successfully on target image drive!\n");
        print("Installation complete. Restarting system...\n");

        for (volatile int delay = 0; delay < 2000000; delay++);

        // Аппаратный авторестарт x86 процессора через PS/2 контроллер клавиатуры (Pulse Reset Line)
        outb(0x64, 0xFE);
    } else {
        print("\nError writing partition signature. Hardware fault.\n");
    }
}

/* Командный шелл операционной системы */
void run_shell() {
    char input_buffer[256];
    char arg_buffer[256];

    while (1) {
        print("VsCore@root:/> ");
        input_string(input_buffer, 256);

        if (strlen(input_buffer) == 0) continue;
        get_argument(input_buffer, arg_buffer);

        if (strcmp(input_buffer, "help") == 0) {
            print("Available commands:\n");
            print("  help    - Show this help message\n");
            print("  install - Install OS structures to target drive\n");
            print("  VsDE    - Start graphical desktop environment\n");
            print("  neofetch- View system logo\n");
            print("  clear   - Clear screen with permanent banner\n");
            print("  ls      - List files in VsCoreFS\n");
            print("  vsano   - Edit/Create file (e.g. vsano main.c)\n");
            print("  cat     - View file contents\n");
            print("  rm      - Delete file\n");
            print("  RunC    - Compile and execute C code\n");
            print("  about   - Credits and team info\n");
            print("  shutdown- Stop system execution\n");
        }
        else if (strcmp(input_buffer, "VsDE") == 0) {
            run_vsde();
        }
        else if (strcmp(input_buffer, "install") == 0) {
            run_installer();
        }
        else if (strcmp(input_buffer, "neofetch") == 0) {
            run_neofetch();
        }
        else if (strcmp(input_buffer, "clear") == 0) {
            run_clear();
        }
        else if (strcmp(input_buffer, "ls") == 0) {
            list_files();
        }
        else if (strncmp(input_buffer, "vsano", 5) == 0) {
            run_vsano(arg_buffer);
        }
        else if (strncmp(input_buffer, "cat", 3) == 0) {
            if (strlen(arg_buffer) == 0) {
                print("Usage: cat [filename]\n");
            } else {
                const char* content = read_file(arg_buffer);
                if (content != 0) {
                    print(content);
                    print("\n");
                } else {
                    print("File not found.\n");
                }
            }
        }
        else if (strncmp(input_buffer, "rm", 2) == 0) {
            if (strlen(arg_buffer) == 0) {
                print("Usage: rm [filename]\n");
            } else {
                if (delete_file(arg_buffer)) {
                    print("File deleted.\n");
                } else {
                    print("File not found.\n");
                }
            }
        }
        else if (strncmp(input_buffer, "RunC", 4) == 0) {
            run_runc(arg_buffer);
        }
        else if (strcmp(input_buffer, "about") == 0) {
            print("VsCore Operating System v1.2\n");
            print("Designed and built entirely from scratch.\n");
            print("Author: Shumatbaev Vladislav\n");
            print("Organization: VladisSystems Team (c)2026\n");
        }
        else if (strcmp(input_buffer, "shutdown") == 0) {
            print("System shutting down. Halted.\n");
            break;
        }
        else {
            print("VsCore Shell: Command '");
            print(input_buffer);
            print("' not found. Type 'help'.\n");
        }
    }
}

void main() {
    init_fs();
    clear_screen();
    run_neofetch();
    run_shell();

    while(1) {
        __asm__ volatile("hlt");
    }
}
