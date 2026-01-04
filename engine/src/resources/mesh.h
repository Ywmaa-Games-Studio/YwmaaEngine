#pragma once

#include "resource_types.h"

//YAPI b8 mesh_load_from_resource(const char* resource_name, Mesh* out_mesh);

YAPI b8 mesh_create(MESH_CONFIG config, Mesh* out_mesh);

YAPI b8 mesh_init(Mesh* m);

YAPI b8 mesh_load(Mesh* m);

YAPI b8 mesh_unload(Mesh* m);

YAPI b8 mesh_destroy(Mesh* m);
