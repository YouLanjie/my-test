/**
 * @file        r3d_rotate.c
 * @author      Chglish
 * @date        2026-07-12
 * @brief       空观旋转.elf
 */

#include "./lib/render3d.h"
#include <stdcountof.h>

#define MAX_FRAME 100000
#define FPS 40

int main(void)
{
	RenderBackend_t *backend = backend_create_utf8(get_winsize_row() - 1, get_winsize_col() - 1);
	Camera_t        *camera = camera_create();
	Obj_t           *objs[] = {
		obj_create_cube(2),
	};
	if (!backend || !camera) goto EXIT_AND_CLEANUP;

	camera->position = (Vec_t){0, 0, 10};
	// double G = 

	printf("\e[2J");
	size_t i = 0;
	for (i = 0; i < MAX_FRAME; ++i) {
		switch (kbhitGetchar()) {
		}

		for (int i = 0; i < countof(objs); i++) {
			if (!objs[i]) continue;
			obj_cast(objs[i], camera, backend);
		}
		backend->render(backend);
		backend->clean(backend);
		sleep_fixed_step(1./FPS);
	}

EXIT_AND_CLEANUP:
	if (backend) backend->destroy(backend);
	if (camera) camera_free(camera);
	for (int i = 0; i < countof(objs); i++) {
		if (objs[i]) obj_free(objs[i]);
	}
	return EXIT_SUCCESS;
}

