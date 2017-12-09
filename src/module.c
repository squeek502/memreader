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
	UDATA_REGISTER_TYPE_WITH_FIELDS(module, MODULE_T)
}