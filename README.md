# ScriptDump
Dump Obfuscated or normal Roblox Scripts and get a readable dump!
## How does this work?
This has it's own Roblox-Luau replica which logs executed code by adding our own implementation of Roblox-Luau to imitate roblox.
## Downsides
Outside of functions passed to :Connect, we cannot log if statements, just the code which gets executed.
We also dont keep the original variable names.
In functions passed to connect, we cannot detect nested if statements and else statements and we dont dump them, we decompile them.
## Compiling
Make sure you have [VCPKG](https://vcpkg.io/en/) installed. Then run ``vcpkg install luau`` and you are ready to compile. (also run ``vcpkg integrate install`` if its your first time installing vcpkg)
## Running
Coming soon.
## Code used which is not mine
- https://github.com/luau-lang/luau - used to run luau code
## Usage of AI
I used AI for helping me find functions, document stuff and write some of the functions. I also commented it in the code when AI wrote a function. **Most of the Code is made by me, but this definetely wouldn't be possible to make for me without AI because Lua in C++ looks really confusing but ChatGPT made me understand it**
