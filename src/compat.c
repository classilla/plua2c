#include "compat.h"

#ifndef api_check
#define api_check(L, o)         /*{ assert(o); }*/
#endif

#define api_incr_top(L)   {api_check(L, L->top < L->ci->top); L->top++;}

void lua_pushclosure (lua_State *L, void *p)
{
  lua_lock(L);
  setclvalue(L->top, p);
  api_incr_top(L);
  lua_unlock(L);
}

void lua_pushtable (lua_State *L, void *p)
{
  lua_lock(L);
  sethvalue(L->top, p);
  api_incr_top(L);
  lua_unlock(L);
}
