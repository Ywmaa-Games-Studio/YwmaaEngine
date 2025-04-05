# AMRT Engine / YWMAA Engine

Made with C & Zig

# why AMRT as a name 

AMRT stands for "أمرت" in arabic which means "I Commanded"
and actually its letters stand for :

أ : العاب (Games)

م : محاكاة (Simulation)

ر : روبوتات (Robotics)

ت : تطبيقات (Apps)

also AMRT is the brand name, while in code you will find the engine is named "YWMAA", and its API is prefixed with the letter "Y".

# Building

Full-Recommended command to build ```zig build --summary all -Doptimize=ReleaseFast```

Build : ```zig build```

Build & Run : ```zig build run```

You can find the binaries in zig-out : ```zig-out/bin/ & zig-out/lib/```

For Build info add ```--summary all``` ex: ```zig build --summary all```

### Build Modes

it is adviced to use ```-Doptimize=ReleaseFast```, as personally I think for a game engine we should look for best runtime performance.

extracted from https://ziglang.org/documentation/master/#Build-Mode

```-Doptimize=Debug```

    Optimizations off and safety on (default)

```-Doptimize=ReleaseSafe```

    Optimizations on and safety on

```-Doptimize=ReleaseFast```

    Optimizations on and safety off

```-Doptimize=ReleaseSmall```

    Size optimizations on and safety off


## Dependencies

### Windows

 - `zig`
 - TO-DO

### Linux

Install these via package manager:
 - `zig`
 - `libwayland-dev`
 - Setup Vulkan SDK for your distro.

You can also check the required dependecies for linux in shell.nix

#### Nix OS

use the provided shell.nix

# Contributing Guide

### Naming Conventions
```
Macros & Constants  ALL_CAPS

Struct              ALL_CAPS
Struct Members      lower_case

Enum                E_ALL_CAPS
Enum Members        ALL_CAPS

Public functions    pfx_lower_case (pfx = two or three letter module prefix)
Private functions   lower_case
Trivial variables   i,x,n,f etc...
Local variables     lower_case
Global variables    g_lower_case (searchable by g_ prefix)
```
#### Variables
variables must/should be named snake_case.

examples:
```
u64 this_is_a_long_int = 42;
f64 this_is_a_double = 3.14159265359f;
b8 this_is_a_bool = True;
```
#### Functions
functions must/should be named snake_case.

examples:
```
void function_name(args);
```

#### Structs & Enums
any struct or enum must be named all upper case.

examples:
```
typedef enum E_RENDERER_BACKEND_API {
    RENDERER_BACKEND_API_VULKAN,
    RENDERER_BACKEND_API_OPENGL,
    RENDERER_BACKEND_API_DIRECTX
} E_RENDERER_BACKEND_API;

typedef struct RENDERER_BACKEND {
    struct PLATFORM_STATE* platform_state;  
} RENDERER_BACKEND;
```
Why ? This helps differentiating the struct name from the struct type, the name is in all snake_case, and the type is UPPER_CASE
### Commenting

It is encouraged to provide good commenting to the contributed code, also it is adviced to have the idea of encapsulating code blocks in comments that shows the start and the end, like:
```
    //START Setup Vulkan instance.
    VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.apiVersion = VK_API_VERSION_1_3;
    app_info.pApplicationName = application_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Ywmaa Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    VkInstanceCreateInfo create_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    create_info.pApplicationInfo = &app_info;

    more code..........................................

    //END Setup Vulkan instance.
```
As seen above the code is encapsulated inside these two comments and with a big START & END that has no space between it and the comment start // 
```
    //START Setup Vulkan instance.

    //END Setup Vulkan instance.
```
This should help others quickly navigate & analyze the code.

# Credits

A lot of the code is taken and tweaked from [kohi](https://github.com/travisvroman/kohi), it serves this project as the base foundation, for the later to be expanded engine.

also this means watching the kohi series is sort of an explanation of what happens here.
