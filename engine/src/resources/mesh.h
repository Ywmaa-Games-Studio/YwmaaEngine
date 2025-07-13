#pragma once

#include "resource_types.h"

b8 mesh_load_from_resource(const char* resource_name, Mesh* out_mesh);

void mesh_unload(Mesh* m);
