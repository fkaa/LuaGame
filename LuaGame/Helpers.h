#pragma once

#include <string>
#include <sstream>

#include "../Lua/lua.hpp"

#define TRACE( s )            \
{                             \
   std::ostringstream os_;    \
   os_ << s << std::endl;                   \
   OutputDebugStringA( os_.str().c_str() );  \
}

class LuaException {
public:
	LuaException(lua_State *L) {
		err = std::string(lua_tostring(L, -1));
		TRACE(err);
	}

	virtual const char* what() const throw()
	{
		return err.c_str();
	}
private:
	std::string err;
};