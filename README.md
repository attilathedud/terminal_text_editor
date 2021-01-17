# Terminal Text Editor
A simple terminal text editor using ncurses, originally written to demonstrate how to create a text buffer and how to handle navigation. The majority of this code is shared with [Clunk](https://github.com/attilathedud/Clunk). This code does not have any of the extra overhead of Clunk, including file management or error checking. The main goal was to produce code that could be used as a starting point.

## The Buffer
When storing several lines of text, the immediate idea is to use an array. Most of us have written code that writes to a file that looks like:
```
char *lines[10] = //some data

for( int i = 0; i < 10; i++ ) {
    fprintf( fp, lines[i] );
}
```

With ncurses, we can use a varient of this code, using mvprintw(y, x, text) to print:
```
char *lines[10] = //some data

for( int i = 0; i < 10; i++ ) {
    mvprintw( i, 0, lines[i] );
}
```

One downside of using arrays is their fixed-sized and memory layout. Both inserting in the middle (and therefore shifting all the following elements) and
creating a larger array (and copying all the data over) have large execution costs. Even dynamic languages, where these mechanisms are handled behind-the-scenes, suffer from these penalties. Obviously this is not an ideal structure for data that is going to be shifted around a lot when editing.

A simple way to structure the data in a text editor is to imagine each line of text as a node. These nodes are then linked in a list. So something like:
```
Hi,
This is some text
 
And this is the end
```

Will become:
```
┌─────┬──┐      
│ Hi     ────┐
└─────┴──┘   │ 
┌────────────────────┬──┐
│ This is some text     ────┐
└────────────────────┴──┘   │
   ┌────────────────────────┘
┌─────┬──┐
│        ────┐
└─────┴──┘   |
┌────────────────────┬──┐
│ And this is the end   |
└────────────────────┴──┘
```

This is commonly known as a (linked-list)[https://www.cs.cmu.edu/~adamchik/15-121/lectures/Linked%20Lists/linked%20lists.html] and is a easy way to implement a dynamic list without the pitfalls of an array.

Each language will have their own way to define or use linked-list, but in C, a basic implementation looks like:
```
typedef struct node
{
    char text[128];
    struct node *next;
} node_t;
```

In our use-case, each node contains a character array and a pointer to the next node. With this, our text "buffer" can be represented by storing the first element in this list and the current element being worked on:
```
typedef struct
{
    node_t *head;
    node_t *current;
} Buffer;
```

## ncurses
There are lots of ncurses' tutorials out there that do a good job explaining how it works. This project just uses the boilerplate code:
```
#include <ncurses.h>

int main(int argc, char **argv)
{
    int ch = 0;

    initscr();  // initialize the screen for curses
    noecho();   // don't repeat keypresses
    cbreak();   // immediately pass input to our program
    keypad(stdscr, TRUE);   // enable keypad and function keys

    do
    {
        erase();    // clear the screen

        // handle input
        // print text
    } while ((ch = getch()) != KEY_F(10));  // exit on f10

    refresh();  // refresh the previous terminal
    endwin();   // clean up ncurses

    return 0;
}
```

To compile, we need to include -lncurses  in our build process:
```
all:
  gcc main.c -Wall -lncurses
```

## A Single Line
With these pieces in place, we can implement a simple proof-of-concept that will allow us to type up to 128 characters on a single line. First, we have a function to see if there is currently a head node. If not, create it, and then place the passed character into the text member of the head.
```
#include <stdlib.h>
#include <ctype.h>

void buffer_insert_character(Buffer *b, int ch, int x, int y) 
{
    if(b->head == NULL)
    {
        b->head = calloc(1, sizeof(node_t));
    }

    b->head->text[x] = ch;
}
```

Next, we will modify the main loop. We will check to see if the handled input is printable - if it is, pass it to be inserted and increase the current x 
position. After that has been handled, we point at the head of the linked-list and iterate through every node, printing each node's text member to the screen.
```
    int x = 0;
    int y = 0;

    Buffer b = { 0 };

    do
    {
        erase();    // clear the screen

        // handle input
        if( isprint(ch) )
        {
            buffer_insert_character(&b, ch, x, y);
            x++;            
        }

        // print text
        node_t *iter = b.head;
        while( iter != NULL ) 
        {
            mvprintw(0, 0, iter->text);
            iter = iter->next;
        } 
    } while ((ch = getch()) != KEY_F(10));  // exit on f10
```

## Navigation
Text editors are not very good if you cannot move your cursor around. The input side of this is easy to implement. We can use a switch, monitor for keys, and adjust our "x" and "y" members accordingly:
```
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
            default:
                if( isprint(ch) )
                {
                    buffer_insert_character(&b, ch, x, y);
                    x++;            
                }
                break;
        }
```

To reiterate the comments, make sure you apply bounds checking when implementing this. Your scroll method will determine your checks. I would advise against using macros like I did in Clunk.

To have the cursor show in the terminal, we need to move to it after we finish printing all the text:
```
// print cursor
move(y, x); 
```

## Text Entry with Navigation
Our previously implementation of inserting characters is now broken, in several ways, with the addition of navigation. First, we will fix the case of inserting past the end of first line. In most languages, this will be caused by the string being null-terminated before getting to the next set of text. This can be fixed by putting a space for every null character before the new text:
```
void buffer_insert_character(Buffer *b, int ch, int x, int y) 
{
    if(b->head == NULL)
    {
        b->head = calloc(1, sizeof(node_t));
        b->current = b->head;
    }
    
    // ensure there are no null characters before our input
    for( int i = 0; i < x; i++ )
    {
        if( b->current->text[i] == 0 )
            b->current->text[i] = ' ';
    }

    b->current->text[x] = ch;
}
```

Now we can type, go right a few spaces, and continue typing. If you navigate left, you will notice you type over characters instead of inserting new ones. This may be what you want in your design, but if you want the more common behavior of inserting, this needs to be fixed too:
```
#include <string.h>

    // insert characters instead of overwriting
    if( b->current->text[x] != 0 )
    {
        for( int i = strlen(b->current->text); i >= x; i-- )
        {
            b->current->text[i+1] = b->current->text[i];    
        }
    }
```

This works by shifting all the characters after and at the current position to the next slot in the array.

With horizontal entry works as expected, we can deal with moving down and up. Like our first section, this will be achieved by moving over (or creating) nodes until we reach our current "y" position. This should be done before the horizontal insertion piece:
```
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
```

We also need to modify our printing code to account for multiple lines:
```
        // print text
        int line = 0;
        node_t *iter = b.head;
        while( iter != NULL ) 
        {
            mvprintw(line++, 0, iter->text);
            iter = iter->next;
        }
```

Navigation and text insertion now work as expected.

## Deleting
Deleting has a similar idea to inserting: shift all the letters back one and throw a null character on the previously last character.

Since a lot of the operations require selecting the current node, we should extract out this functionality:
```
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
```

Our remove function then uses the method we described above:
```
void buffer_remove_character(Buffer *b, int x, int y)
{
    buffer_select_node(b, y);

    for ( int i = x - 1; i < strlen(b->current->text); i++ )
    {
        b->current->text[i] = b->current->text[i + 1];
    }

    b->current->text[strlen(b->current->text)] = 0;    
}
```

And finally we add our case in the input processing:
```
#define KEY_DELETE 127

case KEY_DELETE:
    buffer_remove_character(&b, x, y);
    x--;
    break;
```

Deleting has some edge-cases to consider. For example, if you delete at the beginning of a line, most text-editors will take the current line and append it to the previous line. This can be done by using strcat or equivalent and then nulling out the current line.

## Scrolling and Paging
There are multiple ways to handle text lines that do not fit on a single screen. For example, nano scrolls up and down, but pages the whole screen left and right. Vim scrolls up and down, but wraps content left and right. Clunk scrolls both ways, but initially paged.

Most of them involve keeping track of the current scroll or page amount and using that to increment the draw. Something like:
```
        int line = 0;
        node_t *iter = b.head;
        
        for( int i = 0; i < scroll_y; i++ )
            iter = iter->next;
        
        char buffer[128] = { 0 };
        while( iter != NULL ) 
        {
            strncpy(buffer, iter->text + scroll_x, COLS - 1);
            
            mvprintw(line++, 0, iter->text);
            iter = iter->next;
        }
```

These need to be kept track of when modifying the buffer. Paging can be accomplished in a similar way.
