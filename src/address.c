#include "address.h"


memaddress_t* check_memaddress(lua_State *L, int index)
{
	memaddress_t* addr = (memaddress_t*)luaL_checkudata(L, index, MEMORY_ADDRESS_T);
	return addr;
}

memaddress_t* push_memaddress(lua_State *L)
{
	memaddress_t *addr = (memaddress_t*)lua_newuserdata(L, sizeof(memaddress_t));
	luaL_getmetatable(L, MEMORY_ADDRESS_T);
	lua_setmetatable(L, -2);
	return addr;
}

// for field access
int udata_field_get_memaddress(lua_State *L, void *v)
{
	memaddress_t *memaddress = push_memaddress(L);
	memaddress->ptr = *(LPVOID*)v;
	return 1;
}

static int memaddress_tostring(lua_State *L)
{
	memaddress_t* addr = check_memaddress(L, 1);
	lua_pushfstring(L, "%p", addr->ptr);
	return 1;
}

static const luaL_reg memaddress_meta[] = {
	{ "__tostring", memaddress_tostring },
	{ NULL, NULL }
};

int register_memaddress(lua_State *L)
{
	luaL_newmetatable(L, MEMORY_ADDRESS_T);
	luaL_setfuncs(L, memaddress_meta, 0);
	lua_pop(L, 1);
	return 0;
}