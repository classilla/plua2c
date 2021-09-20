#ifndef compat_h
#define compat_h

#include "lua.h"
#include "lstate.h"

void lua_pushclosure(lua_State *L, void *p);
void lua_pushtable(lua_State *L, void *p);

#endif
