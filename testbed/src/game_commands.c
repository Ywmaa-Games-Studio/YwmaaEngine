#include "game.h"
#include <core/console.h>

#include <core/event.h>

void game_command_exit(CONSOLE_COMMAND_CONTEXT context) {
    PRINT_DEBUG("game exit called!");
    event_fire(EVENT_CODE_APPLICATION_QUIT, 0, (EVENT_CONTEXT){0});
}

void game_setup_commands(GAME* game_instance) {
    console_register_command("exit", 0, game_command_exit);
    console_register_command("quit", 0, game_command_exit);
}
