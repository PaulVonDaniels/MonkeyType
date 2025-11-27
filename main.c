#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define random_word get_random_word(words, word_count)

enum {
    TYPED_COLOR = 1,
    UNTYPED_COLOR = 2,
    MISTAKE_COLOR = 3,
};

typedef struct {
    char *data;
    int head;
    int tail;
    int cap;
} CBuffer;

CBuffer CBuffer_init(int size) {
    CBuffer cbuffer;
    cbuffer.data = (char*)malloc(size);
    cbuffer.cap = size;
    cbuffer.tail = -1;
    cbuffer.head = -1;
    return cbuffer;
}

bool CBuffer_is_empty(CBuffer buffer) {
    return buffer.tail == buffer.head;
}
int CBuffer_count(CBuffer buffer) {
    if (CBuffer_is_empty(buffer))
        return 0;
    int count = 0, tail = buffer.tail;
    while (tail != buffer.head) {
        tail++;
        if (tail == buffer.cap)
            tail = 0;
        count++;
    }
    return count;
}
bool CBuffer_is_full(CBuffer buffer) {
    return CBuffer_count(buffer) == buffer.cap;
}



char * CBuffer_read(CBuffer *buffer, int count) {
    if (CBuffer_is_empty(*buffer))
        return NULL;
    
    char *read = (char*)malloc(count + 1);
    int i = 0;
    
    for (; i < count && buffer->tail != buffer->head; i++) {
        read[i] = buffer->data[buffer->tail];
        buffer->tail++;
        if (buffer->tail == buffer->cap)
            buffer->tail = 0;
    }
    read[i] = '\0';
    return read;
}   


char CBuffer_peak(CBuffer buffer) {
    return buffer.data[buffer.head == 0 ? buffer.cap-1 : buffer.head-1];
};

char CBuffer_bottom(CBuffer buffer) {
    return buffer.data[buffer.tail];
}

char *CBuffer_bottom_n(CBuffer buffer, int n) {
    if (CBuffer_is_empty(buffer) || n <= 0) {
        char *empty = (char*)malloc(1);
        empty[0] = '\0';
        return empty;
    }
    
    char *read = (char*)malloc(n + 1);
    int t = buffer.tail;
    
    for (int i = 0; i < n; i++) {
        if (t == buffer.head) { 
            read[i] = '\0';
            break;
        }
        read[i] = buffer.data[t];
        t++;
        if (t == buffer.cap) {
            t = 0;
        }
        read[i + 1] = '\0';
    }
    
    return read;
}

int CBuffer_write(CBuffer *buffer, const char * data) {
    int i = 0;
    char ch;

    if (buffer->head == -1) {
        buffer->head = 0;
        buffer->tail = 0;
    }

    while ((ch = data[i]) != '\0') {
        i++;
        buffer->data[buffer->head] = ch;
        buffer->head++;
        if (buffer->head == buffer->cap) {
            buffer->head = 0;
        }
    }
    return i;
}

void CBuffer_clear(CBuffer *buffer) {
    buffer->tail = -1;
    buffer->head = -1;
}

typedef struct {
    int pos;
    time_t start_time;
    time_t finish_time;
    int keystrokes_count;
    CBuffer type_tape;
    CBuffer tape;
} Test;

Test Test_init() {
    Test test = {
        0,
        time(NULL),
        0,
        0,
        CBuffer_init(256),
        CBuffer_init(256)
    };
    return test;
}

void Test_finish(Test *test) {
    time(&test->finish_time);
}

float Test_duration(Test test) {
    if (test.finish_time == 0)
        return time(NULL) - test.start_time;
    return test.finish_time  - test.start_time;
}

float Test_wpm(Test test) {
    float wpm;
    time_t td = Test_duration(test);
    wpm = (((float)test.keystrokes_count / td) * 60) / 5;
    return wpm;
}

void Test_write(Test *test, char ch) {
    if (test->finish_time != 0)
        return;

    if (CBuffer_is_empty(test->tape)) {
        return Test_finish(test);
    }

    char tc = CBuffer_bottom(test->tape);

    if (tc != ch) {
        return Test_finish(test);
    }

    test->keystrokes_count++;

    char cha[2];
    cha[0] = ch;
    cha[1] = '\0';
    CBuffer_write(&test->type_tape, cha);
    CBuffer_read(&test->tape, 1);
}

void Test_backspace(Test *test) {
    return;
    if (test->finish_time != 0)
        return;

    if (test->tape.head == 0) {
        test->tape.head = test->tape.cap - 1;
    } else {
        test->tape.head = test->tape.head - 1;
    }
    
}

bool Test_is_running(Test test) {
    return test.finish_time == 0;
}

void Test_render(Test test) {
    char *read, *tread, s_wpm[6], s_time[10];
    int cc = COLS / 2 + COLS % 2,
        mte = COLS - 2,
        mta = COLS / 2,
        ta = test.keystrokes_count < cc ? cc - test.keystrokes_count : 1,
        te = cc - 1,
        t;

    while (CBuffer_count(test.type_tape) > cc - 1 )
        CBuffer_read(&test.type_tape, 1);

    read = CBuffer_bottom_n(test.tape, cc - 1);
    tread = CBuffer_bottom_n(test.type_tape, te);

    attron(COLOR_PAIR(UNTYPED_COLOR));
    mvaddstr(10, cc, read);
    attroff(COLOR_PAIR(UNTYPED_COLOR));
    attron(COLOR_PAIR(TYPED_COLOR));
    mvaddstr(10, ta, tread);
    attroff(COLOR_PAIR(TYPED_COLOR));

    attron(A_REVERSE | A_BOLD );
    if (!Test_is_running(test))
        attron(COLOR_PAIR(MISTAKE_COLOR));
    mvaddch(9, cc, '|');
    mvaddch(10, cc, CBuffer_bottom(test.tape));
    mvaddch(11, cc, '|');
    if (!Test_is_running(test))
        attroff(COLOR_PAIR(MISTAKE_COLOR));
    attroff(A_REVERSE | A_BOLD);

    t = sprintf(s_wpm, "%.2f", Test_wpm(test));
    sprintf(s_time, "%.2f", Test_duration(test));
    mvaddstr(7, cc - t - 4, s_wpm);
    mvaddstr(7, cc + 1, s_time);
    
}

void Test_show_result(Test test) {
    clear();

    int i, j, cc = COLS / 2 + COLS % 2, lc = LINES / 2 + LINES % 2;
    char s_wpm[6], s_time[10], s_keystrokes[6];
    
    
    for (i=0; i<COLS; i++) {
        mvaddch(0, i, '#');
        mvaddch(LINES-1, i, '#');
    }
    for (i=0; i<LINES; i++) {
        mvaddch(i, 0, '#');
        mvaddch(i, COLS-1, '#');
    }

    sprintf(s_wpm, "%.2f", Test_wpm(test));
    sprintf(s_time, "%.2f", Test_duration(test));
    sprintf(s_keystrokes, "%d", test.keystrokes_count);

    mvaddstr(lc - 2, cc - 3, "WPM : "); mvaddstr(lc - 2, cc + 3, s_wpm);
    mvaddstr(lc - 1, cc - 4, "Time : "); mvaddstr(lc - 1, cc + 3, s_time);
    mvaddstr(lc - 0, cc - 10, "Keystrokes : "); mvaddstr(lc - 0, cc + 3, s_keystrokes);

    refresh();
}

char **read_words(int *word_count) {
    FILE * wf = fopen("words.txt", "r");
    if (!wf) {
        printf("Word list not found, exiting program..\n");
        printf("Creat a words.txt with words in it in current directory");
        exit(0);
    }
    char *line = NULL, **words;
    int i;
    size_t len = 0;
    words = (char**)malloc(sizeof(char*)*1024);
    for (i=0; i<1024; i++)
        words[i] = (char*)malloc(33);
    i = 0;
    while (getline(&line, &len, wf) != -1) {
        line[strlen(line)-1] = '\0';
        strcpy(words[i], line);
        i++;
    }
    *word_count = i;
    return words;
}

char *get_random_word(char **words, int word_count) {
    static char word[32];
    strcpy(word, words[rand() % word_count]);
    return word;
}

int main() 
{
    char **words, ch;
    int word_count;
    words = read_words(&word_count);

    if (word_count == 0) {
        printf("There is not words in words.txt populate it");
        exit(0);
    }

    srand(time(NULL));

    Test test = Test_init();


    initscr();
    curs_set(0);
    keypad(stdscr, true);
    noecho();
    start_color();
    init_pair(TYPED_COLOR, COLOR_WHITE, COLOR_BLACK);
    init_pair(UNTYPED_COLOR, COLOR_GREEN, COLOR_BLACK);
    init_pair(MISTAKE_COLOR, COLOR_RED, COLOR_WHITE);


    while (Test_is_running(test)) {
        clear();
    
        while (CBuffer_count(test.tape) < 200) {
            CBuffer_write(&test.tape, random_word);
            CBuffer_write(&test.tape, " ");
        }
        
        Test_render(test);
        refresh();

        ch = getch();


        if (ch == 27) {
            break;
        }
        if (ch == 7 || ch == 8 || ch == 127) {
            continue;
        }
        if (ch != '\n') {
            Test_write(&test, ch);
        }
    }

    Test_render(test);
    refresh();
    getch();
    Test_show_result(test);
    getch();
    getch();
    endwin();

    return 0;
}
