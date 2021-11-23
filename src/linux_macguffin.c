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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <dirent.h>

#include "stack.h"
#include "util.h"
#include "video_detect.h"

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
int process_dir(char *path);
char *push_dir_path(struct Stack *stack, char *parent, char *path);

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
int process_dir(char *path)
{
    // TODO:
    // - Recursively search directory
    // - Store all video files (of supported filetype(s))
    //   - only .mp4 and .mkv initially

    // NOTE: Stack is limited in size
    // TODO: When full, batch process and clear before re-populating?
    struct Stack dirs = stack_init(MB(1));
    {
        push_dir_path(&dirs, NULL, path);
    }

    while (dirs.size) {
        char *popped_path = (char *)stack_pop(&dirs);
        u32 current_dir_path_len = strlen(popped_path);
        char current_dir_path[current_dir_path_len + 1];
        // NOTE: Have to store popped path separately, so further pushes
        // to the directory stack do not clobber it
        strncpy(current_dir_path, popped_path, current_dir_path_len + 1);

        printf("=== Processing: %s ...\n\n", current_dir_path);

        DIR *current_dir = opendir(current_dir_path);
        if (!current_dir) {
            // TODO: @logging
            perror("Failed to open directory path");
            fprintf(stderr, "%s\n", current_dir_path);
            continue;
        }

        struct dirent *current_file;

        while ((current_file = readdir(current_dir))) {
            // Ignore current dir, parent dir and all hidden files
            if (*current_file->d_name == '.')
                continue;

            if (current_file->d_type == DT_DIR) {
                // Push full path
                char *tmp = push_dir_path(&dirs, current_dir_path, current_file->d_name);
                printf("[DEBUG] Found directory - pushing to stack: %s\n\n", tmp);

            } else if (current_file->d_type == DT_REG) {
                // TODO:
                // - Check to see if matches our valid video type(s)
                size_t filename_len = strlen(current_file->d_name);

                char filepath[current_dir_path_len + filename_len + 1];
                strncpy(filepath, current_dir_path, current_dir_path_len + 1);
                strncat(filepath, current_file->d_name, filename_len);

                printf("[DEBUG] Found regular file - checking if mp4: %s\n", filepath);

                // TODO: Push to a file stack, for bulk processing after
                if (!is_mp4(filepath)) {
                    printf("[DEBUG] File is not an mp4\n\n");
                    continue;
                }
                printf("[DEBUG] FOUND mp4\n\n");

                // TODO: Store file details
                // - Should stored path be relative to `root` or full?
            }
        }

        closedir(current_dir);
    }

    stack_free(&dirs);
    return 0;
}

/*
 * Push a directory path onto a stack
 * Guarantee trailing /
 */
char *push_dir_path(struct Stack *stack, char *parent, char *path)
{
    size_t parent_len = 0;
    size_t path_len = strlen(path);
    u8 add_trailing_slash = 0;

    if (parent)
        parent_len = strlen(parent);

    if (path[path_len - 1] != '/') {
        add_trailing_slash = 1;
    }

    // TODO: How to handle situation if stack is full?
    char *pushed = stack_push(stack, parent_len + path_len + add_trailing_slash + 1);
    if (parent)
        memcpy(pushed, parent, parent_len);
    memcpy(pushed + parent_len, path, path_len + 1);

    if (add_trailing_slash) {
        pushed[parent_len + path_len] = '/';
        pushed[parent_len + path_len + 1] = '\0';
    }

    return pushed;
}
