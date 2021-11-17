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

typedef char byte;
typedef uint8_t u8;
typedef uint32_t u32;

#define reset_colour() fputs("\033[0m", stdout);
#define reset() fputs("\033[2J", stdout)
#define clear_screen() fputs("\033[H\033[J", stdout)
#define hidecur() fputs("\033[?25l", stdout)
#define showcur() fputs("\033[?25h", stdout)
#define gotoxy(x,y) fprintf(stdout, "\033[%d;%dH", (y), (x))

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

int add_directory(void)
{
    return 0;
}
