# memreader

[![Build status](https://ci.appveyor.com/api/projects/status/2wvir44l54xuuoau?svg=true)](https://ci.appveyor.com/project/squeek502/memreader)

`memreader` is a [Lua](https://www.lua.org/) module for reading the memory of Windows processes.

```lua
local memreader = require('memreader')

-- Find a window by title and open a handle to its process
local window = memreader.findwindow("Your Window Title")
local process = memreader.openprocess(window.pid)

-- Read the first 8 bytes from the start of the main module's memory
local data_rel = process:readrelative(0, 8)

-- Do the same thing, but using the absolute address this time
local address = process.base
local data_abs = process:read(address, 8)

-- Data is returned as a string of the specified length (not null-terminated)
assert(#data_rel == 8 and #data_abs == 8)
assert(data_rel == data_abs)
```

## Building
To build memreader, you'll need to install [`cmake`](https://cmake.org), some version of [Visual Studio](https://www.visualstudio.com/), and have a Lua `.lib` file that [can be found by `cmake`](https://cmake.org/cmake/help/v3.0/module/FindLua.html) (preferably built with the same compiler you're using to build memreader).

### Using `cmake-gui`
- Run `cmake-gui`
- Browse to memreader directory and set the build directory (typically just add `/build` to the memreader directory path)
- Click Configure
- Select the Generator and hit *Finish*
- Hit Generate and then Open Project to open the project in Visual Studio
- Build the project in Visual Studio as normal

### Using `cmake`
Open a command line in the `memreader` directory and do the following:
```sh
mkdir build
cd build
cmake ..
cmake --build . --config Release
```
If needed, you can specify a [generator](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html) by doing `cmake -G "Visual Studio 14 2015 Win64" ..` instead of `cmake ..`

## API Reference

### `memreader.debugprivilege([state = true])`
Attempts to adjust the access token of the Lua process to set the [`SeDebugPrivilege`](https://msdn.microsoft.com/en-us/library/windows/desktop/bb530716(v=vs.85).aspx) privilege (needed to access the memory of processes owned by other accounts). Calling this may or may not be necessary depending on how Lua is spawned, your use case, etc. 

On success, returns `true`; otherwise, returns `nil, errmsg`.

### `memreader.processes()`
Returns an iterator for all the processes in the system, in `pid, name` pairs. 

Example:
```lua
local process
for pid, name in memreader.processes() do
  if name == "target.exe" then
    process = memreader.openprocess(pid)
    break
  end
end
```

> Relevant WinAPI docs: [`CreateToolhelp32Snapshot`](https://msdn.microsoft.com/en-us/library/windows/desktop/ms682489(v=vs.85).aspx), [`Process32Next`](https://msdn.microsoft.com/en-us/library/windows/desktop/ms684836(v=vs.85).aspx)

### `memreader.findwindow(title)`
Finds a window by title. If found, returns a [`memreader.window`](#memreaderwindow) usertype; otherwise, returns `nil, errmsg`.

*Note: If there are multiple windows with the same title, this will only return the first (in arbitrary order). `findwindow` is faster than iterating with `processes`, but is not as precise or thorough.*

> Relevant WinAPI docs: [`FindWindow`](https://msdn.microsoft.com/en-us/library/windows/desktop/ms633499(v=vs.85).aspx)

### `memreader.openprocess(pid)`
Attempts to open a handle to the process with the given process ID using the flags [`PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ`](https://msdn.microsoft.com/en-us/library/windows/desktop/ms684880(v=vs.85).aspx). On success, returns a [`memreader.process`](#memreaderprocess) usertype; otherwise, returns `nil, errmsg`.

> Relevant WinAPI docs: [`OpenProcess`](https://msdn.microsoft.com/en-us/library/windows/desktop/ms684320(v=vs.85).aspx)

### `memreader.process`

A usertype for process handles.

**Fields (read-only):**

- `process.pid`: The process ID (e.g. `280`)
- `process.name`: The name of the process' main module (e.g. `lua.exe`)
- `process.path`: The full path to the process' main module (e.g. `C:\lua.exe`)
- `process.base`: The base address of the process' main module, as a [`memreader.address`](#memreaderaddress) usertype (e.g. `0000000076EA0000`)

#### `process:read(address, nbytes)`
Reads the specified number of bytes starting at the given address (can be either a number or a [`memreader.address`](#memreaderaddress)) and returns that memory as a string (not null-terminated). On failure, returns `nil, errmsg`.

#### `process:readrelative(offset, nbytes)`
Like `process:read()`, except that `offset` is added to the process' main module's base address to determine the address to start from.

```lua
-- The following are exactly equivalent
process:read(process.base + 0x40, 4)
process:readrelative(0x40, 4)
```

#### `process:modules()`
Returns an iterator for all the modules of the process, as [`memreader.module`](#memreadermodule) usertypes.

Example:
```lua
for module in process:modules() do
  print(module.name, module.base)
end
```

#### `process:exitcode()`
Returns the exit code of the process (if it has exited). If the process is still running, then it will instead return `nil`. On failure, returns `nil, errmsg`.

#### `process:version()`
Retrieves the file and product version info embedded in the process' main module and returns it as a table. On failure, returns `nil, errmsg`.

Structure of the returned table:
```lua
{ 
  file = { major=1, minor=0, build=3, revision=105 },
  product = { major=3, minor=0, build=0, revision=0 }
}
```

### `memreader.module`

A usertype for process modules.

**Fields (read-only):**

- `module.name`: The name of the module (e.g. `lua51.dll`)
- `module.path`: The full path to the module (e.g. `C:\lua51.dll`)
- `module.base`: The base address of the module, as a [`memreader.address`](#memreaderaddress) usertype (e.g. `0000000076EA0000`)
- `module.size`: The size (in bytes) of the module (e.g. `32768`)

### `memreader.address`

A usertype for an address in memory ([`LPVOID`](https://en.wikibooks.org/wiki/Windows_Programming/Handles_and_Data_Types#LPVOID)). Can be manipulated by adding/subtracting it with numbers or other `memreader.address` instances.

Example:
```lua
local a = process.base
local b = a + 0x40
local c = b - a
```

### `memreader.window`

A usertype for window handles.

**Fields (read-only):**

- `window.pid`: The process ID of the window's main thread (e.g. `280`)
- `window.title`: The title of the window (e.g. `Your Window Title`)
