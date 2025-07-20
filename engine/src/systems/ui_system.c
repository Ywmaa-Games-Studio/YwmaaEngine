#include "ui_system.h"
#include "core/ymemory.h"
#include "core/logger.h"
#include "core/ystring.h"
#include "core/event.h"

#include "systems/render_view_system.h"
#include "systems/geometry_system.h"

#include "resources/mesh.h"
#include "renderer/renderer_frontend.h"
#include "resources/ui_text.h"

#include "math/ymath.h"
#include "math/transform.h"

#include "input/input.h"
#define CLAY_IMPLEMENTATION
#include "vendor/clay.h"

// to print camera position
#include "systems/camera_system.h"

const Clay_Color COLOR_LIGHT = (Clay_Color) {224, 215, 210, 255};
const Clay_Color COLOR_RED = (Clay_Color) {168, 66, 28, 255};
const Clay_Color COLOR_ORANGE = (Clay_Color) {225, 138, 50, 255};

typedef struct UI_SYSTEM_STATE {
    UI_SYSTEM_CONFIG config;

    // Array of registered textures.
    Mesh** ui_meshes;
    UI_TEXT** ui_texts;
    u32 ui_mesh_count;
    u32 ui_text_count;
} UI_SYSTEM_STATE;

static UI_SYSTEM_STATE* state_ptr = 0;
static f32 mouse_wheel_y = 0.0f;

/* Global for convenience. Even in 4K this is enough for smooth curves (low radius or rect size coupled with
 * no AA or low resolution might make it appear as jagged curves) */
//static int NUM_CIRCLE_SEGMENTS = 16;
void handle_clay_errors(Clay_ErrorData error_data) {
    PRINT_ERROR("CLAY: %s", error_data.errorText.chars);
}

static inline Clay_Dimensions measure_text(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData)
{
    //f32 scale_factor = config->fontSize/(f32)state_ptr->ui_texts[0]->data->size;
    //PRINT_DEBUG("CLAY: Measuring text: %.*s width: %i", text.length, text.chars, config->letterSpacing);
    u32 width = ui_text_get_text_width(text.chars, state_ptr->ui_texts[0]->data);
    u32 height = ui_text_get_text_height(text.chars, state_ptr->ui_texts[0]->data);

    return (Clay_Dimensions) { (f32) width, (f32) height };
}

UI_PACKET_DATA Clay_RenderClayCommands(Clay_RenderCommandArray *rcommands)
{
    // Invalidate all meshes.
    for (u32 i = 0; i < state_ptr->ui_mesh_count; ++i) {
        Mesh* mesh = state_ptr->ui_meshes[i];
        if (mesh->generation == INVALID_ID_U8) {
            continue; // Skip already invalidated meshes.
        }
        mesh->generation = INVALID_ID_U8;
        if (mesh->geometries[0] != NULL) {
            geometry_system_release(mesh->geometries[0]);
            mesh->geometries[0] = NULL;
        }
    }
    state_ptr->ui_mesh_count = 0;
    state_ptr->ui_text_count = 0;

    for (size_t i = 0; i < rcommands->length; i++) {
        Clay_RenderCommand *rcmd = Clay_RenderCommandArray_Get(rcommands, i);
        const Clay_BoundingBox bounding_box = rcmd->boundingBox;
        
        switch (rcmd->commandType) {
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
                // Create a new mesh for the UI.
                if (state_ptr->ui_mesh_count >= state_ptr->config.max_ui_count) {
                    PRINT_ERROR("UI System: Maximum UI count exceeded.");
                    break;
                }
                Mesh* mesh = state_ptr->ui_meshes[state_ptr->ui_mesh_count++];

                Clay_RectangleRenderData *config = &rcmd->renderData.rectangle;
                
                if (config->cornerRadius.topLeft > 0) {
                    
                } else {
                    // Load up some test UI geometry.
                    GEOMETRY_CONFIG ui_config;
                    ui_config.vertex_size = sizeof(Vertex2D);
                    ui_config.vertex_count = 4;
                    ui_config.index_size = sizeof(u32);
                    ui_config.index_count = 6;
                    string_ncopy(ui_config.material_name, "test_ui_material", MATERIAL_NAME_MAX_LENGTH);
                    string_ncopy(ui_config.name, "test_ui_geometry", GEOMETRY_NAME_MAX_LENGTH);
    
                    const f32 w = bounding_box.width;
                    const f32 h = bounding_box.height;
                    Vertex2D uiverts [4];
                    uiverts[0].position.x = 0.0f;  // 0    3
                    uiverts[0].position.y = 0.0f;  //
                    uiverts[0].texcoord.x = 0.0f;  //
                    uiverts[0].texcoord.y = 0.0f;  // 2    1
    
                    uiverts[1].position.y = h;
                    uiverts[1].position.x = w;
                    uiverts[1].texcoord.x = 1.0f;
                    uiverts[1].texcoord.y = 1.0f;
    
                    uiverts[2].position.x = 0.0f;
                    uiverts[2].position.y = h;
                    uiverts[2].texcoord.x = 0.0f;
                    uiverts[2].texcoord.y = 1.0f;
    
                    uiverts[3].position.x = w;
                    uiverts[3].position.y = 0.0;
                    uiverts[3].texcoord.x = 1.0f;
                    uiverts[3].texcoord.y = 0.0f;
                    ui_config.vertices = uiverts;
                    
                    // Indices - counter-clockwise
                    u32 uiindices[6] = {2, 1, 0, 3, 0, 1};
                    ui_config.indices = uiindices;
    
                    // Get UI geometry from config.
                    mesh->geometries[0] = geometry_system_acquire_from_config(ui_config, true);
                    mesh->transform = transform_from_position((Vector3){bounding_box.x, 0.0f, bounding_box.y});
                    mesh->generation = 0; // Reset generation for new mesh.
                    mesh->geometries[0]->material->diffuse_color.r = config->backgroundColor.r;
                    mesh->geometries[0]->material->diffuse_color.g = config->backgroundColor.g;
                    mesh->geometries[0]->material->diffuse_color.b = config->backgroundColor.b;
                    mesh->geometries[0]->material->diffuse_color.a = config->backgroundColor.r;
                }
            } break;
            case CLAY_RENDER_COMMAND_TYPE_TEXT: {
                Clay_TextRenderData *config = &rcmd->renderData.text;
                if (state_ptr->ui_text_count >= state_ptr->config.max_text_count) {
                    PRINT_ERROR("UI System: Maximum text count exceeded.");
                    break;
                }
                UI_TEXT* text = state_ptr->ui_texts[state_ptr->ui_text_count++];
                ui_text_set_text(text, config->stringContents.chars);
                ui_text_set_position(text, Vector3_create(bounding_box.x, bounding_box.y, 0));

                //TTF_Font *font = rendererData->fonts[config->fontId];
                //TTF_SetTextColor(text, config->textColor.r, config->textColor.g, config->textColor.b, config->textColor.a);
            } break;
/*             case CLAY_RENDER_COMMAND_TYPE_BORDER: {
                Clay_BorderRenderData *config = &rcmd->renderData.border;

                const f32 minRadius = SDL_min(rect.w, rect.h) / 2.0f;
                const Clay_CornerRadius clampedRadii = {
                    .topLeft = SDL_min(config->cornerRadius.topLeft, minRadius),
                    .topRight = SDL_min(config->cornerRadius.topRight, minRadius),
                    .bottomLeft = SDL_min(config->cornerRadius.bottomLeft, minRadius),
                    .bottomRight = SDL_min(config->cornerRadius.bottomRight, minRadius)
                };
                //edges
                SDL_SetRenderDrawColor(rendererData->renderer, config->color.r, config->color.g, config->color.b, config->color.a);
                if (config->width.left > 0) {
                    const f32 starting_y = rect.y + clampedRadii.topLeft;
                    const f32 length = rect.h - clampedRadii.topLeft - clampedRadii.bottomLeft;
                    SDL_FRect line = { rect.x - 1, starting_y, config->width.left, length };
                    SDL_RenderFillRect(rendererData->renderer, &line);
                }
                if (config->width.right > 0) {
                    const f32 starting_x = rect.x + rect.w - (f32)config->width.right + 1;
                    const f32 starting_y = rect.y + clampedRadii.topRight;
                    const f32 length = rect.h - clampedRadii.topRight - clampedRadii.bottomRight;
                    SDL_FRect line = { starting_x, starting_y, config->width.right, length };
                    SDL_RenderFillRect(rendererData->renderer, &line);
                }
                if (config->width.top > 0) {
                    const f32 starting_x = rect.x + clampedRadii.topLeft;
                    const f32 length = rect.w - clampedRadii.topLeft - clampedRadii.topRight;
                    SDL_FRect line = { starting_x, rect.y - 1, length, config->width.top };
                    SDL_RenderFillRect(rendererData->renderer, &line);
                }
                if (config->width.bottom > 0) {
                    const f32 starting_x = rect.x + clampedRadii.bottomLeft;
                    const f32 starting_y = rect.y + rect.h - (f32)config->width.bottom + 1;
                    const f32 length = rect.w - clampedRadii.bottomLeft - clampedRadii.bottomRight;
                    SDL_FRect line = { starting_x, starting_y, length, config->width.bottom };
                    SDL_SetRenderDrawColor(rendererData->renderer, config->color.r, config->color.g, config->color.b, config->color.a);
                    SDL_RenderFillRect(rendererData->renderer, &line);
                }
                //corners
                if (config->cornerRadius.topLeft > 0) {
                    const f32 centerX = rect.x + clampedRadii.topLeft -1;
                    const f32 centerY = rect.y + clampedRadii.topLeft - 1;
                    SDL_Clay_RenderArc(rendererData, (SDL_FPoint){centerX, centerY}, clampedRadii.topLeft,
                        180.0f, 270.0f, config->width.top, config->color);
                }
                if (config->cornerRadius.topRight > 0) {
                    const f32 centerX = rect.x + rect.w - clampedRadii.topRight;
                    const f32 centerY = rect.y + clampedRadii.topRight - 1;
                    SDL_Clay_RenderArc(rendererData, (SDL_FPoint){centerX, centerY}, clampedRadii.topRight,
                        270.0f, 360.0f, config->width.top, config->color);
                }
                if (config->cornerRadius.bottomLeft > 0) {
                    const f32 centerX = rect.x + clampedRadii.bottomLeft -1;
                    const f32 centerY = rect.y + rect.h - clampedRadii.bottomLeft;
                    SDL_Clay_RenderArc(rendererData, (SDL_FPoint){centerX, centerY}, clampedRadii.bottomLeft,
                        90.0f, 180.0f, config->width.bottom, config->color);
                }
                if (config->cornerRadius.bottomRight > 0) {
                    const f32 centerX = rect.x + rect.w - clampedRadii.bottomRight;
                    const f32 centerY = rect.y + rect.h - clampedRadii.bottomRight;
                    SDL_Clay_RenderArc(rendererData, (SDL_FPoint){centerX, centerY}, clampedRadii.bottomRight,
                        0.0f, 90.0f, config->width.bottom, config->color);
                }

            } break;
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
                Clay_BoundingBox boundingBox = rcmd->boundingBox;
                currentClippingRectangle = (SDL_Rect) {
                        .x = boundingBox.x,
                        .y = boundingBox.y,
                        .w = boundingBox.width,
                        .h = boundingBox.height,
                };
                SDL_SetRenderClipRect(rendererData->renderer, &currentClippingRectangle);
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
                SDL_SetRenderClipRect(rendererData->renderer, NULL);
                break;
            } */
            case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
                // Create a new mesh for the UI.
                if (state_ptr->ui_mesh_count >= state_ptr->config.max_ui_count) {
                    PRINT_ERROR("UI System: Maximum UI count exceeded.");
                    break;
                }
                Mesh* mesh = state_ptr->ui_meshes[state_ptr->ui_mesh_count++];

                // Load up some test UI geometry.
                GEOMETRY_CONFIG ui_config;
                ui_config.vertex_size = sizeof(Vertex2D);
                ui_config.vertex_count = 4;
                ui_config.index_size = sizeof(u32);
                ui_config.index_count = 6;
                string_ncopy(ui_config.material_name, "test_ui_material", MATERIAL_NAME_MAX_LENGTH);
                string_ncopy(ui_config.name, "test_ui_geometry", GEOMETRY_NAME_MAX_LENGTH);

                const f32 w = bounding_box.width;
                const f32 h = bounding_box.height;
                Vertex2D uiverts [4];
                uiverts[0].position.x = 0.0f;  // 0    3
                uiverts[0].position.y = 0.0f;  //
                uiverts[0].texcoord.x = 0.0f;  //
                uiverts[0].texcoord.y = 0.0f;  // 2    1

                uiverts[1].position.y = h;
                uiverts[1].position.x = w;
                uiverts[1].texcoord.x = 1.0f;
                uiverts[1].texcoord.y = 1.0f;

                uiverts[2].position.x = 0.0f;
                uiverts[2].position.y = h;
                uiverts[2].texcoord.x = 0.0f;
                uiverts[2].texcoord.y = 1.0f;

                uiverts[3].position.x = w;
                uiverts[3].position.y = 0.0;
                uiverts[3].texcoord.x = 1.0f;
                uiverts[3].texcoord.y = 0.0f;
                ui_config.vertices = uiverts;
                
                // Indices - counter-clockwise
                u32 uiindices[6] = {2, 1, 0, 3, 0, 1};
                ui_config.indices = uiindices;

                // Get UI geometry from config.
                mesh->geometries[0] = geometry_system_acquire_from_config(ui_config, true);
                mesh->transform = transform_from_position((Vector3){bounding_box.x, 0.0f, bounding_box.y});
                mesh->generation = 0; // Reset generation for new mesh.
                //SDL_Texture *texture = (SDL_Texture *)rcmd->renderData.image.imageData;
                break;
            }
            default:
                PRINT_ERROR("Unknown render command type: %d", rcmd->commandType);
        }
    }
    
    UI_PACKET_DATA ui_packet = {0};
    ui_packet.mesh_data.mesh_count = state_ptr->ui_mesh_count;
    ui_packet.mesh_data.meshes = state_ptr->ui_meshes;
    ui_packet.text_count = state_ptr->ui_text_count;
    ui_packet.texts = state_ptr->ui_texts;
    return ui_packet;
}

b8 event_on_mouse_wheel(u16 code, void* sender, void* listener_inst, EVENT_CONTEXT context) {
    if (code == EVENT_CODE_MOUSE_WHEEL) {
        mouse_wheel_y = context.data.i8[0];
    }

    // Event purposely not handled to allow other listeners to get this.
    return false;
}

b8 event_on_window_resize(u16 code, void* sender, void* listener_inst, EVENT_CONTEXT context) {
    if (code == EVENT_CODE_RESIZED) {
        u16 width = context.data.u16[0];
        u16 height = context.data.u16[1];

        // Update internal layout dimensions to support resizing
        Clay_SetLayoutDimensions((Clay_Dimensions) { width, height });

        PRINT_DEBUG("Clay resize: %i, %i", width, height);
    }

    // Event purposely not handled to allow other listeners to get this.
    return false;
}

b8 ui_system_init(u64* memory_requirement, void* state, UI_SYSTEM_CONFIG config){
    if (config.max_ui_count == 0) {
        PRINT_ERROR("ui_system_init - config.max_ui_count must be > 0.");
        return false;
    }
    if (config.max_text_count == 0) {
        PRINT_ERROR("ui_system_init - config.max_text_count must be > 0.");
        return false;
    }

    // Block of memory will contain state structure, then block for array, then block for hashtable.
    u64 struct_requirement = sizeof(UI_SYSTEM_STATE);
    u64 clay_total_memory_size = Clay_MinMemorySize();
    *memory_requirement = struct_requirement;
    if (!state) {
        return true;
    }
    
    state_ptr = state;
    state_ptr->config = config;

    state_ptr->ui_meshes = yallocate_aligned(sizeof(Mesh**), 8, MEMORY_TAG_UI);
    state_ptr->ui_texts = yallocate_aligned(sizeof(UI_TEXT**), 8, MEMORY_TAG_UI);
    /* Initialize Clay */
    Clay_Arena clay_memory = (Clay_Arena) {
        .memory = yallocate_aligned(clay_total_memory_size, 8, MEMORY_TAG_UI),
        .capacity = clay_total_memory_size
    };

    u32 width = renderer_get_framebuffer_width();
    u32 height = renderer_get_framebuffer_height();
    Clay_Initialize(clay_memory, (Clay_Dimensions) { (f32) width, (f32) height }, (Clay_ErrorHandler) { handle_clay_errors });
    Clay_SetMeasureTextFunction(measure_text, NULL);
    state_ptr->ui_mesh_count = 0;
    state_ptr->ui_text_count = 0;
    
    for (u32 i = 0; i < config.max_text_count; ++i) {
        UI_TEXT* text = yallocate_aligned(sizeof(UI_TEXT), 8, MEMORY_TAG_UI);
        if(!ui_text_create(UI_TEXT_TYPE_BITMAP, "JetBrainsMono", 21, "Clay Text", text)) {
            PRINT_ERROR("Failed to load basic ui system text.");
            continue;
        }
        ui_text_set_position(text, Vector3_create(50, 100, 0));
        state_ptr->ui_texts[i] = text;
    }
    // Invalidate all meshes.
    for (u32 i = 0; i < config.max_ui_count; ++i) {
        Mesh* mesh = yallocate_aligned(sizeof(Mesh), 8, MEMORY_TAG_UI);
        // Get UI geometry from config.
        mesh->geometry_count = 1;
        mesh->geometries = yallocate_aligned(sizeof(GEOMETRY*), 8, MEMORY_TAG_ARRAY);
        mesh->geometries[0] = NULL;
        mesh->transform = transform_from_position((Vector3){0.0f, 0.0f, 0.0f});
        mesh->generation = INVALID_ID_U8;
        state_ptr->ui_meshes[i] = mesh;
    }

    event_register(EVENT_CODE_RESIZED, 0, event_on_window_resize);
    event_register(EVENT_CODE_MOUSE_WHEEL, 0, event_on_mouse_wheel);

    return true;
}

UI_PACKET_DATA ui_system_render_commands(Clay_RenderCommandArray *rcommands, f32 delta_time) {
    if (!state_ptr) {
        PRINT_ERROR("UI system not initialized.");
        UI_PACKET_DATA empty_packet = {0};
        return empty_packet; // Return an empty UI_PACKET_DATA
    }
    
    // get mouse position
    i32 mouse_position_x;
    i32 mouse_position_y;
    input_get_mouse_position(&mouse_position_x, &mouse_position_y);
    b8 is_mouse_down = input_is_button_pressed(BUTTON_LEFT);
    // Update internal pointer position for handling mouseover / click / touch events - needed for scrolling & debug tools
    Clay_SetPointerState((Clay_Vector2) { mouse_position_x, mouse_position_y }, is_mouse_down);
    // Get mouse wheel delta
    f32 mouse_wheel_x = 0.0f;
    // Update internal pointer position for handling mouseover / click / touch events - needed for scrolling and debug tools
    Clay_UpdateScrollContainers(true, (Clay_Vector2) { mouse_wheel_x, mouse_wheel_y }, delta_time);

    // Update the bitmap text with camera position. NOTE: just using the default camera for now.
    Camera* world_camera = camera_system_get_default();
    Vector3 pos = camera_position_get(world_camera);
    Vector3 rot = camera_rotation_euler_get(world_camera);
    char camera_position_text_buffer[128];
    char camera_rot_text_buffer[128];
    string_format(
        camera_position_text_buffer,
        "Camera Pos:\n[%.3f, %.3f, %.3f]",
        pos.x, pos.y, pos.z);

    string_format(
        camera_rot_text_buffer,
        "Camera Rot:\n[%.3f, %.3f, %.3f]", 
        rad_to_deg(rot.x), rad_to_deg(rot.y), rad_to_deg(rot.z));
    Clay_String camera_position_clay_string = {0};
    camera_position_clay_string.chars = camera_position_text_buffer;
    camera_position_clay_string.length = string_length(camera_position_text_buffer);
    camera_position_clay_string.isStaticallyAllocated = true;
    Clay_String camera_rotation_clay_string = {0};
    camera_rotation_clay_string.chars = camera_rot_text_buffer;
    camera_rotation_clay_string.length = string_length(camera_rot_text_buffer);
    camera_rotation_clay_string.isStaticallyAllocated = true;
    // All clay layouts are declared between Clay_BeginLayout and Clay_EndLayout
    Clay_BeginLayout();
    CLAY({
        .id = CLAY_ID("Icon"),
        .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = { .width = CLAY_SIZING_FIXED(64), .height = CLAY_SIZING_FIXED(64) }, .padding = CLAY_PADDING_ALL(8), .childGap = 8 },
        .backgroundColor = {1, 1, 1, 255},
    }) {

    }
    CLAY({
        .id = CLAY_ID("Text Layout"),
        .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = { .width = CLAY_SIZING_FIXED(1000), .height = CLAY_SIZING_FIXED(600) }, .padding = CLAY_PADDING_ALL(8), .childGap = 16 },
        .backgroundColor = {1, 1, 1, 0},
    }) {
        CLAY_TEXT(CLAY_STRING("Ywmaa\n\t Engine"), CLAY_TEXT_CONFIG({ .fontSize = 21, .textColor = {255, 255, 255, 255} }));
        CLAY_TEXT(CLAY_STRING("Clay - UI Library"), CLAY_TEXT_CONFIG({ .fontSize = 21, .textColor = {255, 255, 255, 255} }));
        CLAY_TEXT(CLAY_STRING("وشوية كلام والكلام ههههه\n بس شوية كلام"), CLAY_TEXT_CONFIG({ .fontSize = 5, .textColor = {255, 255, 255, 255} }));
        CLAY_TEXT(camera_position_clay_string, CLAY_TEXT_CONFIG({ .fontSize = 21, .textColor = {0, 0, 255, 255} }));
        CLAY_TEXT(camera_rotation_clay_string, CLAY_TEXT_CONFIG({ .fontSize = 21, .textColor = {0, 0, 255, 255} }));
    }
    // All clay layouts are declared between Clay_BeginLayout and Clay_EndLayout
    Clay_RenderCommandArray render_commands = Clay_EndLayout();
    // text count debug
    
    UI_PACKET_DATA ui_packet = Clay_RenderClayCommands(&render_commands);

    return ui_packet;
}

void ui_system_shutdown(void){
    for (u32 i = 0; i < state_ptr->config.max_text_count; ++i) {
        ui_text_destroy(state_ptr->ui_texts[i]);
        yfree(state_ptr->ui_texts[i]);
    }
    yfree(state_ptr->ui_texts);
    // Invalidate all meshes.
    for (u32 i = 0; i < state_ptr->config.max_ui_count; ++i) {
        Mesh* mesh = state_ptr->ui_meshes[i];
        mesh->generation = INVALID_ID_U8;
        geometry_system_release(mesh->geometries[0]);
        yfree(mesh->geometries);
        mesh->geometries = NULL;
        mesh->geometry_count = 0;
    }
}
