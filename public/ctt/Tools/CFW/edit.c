
/*** Include section ***/

// We add them above our includes, because the header
// files we are including use the macros to decide what
// features to expose. These macros remove some compilation
// warnings. See
// https://www.gnu.org/software/libc/manual/html_node/Feature-Test-Macros.html
// for more info.
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

/*** Define section ***/
// Version code
#define EDIT_VERSION "0.0.5"
// Empty buffer
#define ABUF_INIT {NULL, 0}
// Length of a tab stop
#define EDIT_TAB_STOP 4
// Highlight flags
#define HL_HIGHLIGHT_NUMBERS (1 << 0)
#define HL_HIGHLIGHT_STRINGS (1 << 1)
//
// Times to press Ctrl-Q before exiting
#define TTE_QUIT_TIMES 2
// This mimics the Ctrl + whatever behavior, setting the
// 3 upper bits of the character pressed to 0.
#define CTRL_KEY(k) ((k) & 0x1f)

/*** Data section ***/

typedef struct editor_row {
    int idx; // Row own index within the file.
    int size; // Size of the content (excluding NULL term)
    int render_size; // Size of the rendered content
    char* chars; // Row content
    char* render; // Row content "rendered" for screen (for TABs).
    unsigned char* highlight; // This will tell you if a character is part of a string, comment, number...
    int hl_open_comment; // True if the line is part of a ML comment.
} editor_row;

typedef struct editor_syntax {
    // file_type field is the name of the filetype that will be displayed
    // to the user in the status bar.
    char* file_type;
    // file_match is an array of strings, where each string contains a
    // pattern to match a filename against. If the filename matches,
    // then the file will be recognized as having that filetype.
    char** file_match;
    // This will be a NULL-terminated array of strings, each string containing
    // a keyword. To differentiate between the two types of keywords,
    // we’ll terminate the second type of keywords with a pipe (|)
    // character (also known as a vertical bar).
    char** keywords;
    // We let each language specify its own single-line comment pattern.
    char* singleline_comment_start;
    // flags is a bit field that will contain flags for whether to
    // highlight numbers and whether to highlight strings for that
    // filetype.
    char* multiline_comment_start;
    char* multiline_comment_end;
    int flags;
} editor_syntax;

struct editor_config {
    int cursor_x;
    int cursor_y;
    int render_x;
    int row_offset; // Offset of row displayed.
    int col_offset; // Offset of col displayed.
    int screen_rows; // Number of rows that we can show
    int screen_cols; // Number of cols that we can show
    int num_rows; // Number of rows
    editor_row* row;
    int dirty; // To know if a file has been modified since opening.
    char* file_name;
    char status_msg[80];
    time_t status_msg_time;
    char* copied_char_buffer;
    editor_syntax* syntax;
    struct termios orig_termios;
} ec;

// Having a dynamic buffer will allow us to write only one
// time once the screen is refreshing, instead of doing
// a lot of write's.
struct a_buf {
    char* buf;
    int len;
};

enum editor_key {
    BACKSPACE = 0x7f, // 127
    ARROW_LEFT = 0x3e8, // 1000, large value out of the range of a char.
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    PAGE_UP,
    PAGE_DOWN,
    HOME_KEY,
    END_KEY,
    DEL_KEY
};

enum editor_highlight {
    HL_NORMAL = 0,
    HL_SL_COMMENT,
    HL_ML_COMMENT,
    HL_KEYWORD_1,
    HL_KEYWORD_2,
    HL_STRING,
    HL_NUMBER,
    HL_MATCH
};

/*** Filetypes ***/

char* C_HL_extensions[] = {".c", ".h", ".cpp", ".hpp", ".cc", NULL}; // Array must be terminated with NULL.
char* JAVA_HL_extensions[] = {".java", NULL};
char* PYTHON_HL_extensions[] = {".py", NULL};
char* BASH_HL_extensions[] = {".sh", NULL};
char* JS_HL_extensions[] = {".js", ".jsx", NULL};
char* PHP_HL_extensions[] = {".php", NULL};
char* JSON_HL_extensions[] = {".json", ".jsonp", NULL};
char* XML_HL_extensions[] = {".xml", NULL};
char* SQL_HL_extensions[] = {".sql", NULL};
char* RUBY_HL_extensions[] = {".rb", NULL};

char* C_HL_keywords[] = {
    "switch", "if", "while", "for", "break", "continue", "return", "else",
    "struct", "union", "typedef", "static", "enum", "class", "case", "#include",
    "volatile", "register", "sizeof", "typedef", "union", "goto", "const", "auto",
    "#define", "#if", "#endif", "#error", "#ifdef", "#ifndef", "#undef",
    
    "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
    "void|", "bool|", NULL
};

char* JAVA_HL_keywords[] = {
    "switch", "if", "while", "for", "break", "continue", "return", "else",
    "in", "public", "private", "protected", "static", "final", "abstract",
    "enum", "class", "case", "try", "catch", "do", "extends", "implements",
    "finally", "import", "instanceof", "interface", "new", "package", "super",
    "native", "strictfp",
    "synchronized", "this", "throw", "throws", "transient", "volatile",
    
    "byte|", "char|", "double|", "float|", "int|", "long|", "short|",
    "boolean|", NULL
};

char* PYTHON_HL_keywords[] = {
    "and", "as", "assert", "break", "class", "continue", "def", "del", "elif",
    "else", "except", "exec", "finally", "for", "from", "global", "if", "import",
    "in", "is", "lambda", "not", "or", "pass", "print", "raise", "return", "try",
    "while", "with", "yield",
    
    "buffer|", "bytearray|", "complex|", "False|", "float|", "frozenset|", "int|",
    "list|", "long|", "None|", "set|", "str|", "tuple|", "True|", "type|",
    "unicode|", "xrange|", NULL
};

char* BASH_HL_keywords[] = {
    "case", "do", "done", "elif", "else", "esac", "fi", "for", "function", "if",
    "in", "select", "then", "time", "until", "while", "alias", "bg", "bind", "break",
    "builtin", "cd", "command", "continue", "declare", "dirs", "disown", "echo",
    "enable", "eval", "exec", "exit", "export", "fc", "fg", "getopts", "hash", "help",
    "history", "jobs", "kill", "let", "local", "logout", "popd", "pushd", "pwd", "read",
    "readonly", "return", "set", "shift", "suspend", "test", "times", "trap", "type",
    "typeset", "ulimit", "umask", "unalias", "unset", "wait", "printf", NULL
};

char* JS_HL_keywords[] = {
    "break", "case", "catch", "class", "const", "continue", "debugger", "default",
    "delete", "do", "else", "enum", "export", "extends", "finally", "for", "function",
    "if", "implements", "import", "in", "instanceof", "interface", "let", "new",
    "package", "private", "protected", "public", "return", "static", "super", "switch",
    "this", "throw", "try", "typeof", "var", "void", "while", "with", "yield", "true",
    "false", "null", "NaN", "global", "window", "prototype", "constructor", "document",
    "isNaN", "arguments", "undefined",
    
    "Infinity|", "Array|", "Object|", "Number|", "String|", "Boolean|", "Function|",
    "ArrayBuffer|", "DataView|", "Float32Array|", "Float64Array|", "Int8Array|",
    "Int16Array|", "Int32Array|", "Uint8Array|", "Uint8ClampedArray|", "Uint32Array|",
    "Date|", "Error|", "Map|", "RegExp|", "Symbol|", "WeakMap|", "WeakSet|", "Set|", NULL
};

char* PHP_HL_keywords[] = {
    "__halt_compiler", "break", "clone", "die", "empty", "endswitch", "final", "global",
    "include_once", "list", "private", "return", "try", "xor", "abstract", "callable",
    "const", "do", "enddeclare", "endwhile", "finally", "goto", "instanceof", "namespace",
    "protected", "static", "unset", "yield", "and", "case", "continue", "echo", "endfor",
    "eval", "for", "if", "insteadof", "new", "public", "switch", "use", "array", "catch",
    "declare", "else", "endforeach", "exit", "foreach", "implements", "interface", "or",
    "require", "throw", "var", "as", "class", "default", "elseif", "endif", "extends",
    "function", "include", "isset", "print", "require_once", "trait", "while", NULL
};

char* JSON_HL_keywords[] = {
    NULL
};

char* XML_HL_keywords[] = {
    NULL
};

char* SQL_HL_keywords[] = {
    "SELECT", "FROM", "DROP", "CREATE", "TABLE", "DEFAULT", "FOREIGN", "UPDATE", "LOCK",
    "INSERT", "INTO", "VALUES", "LOCK", "UNLOCK", "WHERE", "DINSTINCT", "BETWEEN", "NOT",
    "NULL", "TO", "ON", "ORDER", "GROUP", "IF", "BY", "HAVING", "USING", "UNION", "UNIQUE",
    "AUTO_INCREMENT", "LIKE", "WITH", "INNER", "OUTER", "JOIN", "COLUMN", "DATABASE", "EXISTS",
    "NATURAL", "LIMIT", "UNSIGNED", "MAX", "MIN", "PRECISION", "ALTER", "DELETE", "CASCADE",
    "PRIMARY", "KEY", "CONSTRAINT", "ENGINE", "CHARSET", "REFERENCES", "WRITE",
    
    "BIT|", "TINYINT|", "BOOL|", "BOOLEAN|", "SMALLINT|", "MEDIUMINT|", "INT|", "INTEGER|",
    "BIGINT|", "DOUBLE|", "DECIMAL|", "DEC|" "FLOAT|", "DATE|", "DATETIME|", "TIMESTAMP|",
    "TIME|", "YEAR|", "CHAR|", "VARCHAR|", "TEXT|", "ENUM|", "SET|", "BLOB|", "VARBINARY|",
    "TINYBLOB|", "TINYTEXT|", "MEDIUMBLOB|", "MEDIUMTEXT|", "LONGTEXT|",
    
    "select", "from", "drop", "create", "table", "default", "foreign", "update", "lock",
    "insert", "into", "values", "lock", "unlock", "where", "dinstinct", "between", "not",
    "null", "to", "on", "order", "group", "if", "by", "having", "using", "union", "unique",
    "auto_increment", "like", "with", "inner", "outer", "join", "column", "database", "exists",
    "natural", "limit", "unsigned", "max", "min", "precision", "alter", "delete", "cascade",
    "primary", "key", "constraint", "engine", "charset", "references", "write",
    
    "bit|", "tinyint|", "bool|", "boolean|", "smallint|", "mediumint|", "int|", "integer|",
    "bigint|", "double|", "decimal|", "dec|" "float|", "date|", "datetime|", "timestamp|",
    "time|", "year|", "char|", "varchar|", "text|", "enum|", "set|", "blob|", "varbinary|",
    "tinyblob|", "tinytext|", "mediumblob|", "mediumtext|", "longtext|", NULL
};

char* RUBY_HL_keywords[] = {
    "__ENCODING__", "__LINE__", "__FILE__", "BEGIN", "END", "alias", "and", "begin", "break",
    "case", "class", "def", "defined?", "do", "else", "elsif", "end", "ensure", "for", "if",
    "in", "module", "next", "not", "or", "redo", "rescue", "retry", "return", "self", "super",
    "then", "undef", "unless", "until", "when", "while", "yield", NULL
};

struct editor_syntax HL_DB[] = {
    {
        "c",
        C_HL_extensions,
        C_HL_keywords,
        "//",
        "/*",
        "*/",
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
    },
    {
        "java",
        JAVA_HL_extensions,
        JAVA_HL_keywords,
        "//",
        "/*",
        "*/",
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
    },
    {
        "python",
        PYTHON_HL_extensions,
        PYTHON_HL_keywords,
        "#",
        "'''",
        "'''",
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
    },
    {
        "bash",
        BASH_HL_extensions,
        BASH_HL_keywords,
        "#",
        NULL,
        NULL,
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
    },
    {
        "js",
        JS_HL_extensions,
        JS_HL_keywords,
        "//",
        "/*",
        "*/",
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
    },
    {
        "php",
        PHP_HL_extensions,
        PHP_HL_keywords,
        "//",
        "/*",
        "*/",
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
    },
    {
        "json",
        JSON_HL_extensions,
        JSON_HL_keywords,
        NULL,
        NULL,
        NULL,
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
    },
    {
        "xml",
        XML_HL_extensions,
        XML_HL_keywords,
        NULL,
        NULL,
        NULL,
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
    },
    {
        "sql",
        SQL_HL_extensions,
        SQL_HL_keywords,
        "--",
        "/*",
        "*/",
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
    },
    {
        "ruby",
        RUBY_HL_extensions,
        RUBY_HL_keywords,
        "#",
        "=begin",
        "=end",
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
    }
};

// Size of the "Hightlight Database" (HL_DB).
#define HL_DB_ENTRIES (sizeof(HL_DB) / sizeof(HL_DB[0]))

/*** Append buffer section **/

void abufAppend(struct a_buf* ab, const char* s, int len) 
{
    // Using realloc to get a block of free memory that is
    // the size of the current string + the size of the string
    // to be appended.
    char* newBuf = realloc(ab -> buf, ab -> len + len);
    
    if (newBuf == NULL) {
        return;
    }
    
    // Copying the string s at the end of the current data in
    // the buffer.
    memcpy(&newBuf[ab -> len], s, len);
    ab -> buf = newBuf;
    ab -> len += len;
}

void abufFree(struct a_buf* ab) 
{
    // Deallocating buffer.
    free(ab -> buf);
}

void print_help()
{
    printf("Usage: edit [OPTIONS] [FILE]\n\n");
    printf("\nKEYBINDINGS\n-------------\n\n");
    printf("Keybinding\t\tAction\n\n");
    printf("Ctrl-Q    \t\tExit\n");
    printf("Ctrl-S    \t\tSave\n");
    printf("Ctrl-F    \t\tSearch. Esc, enter and arrows to interact once searching\n");
    printf("Ctrl-E    \t\tFlip line upwards\n");
    printf("Ctrl-D    \t\tFlip line downwards\n");
    printf("Ctrl-C    \t\tCopy line\n");
    printf("Ctrl-X    \t\tCut line\n");
    printf("Ctrl-V    \t\tPaste line\n");
    printf("Ctrl-P    \t\tPause edit (type \"fg\" to resume)\n");
    
    printf("\n\nOPTIONS\n-------\n\n");
    printf("Option        \t\tAction\n\n");
    printf("-h | --help   \t\tPrints the help\n");
    printf("-v | --version\t\tPrints the version of edit\n");
    
    printf("\n\nFor now, usage of ISO 8859-1 is recommended.\n");
}

void editor_clear_screen();
void die(const char* msg)
{
    editor_clear_screen();
    perror(msg);
    printf("\r\t");
    exit(1);
}

int get_win_size(int* rows, int* cols)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        return -1;
    } else {
        *rows = ws.ws_row;
        *cols = ws.ws_col;
        return 0;
    }
}

void editor_clear_screen()
{
    // Writing 4 bytes out to the terminal:
    // - (1 byte) \x1b : escape character
    // - (3 bytes) [2J : Clears the entire screen, see
    // http://vt100.net/docs/vt100-ug/chapter3.html#ED
    // for more info.
    write(STDOUT_FILENO, "\x1b[2J", 4);

    // Writing 3 bytes to reposition the cursor back at
    // the top-left corner, see
    // http://vt100.net/docs/vt100-ug/chapter3.html#CUP
    // for more info.
    write(STDOUT_FILENO, "\x1b[H", 3);
}

void editor_update_win_size()
{
    if (get_win_size(&ec.screen_rows, &ec.screen_cols) == -1) {
        die("Failed to get window size");
    }
    ec.screen_rows -= 2;
}

void editor_refresh_screen();
void editor_handle_sigwinch(int signum)
{
    editor_update_win_size();
    if (ec.cursor_y > ec.screen_rows) {
        ec.cursor_y = ec.screen_rows - 1;
    }
    if (ec.cursor_x > ec.screen_cols) {
        ec.cursor_x = ec.screen_cols - 1;
    }
    editor_refresh_screen();
}

void disable_raw_mode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &ec.orig_termios) == -1) {
        die("Failed to disable raw mode");
    }
}

void console_buf_open();
void enable_raw_mode()
{
    if (tcgetattr(STDIN_FILENO, &ec.orig_termios) == -1) {
        die("Failed to get current terminal state");
    }
    atexit(disable_raw_mode);
    //
    struct termios raw = ec.orig_termios;
    //
    // This disables Ctrl-M, Ctrl-S and Ctrl-Q commands.
    // (BRKINT, INPCK and ISTRIP are not estrictly mandatory,
    // but it is recommended to turn them off in case any
    // system needs it).
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    // Turning off all output processing (\r\n).
    raw.c_oflag &= ~(OPOST);
    // Setting character size to 8 bits per byte (it should be
    // like that on most systems, but whatever).
    raw.c_cflag |= (CS8);
    // Using NOT operator on ECHO | ICANON | IEXTEN | ISIG and
    // then bitwise-AND them with flags field in order to
    // force c_lflag 4th bit to become 0. This disables
    // chars being printed (ECHO) and let us turn off
    // canonical mode in order to read input byte-by-byte
    // instead of line-by-line (ICANON), ISIG disables
    // Ctrl-C command and IEXTEN the Ctrl-V one.
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    // read() function now returns as soon as there is any
    // input to be read.
    raw.c_cc[VMIN] = 0;
    // Forcing read() function to return every 1/10 of a
    // second if there is nothing to read.
    raw.c_cc[VTIME] = 1;
    //
    console_buf_open();
    //
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        die("Failed to set raw mode");
    }
}

void console_buf_open()
{
    // Switch to another terminal buffer in order to be able to restore state at exit
    // by calling consoleBufferClose().
    if (write(STDOUT_FILENO, "\x1b[?47h", 6) == -1) {
        die("Error changing terminal buffer");
    }
}

void console_buf_close()
{
    // Restore console to the state edit opened.
    if (write(STDOUT_FILENO, "\x1b[?9l", 5) == -1 || write(STDOUT_FILENO, "\x1b[?47l", 6) == -1){
        die("Error restoring buffer state");
    }
    /*struct a_buf ab = {.buf = NULL, .len = 0};
     char* buf = NULL;
     if (asprintf(&buf, "\x1b[%d;%dH\r\n", ec.screen_rows + 1, 1) == -1)
     die("Error restoring buffer state");
     abufAppend(&ab, buf, strlen(buf));
     free(buf);
     
     if (write(STDOUT_FILENO, ab.buf, ab.len) == -1)
     die("Error restoring buffer state");
     abufFree(&ab);*/
    
    editor_clear_screen();
}

void editor_handle_sigcont(int signum)
{
    disable_raw_mode();
    console_buf_open();
    enable_raw_mode();
    editor_refresh_screen();
}

void init_editor()
{
    ec.cursor_x = 0;
    ec.cursor_y = 0;
    ec.render_x = 0;
    ec.row_offset = 0;
    ec.col_offset = 0;
    ec.num_rows = 0;
    ec.row = NULL;
    ec.dirty = 0;
    ec.file_name = NULL;
    ec.status_msg[0] = '\0';
    ec.status_msg_time = 0;
    ec.copied_char_buffer = NULL;
    ec.syntax = NULL;
    //
    editor_update_win_size();
    //
    signal(SIGWINCH, editor_handle_sigwinch);
    signal(SIGCONT, editor_handle_sigcont);
}

int handle_args(int argc, char** argv)
{
    if (argc == 1) {
        return 0;
    }
    
    if (argc >= 2) {
        if (strncmp(argv[1], "-h", 2) == 0 || strncmp(argv[1], "--help", 6) == 0) {
            print_help();
            return -1;
        } else if(strncmp(argv[1], "-v", 2) == 0 || strncmp(argv[1], "--version", 9) == 0) {
            printf("edit - version %s\n", EDIT_VERSION);
            return -1;
        }
    }
    
    return 1;
}

void editor_set_status_msg(const char* msg, ...)
{
    va_list args;
    va_start(args, msg);
    vsnprintf(ec.status_msg, sizeof(ec.status_msg), msg, args);
    va_end(args);
    ec.status_msg_time = time(NULL);
}

/*** Syntax highlighting ***/

int is_separator(int c)
{
    // strchr() looks to see if any one of the characters in the first string
    // appear in the second string. If so, it returns a pointer to the
    // character in the second string that matched. Otherwise, it
    // returns NULL.
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[]:;", c) != NULL;
}

int is_also_number(int c) 
{
    return c == '.' || c == 'x' || c == 'a' || c == 'b' || c == 'c' || c == 'd' || c == 'e' || c == 'f';
}

void editor_update_syntax(editor_row* row) 
{
    row -> highlight = realloc(row -> highlight, row -> render_size);
    // void * memset ( void * ptr, int value, size_t num );
    // Sets the first num bytes of the block of memory pointed by ptr to
    // the specified value. With this we set all characters to HL_NORMAL.
    memset(row -> highlight, HL_NORMAL, row -> render_size);
    
    if (ec.syntax == NULL) {
        return;
    }
    
    char** keywords = ec.syntax -> keywords;
    
    char* scs = ec.syntax -> singleline_comment_start;
    char* mcs = ec.syntax -> multiline_comment_start;
    char* mce = ec.syntax -> multiline_comment_end;
    
    int scs_len = scs ? strlen(scs) : 0;
    int mcs_len = mcs ? strlen(mcs) : 0;
    int mce_len = mce ? strlen(mce) : 0;
    
    int prev_sep = 1; // True (1) if the previous char is a separator, false otherwise.
    int in_string = 0; // If != 0, inside a string. We also keep track if it's ' or "
    int in_comment = (row -> idx > 0 && ec.row[row -> idx - 1].hl_open_comment); // This is ONLY used on ML comments.
    int i = 0;
    while (i < row -> render_size) {
        char c = row -> render[i];
        // Highlight type of the previous character.
        unsigned char prev_highlight = (i > 0) ? row -> highlight[i - 1] : HL_NORMAL;
        
        if (scs_len && !in_string && !in_comment) {
            // int strncmp ( const char * str1, const char * str2, size_t num );
            // Compares up to num characters of the C string str1 to those of the C string str2.
            // This function starts comparing the first character of each string. If they are
            // equal to each other, it continues with the following pairs until the characters
            // differ, until a terminating null-character is reached, or until num characters
            // match in both strings, whichever happens first.
            if (!strncmp(&row -> render[i], scs, scs_len)) {
                memset(&row -> highlight[i], HL_SL_COMMENT, row -> render_size - i);
                break;
            }
        }
        
        if (mcs_len && mce_len && !in_string) {
            if (in_comment) {
                row -> highlight[i] = HL_ML_COMMENT;
                if (!strncmp(&row -> render[i], mce, mce_len)) {
                    memset(&row -> highlight[i], HL_ML_COMMENT, mce_len);
                    i += mce_len;
                    in_comment = 0;
                    prev_sep = 1;
                    continue;
                } else {
                    i++;
                    continue;
                }
            } else if (!strncmp(&row -> render[i], mcs, mcs_len)) {
                memset(&row -> highlight[i], HL_ML_COMMENT, mcs_len);
                i += mcs_len;
                in_comment = 1;
                continue;
            }
            
        }
        
        if (ec.syntax -> flags & HL_HIGHLIGHT_STRINGS) {
            if (in_string) {
                row -> highlight[i] = HL_STRING;
                // If we’re in a string and the current character is a backslash (\),
                // and there’s at least one more character in that line that comes
                // after the backslash, then we highlight the character that comes
                // after the backslash with HL_STRING and consume it. We increment
                // i by 2 to consume both characters at once.
                if (c == '\\' && i + 1 < row -> render_size) {
                    row -> highlight[i + 1] = HL_STRING;
                    i += 2;
                    continue;
                }
                
                if (c == in_string)
                    in_string = 0;
                i++;
                prev_sep = 1;
                continue;
            } else {
                if (c == '"' || c == '\'') {
                    in_string = c;
                    row -> highlight[i] = HL_STRING;
                    i++;
                    continue;
                }
            }
        }
        
        if (ec.syntax -> flags & HL_HIGHLIGHT_NUMBERS) {
            if ((isdigit(c) && (prev_sep || prev_highlight == HL_NUMBER)) || (is_also_number(c) && prev_highlight == HL_NUMBER)) {
                row -> highlight[i] = HL_NUMBER;
                i++;
                prev_sep = 0;
                continue;
            }
        }
        
        if (prev_sep) {
            int j = 0;
            for (j = 0; keywords[j]; j++) {
                int kw_len = strlen(keywords[j]);
                int kw_2 = keywords[j][kw_len - 1] == '|';
                if (kw_2) {
                    kw_len--;
                }
                
                // Keywords require a separator both before and after the keyword.
                if (!strncmp(&row -> render[i], keywords[j], kw_len) && is_separator(row -> render[i + kw_len])) {
                    memset(&row -> highlight[i], kw_2 ? HL_KEYWORD_2 : HL_KEYWORD_1, kw_len);
                    i += kw_len;
                    break;
                }
            }
            if (keywords[j] != NULL) {
                prev_sep = 0;
                continue;
            }
        }
        
        prev_sep = is_separator(c);
        i++;
    }
    
    int changed = (row -> hl_open_comment != in_comment);
    // This tells us whether the row ended as an unclosed multi-line
    // comment or not.
    row -> hl_open_comment = in_comment;
    // A user could comment out an entire file just by changing one line.
    // So it seems like we need to update the syntax of all the lines
    // following the current line. However, we know the highlighting
    // of the next line will not change if the value of this line’s
    // // // hl_open_comment did not change. So we check if it changed, and
    // // only call editorUpdateSyntax() on the next line if
    // hl_open_comment changed (and if there is a next line in the file).
    // Because editorUpdateSyntax() keeps calling itself with the next
    // line, the change will continue to propagate to more and more lines
    // until one of them is unchanged, at which point we know that all
    // the lines after that one must be unchanged as well.
    if (changed && row -> idx + 1 < ec.num_rows) {
        editor_update_syntax(&ec.row[row -> idx + 1]);
    }
}

void editor_select_syntax_highlight() 
{
    ec.syntax = NULL;
    if (ec.file_name == NULL) {
        return;
    }
    
    for (unsigned int j = 0; j < HL_DB_ENTRIES; j++) {
        struct editor_syntax* es = &HL_DB[j];
        unsigned int i = 0;
        
        while (es -> file_match[i]) {
            char* p = strstr(ec.file_name, es -> file_match[i]);
            if (p != NULL) {
                // Returns a pointer to the first occurrence of str2 in str1,
                // or a null pointer if str2 is not part of str1.
                int pat_len = strlen(es -> file_match[i]);
                if (es -> file_match[i][0] != '.' || p[pat_len] == '\0') {
                    ec.syntax = es;
                    
                    int file_row;
                    for (file_row = 0; file_row < ec.num_rows; file_row++) {
                        editor_update_syntax(&ec.row[file_row]);
                    }
                    
                    return;
                }
            }
            i++;
        }
    }
}

/*** Row operations ***/

void editor_update_row(editor_row* row) 
{
    // First, we have to loop through the chars of the row
    // and count the tabs in order to know how much memory
    // to allocate for render. The maximum number of characters
    // needed for each tab is 8. row->size already counts 1 for
    // each tab, so we multiply the number of tabs by 7 and add
    // that to row->size to get the maximum amount of memory we'll
    // need for the rendered row.
    int tabs = 0;
    for (int j = 0; j < row -> size; j++) {
        if (row -> chars[j] == '\t')
            tabs++;
    }
    free(row -> render);
    row -> render = malloc(row -> size + tabs * (EDIT_TAB_STOP - 1) + 1);
    
    // After allocating the memory, we check whether the current character
    // is a tab. If it is, we append one space (because each tab must
    // advance the cursor forward at least one column), and then append
    // spaces until we get to a tab stop, which is a column that is
    // divisible by 8
    int idx = 0;
    for (int j = 0; j < row -> size; j++) {
        if (row -> chars[j] == '\t') {
            row -> render[idx++] = ' ';
            while (idx % EDIT_TAB_STOP != 0) {
                row -> render[idx++] = ' ';
            }
        } else {
            row -> render[idx++] = row -> chars[j];
        }
    }
    row -> render[idx] = '\0';
    row -> render_size = idx;
    
    editor_update_syntax(row);
}

void editor_insert_row(int at, char* s, size_t line_len) 
{
    if (at < 0 || at > ec.num_rows) {
        return;
    }
    
    ec.row = realloc(ec.row, sizeof(editor_row) * (ec.num_rows + 1));
    memmove(&ec.row[at + 1], &ec.row[at], sizeof(editor_row) * (ec.num_rows - at));
    
    for (int j = at + 1; j <= ec.num_rows; j++) {
        ec.row[j].idx++;
    }
    
    ec.row[at].idx = at;
    
    ec.row[at].size = line_len;
    ec.row[at].chars = malloc(line_len + 1); // We want to add terminator char '\0' at the end
    memcpy(ec.row[at].chars, s, line_len);
    ec.row[at].chars[line_len] = '\0';
    
    ec.row[at].render_size = 0;
    ec.row[at].render = NULL;
    ec.row[at].highlight = NULL;
    ec.row[at].hl_open_comment = 0;
    editor_update_row(&ec.row[at]);
    
    ec.num_rows++;
    ec.dirty++;
}

void editor_free_row(editor_row* row) 
{
    free(row -> render);
    free(row -> chars);
    free(row -> highlight);
}

void editor_del_row(int at) 
{
    if (at < 0 || at >= ec.num_rows) {
        return;
    }
    editor_free_row(&ec.row[at]);
    memmove(&ec.row[at], &ec.row[at + 1], sizeof(editor_row) * (ec.num_rows - at - 1));
    
    for (int j = at; j < ec.num_rows - 1; j++) {
        ec.row[j].idx--;
    }
    
    ec.num_rows--;
    ec.dirty++;
}

// -1 down, 1 up
void editor_flip_row(int dir) 
{
    editor_row c_row = ec.row[ec.cursor_y];
    ec.row[ec.cursor_y] = ec.row[ec.cursor_y - dir];
    ec.row[ec.cursor_y - dir] = c_row;
    
    ec.row[ec.cursor_y].idx += dir;
    ec.row[ec.cursor_y - dir].idx -= dir;
    
    int first = (dir == 1) ? ec.cursor_y - 1 : ec.cursor_y;
    editor_update_syntax(&ec.row[first]);
    editor_update_syntax(&ec.row[first] + 1);
    if (ec.num_rows - ec.cursor_y > 2) {
        editor_update_syntax(&ec.row[first] + 2);
    }
    ec.cursor_y -= dir;
    ec.dirty++;
}

void editor_copy(int cut) 
{
    ec.copied_char_buffer = realloc(ec.copied_char_buffer, strlen(ec.row[ec.cursor_y].chars) + 1);
    strcpy(ec.copied_char_buffer, ec.row[ec.cursor_y].chars);
    editor_set_status_msg(cut ? "Content cut" : "Content copied");
}

void editor_cut() 
{
    editor_copy(-1);
    editor_del_row(ec.cursor_y);
    if (ec.num_rows - ec.cursor_y > 0) {
        editor_update_syntax(&ec.row[ec.cursor_y]);
    }
    if (ec.num_rows - ec.cursor_y > 1) {
        editor_update_syntax(&ec.row[ec.cursor_y + 1]);
    }
    ec.cursor_x = ec.cursor_y == ec.num_rows ? 0 : ec.row[ec.cursor_y].size;
}

void editor_row_append_str(editor_row* row, char* s, size_t len);
void editor_paste() 
{
    if (ec.copied_char_buffer == NULL) {
        return;
    }
    if (ec.cursor_y == ec.num_rows) {
        editor_insert_row(ec.cursor_y, ec.copied_char_buffer, strlen(ec.copied_char_buffer));
    } else {
        editor_row_append_str(&ec.row[ec.cursor_y], ec.copied_char_buffer, strlen(ec.copied_char_buffer));
    }
    ec.cursor_x += strlen(ec.copied_char_buffer);
}

void editor_row_insert_char(editor_row* row, int at, int c) 
{
    if (at < 0 || at > row -> size) {
        at = row -> size;
    }
    // We need to allocate 2 bytes because we also have to make room for
    // the null byte.
    row -> chars = realloc(row -> chars, row -> size + 2);
    // memmove it's like memcpy(), but is safe to use when the source and
    // destination arrays overlap
    memmove(&row -> chars[at + 1], &row -> chars[at], row -> size - at + 1);
    row -> size++;
    row -> chars[at] = c;
    editor_update_row(row);
    ec.dirty++; // This way we can see "how dirty" a file is.
}

void editor_insert_newline() 
{
    // If we're at the beginning of a line, all we have to do is insert
    // a new blank row before the line we're on.
    if (ec.cursor_x == 0) {
        editor_insert_row(ec.cursor_y, "", 0);
        // Otherwise, we have to split the line we're on into two rows.
    } else {
        editor_row* row = &ec.row[ec.cursor_y];
        editor_insert_row(ec.cursor_y + 1, &row -> chars[ec.cursor_x], row -> size - ec.cursor_x);
        row = &ec.row[ec.cursor_y];
        row -> size = ec.cursor_x;
        row -> chars[row -> size] = '\0';
        editor_update_row(row);
    }
    ec.cursor_y++;
    ec.cursor_x = 0;
}

void editor_row_append_str(editor_row* row, char* s, size_t len) 
{
    row -> chars = realloc(row -> chars, row -> size + len + 1);
    memcpy(&row -> chars[row -> size], s, len);
    row -> size += len;
    row -> chars[row -> size] = '\0';
    editor_update_row(row);
    ec.dirty++;
}

void editor_row_del_char(editor_row* row, int at) 
{
    if (at < 0 || at >= row -> size) {
        return;
    }
    // Overwriting the deleted character with the characters that come
    // after it.
    memmove(&row -> chars[at], &row -> chars[at + 1], row -> size - at);
    row -> size--;
    editor_update_row(row);
    ec.dirty++;
}

/*** Editor operations ***/

void editor_insert_char(int c) 
{
    // If this is true, the cursor is on the tilde line after the end of
    // the file, so we need to append a new row to the file before inserting
    // a character there.
    if (ec.cursor_y == ec.num_rows) {
        editor_insert_row(ec.num_rows, "", 0);
    }
    editor_row_insert_char(&ec.row[ec.cursor_y], ec.cursor_x, c);
    ec.cursor_x++; // This way we can see "how dirty" a file is.
}

void editor_del_char() 
{
    // If the cursor is past the end of the file, there's nothing to delete.
    if (ec.cursor_y == ec.num_rows) {
        return;
    }
    // Cursor is at the beginning of a file, there's nothing to delete.
    if (ec.cursor_x == 0 && ec.cursor_y == 0) {
        return;
    }
    
    editor_row* row = &ec.row[ec.cursor_y];
    if (ec.cursor_x > 0) {
        editor_row_del_char(row, ec.cursor_x - 1);
        ec.cursor_x--;
        // Deleting a line and moving up all the content.
    } else {
        ec.cursor_x = ec.row[ec.cursor_y - 1].size;
        editor_row_append_str(&ec.row[ec.cursor_y -1], row -> chars, row -> size);
        editor_del_row(ec.cursor_y);
        ec.cursor_y--;
    }
}


int editor_read_key() 
{
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        // Ignoring EAGAIN to make it work on Cygwin.
        if (nread == -1 && errno != EAGAIN) {
            die("Error reading input");
        }
    }
    
    // Check escape sequences, if first byte
    // is an escape character then...
    if (c == '\x1b') {
        char seq[3];
        
        if (read(STDIN_FILENO, &seq[0], 1) != 1 ||
            read(STDIN_FILENO, &seq[1], 1) != 1)
            return '\x1b';
        
        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1)
                    return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                            // Home and End keys may be sent in many ways depending on the OS
                            // \x1b[1~, \x1b[7~, \x1b[4~, \x1b[8~
                        case '1':
                        case '7':
                            return HOME_KEY;
                        case '4':
                        case '8':
                            return END_KEY;
                            // Del key is sent as \x1b[3~
                        case '3':
                            return DEL_KEY;
                            // Page Up and Page Down send '\x1b', '[', '5' or '6' and '~'.
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                    }
                }
            } else {
                switch (seq[1]) {
                        // Arrow keys send multiple bytes starting with '\x1b', '[''
                        // and followed by an 'A', 'B', 'C' or 'D' depending on which
                        // arrow is pressed.
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                        // Home key can also be sent as \x1b[H
                    case 'H': return HOME_KEY;
                        // End key can also be sent as \x1b[F
                    case 'F': return END_KEY;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                    // Yes, Home key can ALSO be sent as \x1bOH
                case 'H': return HOME_KEY;
                    // And... End key as \x1bOF
                case 'F': return END_KEY;
            }
        }
        return '\x1b';
    } else {
        return c;
    }
}


/*** Input section ***/

char* editor_prompt(char* prompt, void (*callback)(char*, int)) 
{
    size_t buf_size = 128;
    char* buf = malloc(buf_size);
    
    size_t buf_len = 0;
    buf[0] = '\0';
    
    while (1) {
        editor_set_status_msg(prompt, buf);
        editor_refresh_screen();
        
        int c = editor_read_key();
        if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
            if (buf_len != 0)
                buf[--buf_len] = '\0';
        } else if (c == '\x1b') {
            editor_set_status_msg("");
            if (callback)
                callback(buf, c);
            free(buf);
            return NULL;
        } else if (c == '\r') {
            if (buf_len != 0) {
                editor_set_status_msg("");
                if (callback)
                    callback(buf, c);
                return buf;
            }
        } else if (!iscntrl(c) && isprint(c)) {
            if (buf_len == buf_size - 1) {
                buf_size *= 2;
                buf = realloc(buf, buf_size);
            }
            buf[buf_len++] = c;
            buf[buf_len] = '\0';
        }
        
        if (callback)
            callback(buf, c);
    }
}

void editor_move_cursor(int key) 
{
    editor_row* row = (ec.cursor_y >= ec.num_rows) ? NULL : &ec.row[ec.cursor_y];
    
    switch (key) {
        case ARROW_LEFT:
            if (ec.cursor_x != 0)
                ec.cursor_x--;
            // If <- is pressed, move to the end of the previous line
            else if (ec.cursor_y > 0) {
                ec.cursor_y--;
                ec.cursor_x = ec.row[ec.cursor_y].size;
            }
            break;
        case ARROW_RIGHT:
            if (row && ec.cursor_x < row -> size)
                ec.cursor_x++;
            // If -> is pressed, move to the start of the next line
            else if (row && ec.cursor_x == row -> size) {
                ec.cursor_y++;
                ec.cursor_x = 0;
            }
            break;
        case ARROW_UP:
            if (ec.cursor_y != 0)
                ec.cursor_y--;
            break;
        case ARROW_DOWN:
            if (ec.cursor_y < ec.num_rows)
                ec.cursor_y++;
            break;
    }
    
    // Move cursor_x if it ends up past the end of the line it's on
    row = (ec.cursor_y >= ec.num_rows) ? NULL : &ec.row[ec.cursor_y];
    int row_len = row ? row -> size : 0;
    if (ec.cursor_x > row_len) {
        ec.cursor_x = row_len;
    }
}

/*** File I/O ***/

void editor_open(char* filename)
{
    free(ec.file_name);
    ec.file_name = strdup(filename);
    //
    editor_select_syntax_highlight();
    //
    FILE* file = fopen(filename, "r+");
    if (!file) {
        die("Failed to open the file");
    }
    
    char* line = NULL;
    // Unsigned int of at least 16 bit.
    size_t line_cap = 0;
    // Bigger than int
    ssize_t line_len;
    while ((line_len = getline(&line, &line_cap, file)) != -1) {
        // We already know each row represents one line of text, there's no need
        // to keep carriage return and newline characters.
        if (line_len > 0 && (line[line_len - 1] == '\n' || line[line_len - 1] == '\r')) {
            line_len--;
        }
        editor_insert_row(ec.num_rows, line, line_len);
    }
    free(line);
    fclose(file);
    ec.dirty = 0;
}

char* editor_rows_to_str(int* buf_len) 
{
    int total_len = 0;
    int j;
    // Adding up the lengths of each row of text, adding 1
    // to each one for the newline character we'll add to
    // the end of each line.
    for (j = 0; j < ec.num_rows; j++) {
        total_len += ec.row[j].size + 1;
    }
    *buf_len = total_len;
    
    char* buf = malloc(total_len);
    char* p = buf;
    // Copying the contents of each row to the end of the
    // buffer, appending a newline character after each
    // row.
    for (j = 0; j < ec.num_rows; j++) {
        memcpy(p, ec.row[j].chars, ec.row[j].size);
        p += ec.row[j].size;
        *p = '\n';
        p++;
    }
    
    return buf;
}

void editor_save() 
{
    if (ec.file_name == NULL) {
        ec.file_name = editor_prompt("Save as: %s (ESC to cancel)", NULL);
        if (ec.file_name == NULL) {
            editor_set_status_msg("Save aborted");
            return;
        }
        editor_select_syntax_highlight();
    }
    
    int len;
    char* buf = editor_rows_to_str(&len);
    
    // We want to create if it doesn't already exist (O_CREAT flag), giving
    // 0644 permissions (the standard ones). O_RDWR stands for reading and
    // writing.
    int fd = open(ec.file_name, O_RDWR | O_CREAT, 0644);
    if (fd != -1) {
        // ftruncate sets the file's size to the specified length.
        if (ftruncate(fd, len) != -1) {
            // Writing the file.
            if (write(fd, buf, len) == len) {
                close(fd);
                free(buf);
                ec.dirty = 0;
                editor_set_status_msg("%d bytes written to disk", len);
                return;
            }
        }
        close(fd);
    }
    
    free(buf);
    editor_set_status_msg("Cant's save file. Error occurred: %s", strerror(errno));
}

/*** Search section ***/

int editor_row_renderx_to_cursorx(editor_row* row, int render_x);
void editor_search_callback(char* query, int key) 
{
    // Index of the row that the last match was on, -1 if there was
    // no last match.
    static int last_match = -1;
    // 1 for searching forward and -1 for searching backwards.
    static int direction = 1;
    
    static int saved_highlight_line;
    static char* saved_hightlight = NULL;
    
    if (saved_hightlight) {
        memcpy(ec.row[saved_highlight_line].highlight, saved_hightlight, ec.row[saved_highlight_line].render_size);
        free(saved_hightlight);
        saved_hightlight = NULL;
    }
    
    // Checking if the user pressed Enter or Escape, in which case
    // they are leaving search mode so we return immediately.
    if (key == '\r' || key == '\x1b') {
        last_match = -1;
        direction = 1;
        return;
    } else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
        direction = 1;
    } else if (key == ARROW_LEFT || key == ARROW_UP) {
        if (last_match == -1) {
            // If nothing matched and the left or up arrow key was pressed
            return;
        }
        direction = -1;
    } else {
        last_match = -1;
        direction = 1;
    }
    
    int current = last_match;
    for (int i = 0; i < ec.num_rows; i++) {
        current += direction;
        if (current == -1) {
            current = ec.num_rows - 1;
        } else if (current == ec.num_rows) {
            current = 0;
        }
        
        editor_row* row = &ec.row[current];
        // We use strstr to check if query is a substring of the
        // current row. It returns NULL if there is no match,
        // oterwhise it returns a pointer to the matching substring.
        char* match = strstr(row -> render, query);
        if (match) {
            last_match = current;
            ec.cursor_y = current;
            ec.cursor_x = editor_row_renderx_to_cursorx(row, match - row -> render);
            // We set this like so to scroll to the bottom of the file so
            // that the next screen refresh will cause the matching line to
            // be at the very top of the screen.
            ec.row_offset = ec.num_rows;
            
            saved_highlight_line = current;
            saved_hightlight = malloc(row -> render_size);
            memcpy(saved_hightlight, row -> highlight, row -> render_size);
            memset(&row -> highlight[match - row -> render], HL_MATCH, strlen(query));
            break;
        }
    }
}

void editor_search() 
{
    int saved_cursor_x = ec.cursor_x;
    int saved_cursor_y = ec.cursor_y;
    int saved_col_offset = ec.col_offset;
    int saved_row_offset = ec.row_offset;
    
    char* query = editor_prompt("Search: %s (Use ESC / Enter / Arrows)", editor_search_callback);
    
    if (query) {
        free(query);
        // If query is NULL, that means they pressed Escape, so in that case we
        // restore the cursor previous position.
    } else {
        ec.cursor_x = saved_cursor_x;
        ec.cursor_y = saved_cursor_y;
        ec.col_offset = saved_col_offset;
        ec.row_offset = saved_row_offset;
    }
}

int editor_row_cursorx_to_renderx(editor_row* row, int cursor_x)
{
    int render_x = 0;
    for(int j = 0; j < cursor_x; j++)
    {
        if (row->chars[j] == '\t') {
            render_x += (EDIT_TAB_STOP - 1) - (render_x % EDIT_TAB_STOP);
        }
        render_x++;
    }
    return render_x;
}

int editor_row_renderx_to_cursorx(editor_row* row, int render_x) 
{
    int cur_render_x = 0;
    int cursor_x = 0;
    for (cursor_x = 0; cursor_x < row->size; cursor_x++) {
        if (row->chars[cursor_x] == '\t'){
            cur_render_x += (EDIT_TAB_STOP - 1) - (cur_render_x % EDIT_TAB_STOP);
        }
        cur_render_x++;
        
        if (cur_render_x > render_x){
            return cursor_x;
        }
    }
    return cursor_x;
}

void editor_scroll()
{
    ec.render_x = 0;
    if (ec.cursor_y < ec.num_rows) {
        ec.render_x = editor_row_cursorx_to_renderx(&ec.row[ec.cursor_y], ec.cursor_x);
    }
    // The first if statement checks if the cursor is above the visible window,
    // and if so, scrolls up to where the cursor is. The second if statement checks
    // if the cursor is past the bottom of the visible window, and contains slightly
    // more complicated arithmetic because ec.row_offset refers to what's at the top
    // of the screen, and we have to get ec.screen_rows involved to talk about what's
    // at the bottom of the screen.
    if (ec.cursor_y < ec.row_offset) {
        ec.row_offset = ec.cursor_y;
    }
    if (ec.cursor_y >= ec.row_offset + ec.screen_rows) {
        ec.row_offset = ec.cursor_y - ec.screen_rows + 1;
    }

    if (ec.cursor_x < ec.col_offset) {
        ec.col_offset = ec.cursor_x;
    }
    if (ec.cursor_x >= ec.col_offset + ec.screen_cols) {
        ec.col_offset = ec.cursor_x - ec.screen_cols + 1;
    }
}

void editor_draw_welcome_msg(struct a_buf* ab)
{
    char welcome[80];
    // Using snprintf to truncate message in case the terminal
    // is too tiny to handle the entire string.
    int welcome_len = snprintf(welcome, sizeof(welcome), "edit %s", EDIT_VERSION);
    if (welcome_len > ec.screen_cols) {
        welcome_len = ec.screen_cols;
    }
    // Centering the message.
    int padding = (ec.screen_cols - welcome_len) / 2;
    // Remember that everything != 0 is true.
    if (padding) {
        abufAppend(ab, "~", 1);
        padding--;
    }
    while (padding--) {
        abufAppend(ab, " ", 1);
    }
    abufAppend(ab, welcome, welcome_len);
}

int editor_syntax_to_color(int highlight)
{
    // We return ANSI codes for colors.
    // See https://en.wikipedia.org/wiki/ANSI_escape_code#Colors
    // for a list of them.
    switch (highlight) {
        case HL_SL_COMMENT:
        case HL_ML_COMMENT: return 36;
        case HL_KEYWORD_1: return 31;
        case HL_KEYWORD_2: return 32;
        case HL_STRING: return 33;
        case HL_NUMBER: return 35;
        case HL_MATCH: return 34;
        default: return 37;
    }
}

void editor_draw_rows(struct a_buf* ab)
{
    for (int y = 0; y < ec.screen_rows; y++) {
        int file_row = y + ec.row_offset;
        if(file_row >= ec.num_rows) {
            if (ec.num_rows == 0 && y == ec.screen_rows / 3) {
                editor_draw_welcome_msg(ab);
            } else {
                abufAppend(ab, "~", 1);
            }
        } else {
            int len = ec.row[file_row].render_size - ec.col_offset;
            // len can be a negative number, meaning the user scrolled
            // horizontally past the end of the line. In that case, we set
            // len to 0 so that nothing is displayed on that line.
            if (len < 0) {
                len = 0;
            }
            if (len > ec.screen_cols) {
                len = ec.screen_cols;
            }
            char* c = &ec.row[file_row].render[ec.col_offset];
            unsigned char* highlight = &ec.row[file_row].highlight[ec.col_offset];
            int current_color = -1;
            for (int j = 0; j < len; j++) {
                // Displaying nonprintable characters as (A-Z, @, and ?).
                if (iscntrl(c[j])) {
                    char sym = (c[j] <= 26) ? '@' + c[j] : '?';
                    abufAppend(ab, "\x1b[7m", 4);
                    abufAppend(ab, &sym, 1);
                    abufAppend(ab, "\x1b[m", 3);
                    if (current_color != -1) {
                        char buf[16];
                        int c_len = snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
                        abufAppend(ab, buf, c_len);
                    }
                } else if (highlight[j] == HL_NORMAL) {
                    if (current_color != -1) {
                        abufAppend(ab, "\x1b[39m", 5);
                        current_color = -1;
                    }
                    abufAppend(ab, &c[j], 1);
                } else {
                    int color = editor_syntax_to_color(highlight[j]);
                    // We only use escape sequence if the new color is different
                    // from the last character's color.
                    if (color != current_color) {
                        current_color = color;
                        char buf[16];
                        int c_len = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
                        abufAppend(ab, buf, c_len);
                    }
                    
                    abufAppend(ab, &c[j], 1);
                }
            }
            abufAppend(ab, "\x1b[39m", 5);
        }
        // Redrawing each line instead of the whole screen.
        abufAppend(ab, "\x1b[K", 3);
        // Addind a new line
        abufAppend(ab, "\r\n", 2);
    }
}

void editor_draw_status_bar(struct a_buf* ab)
{
    // This switches to inverted colors.
    // NOTE:
    // The m command (Select Graphic Rendition) causes the text printed
    // after it to be printed with various possible attributes including
    // bold (1), underscore (4), blink (5), and inverted colors (7). An
    // argument of 0 clears all attributes (the default one). See
    // http://vt100.net/docs/vt100-ug/chapter3.html#SGR for more info.
    abufAppend(ab, "\x1b[7m", 4);
    //
    char status[80], r_status[80];
    // Showing up to 20 characters of the filename, followed by the number of lines.
    int len = snprintf(status, sizeof(status), " Editing: %.20s %s", ec.file_name ? ec.file_name : "New file", ec.dirty ? "(modified)" : "");
    int col_size = ec.row && ec.cursor_y <= ec.num_rows - 1 ? col_size = ec.row[ec.cursor_y].size : 0;
    int r_len = snprintf(r_status, sizeof(r_status), "%d/%d lines  %d/%d cols ", ec.cursor_y + 1 > ec.num_rows ? ec.num_rows : ec.cursor_y + 1, ec.num_rows, ec.cursor_x + 1 > col_size ? col_size : ec.cursor_x + 1, col_size);
    if (len > ec.screen_cols){
        len = ec.screen_cols;
    }
    abufAppend(ab, status, len);
    while (len < ec.screen_cols) {
        if (ec.screen_cols - len == r_len) {
            abufAppend(ab, r_status, r_len);
            break;
        } else {
            abufAppend(ab, " ", 1);
            len++;
        }
    }
    // This switches back to normal colors.
    abufAppend(ab, "\x1b[m", 3);
    //
    abufAppend(ab, "\r\n", 2);
}

void editor_draw_msg_bar(struct a_buf* ab)
{
    // Clearing the message bar.
    abufAppend(ab, "\x1b[K", 3);
    int msg_len = strlen(ec.status_msg);
    if (msg_len > ec.screen_cols){
        msg_len = ec.screen_cols;
    }
    // We only show the message if its less than 5 secons old, but
    // remember the screen is only being refreshed after each keypress.
    if (msg_len && time(NULL) - ec.status_msg_time < 5){
        abufAppend(ab, ec.status_msg, msg_len);
    }
}

void editor_refresh_screen()
{
    editor_scroll();
    //
    struct a_buf ab = ABUF_INIT;
    //
    // Hiding the cursor while the screen is refreshing.
    // See http://vt100.net/docs/vt100-ug/chapter3.html#S3.3.4
    // for more info.
    abufAppend(&ab, "\x1b[?25l", 6);
    abufAppend(&ab, "\x1b[H", 3);
    //
    editor_draw_rows(&ab);
    editor_draw_status_bar(&ab);
    editor_draw_msg_bar(&ab);
    //
    // Moving the cursor where it should be.
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (ec.cursor_y - ec.row_offset) + 1, (ec.render_x - ec.col_offset) + 1);
    abufAppend(&ab, buf, (int)strlen(buf));
    //
    // Showing again the cursor.
    abufAppend(&ab, "\x1b[?25h", 6);
    //
    // Writing all content at once
    write(STDOUT_FILENO, ab.buf, ab.len);
    abufFree(&ab);
}

void editor_handle_key_press()
{
    static int quit_times = TTE_QUIT_TIMES;
    
    int c = editor_read_key();
    
    switch (c) {
        case '\r': // Enter key
            editor_insert_newline();
            break;
        case CTRL_KEY('q'):
            if (ec.dirty && quit_times > 0) {
                editor_set_status_msg("Warning! File has unsaved changes. Press Ctrl-Q %d more time%s to quit", quit_times, quit_times > 1 ? "s" : "");
                quit_times--;
                return;
            }
            editor_clear_screen();
            console_buf_close();
            exit(0);
            break;
        case CTRL_KEY('s'):
            editor_save();
            break;
        case CTRL_KEY('e'):
            if (ec.cursor_y > 0 && ec.cursor_y <= ec.num_rows - 1)
                editor_flip_row(1);
            break;
        case CTRL_KEY('d'):
            if (ec.cursor_y < ec.num_rows - 1)
                editor_flip_row(-1);
            break;
        case CTRL_KEY('x'):
            if (ec.cursor_y < ec.num_rows)
                editor_cut();
            break;
        case CTRL_KEY('c'):
            if (ec.cursor_y < ec.num_rows)
                editor_copy(0);
            break;
        case CTRL_KEY('v'):
            editor_paste();
            break;
        case CTRL_KEY('p'):
            console_buf_close();
            kill(0, SIGTSTP);
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            editor_move_cursor(c);
            break;
        case PAGE_UP:
        case PAGE_DOWN:
        { // You can't declare variables directly inside a switch statement.
            if (c == PAGE_UP)
                ec.cursor_y = ec.row_offset;
            else if (c == PAGE_DOWN)
                ec.cursor_y = ec.row_offset + ec.screen_rows - 1;
            
            int times = ec.screen_rows;
            while (times--)
                editor_move_cursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
        }
            break;
        case HOME_KEY:
            ec.cursor_x = 0;
            break;
        case END_KEY:
            if (ec.cursor_y < ec.num_rows)
                ec.cursor_x = ec.row[ec.cursor_y].size;
            break;
        case CTRL_KEY('f'):
            editor_search();
            break;
        case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
            if (c == DEL_KEY) {
                editor_move_cursor(ARROW_RIGHT);
            }
            editor_del_char();
            break;
        case CTRL_KEY('l'):
        case '\x1b': // Escape key
            break;
        default:
            editor_insert_char(c);
            break;
    }
    
    quit_times = TTE_QUIT_TIMES;
}

int main(int argc, char** argv)
{
    init_editor();
    int arg_resp = handle_args(argc, argv);
    if (arg_resp == 1) {
        editor_open(argv[1]);
    } else if(arg_resp == -1) {
        return 0;
    }
    enable_raw_mode();
    editor_set_status_msg(" Ctrl-Q to quit | Ctrl-S to save | (edit -h | --help for more info)");
    
    while (1) {
        editor_refresh_screen();
        editor_handle_key_press();
    }
}


