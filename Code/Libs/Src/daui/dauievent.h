#ifndef DAUIEVENT_H
#define DAUIEVENT_H
//
// DAUIEvent.h - Standard system events for the DA User Interface system
//

//
// Include files
//

#include <typedefs.h>

//
// Standard user interface messages.
// NOTE: The standard messages all have their high bit set. App specific messages should not use the high bit.
//

const U32 SYS_MESSAGE_START = 0x80000000;

// Mouse messages and structures
const U32 SYS_MOUSE_START = SYS_MESSAGE_START;

const U32 SYS_MOUSE_MOVE = SYS_MOUSE_START + 0;
const U32 SYS_MOUSE_LEFT_DOWN = SYS_MOUSE_START + 1;
const U32 SYS_MOUSE_LEFT_UP = SYS_MOUSE_START + 2;
const U32 SYS_MOUSE_RIGHT_DOWN = SYS_MOUSE_START + 3;
const U32 SYS_MOUSE_RIGHT_UP = SYS_MOUSE_START + 4;
const U32 SYS_KEY_DOWN = SYS_MOUSE_START + 5;
const U32 SYS_KEY_UP = SYS_MOUSE_START + 6;


const U32 SYS_BUTTON_START = SYS_MESSAGE_START + 32;

const U32 SYS_BUTTON_PRESSED = SYS_BUTTON_START + 0;

struct MousePos { int x; int y; };
struct KeyData { unsigned int key; unsigned int modflags; };
struct ControlEvent { const char *name; unsigned long id; };

#endif
