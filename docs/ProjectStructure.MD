# Project Structure

## engine folder

here are all of the engine code.

### Special Files (Entry, defines, etc)

-- src/entry.h: The Entry Point

-- src/defines.h: Special definitions for the variable names and detects the platform.

### 1- src/core

This contains the main engine code, such as:

-- Engine itself (application.c & application.h) also contains the Game Loop

-- Engine Logger (logger.c & logger.h)

-- Engine Simple Asserts (asserts.c)

-- Engine Memory Manager (ymemory.c & ymemory.h)

-- Engine Event Manager (event.c & event.h)

### 2- src/input

-- Engine Input System (input.c & input.h)

### 3- src/variants

This contains special data types for variables that are created in this project and used by the engine, such as darray (dynamic array), etc.

### 4- src/platform

The Platform Abstraction for the engine, this is basically the layer between the engine & the target platform (windows/linux/etc)