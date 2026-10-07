// Experimental Win64 benchmark compatibility shim: Lua 5.4-facing UnLua to LuaJIT 2.1.
// Changes are isolated to this plugin candidate; they do not imply upstream UnLua support.
#pragma once
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4668)
#endif
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#include <cmath>

#ifndef LUA_OK
#define LUA_OK 0
#endif
#ifndef LUA_GNAME
#define LUA_GNAME "_G"
#endif
#define LUA_OPEQ 0
inline int lua_compare(lua_State* L, int Left, int Right, int Operation)
{
    if (Operation == LUA_OPEQ) { return lua_equal(L, Left, Right); }
    return luaL_error(L, "Benchmark LuaJIT adapter supports equality comparison only");
}
inline int lua_absindex(lua_State* L, int Index)
{
    return Index > 0 || Index <= LUA_REGISTRYINDEX ? Index : lua_gettop(L) + Index + 1;
}
inline size_t lua_rawlen(lua_State* L, int Index) { return lua_objlen(L, Index); }
inline int lua_isinteger(lua_State* L, int Index)
{
    if (lua_type(L, Index) != LUA_TNUMBER) { return 0; }
    const lua_Number Number = lua_tonumber(L, Index);
    return std::isfinite(Number) && Number == static_cast<lua_Number>(lua_tointeger(L, Index));
}
inline int BenchLuaGetField(lua_State* L, int Index, const char* Key)
{
    (lua_getfield)(L, Index, Key); return lua_type(L, -1);
}
inline int BenchLuaGetTable(lua_State* L, int Index)
{
    (lua_gettable)(L, Index); return lua_type(L, -1);
}
inline int BenchLuaRawGet(lua_State* L, int Index)
{
    (lua_rawget)(L, Index); return lua_type(L, -1);
}
inline int BenchLuaRawGetI(lua_State* L, int Index, lua_Integer Key)
{
    (lua_rawgeti)(L, Index, static_cast<int>(Key)); return lua_type(L, -1);
}
inline int BenchLuaResume(lua_State* L, lua_State*, int Arguments)
{
    return (lua_resume)(L, Arguments);
}
inline int BenchLuaNewMetatable(lua_State* L, const char* Name)
{
    const int Created = (luaL_newmetatable)(L, Name);
    if (Created)
    {
        lua_pushstring(L, Name);
        lua_setfield(L, -2, "__name");
    }
    return Created;
}
#define lua_getfield BenchLuaGetField
#define lua_gettable BenchLuaGetTable
#define lua_rawget BenchLuaRawGet
#define lua_rawgeti BenchLuaRawGetI
#define lua_resume BenchLuaResume
#define luaL_newmetatable BenchLuaNewMetatable

inline void lua_pushglobaltable(lua_State* L) { lua_pushvalue(L, LUA_GLOBALSINDEX); }
inline void lua_copy(lua_State* L, int From, int To)
{
    To = lua_absindex(L, To); lua_pushvalue(L, From); lua_replace(L, To);
}
inline int lua_geti(lua_State* L, int Index, lua_Integer Key)
{
    Index = lua_absindex(L, Index); lua_pushinteger(L, Key); return lua_gettable(L, Index);
}
inline void lua_seti(lua_State* L, int Index, lua_Integer Key)
{
    Index = lua_absindex(L, Index); lua_pushinteger(L, Key); lua_insert(L, -2); lua_settable(L, Index);
}
inline int lua_rawgetp(lua_State* L, int Index, const void* Key)
{
    Index = lua_absindex(L, Index); lua_pushlightuserdata(L, const_cast<void*>(Key)); return lua_rawget(L, Index);
}
inline void lua_rawsetp(lua_State* L, int Index, const void* Key)
{
    Index = lua_absindex(L, Index); lua_pushlightuserdata(L, const_cast<void*>(Key)); lua_insert(L, -2); lua_rawset(L, Index);
}
inline int lua_getuservalue(lua_State* L, int Index) { lua_getfenv(L, Index); return lua_type(L, -1); }
inline void lua_setuservalue(lua_State* L, int Index) { lua_setfenv(L, Index); }
inline const char* luaL_tolstring(lua_State* L, int Index, size_t* Length)
{
    if (!luaL_callmeta(L, Index, "__tostring"))
    {
        switch (lua_type(L, Index))
        {
        case LUA_TNUMBER: case LUA_TSTRING: lua_pushvalue(L, Index); break;
        case LUA_TBOOLEAN: lua_pushstring(L, lua_toboolean(L, Index) ? "true" : "false"); break;
        case LUA_TNIL: lua_pushliteral(L, "nil"); break;
        default: lua_pushfstring(L, "%s: %p", luaL_typename(L, Index), lua_topointer(L, Index)); break;
        }
    }
    return lua_tolstring(L, -1, Length);
}
inline void luaL_requiref(lua_State* L, const char* Name, lua_CFunction Open, int Global)
{
    lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
    lua_getfield(L, -1, Name);
    if (!lua_toboolean(L, -1))
    {
        lua_pop(L, 1); lua_pushcfunction(L, Open); lua_pushstring(L, Name); lua_call(L, 1, 1);
        lua_pushvalue(L, -1); lua_setfield(L, -3, Name);
    }
    lua_remove(L, -2);
    if (Global) { lua_pushvalue(L, -1); lua_setglobal(L, Name); }
}
