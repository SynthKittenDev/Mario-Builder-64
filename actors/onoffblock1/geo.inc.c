#include "src/game/envfx_snow.h"

const GeoLayout onoffblock1_geo[] = {
	GEO_CULLING_RADIUS(500),
	GEO_OPEN_NODE(),
		GEO_DISPLAY_LIST(LAYER_OPAQUE, onoffblock1_sm64_mesh_mesh_layer_1),
		GEO_DISPLAY_LIST(LAYER_OPAQUE, onoffblock1_material_revert_render_settings),
	GEO_CLOSE_NODE(),
	GEO_END(),
};
