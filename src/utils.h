#ifndef MEMREADER_UTILS_H
#define MEMREADER_UTILS_H

#include "memreader.h"

#define MT_PREFIX(t) "memreader." #t

int push_error(lua_State *L, const char* msg);
int push_last_error(lua_State *L);
const char* get_lua_string(lua_State *L, int index);

// Userdata Field Handling
int udata_field_get_int(lua_State *L, void *v);
int udata_field_set_int(lua_State *L, void *v);
int udata_field_get_string(lua_State *L, void *v);
int udata_field_set_string(lua_State *L, void *v);

typedef int(*udata_field_fn) (lua_State *L, void *v);

typedef const struct {
	const char *name;
	udata_field_fn func; // get or set function for type of member
	size_t offset; // offset of member within the struct
}  udata_field_info;

typedef udata_field_info* udata_field_reg;

void register_udata_fields(lua_State *L, udata_field_reg l);
int udata_field_proxy(lua_State *L);
int udata_field_index_handler(lua_State *L);
int udata_field_newindex_handler(lua_State *L);

#endif