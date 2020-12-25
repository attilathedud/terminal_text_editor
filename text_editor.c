// To compile, make sure to link ncurses:
// gcc -o editor text_editor.c -Wall -lncurses
#include <ncurses.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define KEY_DELETE 127

typedef struct node
{
    char text[128];
    struct node *next;
} node_t;

typedef struct
{
    node_t *head;
    node_t *current;
} Buffer;

void buffer_select_node(Buffer *b, int y)
{
    if(b->head == NULL)
    {
        b->head = calloc(1, sizeof(node_t));
        b->current = b->head;
    }

    // iterate until we reach our y node
    b->current = b->head;
    for( int i = 0; i < y; i++ )
    {
        if( b->current->next == NULL )
        {
            b->current->next = calloc(1, sizeof(node_t));
        }
        b->current = b->current->next;
    }
}

void buffer_remove_character(Buffer *b, int x, int y)
{
    buffer_select_node(b, y);

    for ( int i = x - 1; i < strlen(b->current->text); i++ )
    {
        b->current->text[i] = b->current->text[i + 1];
    }

    b->current->text[strlen(b->current->text)] = 0;    
}

void buffer_insert_character(Buffer *b, int ch, int x, int y) 
{
    buffer_select_node(b, y);

    // ensure there are no null characters before our input    
    for( int i = 0; i < x; i++ )
    {
        if( b->current->text[i] == 0 )
            b->current->text[i] = ' ';
    }

    // insert characters instead of overwriting
    if( b->current->text[x] != 0 )
    {
        for( int i = strlen(b->current->text); i >= x; i-- )
        {
            b->current->text[i+1] = b->current->text[i];    
        }
    }

    b->current->text[x] = ch;
}

int main(int argc, char **argv)
{
    int ch = 0;
    int x = 0;
    int y = 0;

    Buffer b = { 0 };

    initscr();  // initialize the screen for curses
    noecho();   // don't repeat keypresses
    cbreak();   // immediately pass input to our program
    keypad(stdscr, TRUE);   // enable keypad and function keys

    do
    {
        erase();    // clear the screen

        // handle input
        switch(ch)
        {
            // make sure to add checks for the edges of the screen
            case KEY_RIGHT:
                x++;
                break;
            case KEY_LEFT:
                x--;
                break;
            case KEY_UP:
                y--;
                break;
            case KEY_DOWN:
                y++;
                break;
            case KEY_DELETE:
                buffer_remove_character(&b, x, y);
                x--;
                break;
            default:
                if( isprint(ch) )
                {
                    buffer_insert_character(&b, ch, x, y);
                    x++;            
                }
                break;
        }
        
        // print text
        int line = 0;
        node_t *iter = b.head;
        while( iter != NULL ) 
        {
            mvprintw(line++, 0, iter->text);
            iter = iter->next;
        }

        // print cursor
        move(y, x); 
    } while ((ch = getch()) != KEY_F(10));  // exit on f10

    refresh();  // refresh the previous terminal
    endwin();   // clean up ncurses

    return 0;
}
