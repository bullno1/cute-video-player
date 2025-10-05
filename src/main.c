#define PLM_NO_STDIO
#include <stddef.h>
#include <stdio.h>
#include <cute.h>
#include <pl_mpeg.h>
#include "cute_video.h"

int
main(int argc, const char* argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: cute-video-player <file.mpg>\n");
		return 1;
	}

	int exit_code = EXIT_FAILURE;
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT;
	cf_make_app("cute video player", 0, 0, 0, 640, 480, options, argv[0]);
	cf_fs_mount(cf_fs_get_working_directory(), "/", true);

	CF_File* file = cf_fs_open_file_for_read(argv[1]);
	if (file == NULL) {
		fprintf(stderr, "Could not open file\n");
		return 1;
	}

	cute_video_t* cv = cute_video_init(file, NULL);
	if (cv == NULL) {
		fprintf(stderr, "Could not decode video\n");
		goto end;
	}

	// Query extra info directly from pl_mpeg
	cute_video_info_t info = cute_video_info(cv);
	printf("Video length: %fs\n", plm_get_duration(info.plm));

	// Adjust window size
	cf_app_set_size(info.width, info.height);
	cf_app_set_canvas_size(info.width, info.height);
	cf_draw_projection(cf_ortho_2d(0, 0, (float)info.width, (float)info.height));

	while (cf_app_is_running()) {
		cf_app_update(NULL);

		cute_video_update(cv, CF_DELTA_TIME);
		cute_video_render(cv);

		cf_app_draw_onto_screen(true);

		if (cute_video_ended(cv)) {
			break;
		}
	}

	exit_code = EXIT_SUCCESS;
end:
	cute_video_cleanup(cv);
	cf_fs_close(file);
	cf_destroy_app();

	return exit_code;
}
