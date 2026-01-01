#pragma once
#include <defines.h>

struct RENDER_PACKET;
struct APPLICATION;
struct FRAME_DATA;

YAPI b8 application_boot(struct APPLICATION* application_instance);

YAPI b8 application_init(struct APPLICATION* application_instance);

YAPI b8 application_update(struct APPLICATION* application_instance, const struct FRAME_DATA* p_frame_data);

YAPI b8 application_render(struct APPLICATION* application_instance, struct RENDER_PACKET* packet, const struct FRAME_DATA* p_frame_data);

YAPI void application_on_resize(struct APPLICATION* application_instance, u32 width, u32 height);

YAPI void application_shutdown(struct APPLICATION* application_instance);

YAPI void application_lib_on_unload(struct APPLICATION* application_instance);

YAPI void application_lib_on_load(struct APPLICATION* application_instance);
