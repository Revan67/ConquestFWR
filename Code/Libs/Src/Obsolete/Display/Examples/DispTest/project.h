
#ifndef PROJECT_H
#define PROJECT_H

#include "windows.h"
#include <assert.h>
//#include "resource.h"
#include <stdio.h>		// sprintf
#include <string.h>

#define Warning(msg) MessageBox(0,msg,"Warning:",MB_OK)

#define countof(list) (sizeof(list)/sizeof(list[0]))

#endif // PROJECT_H
