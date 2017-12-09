#include "window.h"

window_t* push_window(lua_State *L)
{
	window_t *window = (window_t*)lua_newuserdata(L, sizeof(window_t));
	luaL_getmetatable(L, WINDOW_T);
	lua_setmetatable(L, -2);
	return window;
}

void init_window(window_t * window, HWND handle, const LPTSTR title)
{
	window->handle = handle;
	GetWindowThreadProcessId(handle, &window->processId);
	GetWindowModuleFileName(handle, window->fileName, sizeof(window->fileName) / sizeof(TCHAR));
	if (!title)
		GetWindowText(handle, window->title, sizeof(window->title) / sizeof(TCHAR));
	else
		strncpy_s(window->title, sizeof(window->title), title, sizeof(window->title) / sizeof(TCHAR));
}

static const luaL_reg window_meta[] = {
	{ NULL, NULL }
};
static const luaL_reg window_methods[] = {
	{ NULL, NULL }
};
static udata_field_info window_getters[] = {
	{ "filename", udata_field_get_string, offsetof(window_t, fileName) },
	{ "title", udata_field_get_string, offsetof(window_t, title) },
	{ "pid", udata_field_get_int, offsetof(window_t, processId) },
	{ NULL, NULL }
};
static udata_field_info window_setters[] = {
	{ NULL, NULL }
};

int register_window(lua_State *L)
{
	// store methods in a table outside the metatable
	luaL_newlib(L, window_methods);
	int methods = lua_gettop(L);

	// only put the metamethods in the metatable itself
	luaL_newmetatable(L, WINDOW_T);
	luaL_setfuncs(L, window_meta, 0);
	int metatable = lua_gettop(L);

	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methods);
	lua_rawset(L, metatable); // metatable.__metatable = methods

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, metatable);
	register_udata_fields(L, window_getters); // put getters in the metatable
	lua_pushvalue(L, methods);
	// push udata_field_index_handler with the metatable and methods as upvalues
	lua_pushcclosure(L, udata_field_index_handler, 2);
	lua_rawset(L, metatable); // metatable.__index = udata_field_index_handler(metatable, methods)

	lua_pushliteral(L, "__newindex");
	lua_newtable(L); // table for setters
	register_udata_fields(L, window_setters);
	// push udata_field_newindex_handler with the setters table as an upvalue
	lua_pushcclosure(L, udata_field_newindex_handler, 1);
	lua_rawset(L, metatable); // metatable.__newindex = udata_field_newindex_handler(setters)

	lua_pop(L, 2); // pop metatable and methods table
	return 0;
}