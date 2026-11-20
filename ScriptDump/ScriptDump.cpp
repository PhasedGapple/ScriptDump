#include <iostream>
#include <fstream>
#include <string>
#include <curl/curl.h>
#include <luau/lua.h>
#include <luau/lualib.h>
#include <luau/luacode.h>
#include <random>

std::ofstream file;


std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<int> dist(1000, 9999);


size_t Write(void* data, size_t size, size_t nmemb, std::string* out)
{
    out->append((char*)data, size * nmemb);
    return size * nmemb;
}


int HttpGet(lua_State* L)
{
    CURL* curl = curl_easy_init();
    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL, luaL_checkstring(L, 2));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    lua_pushlstring(L, response.c_str(), response.size());

    return 1;
}

int lua_loadstring(lua_State* L)
{
    size_t len;
    const char* source = luaL_checklstring(L, 1, &len);
    const char* chunkname = luaL_optstring(L, 2, source);

    size_t bytecodeSize;
    char* bytecode = luau_compile(source, len, nullptr, &bytecodeSize);

    int result = luau_load(L, chunkname, bytecode, bytecodeSize, 0);
    free(bytecode);

    if (result == 0)
        return 1;

    lua_pushnil(L);
    lua_insert(L, -2);
    return 2;
}


void dump(lua_State* L, const char* name)
{
    int args = lua_gettop(L);

    file << name << "(\"";

    for (int i = 1; i <= args; i++)
    {
        const char* str = luaL_tolstring(L, i, nullptr);
        file << str;
        lua_pop(L, 1);

        if (i < args)
            file << "\", \"";
    }

    file << "\")\n";
}


int func(lua_State* L)
{
    int args = lua_gettop(L);
    const char* name = lua_tostring(L, lua_upvalueindex(1));
    dump(L, name);

    std::string originalName = "original_";
    originalName += name;
    lua_getglobal(L, originalName.c_str());

    lua_insert(L, 1);
    lua_pcall(L, args, LUA_MULTRET, 0);

    return lua_gettop(L);
}

void hook(lua_State* L, const char* name)
{

    lua_getglobal(L, name);
    std::string originalName = "original_";
    originalName += name;
    lua_setglobal(L, originalName.c_str());


    lua_pushstring(L, name);
    lua_pushcclosure(L, func, name, 1);
    lua_setglobal(L, name);
}


int WaitForChild(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);

    const char* name = luaL_checkstring(L, 2);

    lua_getfield(L, 1, name);

    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        lua_newtable(L);

        lua_getglobal(L, "SharedMeta");
        lua_setmetatable(L, -2);

        lua_pushvalue(L, -1);
        lua_setfield(L, 1, name);
    }

    return 1;
}

void registerEnum(lua_State* L)
{
    lua_newtable(L); // Enum
    lua_newtable(L); // Enum.ZIndexBehavior
    lua_newtable(L); // Enum.ZIndexBehavior.Global

    lua_pushstring(L, "Global");
    lua_setfield(L, -2, "Name");

    lua_setfield(L, -2, "Global");
    lua_newtable(L); // Enum.ZIndexBehavior.Sibling

    lua_pushstring(L, "Sibling");
    lua_setfield(L, -2, "Name");

    lua_setfield(L, -2, "Sibling");

    lua_setfield(L, -2, "ZIndexBehavior");
    
    lua_setglobal(L, "Enum");
}

void registerGame(lua_State* L)
{
    lua_newtable(L); // Shared Metatable (Stores functions which are globally avaiable for the game table and its subtables)
    lua_pushvalue(L, -1);
    lua_setglobal(L, "SharedMeta");
    lua_pushcfunction(L, WaitForChild, "WaitForChild");
    lua_setfield(L, -2, "WaitForChild");

    lua_newtable(L);
    lua_pushvalue(L, -2);
    lua_setfield(L, -2, "__index");

    
    lua_newtable(L); // game
    lua_pushvalue(L, -2);
    lua_setmetatable(L, -2);

    lua_pushcfunction(L, HttpGet, "HttpGet");
    lua_setfield(L, -2, "HttpGet");

    // How does stacking work in this shi??????  I DONT UNDERSTAND LUAU IN C++
    lua_newtable(L); // game.Players
    lua_pushvalue(L, -3);
    lua_setmetatable(L, -2);

    lua_newtable(L); // game.Players.LocalPlayer
    lua_pushvalue(L, -4);
    lua_setmetatable(L, -2);

    lua_setfield(L, -2, "LocalPlayer");
    lua_setfield(L, -2, "Players");

    lua_setglobal(L, "game");

    lua_pop(L, 2);
}

// FindTable is AI-generated
bool FindTable(lua_State* L, int parent, int target, std::string path)
{
    parent = lua_absindex(L, parent);
    target = lua_absindex(L, target);

    lua_pushnil(L);

    while (lua_next(L, parent) != 0)
    {
        if (lua_istable(L, -1))
        {
            std::string name = path + "." + lua_tostring(L, -2);

            if (lua_rawequal(L, -1, target))
            {
                lua_pop(L, 2);
                file << name << std::endl;
                return true;
            }

            if (FindTable(L, -1, target, name))
            {
                lua_pop(L, 2);
                return true;
            }
        }

        lua_pop(L, 1);
    }

    return false;
}


int InstanceNew(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    
    lua_newtable(L);
    int storage = lua_gettop(L);

    lua_pushstring(L, name);
    lua_setfield(L, -2, "type");

    int id = dist(gen);

    lua_newtable(L);
    int proxy = lua_gettop(L);
    lua_newtable(L);

    lua_pushstring(L, std::string("i" + std::to_string(id)).c_str());
    lua_setfield(L, storage, "id");
    file << "local i" << std::to_string(id) << " = Instance.new(\"" << name << "\")\n";

    lua_pushvalue(L, storage);
    lua_pushcclosure(L, [](lua_State* L) -> int {
        lua_pushvalue(L, 2);
        lua_rawget(L, lua_upvalueindex(1));
        return 1;
        }, "InstanceIndex", 1);
    lua_setfield(L, -2, "__index");

    lua_pushvalue(L, storage);
    lua_pushstring(L, std::to_string(id).c_str());
    lua_pushcclosure(L, [](lua_State* L) -> int {
        const char* key = luaL_checkstring(L, 2);

        file << "i" << lua_tostring(L, lua_upvalueindex(2))
            << "." << key << " = ";

        if (lua_isnumber(L, 3)) {
            file << lua_tostring(L, -1) << "\n";
        }
        else if (lua_istable(L, 3)) {
            lua_getglobal(L, "game");
            if (!FindTable(L, -1, 3, "game"))
            {
                lua_pop(L, 1);
                lua_getglobal(L, "Enum");
                if (!FindTable(L, -1, 3, "Enum")) {
                    lua_pop(L, 1);
                    lua_getfield(L, 3, "id");
                    if (!lua_isnil(L, -1)) {
                        const char* id = lua_tostring(L, -1);
                        file << id;
                    }
                    else
                        file << "nil\n";
                }
            }
        }
        else if (lua_isstring(L, 3)) {
            luaL_tolstring(L, 3, nullptr);
            file << "\"" << lua_tostring(L, -1) << "\"\n";
        }
        else if (lua_isboolean(L, 3)) {
            luaL_tolstring(L, 3, nullptr);
            file << lua_tostring(L, -1) << "\n";
        }

        lua_pop(L, 1);

        lua_pushvalue(L, 2);
        lua_pushvalue(L, 3);
        lua_rawset(L, lua_upvalueindex(1));

        return 0;
        }, "InstanceChanged", 2);
    lua_setfield(L, -2, "__newindex");

    lua_setmetatable(L, proxy);
    lua_remove(L, storage);

    return 1;
}

void Instance(lua_State* L)
{
    lua_newtable(L);

    lua_pushcfunction(L, InstanceNew, "new");
    lua_setfield(L, -2, "new");
    lua_pushcfunction(L, InstanceNew, "New");
    lua_setfield(L, -2, "New");

    lua_setglobal(L, "Instance");
}


int main()
{
    file.open("dump.lua");
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    lua_pushcfunction(L, lua_loadstring, "loadstring");
    lua_setglobal(L, "loadstring");
    
    hook(L, "print");
    hook(L, "loadstring");

    registerEnum(L);
    registerGame(L);
    Instance(L);

    const char* code = R"(
    loadstring(game:HttpGet("https://raw.githubusercontent.com/XNEOFF/FlyGuiV3/main/FlyGuiV3.txt"))()
)";

    size_t bytecodeSize;
    char* bytecode = luau_compile(code, strlen(code), nullptr, &bytecodeSize);
    
    if (luau_load(L, "script", bytecode, bytecodeSize, 0) == 0)
        lua_pcall(L, 0, 0, 0);
    else
        std::cerr << lua_tostring(L, -1) << '\n';

    free(bytecode);
    lua_close(L);
}