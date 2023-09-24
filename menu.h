#ifndef _MENU_H
#define _MENU_H

enum Cmd { NAME, DRAW, DOWN, UP, POLL, MAIN };

typedef char*(*Device)(Cmd,int,int);

void menu(Device);

#endif //_MENU_H
