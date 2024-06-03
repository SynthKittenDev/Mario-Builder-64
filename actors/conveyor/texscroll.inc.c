void scroll_conveyor_000_displaylist_mesh_layer_1_vtx_1() {
	int i = 0;
	int count = 16;
	int width = 32 * 0x20;

	static int currentX = 0;
	int deltaX;
	Vtx *vertices = segmented_to_virtual(conveyor_000_displaylist_mesh_layer_1_vtx_1);

	deltaX = (int)(4.0 * 0x20) % width;

	if (absi(currentX) > width) {
		deltaX -= (int)(absi(currentX) / width) * width * signum_positive(deltaX);
	}

	for (i = 0; i < count; i++) {
		vertices[i].n.tc[0] += deltaX;
	}
	currentX += deltaX;
}

void scroll_actor_geo_conveyor() {
	scroll_conveyor_000_displaylist_mesh_layer_1_vtx_1();
}
