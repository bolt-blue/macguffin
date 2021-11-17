// TODO: CLI mode (can add GUI later)
// - Platform-dependent setup
// - Take one or more input directories
// - Recursively find all video files
//   - valid types: mkv, mp4, m4v, mpg, mpeg, avi, VIDEO_TS(?)
// - Store file paths relative to input path(s)
// - Read metadata
//   - 
// - Write any changes to metadata
//   - [create new fields as necessary]

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <dirent.h>

typedef char byte;
typedef uint8_t u8;
typedef uint32_t u32;

#define reset_colour() fputs("\033[0m", stdout);
#define reset() fputs("\033[2J", stdout)
#define clear_screen() fputs("\033[H\033[J", stdout)
#define hidecur() fputs("\033[?25l", stdout)
#define showcur() fputs("\033[?25h", stdout)
#define gotoxy(x,y) fprintf(stdout, "\033[%d;%dH", (y), (x))

#define be_to_le_u32(be)  \
    ((be << 24) & 0xff000000) |    \
    ((be << 8)  & 0xff0000)   |    \
    ((be >> 8)  & 0xff00)     |    \
    be >> 24;

#define MENU_HEAD_W 21

enum CHOICE {
    SEARCH = 0x1,
    BROWSE,
    DIRADD,
    QUIT
};

// TODO:
// - Print error message function
//   - clear screen
//   - print message
//   - await keypress
//   - show main menu
int get_input(char *buf, size_t len);
void print_menu(char *title, char *message, char *options[], int opt_len, int width);
int main_menu(void);
int add_directory(void);
int process_dir(char *dir_path);
int is_mp4(char *filepath);

int main(int argc, char **argv)
{
    // TODO:
    // - Parse args ?
    //   ~ gui mode
    // - Call into core code as a service

    while (1) {
        clear_screen();
        int choice = main_menu();
        switch (choice) {
            case SEARCH:
                break;
            case BROWSE:
                break;
            case DIRADD:
                add_directory();
                break;
            case QUIT:
                goto EXIT;
        }
    }

EXIT:
    // TODO: Any necessary clean up
    clear_screen();

    return 0;
}

/*
 * Returns:
 * 1: newline found, clean input
 * 0: newline not found, text remains in buffer
 */
int get_input(char *buf, size_t len)
{
    u8 status = 0;

    fgets(buf, len, stdin);

    // If newline found in input, replace with nul and declare as clean
    for (int i = 0; i < len; i++) {
        if (buf[i] == '\n') {
            buf[i] = '\0';
            status = 1;
            break;
        }
    }

    // If unclean, clear whatever remains in stdin buffer
    if (status == 0) {
        u8 cleared = 0;
        do {
            char tmp[64];
            char *cur = tmp;
            fgets(tmp, 64, stdin);
            while (*cur) {
                if (*cur == '\n' || *cur == EOF) {
                    cleared = 1;
                    break;
                }
                cur++;
            }
        } while (!cleared);
    }

    return status;
}

/*
 * Parameters:
 *   title:   menu header [required]
 *   message: subtitle/message prompt [optional]
 *   options: list of string choices [optional]
 *   opt_len: length of options array
 *   width:   used to determine width of menu header [optional]
 * Warning: Expects title and message to be nul-terminated
 */
void print_menu(char *title, char *message, char *options[], int opt_len, int width)
{
    static const char *divider = "========================================";
    if (!width)
        width = 80;
    int title_len = strlen(title);
    int div_len = width - title_len - 2;

    printf("%.*s %s %.*s\n", div_len, divider, title, div_len, divider);

    if (message)
        printf("%s\n", message);

    if (options) {
        for (int i = 0; i < opt_len; i++) {
            printf("%d. %s\n", i + 1, options[i]);
        }
    }
}

int main_menu(void)
{
    char *options[] = {"Search", "Browse", "Add directory", "Quit"};
    int num_options = sizeof(options) / sizeof(char *);
    print_menu("MAIN MENU", NULL, options, num_options, MENU_HEAD_W);

#define INPUTSZ 3
    // NOTE: Our input size must include space for newline and nul chars
    // even though any newline shall be converted to nul by get_input()
    // TODO: Need a more clean approach to the above - feels hacky
    char input[INPUTSZ];
    enum CHOICE choice;
    u8 invalid = 0;
    while (1) {
        printf("> ");

        get_input(input, INPUTSZ);

        char *cur = input;
        while (*cur) {
            if (!isdigit(*cur++)) {
                invalid = 1;
                break;
            }
        }

        if (invalid) {
            printf("Invalid input. Please try again\n");
            invalid = 0;
            continue;
        }

        choice = atoi(input);
        if (choice > 0 && choice < 5)
            break;
        else
            printf("Please choose one of the available options.\n");
    };

    return choice;
}

/*
 * Return:
 *  -1: Buffer exceeded. Abort
 *  -2: Invalid directory path
 */
int add_directory(void)
{
    clear_screen();

    // TODO:
    // - Should we limit the path length?
    //   or should this be dynamic?
    // - Tab completion (not so trivial... ?)
    print_menu("Add Directory", "Enter full path", NULL, 0, MENU_HEAD_W);
    printf("> ");

    char path_buffer[256];
    u8 chk = get_input(path_buffer, sizeof(path_buffer));

    if (chk == 0) {
        printf("Input exceeded the buffer size. Aborting\n");
        return -1;
    }

    process_dir(path_buffer);

    return 0;
}

/*
 * Return:
 * -1: Failed to open directory
 */
int process_dir(char *dir_path)
{
    // TODO:
    // - Recursively search directory
    // - Store all video files (of supported filetype(s))
    //   - only .mp4 and .mkv initially
    printf("Processing...\n");

    DIR *root_dir = opendir(dir_path);
    if (!root_dir) {
        perror("Failed to open directory path");
        return -1;
    }

    struct dirent *current;
    char *filetype;

    while ((current = readdir(root_dir))) {
        // Ignore current dir, parent dir and all hidden files
        if (*current->d_name == '.')
            continue;
        if (current->d_type == DT_DIR) {
            // TODO: Add to stack for later recursion
            filetype = "Directory";
        } else if (current->d_type == DT_REG) {
            // TODO:
            // - Check to see if matches our valid video type(s)
            filetype = "Regular file";

            printf("%s: %s\n", current->d_name, filetype);

            // TODO: Handle any lack of trailing '/' only once,
            // not for every file!
            size_t filename_len = strlen(current->d_name);
            size_t dir_path_len = strlen(dir_path);

            u8 add_trailing_slash = 0;
            if (dir_path[dir_path_len - 1] != '/') {
                add_trailing_slash = 1;
            }

            char filepath[dir_path_len + add_trailing_slash + filename_len + 1];
            strncpy(filepath, dir_path, dir_path_len + 1);

            if (add_trailing_slash) {
                filepath[dir_path_len] = '/';
                filepath[dir_path_len + 1] = '\0';
            }

            strncat(filepath, current->d_name, filename_len);

            if (!is_mp4(filepath)) {
                printf("[DEBUG] File is not an mp4\n");
                continue;
            }

            printf("[DEBUG] File is an mp4\n");

            // TODO: Store file details
            // - Should stored path be relative to `root` or full?

            printf("\n");
        }
    }

    return 0;
}

struct MP4_Head {
    u32 offset;
    char ftyp[4];
    char major_brand[4];
    u32 major_brand_ver;
};

/*
 * Return:
 *   1: File is an mp4
 *   0: File is not an mp4
 *  -1: Failed to open file
 */
int is_mp4(char *filepath)
{
    FILE *f = fopen(filepath, "r");
    if (!f)
        return -1;

    // [see: ./_REF/mp4-layout.txt : https://xhelmboyx.tripod.com/formats/mp4-layout.txt]
    // TODO: Make sure we have enough to recognise an mp4
    struct MP4_Head header;
    fread(&header, sizeof(header), 1, f);

    header.offset = be_to_le_u32(header.offset);
    header.major_brand_ver = be_to_le_u32(header.major_brand_ver);

    if (strncmp(header.ftyp, "ftyp", 4) != 0) {
        return 0;
    }

    // TODO: "3gp" may have varied single ASCII 4th char. Confirm
    // - Confirm before using
    // - If using, how to match?
    char *brands[] = {"isom", "iso2", "mp41", "mp42", "qt  ", "avc1", "mmp4", "mp71"};

    u8 match = 0;
    for (int i = 0, len = sizeof(brands) / sizeof(*brands); i < len; i++) {
        if (strncmp(header.major_brand, brands[i], 4) == 0) {
            match = 1;
            break;
        }
    }

    //printf("[DEBUG] Offset: %d\n", header.offset);
    //printf("[DEBUG] ftyp: %.4s\n", header.ftyp);
    //printf("[DEBUG] Major Brand: %.4s\n", header.major_brand);
    //printf("[DEBUG] Major Brand Version: %d\n", header.major_brand_ver);

    return match;
}
