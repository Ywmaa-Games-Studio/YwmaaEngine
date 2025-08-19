#pragma once

#include "resource_types.h"

YAPI b8 mesh_load_from_resource(const char* resource_name, Mesh* out_mesh);

YAPI void mesh_unload(Mesh* m);
