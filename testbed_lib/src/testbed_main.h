#pragma once
#include <defines.h>

struct RENDER_PACKET;
struct APPLICATION;

YAPI b8 application_boot(struct APPLICATION* application_instance);

YAPI b8 application_init(struct APPLICATION* application_instance);

YAPI b8 application_update(struct APPLICATION* application_instance, f32 delta_time);

YAPI b8 application_render(struct APPLICATION* application_instance, struct RENDER_PACKET* packet, f32 delta_time);

YAPI void application_on_resize(struct APPLICATION* application_instance, u32 width, u32 height);

YAPI void application_shutdown(struct APPLICATION* application_instance);

YAPI void application_lib_on_unload(struct APPLICATION* application_instance);

YAPI void application_lib_on_load(struct APPLICATION* application_instance);
