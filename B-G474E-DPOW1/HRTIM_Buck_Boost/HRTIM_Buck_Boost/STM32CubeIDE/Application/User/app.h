#ifndef APP_H
#define APP_H

#include "main.h"        // for handles, enums

typedef enum {
    APP_MODE_BUCK,
    APP_MODE_BOOST,
    APP_MODE_DE_ENERGIZE,
    APP_MODE_FAULT
} AppMode_t;

extern AppMode_t appMode;

void APP_Init(void);
void APP_Task(void);

#endif // APP_H
