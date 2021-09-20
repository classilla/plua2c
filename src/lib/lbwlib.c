#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

static int and_b (lua_State *L);
static int or_b (lua_State *L);
static int not_b (lua_State *L);
static int xor_b (lua_State *L);

static int and_b (lua_State *L) {
  unsigned long n1 = luaL_check_long(L, 1);
  unsigned long n2 = luaL_check_long(L, 2);
  lua_pushnumber(L, n1 & n2);
  return 1;
}

static int or_b (lua_State *L) {
  unsigned long n1 = luaL_check_long(L, 1);
  unsigned long n2 = luaL_check_long(L, 2);
  lua_pushnumber(L, n1 | n2);
  return 1;
}

static int not_b (lua_State *L) {
  unsigned long n = luaL_check_long(L, 1);
  lua_pushnumber(L, ~n);
  return 1;
}

static int xor_b (lua_State *L) {
  unsigned long n1 = luaL_check_long(L, 1);
  unsigned long n2 = luaL_check_long(L, 2);
  lua_pushnumber(L, n1 ^ n2);
  return 1;
}

static const struct luaL_reg bwlib[] = {
  {"andb", and_b},
  {"orb", or_b},
  {"notb", not_b},
  {"xorb", xor_b},
  {NULL, NULL}
};

LUALIB_API int luaopen_bitwise (lua_State *L) {
  luaL_openlib(L, "bit", bwlib, 0);
  return 1;
}
