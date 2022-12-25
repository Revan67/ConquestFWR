//
// <input.h>
//

#ifndef INPUT_H
#define INPUT_H

struct KeyboardMessage
{
    BOOL32  pressed;
    U32     key;
};

struct MouseMessage
{
    U32     message;
    S32     x, y;
    BOOL32  left_pressed;
    BOOL32  right_pressed;
};

typedef void (* KBCallback)(KeyboardMessage kb);

struct KeyboardDriver
{
    BOOL32 keyboard_array[1024];

    KBCallback notify;
    
    KeyboardDriver()
    {
        memset(keyboard_array, 0, sizeof(BOOL32) * 1024);
        notify = NULL;
    }

    BOOL32 pressed(DWORD key)
    {
        return keyboard_array[key];
    }
           
};

extern KeyboardDriver *keyboard;

#endif