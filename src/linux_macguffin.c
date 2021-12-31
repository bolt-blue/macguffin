// TODO: CLI mode (can add GUI later)
// - Platform-dependent setup
// - Recursively find all video files
//   - valid types: mkv, mp4, m4v, mpg, mpeg, avi, VIDEO_TS(?)
// - Store file paths relative to input path(s)
// - Prevent tracking the same directories more than once
// - Read metadata
//   - 
// - Write any changes to metadata
//   - [create new fields as necessary]

#include "macguffin.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <termios.h>

#include <sys/types.h>
#include <dirent.h>

#include "dynarr.h"
#include "stack.h"
#include "util.h"
#include "video_detect.h"

#define MENU_HEAD_W 21
// TODO: Store these in a config file
#define DATADIR "data/"
#define DATAFILE DATADIR "store"

// TODO:
// - Print error message function
//   - clear screen
//   - print message
//   - await keypress
//   - show main menu
// - Separate out CLI functions
// - Separate out non-platform-specific code
internal char *get_input(char *prompt);
i32 get_int(u32 lo, u32 hi, char *prompt);
internal char *get_line(FILE *stream);
void print_menu(char *title, char *message, char *options[], int opt_len, int width);
internal char await_user(char *prompt);
int main_menu(void);
int process_dir(struct AppState *state, char *path);
char *push_dir_path(struct Stack *stack, char *parent, char *path);
void browse(struct AppState *state);
void add_tags(struct AppState *state, struct Video *v);
void remove_tags(struct AppState *state, struct Video *v);
void create_tags(struct AppState *state, struct Video *v);
internal void print_tags(struct TagHash *tag_table, struct Video *v);

int main(int argc, char **argv)
{
    // TODO:
    // - Parse args ?
    //   ~ gui mode
    // - Call into core code as a service

    // Setup memory
    // TODO: Have only one syscall allocation for the whole process
    // - Handle sub-allocations ourselves
    struct AppState state;
    state.scratch = stack_init(MB(1));
    state.strings = stack_init(MB(1));
    state.tracked_dirs = dynarr_init(sizeof(struct RootDir), KB(4));
    state.tags = taghash_init(KB(4));

    load_state(&state);

    while (1) {
        clear_screen();
        int choice = main_menu();
        switch (choice) {
            case SEARCH:
            {
            } break;

            case BROWSE:
            {
                browse(&state);
            } break;

            case DIRADD:
            {
                if (add_directory(&state) == 0)
                    save_state(&state);
            } break;

            case QUIT:
            {
                goto EXIT;
            }
        }
    }

EXIT:
    // Clean up
    taghash_free(&state.tags);
    dynarr_free(&state.tracked_dirs);
    stack_free(&state.strings);
    stack_free(&state.scratch);
    clear_screen();

    return 0;
}

char *get_input(char *prompt)
{
    if (!prompt) prompt = "> ";
    printf("%s", prompt);
    return get_line(stdin);
}

/*
 * NOTE: Currently only really suitable for positive integer ranges
 * Return:
 * Success: Positive int in range lo to hi, inclusive
 * Abort:   -1 (no input)
 */
i32 get_int(u32 lo, u32 hi, char *prompt)
{
    char *input;
    u32 choice;
    while (1) {
PROMPT:
        input = get_input(prompt);
        if (!input || !*input)
            return -1;

        char *cur = input;
        while (*cur) {
            if (!isdigit(*cur)) {
                printf("Invalid input. Please try again\n");
                goto PROMPT;
            }
            cur++;
        }

        choice = atoi(input);
        if (choice >= lo && choice <= hi)
            break;
        else
            printf("Invalid option. Please try again\n");
    }
    return choice;
}

/*
 * Read a whole line from stream
 *
 * Only stops at a newline char. The newline char is replaced with a nul byte.
 * If line length exceeds the buffer size, the buffer is dynamically
 * (re-)allocated; this will repeat until the buffer is large enough.
 *
 * Warning: The memory for the returned pointer will be overwritten by
 * subsequent calls. Safe copying is left to the caller.
 *
 * Returns:
 *     *: Success
 *  NULL: Failure. Something went wrong
 *
 * TODO:
 * - Be more clear about errors - use errno
 * - Have some sensible max buffer length, in case no newline is present in
 *   e.g. a stream of many gigabytes
 */
#define DEFAULT_BUFSZ 256
#define SENTINEL 0x2  // ASCII STX - Start of Text character
char *get_line(FILE *stream)
{
    // TODO:
    // - Start immediately with a dynamically alloc'd buffer ?
    static char default_buffer[DEFAULT_BUFSZ];
    static char *buf = default_buffer;
    static int len = DEFAULT_BUFSZ;
    static int pos = 0;
    static char using_default = 1;

    while (1) {
        // Enable differentiation between error and EOF
        buf[pos] = SENTINEL;
        if (!fgets(buf + pos, len - pos, stream) && buf[pos] == SENTINEL) {
            // fgets failed
            return NULL;
        }

        for (; pos < len; pos++) {
            if (buf[pos] == '\n') {
                buf[pos] = '\0';
                goto CLEANUP;
            }
        }

        len *= 2;
        if (!using_default) {
            buf = realloc(buf, len);
        } else {
            char *newbuf = malloc(len);
            memcpy(newbuf, buf, len / 2);
            buf = newbuf;
            using_default = 0;
        }
        if (!buf)
            return NULL;
        // Step back one, to account for the nul byte introduced by fgets
        pos -= 1;
    }

CLEANUP:
    pos = 0;
    return buf;
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

char await_user(char *prompt)
{
    struct termios ttystate;
    tcgetattr(STDIN_FILENO, &ttystate);

    // NOTE: Canonical mode
    // - When enabled (default), reads will wait for 'Enter' keypress
    // - When disabled, reads will process immediately

    // Enable immediate read from stdin and disable keypress echo
    ttystate.c_lflag &= ~ICANON & ~ECHO;
    // Read after single byte received
    ttystate.c_cc[VMIN] = 1;
    // Update terminal attributes
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);

    if (prompt) printf("%s", prompt);
    char key = fgetc(stdin);

    // Reset to default behaviour
    ttystate.c_lflag |= ICANON | ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
    return key;
}

int main_menu(void)
{
    char *options[] = {"Search", "Browse", "Add directory", "Quit"};
    int num_options = sizeof(options) / sizeof(char *);
    print_menu("MAIN MENU", NULL, options, num_options, MENU_HEAD_W);

    enum CHOICE choice;
    choice = get_int(1, 4, NULL);

    return choice;
}

/*
 * Return:
 *   0: Success
 *  -1: Error reading input
 *  -2: Cancelled
 *  -3: Failure during processing
 * TODO: We currently never have a failure state during processing
 * - Do we need one?
 *   - Determine and fix as necessary
 */
int add_directory(struct AppState *state)
{
    clear_screen();

    // TODO: Tab completion (not so trivial... ?)
    print_menu("Add Directory", "Enter full path, or an empty line to cancel", NULL, 0, MENU_HEAD_W);

    char *path_buffer;
    while (1) {
        printf("> ");
        path_buffer = get_line(stdin);

        // NOTE: Passing an empty string to opendir() (via process_dir()) gets
        // intepreted (wrongly, imo) as `/`. So we make sure that the buffer
        // contains some text
        if (!path_buffer)
            return -1;
        else if (!*path_buffer)
            return -2;

        // TODO: Only process directories that are not already being tracked
        u32 res = process_dir(state, path_buffer);
        if (res == 0) {
            break;
        } else if (res == -1) {
            // TODO: @logging
            perror("Failed");
            await_user("Press any key to try again\n");
        }
    }
    return 0;
}

/*
 * Return:
 *  0: Success
 * -1: Failed to open directory
 */
int process_dir(struct AppState *state, char *path)
{
    // TODO:
    // - Store all video files (of supported filetype(s))
    //   - only .mp4 and .mkv initially

    if (!opendir(path)) {
        return -1;
    }

    // NOTE: Stack is limited in size
    // TODO: When full, batch process and clear before re-populating?
    struct Stack potentials = stack_init(MB(1));
    struct Stack dirs = stack_init(MB(1));
    push_dir_path(&dirs, NULL, path);

    while (dirs.size) {
        char *popped_path = (char *)stack_pop(&dirs);
        u32 current_dir_path_len = strlen(popped_path);
        // NOTE: This is only going to be handling data that's coming out of
        // `dirent` and so should be safe from things like stack smashing
        // TODO: Confirm this
        char current_dir_path[current_dir_path_len + 1];
        // NOTE: Have to store popped path separately, so further pushes
        // to the directory stack do not clobber it
        strncpy(current_dir_path, popped_path, current_dir_path_len + 1);

        printf("=== Processing directory: %s\n", current_dir_path);

        DIR *current_dir = opendir(current_dir_path);
        if (!current_dir) {
            // TODO: @logging
            perror("Failed to open directory path. Continuing.");
            fprintf(stderr, "%s\n", current_dir_path);
            continue;
        }

        struct dirent *current_file;

        while ((current_file = readdir(current_dir))) {
            // Ignore current dir, parent dir and all hidden files
            if (*current_file->d_name == '.')
                continue;

            if (current_file->d_type == DT_DIR) {
                push_dir_path(&dirs, current_dir_path, current_file->d_name);

            } else if (current_file->d_type == DT_REG) {
                // Store for later bulk checking
                // NOTE: All paths stored in full for simplicity.
                // We don't really care about directory structure, just files.
                // This may change at a later stage.
                size_t filename_len = strlen(current_file->d_name);
                char *filepath = stack_push(&potentials, current_dir_path_len + filename_len + 1);

                strncpy(filepath, current_dir_path, current_dir_path_len + 1);
                strncat(filepath, current_file->d_name, filename_len);
            }
        }

        closedir(current_dir);
    }

    // TODO: Properly store all tracked directories
    // - Retain `path`
    // - Store all sub-paths relative to path ?
    struct RootDir new_root;

    // Retain path distinctly in our strings array
    u32 path_len = strlen(path) + 1;
    new_root.path = stack_push(&state->strings, path_len);
    strncpy(new_root.path, path, path_len);

    // TODO: First determine number of video files, and allocate only as much
    // memory as necessary
    new_root.videos = dynarr_init(sizeof(struct Video), KB(16));

    struct Stack *strings = &state->strings;
    char *potential;

    while ((potential = stack_pop(&potentials))) {
        printf("=== Processing file: %s\n", potential);

        u32 filepath_len = strlen(potential) + 1;

        // TODO:
        // - Multi-thread this
        // - Attempt to extract meta data; at least for:
        //   - title
        //   - year
        //   - duration
        if (is_mp4(potential)) {
            struct Video video = {0};
            video.filepath = stack_push(strings, filepath_len);
            strncpy(video.filepath, potential, filepath_len);
            dynarr_add(&new_root.videos, &video);
        }
    }

    // Update state with newly tracked directory
    dynarr_add(&state->tracked_dirs, &new_root);

    // Clean up temporary data
    stack_free(&potentials);
    stack_free(&dirs);

#ifndef NDEBUG
    // @debug
    if (state->tracked_dirs.size) {
        u32 num_tracked = 0;
        DEBUG_PRINT("Currently tracked files:\n");
        for (int i = 0; i < state->tracked_dirs.size; i++) {
            struct RootDir cur_dir = *(struct RootDir *)dynarr_at(&state->tracked_dirs, i);
            DEBUG_PRINTF("\tRoot Directory: %s\n", cur_dir.path);
            num_tracked += cur_dir.videos.size;
            for (int j = 0; j < cur_dir.videos.size; j++) {
                struct Video *video = (struct Video *)dynarr_at(&cur_dir.videos, j);
                DEBUG_PRINTF("\t\t%s\n", video->filepath);
            }
        }
        DEBUG_PRINT("End of files\n");
        DEBUG_PRINTF("Tracking %d files\n", num_tracked);
    }
#endif

    await_user("Press any key to continue...\n");

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

void browse(struct AppState *state)
{
    struct DynArr tracked = state->tracked_dirs;
    if (!tracked.size)
        return;

    u32 r_id = 0;
    u32 v_id = 0;
    u8 running = 1;

    struct DynArr videos = ((struct RootDir *)dynarr_at(&tracked, r_id))->videos;

    while(running) {
        clear_screen();

        struct Video *v = dynarr_at(&videos, v_id);

        printf("Title: %s\n", v->title);
        printf("Year: %d\n", v->year);
        printf("Duration: %d\n", v->duration);
        printf("Filepath: %s\n", v->filepath);

        printf("Tags: ");
        print_tags(&state->tags, v);
        printf("\n");

        printf("<-- | -->\n");
        printf("a/h | d/l\n\n");
        printf( "t: Add tag(s) | r: Remove tag(s) | c: Create new tag(s) | "
               "q: Main menu\n");

        unsigned char key = await_user(NULL);
        switch (key) {
        case 'a': // fall through
        case 'h': {
            if (v_id > 0) {
                v_id--;
            } else {
                if (r_id > 0)
                    r_id--;
                else
                    r_id = tracked.size - 1;
                videos = ((struct RootDir *)dynarr_at(&tracked, r_id))->videos;
                v_id = videos.size - 1;
            }
        } break;

        case 'd': // fall through
        case 'l': {
            if (v_id < videos.size - 1) {
                v_id++;
            } else {
                if (r_id < tracked.size - 1)
                    r_id++;
                else
                    r_id = 0;
                videos = ((struct RootDir *)dynarr_at(&tracked, r_id))->videos;
                v_id = 0;
            }
        } break;

        case 't': {
            add_tags(state, v);
        } break;

        case 'r': {
            remove_tags(state, v);
        } break;

        case 'c': {
            create_tags(state, v);
        } break;

        case 'q': {
            running = 0;
        } break;
        }
    }
}

void add_tag(tagid_t tid, struct Video *v)
{
    if (!v->tags.base) {
        // First tag for this video
        v->tags = dynarr_init(sizeof(tagid_t), DEFAULT_TAG_AMT);
    }
    dynarr_add(&v->tags, &tid);
}

void add_tags(struct AppState *state, struct Video *v)
{
    // TODO:
    // - Use the menu choice to get correct tag id from the array
    // - Add tag id to this video
    u32 tag_count = state->tags.count;
    tagid_t *tag_ids = stack_push(&state->scratch, sizeof(tagid_t) * tag_count);
    char **tag_texts = stack_push(&state->scratch, sizeof(char *) * tag_count);
    {
        // Copy all available tags into a plain array
        u32 i = 0;
        tagnode_t *node;
        TH_FOR_EACH(&state->tags, node) {
            tag_ids[i] = node->tag.id;
            tag_texts[i] = node->tag.text;
            i++;
        }

        // Create a volatile copy of the video tags
        u32 vtag_count = v->tags.size;
        tagid_t vtag_ids[vtag_count];
        for (u32 i = 0; i < vtag_count; i++) {
            vtag_ids[i] = *(tagid_t*)dynarr_at(&v->tags, i);
        }
        // Remove any tags that this video already has
        for (u32 i = 0; i < tag_count; i++) {
            for (u32 j = 0; j < vtag_count; j++) {
                if (vtag_ids[j] == tag_ids[i]) {
                    // Swap out and decrement
                    if (i < tag_count - 1) {
                        tag_ids[i] = tag_ids[tag_count-1];
                        tag_texts[i] = tag_texts[tag_count-1];
                    }
                    tag_count--;
                    // Don't need to use this vtag in future checks
                    if (j < vtag_count - 1)
                        vtag_ids[j] = vtag_ids[vtag_count-1];
                    vtag_count--;
                    // Step back to check swapped value next
                    i--;
                    j--;
                    break;
                }
            }
        }
    }

    printf("\n");
    if (tag_count) {
        print_menu("AVAILABLE TAGS", NULL, tag_texts, tag_count, MENU_HEAD_W);
        u32 choice = get_int(1, tag_count, NULL);
        if (choice != -1)
            add_tag(tag_ids[choice-1], v);
    } else {
        printf("No tags available\n");
        await_user("Press any key to continue...\n");
    }
}

void remove_tags(struct AppState *state, struct Video *v)
{
    // TODO:
    // - List tags currently associated with this video, /numbered/
    // - User selects number; remove tag id from v
    // - Loop until "break signal" from user
}

void create_tags(struct AppState *state, struct Video *v)
{
    // TODO: Clean this mess up!
    printf("\n");
    printf("Create a new tag and add it to the current video.\n");
    printf("Input nothing to return\n");
    char *input;
    while ((input = get_input("Tag> ")) && *input) {
        struct Tag tmp;
        if (!taghash_find_by_text(&state->tags, input, &tmp)) {
            // Make sure we have a permanent copy of the tag
            u32 input_len = strlen(input) + 1;
            char *tag_text = (char *)stack_push(&state->strings, input_len);
            strncpy(tag_text, input, input_len);
            tmp.id = taghash_insert(&state->tags, tag_text);
            fprintf(stdout, "Created tag '%s'\n", tag_text);
        } else{
            fprintf(stdout, "[INFO] Tag already exists.\n");
        }
        add_tag(tmp.id, v);
    }
}

void print_tags(struct TagHash *tag_table, struct Video *v)
{
    for (u32 i = 0; i < v->tags.size; i++) {
        struct Tag t;
        char *tag_text;
        tagid_t tid = *(tagid_t *)dynarr_at(&v->tags, i);
        if (taghash_find_by_id(tag_table, tid, &t))
            tag_text = t.text;
        else
            // This should never happen!
            tag_text = "MISSING";
        printf("[%s] ", tag_text);
    }
    printf("\n");
}

/*
 * Returns:
 * 0: Success
 * 1: Failed to open file for reading
 */
int load_state(struct AppState *state)
{
    FILE *datafile = fopen(DATAFILE, "r");
    if (!datafile)
        return 1;

    // TODO:
    // - For count num tracked directories
    //   - Create new RootDir struct
    //   - Push directory path string
    //   - Store pointer
    //   - Allocate dynarr memory for videos
    //   - For count num_videos
    //     - Create new Video struct
    //     - Push filepath string
    //     - Store pointer
    //     - If exists, push title string
    //     - Store pointer
    //     - If exists, store year
    //     - If exists, store duration

    int num_tracked;
    fscanf(datafile, "%d\n", &num_tracked);

    char *read_buf;
    int read_buf_len;

    for (int i = 0; i < num_tracked; i++) {
        struct RootDir cur_root = {};
        int num_videos;

        // Read root directory path
        read_buf = get_line(datafile);
        // Include the nul byte
        read_buf_len = strlen(read_buf) + 1;

        // Read video count for this directory
        fscanf(datafile, "%d\n", &num_videos);

        // Store directory path
        // TODO: We're doing this enough times; separate out to own function
        cur_root.path = stack_push(&state->strings, read_buf_len);
        strncpy(cur_root.path, read_buf, read_buf_len);

        // Prepare for video data storage
        // We can allocate only the memory needed for now
        cur_root.videos = dynarr_init(sizeof(struct Video), num_videos);

        for (int i = 0; i < num_videos; i++) {
            struct Video cur_vid = {};

            // Read Filepath
            read_buf = get_line(datafile);
            read_buf_len = strlen(read_buf) + 1;

            cur_vid.filepath = stack_push(&state->strings, read_buf_len);
            strncpy(cur_vid.filepath, read_buf, read_buf_len);

            // Read Title
            read_buf = get_line(datafile);
            read_buf_len = strlen(read_buf) + 1;
            if (strcmp(read_buf, "(null)") != 0) {
                cur_vid.title = stack_push(&state->strings, read_buf_len);
                strncpy(cur_vid.title, read_buf, read_buf_len);
            }

            // Read Year
            fscanf(datafile, "%hd\n", &cur_vid.year);

            // Read Duration
            fscanf(datafile, "%hd\n", &cur_vid.duration);

            dynarr_add(&cur_root.videos, &cur_vid);
        }
        dynarr_add(&state->tracked_dirs, &cur_root);
    }

    fclose(datafile);

    return 0;
}

/*
 * Returns:
 * 0: Success
 * 1: Failed to open file for writing
 */
int save_state(struct AppState *state)
{
    // TODO:
    // - First store in a temporary file
    // - On successful write:
    //   - Rename new file to overwrite old
    FILE *datafile = fopen(DATAFILE, "w");
    if (!datafile)
        return 1;

    struct DynArr tracked = state->tracked_dirs;
    struct RootDir *cur_root;

    fprintf(datafile, "%d\n", tracked.size);
    DYN_FOR_EACH(&tracked, cur_root) {
        struct DynArr videos = cur_root->videos;
        struct Video *video;

        fprintf(datafile, "%s\n", cur_root->path);
        fprintf(datafile, "%d\n", videos.size);

        DYN_FOR_EACH(&videos, video) {
            fprintf(datafile, "%s\n", video->filepath);
            fprintf(datafile, "%s\n", video->title);
            fprintf(datafile, "%d\n", video->year);
            fprintf(datafile, "%d\n", video->duration);
        }
    }

    fclose(datafile);

    return 0;
}
