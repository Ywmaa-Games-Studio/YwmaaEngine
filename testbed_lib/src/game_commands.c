#include <core/console.h>
#include <application_types.h>
#include <core/event.h>

void game_command_exit(CONSOLE_COMMAND_CONTEXT context) {
    PRINT_DEBUG("game exit called!");
    event_fire(EVENT_CODE_APPLICATION_QUIT, 0, (EVENT_CONTEXT){0});
}

void game_setup_commands(APPLICATION* game_instance) {
    console_register_command("exit", 0, game_command_exit);
    console_register_command("quit", 0, game_command_exit);
}

void game_remove_commands(APPLICATION* game_instance) {
    console_unregister_command("exit");
    console_unregister_command("quit");
}
