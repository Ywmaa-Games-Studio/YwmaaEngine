# Input

## What is it ?

The Engine's Input System

## File

```
#include <input/input.h>
```

## Functions

#### keyboard input

```
b8 input_is_key_down(KEYS key)
```

```
b8 input_is_key_up(KEYS key)
```

```
b8 input_was_key_down(KEYS key)
```

```
b8 input_was_key_up(KEYS key)
```

#### mouse input

```
b8 input_is_button_down(BUTTONS button)
```

```
b8 input_is_button_up(BUTTONS button)
```

```
b8 input_was_button_down(BUTTONS button)
```

```
b8 input_was_button_up(BUTTONS button)
```

```
void input_get_mouse_position(i32* x, i32* y)
```

```
void input_get_previous_mouse_position(i32* x, i32* y)
```