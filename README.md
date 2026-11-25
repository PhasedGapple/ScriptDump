# ScriptDump
Dump Obfuscated or normal Roblox Scripts and get a readable dump!
## How does this work?
This has it's own Roblox-Luau replica which logs executed code by adding our own implementation of Roblox-Luau to imitate roblox.
## Scripts to test this on
- To demonstrate it mostly working: https://raw.githubusercontent.com/XNEOFF/FlyGuiV3/main/FlyGuiV3.txt
- To see as an example what doesnt get dumped: https://gist.github.com/RedoGaming/2e5943498c487ea97df0d436dd35c234
## Downsides / Limitations
- It would be too time consuming to implement every function, so only the more simpler scripts will dump flawlessly.
- Most external UI Libraries aren't supported, make sure your script has it built-in.
- We cannot log if statements or functions, just the code which gets executed.
- We also dont keep the original variable names.
## Compiling
Make sure you have [VCPKG](https://vcpkg.io/en/) installed. Then run ``vcpkg install luau`` and you are ready to compile. (also run ``vcpkg integrate install`` if its your first time installing vcpkg). Then use your preferred compiler to compile using the slnx or vcxproj file. I personally used Visual Studio 2026 with the built-in MSVC compiler.
## Running
Head over to [Releases](https://github.com/PhasedGapple/ScriptDump/releases) to download the executable.
Drag your file which includes the LuaU code into the executable. \
Then, there will be 2 files. Make sure ``dump.lua`` and ``loadstringcalls.lua`` are present. \
If the script isnt fully supported, there should be an error in the window when dragging the file. If you want to know an error, just open the file in any terminal so the window wont close after it quits. There you will see the error message. \v
``dump.lua`` contains the functions and registered variables (when a supported script is ran this should be enough), while ``loadstringscalls.lua`` contains every call to loadstring().
## Code used which is not mine
- https://github.com/luau-lang/luau - used to run luau code
## Usage of AI
I used AI for helping me find functions, document stuff and write some of the functions. I also commented it in the code when AI wrote a function. **Most of the Code is made by me, but this definetely wouldn't be possible to make for me without AI because Lua in C++ looks really confusing but ChatGPT made me understand it**
