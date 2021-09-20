#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "lua.h"
#include "compat.h"
#include "lauxlib.h"
#include "lualib.h"
#include "ltable.h"
#include "lstring.h"
#include "ldo.h"

static int lua_pack(lua_State *L);
static int lua_unpack(lua_State *L);
static void invert(unsigned char *buf, int size);

static int lua_pack(lua_State *L)
{
  char c, *format;
  int i, n, k, len, size, arg, count;
  int littleEndian;
  unsigned char *buf, *ptr;
  unsigned char b;
  unsigned short w;
  unsigned long l;
  float f;
  double d;
  char *s;

  format = (char *)luaL_check_string(L, 1);

  if (!lua_istable(L, 2)) {
    errno = EINVAL;
    return 0;
  }

  if ((n = luaL_getn(L, 2)) == 0) {
    errno = EINVAL;
    return 0;
  }

  littleEndian = lua_isnone(L, 3) ? 0 : (int)lua_toboolean(L, 3);
  errno = 0;

  for (i = 0, size = 0, arg = 1, count = 0; arg <= n && (c = format[i]); i++) {
    if (isdigit(c))
      count = count*10 + c - '0';
    else {
      lua_rawgeti(L, 2, arg++);
      if (c != 'S' && count == 0)
        count = 1;
      switch (c) {
        case 'b':
        case 'B': size += 1*count; break;
        case 'w':
        case 'W': size += 2*count; break;
        case 'l':
        case 'L': size += 4*count; break;
        case 'F': size += 4*count; break;
        case 'D': size += 8*count; break;
        case 'S':
          if (count == 0) {
            s = (char *)luaL_check_string(L, lua_gettop(L));
            size += strlen(s)+1;
          } else
            size += count;
          break;
        default:
          errno = EINVAL;
          return 0;
      }
      count = 0;
      lua_pop(L, 1);
    }
  }

  if ((buf = malloc(size)) == NULL) {
    errno = ENOMEM;
    return 0;
  }

  for (i = 0, ptr = buf, arg = 1, count = 0; arg <= n && (c = format[i]); i++) {
    if (isdigit(c))
      count = count*10 + c - '0';
    else {
      if (c != 'S' && count == 0)
        count = 1;
      switch (c) {
        case 'b': // signed byte 
        case 'B': // byte 
          for (k = 0; k < count; k++) {
            lua_rawgeti(L, 2, arg++);
            b = (unsigned char)luaL_check_int(L, lua_gettop(L));
            *ptr = b;
            ptr += 1;
            lua_pop(L, 1);
          }
          break;
        case 'w': // signed word (16 bits) 
        case 'W': // word (16 bits)
          for (k = 0; k < count; k++) {
            lua_rawgeti(L, 2, arg++);
            w = (unsigned short)luaL_check_int(L, lua_gettop(L));
            if (littleEndian) invert((unsigned char *)&w, sizeof(short));
            (void)memcpy(ptr, &w, sizeof(short));
            ptr += 2;
            lua_pop(L, 1);
          }
          break;
        case 'l': // signed long word (32 bits) 
        case 'L': // long word (32 bits)
          for (k = 0; k < count; k++) {
            lua_rawgeti(L, 2, arg++);
            l = (unsigned long)luaL_check_long(L, lua_gettop(L));
            if (littleEndian) invert((unsigned char *)&l, sizeof(long));
            (void)memcpy(ptr, &l, sizeof(long));
            ptr += 4;
            lua_pop(L, 1);
          }
          break;
        case 'F': // float
          for (k = 0; k < count; k++) {
            lua_rawgeti(L, 2, arg++);
            f = (float)luaL_check_number(L, lua_gettop(L));
            if (littleEndian) invert((unsigned char *)&f, sizeof(float));
            (void)memcpy(ptr, &f, sizeof(float));
            ptr += 4;
            lua_pop(L, 1);
          }
          break;
        case 'D': // double
          for (k = 0; k < count; k++) {
            lua_rawgeti(L, 2, arg++);
            d = luaL_check_number(L, lua_gettop(L));
            if (littleEndian) invert((unsigned char *)&d, sizeof(double));
            (void)memcpy(ptr, &d, sizeof(double));
            ptr += 8;
            lua_pop(L, 1);
          }
          break;
        case 'S': // string
          lua_rawgeti(L, 2, arg++);
          s = (char *)luaL_check_string(L, lua_gettop(L));
          if (count == 0) {
            len = strlen(s)+1;
            (void)memcpy(ptr, s, len);
            ptr += len;
          } else {
            len = strlen(s);
            if (len > count)
              len = count;
            (void)memcpy(ptr, s, len);
            ptr += count;
          }
          lua_pop(L, 1);
      }
      count = 0;
    }
  }

  lua_pushlstring(L, (char *)buf, size);
  free(buf);
  return 1;
}

static int lua_unpack(lua_State *L)
{
  char c, *format;
  int i, k, n, len, arg, count, start;
  unsigned char *buf, *ptr, *end;
  unsigned char b;
  unsigned short w;
  unsigned long l;
  float f;
  double d;
  Table *table;
  TObject *value;
  lua_Number ln;

  errno = 0;
  format = (char *)luaL_check_string(L, 1);
  buf = (unsigned char *)luaL_check_string(L, 2);
  start = luaL_opt_int(L, 3, 0);
  end = buf + lua_strlen(L, 2);

  if ((n = lua_strlen(L, 1)) <= 0) {
    errno = EINVAL;
    return 0;
  }

  if (start < 0 || buf+start >= end) {
    errno = EINVAL;
    return 0;
  }

  table = luaH_new(L, n, 0);

  for (i = 0, ptr = buf, arg = 1, count = 0; (c = format[i]); i++)
    if (isdigit(c))
      count = count*10 + c - '0';
    else {
      if (c != 'S' && count == 0)
        count = 1;
      switch (c) {
        case 'b': // signed byte 
        case 'B': // byte 
          for (k = 0; k < count; k++) {
            if (ptr+1 > end)
              b = 0;
            else
              b = *ptr;
            value = luaH_setnum(L, table, arg++);
            ttype(value) = LUA_TNUMBER;
            if (c == 'b') ln = (char)b; else ln = b;
            nvalue(value) = ln;
            ptr += 1;
          }
          break;
        case 'w': // signed word (16 bits)
        case 'W': // word (16 bits)
          for (k = 0; k < count; k++) {
            if (ptr+2 > end)
              w = 0;
            else
              (void)memcpy(&w, ptr, 2);
            value = luaH_setnum(L, table, arg++);
            ttype(value) = LUA_TNUMBER;
            if (c == 'w') ln = (short)w; else ln = w;
            nvalue(value) = ln;
            ptr += 2;
          }
          break;
        case 'l': // signed long word (32 bits)
        case 'L': // long word (32 bits)
          for (k = 0; k < count; k++) {
            if (ptr+4 > end)
              l = 0;
            else
              (void)memcpy(&l, ptr, 4);
            value = luaH_setnum(L, table, arg++);
            ttype(value) = LUA_TNUMBER;
            if (c == 'l') ln = (long)l; else ln = l;
            nvalue(value) = ln;
            ptr += 4;
          }
          break;
        case 'F': // float
          for (k = 0; k < count; k++) {
            if (ptr+4 > end)
              f = 0.0;
            else
              (void)memcpy(&f, ptr, 4);
            value = luaH_setnum(L, table, arg++);
            ttype(value) = LUA_TNUMBER;
            nvalue(value) = f;
            ptr += 4;
          }
          break;
        case 'D': // double
          for (k = 0; k < count; k++) {
            if (ptr+8 > end)
              d = 0.0;
            else
              (void)memcpy(&d, ptr, 8);
            value = luaH_setnum(L, table, arg++);
            ttype(value) = LUA_TNUMBER;
            nvalue(value) = d;
            ptr += 8;
          }
          break;
        case 'S': // string
          value = luaH_setnum(L, table, arg++);
          ttype(value) = LUA_TSTRING;

          if (ptr < end) {
            if (count == 0) {
              len = strlen((char *)ptr)+1;
              setsvalue(value, luaS_new(L, (char *)ptr));
              ptr += len;
            } else {
              setsvalue(value, luaS_newlstr(L, (char *)ptr, count));
              ptr += count;
            }
          } else
            setsvalue(value, luaS_new(L, ""));
      }
      count = 0;
    }

  lua_pushtable(L, table);
  return 1;
}

#ifdef PALMOS
static int lua_md5(lua_State *L)
{
  UInt8 *s;
  size_t n;
  UInt8 digest[16];

  s = (UInt8 *)luaL_checklstring(L, 1, &n);
  if (EncDigestMD5(s, n, digest) != 0)
    return 0;

  lua_pushlstring(L, (char *)digest, 16);
  return 1;
}
#endif

static void invert(unsigned char *buf, int size)
{
  int i;
  unsigned char aux[8];

  if (size > 8) size = 8;
  (void)memcpy(aux, buf, size);
  for (i = 0; i < size; i++)
    buf[size-i-1] = aux[i];
}

static const struct luaL_reg binlib[] = {
  {"pack", lua_pack},
  {"unpack", lua_unpack},
#ifdef PALMOS
  {"md5", lua_md5},
#endif
  {NULL, NULL}
};

LUALIB_API int luaopen_pack (lua_State *L) {
  luaL_openlib(L, "bin", binlib, 0);
  return 1;
}
