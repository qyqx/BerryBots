/*
** Library initialization.
** Copyright (C) 2005-2013 Mike Pall. See Copyright Notice in luajit.h
**
** Major parts taken verbatim from the Lua interpreter.
** Copyright (C) 1994-2008 Lua.org, PUC-Rio. See Copyright Notice in lua.h
*/

#define lib_init_c
#define LUA_LIB

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "lj_arch.h"

static const luaL_Reg lj_lib_load[] = {
  { "",			luaopen_base },
  { LUA_LOADLIBNAME,	luaopen_package },
  { LUA_TABLIBNAME,	luaopen_table },
  // @Voidious: Disable I/O for BerryBots, only allow dofile/loadfile/require
  //            from below base directory.
  //{ LUA_IOLIBNAME,	luaopen_io },
  // @Voidious: Disable OS library for BerryBots.
  //{ LUA_OSLIBNAME,	luaopen_os },
  { LUA_STRLIBNAME,	luaopen_string },
  { LUA_MATHLIBNAME,	luaopen_math },
  // @Voidious: Disable debug library for BerryBots.
  //{ LUA_DBLIBNAME,	luaopen_debug },
  { LUA_BITLIBNAME,	luaopen_bit },
  // @Voidious: Disable JIT control library for BerryBots.
  //{ LUA_JITLIBNAME,	luaopen_jit },
  { NULL,		NULL }
};

static const luaL_Reg lj_lib_preload[] = {
#if LJ_HASFFI
  { LUA_FFILIBNAME,	luaopen_ffi },
#endif
  { NULL,		NULL }
};

LUALIB_API void luaL_openlibs(lua_State *L)
{
  const luaL_Reg *lib;
  for (lib = lj_lib_load; lib->func; lib++) {
    lua_pushcfunction(L, lib->func);
    lua_pushstring(L, lib->name);
    lua_call(L, 1, 0);
  }
  luaL_findtable(L, LUA_REGISTRYINDEX, "_PRELOAD",
		 sizeof(lj_lib_preload)/sizeof(lj_lib_preload[0])-1);
  for (lib = lj_lib_preload; lib->func; lib++) {
    lua_pushcfunction(L, lib->func);
    lua_setfield(L, -2, lib->name);
  }
  lua_pop(L, 1);
}

