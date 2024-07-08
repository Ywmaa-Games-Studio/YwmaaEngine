# AMRT Engine / YWMAA Engine

made by C

# why AMRT as a name 

AMRT stands for "أمرت" in arabic which means "I Commanded"
and actually its letters stand for :

أ : العاب (Games)

م : محاكاة (Simulation)

ر : روبوتات (Robotics)

ت : تطبيقات (Apps)

also AMRT is the brand name, while in code you will find the engine is named "YWMAA", and its API is prefixed with the letter "Y".

# Building

## Windows

TO-DO

## Linux

Install these via package manager:
 - `clang`
 - `libx11-dev`
 - `libxkbcommon-x11-dev`
 - `libx11-xcb-dev`

You can also check the required dependecies for linux in shell.nix

and run ```./build-all.sh```

### Nix OS

use the provided shell.nix

and run ```./build-all.sh```


# Contributing Guide

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
