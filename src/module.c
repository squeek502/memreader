#include "module.h"
#include "address.h"

#include <psapi.h>

void init_module(module_t * module, HMODULE handle, HANDLE process)
{
	module->handle = handle;
	GetModuleBaseName(process, handle, module->name, sizeof(module->name) / sizeof(TCHAR));
	GetModuleFileNameEx(process, handle, module->path, sizeof(module->path) / sizeof(TCHAR));
}

module_t* push_module(lua_State *L)
{
	module_t *mod = (module_t*)lua_newuserdata(L, sizeof(module_t));
	luaL_getmetatable(L, MODULE_T);
	lua_setmetatable(L, -2);
	return mod;
}

static const luaL_reg module_meta[] = {
	{ NULL, NULL }
};
static const luaL_reg module_methods[] = {
	{ NULL, NULL }
};
static udata_field_info module_getters[] = {
	{ "name", udata_field_get_string, offsetof(module_t, name) },
	{ "path", udata_field_get_string, offsetof(module_t, path) },
	{ "base", udata_field_get_memaddress, offsetof(module_t, handle) },
	{ NULL, NULL }
};
static udata_field_info module_setters[] = {
	{ NULL, NULL }
};

int register_module(lua_State *L)
{
	// store methods in a table outside the metatable
	luaL_newlib(L, module_methods);
	int methods = lua_gettop(L);

	// only put the metamethods in the metatable itself
	luaL_newmetatable(L, MODULE_T);
	luaL_setfuncs(L, module_meta, 0);
	int metatable = lua_gettop(L);

	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methods);
	lua_rawset(L, metatable); // metatable.__metatable = methods

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, metatable);
	register_udata_fields(L, module_getters); // put getters in the metatable
	lua_pushvalue(L, methods);
	// push udata_field_index_handler with the metatable and methods as upvalues
	lua_pushcclosure(L, udata_field_index_handler, 2);
	lua_rawset(L, metatable); // metatable.__index = udata_field_index_handler(metatable, methods)

	lua_pushliteral(L, "__newindex");
	lua_newtable(L); // table for setters
	register_udata_fields(L, module_setters);
	// push udata_field_newindex_handler with the setters table as an upvalue
	lua_pushcclosure(L, udata_field_newindex_handler, 1);
	lua_rawset(L, metatable); // metatable.__newindex = udata_field_newindex_handler(setters)

	lua_pop(L, 2); // pop metatable and methods table
	return 0;
}