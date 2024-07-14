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
b8 input_is_key_down(E_KEYS key)
```

```
b8 input_is_key_up(E_KEYS key)
```

```
b8 input_was_key_down(E_KEYS key)
```

```
b8 input_was_key_up(E_KEYS key)
```

#### mouse input

```
b8 input_is_button_down(E_BUTTONS button)
```

```
b8 input_is_button_up(E_BUTTONS button)
```

```
b8 input_was_button_down(E_BUTTONS button)
```

```
b8 input_was_button_up(E_BUTTONS button)
```

```
void input_get_mouse_position(i32* x, i32* y)
```

```
void input_get_previous_mouse_position(i32* x, i32* y)
```