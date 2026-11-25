#include <iostream>
#include <fstream>
#include <string>
#include <curl/curl.h>
#include <luau/lua.h>
#include <luau/lualib.h>
#include <luau/luacode.h>
#include <luau/lobject.h>
#include <random>

std::ofstream file;
std::ofstream loadstringfile;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<int> dist(1000, 9999);

std::string lastid;


int InstanceNew(lua_State* L);


size_t Write(void* data, size_t size, size_t nmemb, std::string* out)
{
    out->append((char*)data, size * nmemb);
    return size * nmemb;
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
                file << name;
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

// This function is heavily modified by AI
int Connect(lua_State* L)
{
    luaL_checktype(L, 2, LUA_TFUNCTION);

    if (!lua_isLfunction(L, 2))
    {
        std::cout << "Faulty code! An object which isnt a variable has been passed to :Connect()\n";
        return 0;
    }

    lua_getfield(L, 1, "_eventname");
    const char* event = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "_owner");
    lua_getglobal(L, "game");

    bool found = FindTable(L, -1, -2, "game");

    lua_pop(L, 2);

    if (!found)
    {
        lua_getfield(L, 1, "id");
        file << lua_tostring(L, -1);
        lua_pop(L, 1);
    }

    lua_pop(L, 1);

    file << "." << event << ":Connect(function() end) -- functions being sent to :Connect aren't logged.\n";

    return 0;
}

int Wait(lua_State* L)
{
    return 0;
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


void ldump(lua_State* L, const char* name) // used for loadstring only!
{
    int args = lua_gettop(L);

    loadstringfile << name << "(\"";

    for (int i = 1; i <= args; i++)
    {
        const char* str = luaL_tolstring(L, i, nullptr);
        loadstringfile << str;
        lua_pop(L, 1);

        if (i < args)
            loadstringfile << "\", \"";
    }

    loadstringfile << "\")\n";
}


int lfunc(lua_State* L) // used for loadstring only!
{
    int args = lua_gettop(L);
    const char* name = lua_tostring(L, lua_upvalueindex(1));
    ldump(L, name);

    std::string originalName = "original_";
    originalName += name;
    lua_getglobal(L, originalName.c_str());

    lua_insert(L, 1);
    lua_pcall(L, args, LUA_MULTRET, 0);

    return lua_gettop(L);
}

void hook(lua_State* L, const char* name, bool isloadstring = false)
{
    lua_getglobal(L, name);
    std::string originalName = "original_";
    originalName += name;
    lua_setglobal(L, originalName.c_str());

    lua_pushstring(L, name);
    if (isloadstring)
        lua_pushcclosure(L, lfunc, "name", 1);
    else
        lua_pushcclosure(L, func, name, 1);

    lua_setglobal(L, name);
}

// i took some improvements from ai for this function
int WaitForChild(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    const char* name = luaL_checkstring(L, 2);

    lua_getfield(L, 1, name);

    if (!lua_isnil(L, -1))
        return 1;

    lua_pop(L, 1);

    lua_pushcfunction(L, InstanceNew, "new");
    lua_pushstring(L, ("!" + std::string(name)).c_str());
    lua_call(L, 1, 1);

    lua_pushstring(L, name);
    lua_pushvalue(L, -2);
    lua_rawset(L, 1);
     
    return 1;
}

// GPTChat told me to use this function instead for GetService
int GetService(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    const char* name = luaL_checkstring(L, 2);

    lua_getfield(L, 1, name);

    if (!lua_isnil(L, -1))
        return 1;

    lua_pop(L, 1);

    lua_pushcfunction(L, InstanceNew, "new");
    lua_pushstring(L, ("!" + std::string(name)).c_str());
    lua_call(L, 1, 1);

    lua_pushvalue(L, -1);
    lua_setfield(L, 1, name);

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


    lua_newtable(L); // Enum.Font - soo many fonts😭ALSO I DIDNT USE AI SO DONT CALL THIS LINE AI BECAUSE I PUT EMOJI

    const char* fonts[] = {
        "Legacy", "Arial", "ArialBold",
        "SourceSans", "SourceSansBold", "SourceSansLight",
        "SourceSansItalic", "Bodoni", "Garamond",
        "Cartoon", "Code", "Highway", "SciFi",
        "Arcade", "Fantasy", "Antique",
        "SourceSansSemibold",
        "Gotham", "GothamMedium", "GothamBold", "GothamSemibold", "GothamBlack",
        "AmaticSC", "Bangers", "Creepster", "DenkOne",
        "Fondamento", "FredokaOne", "GrenzeGotisch",
        "IndieFlower", "JosefinSans", "Jura", "Kalam",
        "LuckiestGuy", "Merriweather", "Michroma", "Nunito",
        "Oswald", "PatrickHand", "PermanentMarker",
        "Roboto", "RobotoCondensed", "RobotoMono",
        "Sarpanch", "SpecialElite", "TitilliumWeb", "Ubuntu",
        "BuilderSans", "BuilderSansMedium", "BuilderSansBold",
        "BuilderSansExtraBold", "Arimo", "ArimoBold",
        "Unknown"
    };

    for (const char* font : fonts)
    {
        lua_newtable(L);

        lua_pushstring(L, font);
        lua_setfield(L, -2, "Name");

        lua_setfield(L, -2, font);
    }
    
    lua_setfield(L, -2, "Font");

    lua_newtable(L); // Enum.ScaleType

    const char* ScaleTypes[] = {
        "Stretch", "Slice", "Tile", "Fit", "Crop"
    };

    for (const char* ScaleType : ScaleTypes)
    {
        lua_newtable(L);

        lua_pushstring(L, ScaleType);
        lua_setfield(L, -2, "Name");
        
        lua_setfield(L, -2, ScaleType);
    }

    lua_setfield(L, -2, "ScaleType");

    lua_newtable(L); // Enum.TextXAlignment

    const char* TextXAlignments[] = {
        "Left", "Right", "Center"
    };

    for (const char* alignment : TextXAlignments)
    {
        lua_newtable(L);
        
        lua_pushstring(L, alignment);
        lua_setfield(L, -2, "Name");

        lua_setfield(L, -2, alignment);
    }

    lua_setfield(L, -2, "TextXAlignment");

    lua_newtable(L); // Enum.TextYAlignment

    const char* TextYAlignments[] = {
        "Top", "Bottom", "Center"
    };

    for (const char* alignment : TextYAlignments)
    {
        lua_newtable(L);

        lua_pushstring(L, alignment);
        lua_setfield(L, -2, "Name");

        lua_setfield(L, -2, alignment);
    }

    lua_setfield(L, -2, "TextYAlignment");

    lua_newtable(L); // Enum.SortOrder
    
    const char* SortOrders[] = {
        "Name", "LayoutOrder"
    };

    for (const char* order : SortOrders)
    {
        lua_newtable(L);
        
        lua_pushstring(L, order);
        lua_setfield(L, -2, "Name");

        lua_setfield(L, -2, order);
    }

    lua_setfield(L, -2, "SortOrder");

    lua_newtable(L); // Enum.HorizontalAlignment

    const char* HZAlignments[] = {
        "Center", "Left", "Right"
    };

    for (const char* alignment : HZAlignments)
    {
        lua_newtable(L);

        lua_pushstring(L, alignment);
        lua_setfield(L, -2, "Name");

        lua_setfield(L, -2, alignment);
    }

    lua_setfield(L, -2, "HorizontalAlignment");

    lua_newtable(L); // Enum.VerticalAlignment

    const char* VAlignments[] = {
        "Top", "Bottom", "Center"
    };

    for (const char* alignment : VAlignments)
    {
        lua_newtable(L);

        lua_pushstring(L, alignment);
        lua_setfield(L, -2, "Name");

        lua_setfield(L, -2, alignment);
    }

    lua_setfield(L, -2, "VerticalAlignment");

    lua_newtable(L); // Enum.StartCorner

    const char* Corners[] = {
        "TopLeft", "TopRight", "BottomLeft", "BottomRight"
    };

    for (const char* corner : Corners)
    {
        lua_newtable(L);

        lua_pushstring(L, corner);
        lua_setfield(L, -2, "Name");

        lua_setfield(L, -2, corner);
    }

    lua_setfield(L, -2, "StartCorner");

    lua_newtable(L); // Enum.KeyCode

    const char* KeyCodes[] = {
        "Unknown", "A", "B", "C", "D", "E", "F", "G",
        "H", "I", "J", "K", "L", "M", "N", "O",
        "P", "Q", "R", "S", "T", "U", "V", "W",
        "X", "Y", "Z",
        "One", "Two", "Three", "Four", "Five",
        "Six", "Seven", "Eight", "Nine", "Zero",
        "Space", "Return", "Escape", "Backspace", "Tab",
        "LeftShift", "RightShift", "LeftControl", "RightControl",
        "LeftAlt", "RightAlt",
        "Up", "Down", "Left", "Right",
        "Insert", "Delete", "Home", "End",
        "PageUp", "PageDown",
        "F1", "F2", "F3", "F4", "F5", "F6",
        "F7", "F8", "F9", "F10", "F11", "F12"
    };

    for (const char* Key : KeyCodes)
    {
        lua_newtable(L);

        lua_pushstring(L, Key);
        lua_setfield(L, -2, "Name");

        lua_setfield(L, -2, Key);
    }

    lua_setfield(L, -2, "KeyCode");

    lua_setglobal(L, "Enum");
}

int SetCore(lua_State* L)
{
    const char* key = luaL_checkstring(L, 2);

    file << "\ngame:GetService(\"StarterGui\"):SetCore(\"" << key << "\", ";

    bool first = true;

    // Following code is AI-generated.
    if (lua_istable(L, 3))
    {
        bool first = true;
        file << "{";

        lua_pushnil(L);
        while (lua_next(L, 3) != 0)
        {
            if (!first)
                file << ", ";

            first = false;

            if (lua_isstring(L, -2))
                file << lua_tostring(L, -2) << " = ";

            if (lua_isstring(L, -1))
                file << "\"" << lua_tostring(L, -1) << "\"";
            else if (lua_isnumber(L, -1))
                file << lua_tonumber(L, -1);
            else if (lua_isboolean(L, -1))
                file << (lua_toboolean(L, -1) ? "true" : "false");
            else
                file << "nil";

            lua_pop(L, 1);
        }

        file << "}";
    }
    else if (lua_isstring(L, 3))
        file << "\"" << lua_tostring(L, 3) << "\"";
    else if (lua_isnumber(L, 3))
        file << lua_tonumber(L, 3);
    else if (lua_isboolean(L, 3))
        file << (lua_toboolean(L, 3) ? "true" : "false");
    else
        file << "nil";

    file << ")\n";


    return 0;
}

int Vector2_new(lua_State* L)
{
    float x = luaL_optnumber(L, 1, 0);
    float y = luaL_optnumber(L, 2, 0);

    lua_newtable(L);

    lua_pushnumber(L, x);
    lua_setfield(L, -2, "X");

    lua_pushnumber(L, x);
    lua_setfield(L, -2, "Y");

    return 1;
}

void registerVector2(lua_State* L)
{
    lua_newtable(L);

    lua_pushcfunction(L, Vector2_new, "new");
    lua_setfield(L, -2, "new");

    lua_pushcfunction(L, Vector2_new, "zero");
    lua_setfield(L, -2, "zero");

    lua_pushcfunction(L, Vector2_new, "one");
    lua_setfield(L, -2, "one");

    lua_setglobal(L, "Vector2");
}

int Rectnew(lua_State* L)
{
    double minX = luaL_optnumber(L, 1, 0);
    double maxX = luaL_optnumber(L, 2, 0);
    double minY = luaL_optnumber(L, 3, 0);
    double maxY = luaL_optnumber(L, 4, 0);

    lua_newtable(L);

    lua_newtable(L);
    lua_pushnumber(L, minX);
    lua_setfield(L, -2, "X");
    lua_pushnumber(L, minY);
    lua_setfield(L, -2, "Y");
    lua_setfield(L, -2, "Min");

    lua_newtable(L);
    lua_pushnumber(L, maxX);
    lua_setfield(L, -2, "X");
    lua_pushnumber(L, maxY);
    lua_setfield(L, -2, "Y");
    lua_setfield(L, -2, "Max");

    return 1;
}

void registerRect(lua_State* L)
{
    lua_newtable(L);

    lua_pushcfunction(L, Rectnew, "new");
    lua_setfield(L, -2, "new");

    lua_setglobal(L, "Rect");
}

int UDim_new(lua_State* L)
{
    double scale = luaL_optnumber(L, 1, 0);
    double offset = luaL_optnumber(L, 2, 0);

    lua_newtable(L);

    lua_pushnumber(L, scale);
    lua_setfield(L, -2, "Scale");

    lua_pushnumber(L, offset);
    lua_setfield(L, -2, "Offset");

    return 1;
}

void registerUDim(lua_State* L)
{
    lua_newtable(L);

    lua_pushcfunction(L, UDim_new, "new");
    lua_setfield(L, -2, "new");
    lua_setglobal(L, "UDim");
}


int GetMouse(lua_State* L)
{
    lua_newtable(L);

    lua_pushnumber(L, 0);
    lua_setfield(L, -2, "X");

    lua_pushnumber(L, 0);
    lua_setfield(L, -2, "X");

    return 1;
}

int TweenInfo_new(lua_State* L)
{
    lua_newtable(L);

    lua_pushvalue(L, 1);
    lua_setfield(L, -2, "Time");

    lua_pushvalue(L, 2);
    lua_setfield(L, -2, "EasingStyle");

    return 1;
}

int TweenPlay(lua_State* L)
{
    return 0;
}

int TweenCreate(lua_State* L)
{
    lua_newtable(L);

    lua_pushcfunction(L, TweenPlay, "Play");
    lua_setfield(L, -2, "Play");

    return 1;
}

void registerGame(lua_State* L)
{
    lua_newtable(L); // Shared Metatable (Stores functions which are globally avaiable for the game table and its subtables)
    lua_pushvalue(L, -1);
    lua_setglobal(L, "SharedMeta");
    lua_pushcfunction(L, WaitForChild, "WaitForChild");
    lua_setfield(L, -2, "WaitForChild");

    lua_pushcfunction(L, WaitForChild, "FindFirstChild");
    lua_setfield(L, -2, "FindFirstChild");

    lua_pushcfunction(L, GetService, "GetService");
    lua_setfield(L, -2, "GetService");

    lua_pushcfunction(L, WaitForChild, "FindFirstChildWhichIsA");
    lua_setfield(L, -2, "FindFirstChildWhichIsA");

    lua_pushcfunction(L, HttpGet, "HttpGet");
    lua_setfield(L, -2, "HttpGet");

    lua_pushcfunction(L, GetMouse, "GetMouse");
    lua_setfield(L, -2, "GetMouse");


    lua_newtable(L); // game.StarterGui
    lua_pushcfunction(L, SetCore, "SetCore");
    lua_setfield(L, -2, "SetCore");
    lua_setfield(L, -2, "StarterGui");

    lua_newtable(L);
    lua_pushvalue(L, -2);
    lua_setfield(L, -2, "__index");

    
    lua_newtable(L); // game
    lua_pushvalue(L, -2);
    lua_setmetatable(L, -2);

    lua_pushcfunction(L, InstanceNew, "new");
    lua_pushstring(L, "!Workspace"); // game.Workspace
    lua_call(L, 1, 1);

    lua_pushstring(L, "FallenPartsDestroyHeight");
    lua_pushnumber(L, -500);
    lua_rawset(L, -3);

    lua_pushvalue(L, -1); // game.Workspace = Workspace
    lua_setfield(L, -3, "Workspace");
    lua_setglobal(L, "workspace");


    // How does stacking work in this shi??????  I DONT UNDERSTAND LUAU IN C++
    lua_newtable(L); // game.Players
    lua_pushvalue(L, -3);
    lua_setmetatable(L, -2);

    lua_newtable(L); // game.Players.LocalPlayer
    
    lua_newtable(L); // game.Players.LocalPlayer.Character
    lua_pushcfunction(L, InstanceNew, "new");
    lua_pushstring(L, "Character");
    lua_call(L, 1, 1);
    lua_setfield(L, -2, "Character");

    lua_newtable(L); // game.Players.LocalPlayer.CharacterAdded
    lua_pushcfunction(L, Connect, "Connect");
    lua_setfield(L, -2, "Connect");

    lua_pushcfunction(L, Connect, "connect");
    lua_setfield(L, -2, "connect");

    lua_pushcfunction(L, Wait, "Wait");
    lua_setfield(L, -2, "Wait");

    lua_pushcfunction(L, Wait, "wait");
    lua_setfield(L, -2, "wait");
    
    lua_newtable(L); // game.TweenService
    lua_pushcfunction(L, TweenCreate, "Create");
    lua_setfield(L, -2, "Create");
    lua_setfield(L, -2, "TweenService");

    lua_pushstring(L, "CharacterAdded");
    lua_setfield(L, -2, "_eventname");

    lua_pushstring(L, "game:GetService(\"Players\").LocalPlayer");
    lua_setfield(L, -2, "id");

    lua_setfield(L, -2, "CharacterAdded");

    lua_newtable(L);
    lua_getglobal(L, "SharedMeta");
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);

    lua_getfield(L, -1, "Character");
    lua_newtable(L);
    lua_getglobal(L, "SharedMeta");
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);
    lua_pop(L, 1);

    lua_setfield(L, -2, "LocalPlayer");
    lua_setfield(L, -2, "Players");

    lua_setglobal(L, "game");

    lua_pop(L, 2);
}



bool isColor3(lua_State* L)
{
    int table = lua_absindex(L, 3);
    
    lua_getfield(L, table, "R");
    bool hasR = !lua_isnil(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, table, "G");
    bool hasG = !lua_isnil(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, table, "B");
    bool hasB = !lua_isnil(L, -1);
    lua_pop(L, 1);

    if (hasR && hasG && hasB)
        return true;
    else
        return false;
}


// Made by me at first - used AI for some small modifications to remove Runtime Errors
bool isUDim2(lua_State* L)
{
    int table = lua_absindex(L, 3);

    lua_getfield(L, table, "X");
    bool hasX = lua_istable(L, -1);

    if (hasX) {
        lua_getfield(L, -1, "Scale");
        hasX = !lua_isnil(L, -1);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    lua_getfield(L, table, "Y");
    bool hasY = lua_istable(L, -1);

    if (hasY) {
        lua_getfield(L, -1, "Scale");
        hasY = !lua_isnil(L, -1);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    return hasX && hasY;
}

int InstanceNew(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    bool silent = name[0] == '!';
    if (silent)
        name++;

    lua_newtable(L);
    int storage = lua_gettop(L);

    lua_pushstring(L, name);
    lua_setfield(L, -2, "type");

    lua_pushstring(L, name);
    lua_setfield(L, -2, "Name");

    int id = dist(gen);

    lua_newtable(L);
    int proxy = lua_gettop(L);
    lua_newtable(L);

    lua_pushstring(L, std::string("i" + std::to_string(id)).c_str());
    lua_setfield(L, storage, "id");
    if (!silent) {
        if (strcmp(name, "Character") == 0)
            file << "local i" << std::to_string(id) << " = game.Players.LocalPlayer.Character\n";
        else if (name[1] == '!')
            name++;
        else
            file << "local i" << std::to_string(id) << " = Instance.new(\"" << name << "\")\n";
    }

    lua_pushvalue(L, storage);
    lua_pushcclosure(L, [](lua_State* L) -> int {
        // ty gptchat for this nice idea
        if (lua_isstring(L, 2) && strcmp(lua_tostring(L, 2), "_storage") == 0) {
            lua_pushvalue(L, lua_upvalueindex(1));
            return 1;
        }
        // that was about all of the idea, just this if

        lua_pushvalue(L, 2);
        lua_rawget(L, lua_upvalueindex(1));

        if (!lua_isnil(L, -1))
            return 1;

        lua_pop(L, 1);

        lua_getglobal(L, "SharedMeta");
        lua_pushvalue(L, 2);
        lua_rawget(L, -2);
        lua_remove(L, -2);

        return 1;
        }, "InstanceIndex", 1);
    lua_setfield(L, -2, "__index");

    lua_pushvalue(L, storage);
    lua_pushstring(L, std::to_string(id).c_str());
    lua_pushcclosure(L, [](lua_State* L) -> int {
        const char* key = luaL_checkstring(L, 2);
        int base = lua_gettop(L);
        std::string id = ("i" + std::string(lua_tostring(L, lua_upvalueindex(2))));

        if (!(lastid == id))
        {
            file << "\n";
            lastid = id;
        }

        file << id << "." << key << " = ";

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
                        file << id << "\n";
                        lua_pop(L, 1);
                    }
                    else if (isColor3(L))
                    {
                        lua_pop(L, 1);
                        lua_getfield(L, 3, "R");
                        double r = lua_tonumber(L, -1);
                        lua_pop(L, 1);

                        lua_getfield(L, 3, "G");
                        double g = lua_tonumber(L, -1);
                        lua_pop(L, 1);

                        lua_getfield(L, 3, "B");
                        double b = lua_tonumber(L, -1);
                        lua_pop(L, 1);


                        file << "Color3.new(" << r << ", " << g << ", " << b << ")\n";
                    }
                    else if (isUDim2(L))
                    {
                        lua_pop(L, 1);
                        
                        lua_getfield(L, 3, "X");
                        lua_getfield(L, -1, "Scale");
                        double xs = lua_tonumber(L, -1);
                        lua_pop(L, 1);
                        lua_getfield(L, -1, "Offset");
                        double xo = lua_tonumber(L, -1);
                        lua_pop(L, 2);


                        lua_getfield(L, 3, "Y");
                        lua_getfield(L, -1, "Scale");
                        double ys = lua_tonumber(L, -1);
                        lua_pop(L, 1);
                        lua_getfield(L, -1, "Offset");
                        double yo = lua_tonumber(L, -1);
                        lua_pop(L, 2);


                        file << "UDim2.new(" << xs << ", " << xo << ", " << ys << ", " << yo << ")\n";
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
        
        if (strcmp(key, "Parent") == 0 && lua_istable(L, 3))
        {
            lua_getfield(L, lua_upvalueindex(1), "Name");
            lua_pushvalue(L, 1);
            lua_rawset(L, 3);
        }
        lua_settop(L, base);

        lua_pushvalue(L, 2);
        lua_pushvalue(L, 3);
        lua_rawset(L, lua_upvalueindex(1));

        return 0;
        }, "InstanceChanged", 2);
    lua_setfield(L, -2, "__newindex");

    lua_setmetatable(L, proxy);
    
    // Used AI to make the Eventlist because it is alot to do it alone
    const char* connectEvents[] = {
        "MouseButton1Click",
        "MouseButton1Down",
        "MouseButton1Up",
        "MouseEnter",
        "MouseLeave",
        "InputBegan",
        "InputEnded",
        "InputChanged",
        "RenderStepped",
        "Heartbeat",
        "Stepped",
        "Died",
        "CharacterAdded",
        "CharacterRemoving",
        "Changed",
        "AncestryChanged",
        "Touched",
        "TouchEnded"
    };

    for (const char* eventname : connectEvents)
    {
        lua_newtable(L); // Event Name

        lua_pushcfunction(L, Connect, "Connect");
        lua_setfield(L, -2, "Connect");

        lua_pushcfunction(L, Connect, "connect");
        lua_setfield(L, -2, "connect");

        lua_pushstring(L, std::string("i" + std::to_string(id)).c_str());
        lua_setfield(L, -2, "id");

        lua_pushvalue(L, proxy);
        lua_setfield(L, -2, "_owner");

        lua_pushstring(L, eventname);
        lua_setfield(L, -2, "_eventname");

        lua_setfield(L, storage, eventname);
    }

    lua_remove(L, storage);

    return 1;
}

int Color3fromRGB(lua_State* L)
{
    int r = luaL_checkinteger(L, 1);
    int g = luaL_checkinteger(L, 2);
    int b = luaL_checkinteger(L, 3);

    lua_newtable(L);

    lua_pushnumber(L, r / 255.0);
    lua_setfield(L, -2, "R");

    lua_pushnumber(L, g / 255.0);
    lua_setfield(L, -2, "G");

    lua_pushnumber(L, b / 255.0);
    lua_setfield(L, -2, "B");

    return 1;
}

int Color3new(lua_State* L)
{
    double r = luaL_checkinteger(L, 1);
    double g = luaL_checkinteger(L, 2);
    double b = luaL_checkinteger(L, 3);

    lua_newtable(L);

    lua_pushnumber(L, r);
    lua_setfield(L, -2, "R");

    lua_pushnumber(L, g);
    lua_setfield(L, -2, "G");

    lua_pushnumber(L, b);
    lua_setfield(L, -2, "B");

    return 1;
}


int UDim2new(lua_State* L)
{
    double xs = luaL_checknumber(L, 1);
    double xo = luaL_checknumber(L, 2);
    double ys = luaL_checknumber(L, 3);
    double yo = luaL_checknumber(L, 4);

    lua_newtable(L); // Return Table


    lua_newtable(L); // X
    lua_pushnumber(L, xs);
    lua_setfield(L, -2, "Scale");
    lua_pushnumber(L, xo);
    lua_setfield(L, -2, "Offset");
    lua_setfield(L, -2, "X");

    lua_newtable(L); // Y
    lua_pushnumber(L, ys);
    lua_setfield(L, -2, "Scale");
    lua_pushnumber(L, yo);
    lua_setfield(L, -2, "Offset");
    lua_setfield(L, -2, "Y");

    return 1;
}

void registerColor3(lua_State* L)
{
    lua_newtable(L); // Color3

    lua_pushcfunction(L, Color3fromRGB, "fromRGB");
    lua_setfield(L, -2, "fromRGB");

    lua_pushcfunction(L, Color3new, "new");
    lua_setfield(L, -2, "new");

    lua_setglobal(L, "Color3");
}

void registerUDim2(lua_State* L)
{
    lua_newtable(L);

    lua_pushcfunction(L, UDim2new, "new");
    lua_setfield(L, -2, "new");

    lua_setglobal(L, "UDim2");
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
    loadstringfile.open("loadstringcalls.lua");

    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    lua_pushcfunction(L, lua_loadstring, "loadstring");
    lua_setglobal(L, "loadstring");
    
    hook(L, "print");
    hook(L, "loadstring", true);

    registerEnum(L);
    registerGame(L);
    registerColor3(L);
    registerUDim(L);
    registerUDim2(L);
    registerRect(L);
    registerVector2(L);
    Instance(L);

    const char* code = R"(
    loadstring(game:HttpGet("https://raw.githubusercontent.com/0x100C/Roblox-Scripts/refs/heads/main/s0ulzV3.lua"))()
)";

    size_t bytecodeSize;
    char* bytecode = luau_compile(code, strlen(code), nullptr, &bytecodeSize);
    
    int result = luau_load(L, "script", bytecode, bytecodeSize, 0);

    if (result == 0)
    {
        result = lua_pcall(L, 0, 0, 0);

        if (result != 0)
            std::cerr << "Runtime error: " << lua_tostring(L, -1) << '\n';
    }
    else
    {
        std::cerr << "Load error: " << lua_tostring(L, -1) << '\n';
    }

    free(bytecode);
    lua_close(L);
}