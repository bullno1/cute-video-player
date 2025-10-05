#ifndef CUTE_VIDEO_H
#define CUTE_VIDEO_H

#include <cute_file_system.h>
#include <cute_graphics.h>

typedef struct cute_video_s cute_video_t;
struct plm_t;

typedef struct cute_video_options_s {
	size_t buffer_size;
	int audio_stream;
} cute_video_options_t;

typedef struct cute_video_info_s {
	struct plm_t* plm;
	int width;
	int height;
} cute_video_info_t;

cute_video_t*
cute_video_init(CF_File* source, cute_video_options_t* options);

void
cute_video_cleanup(cute_video_t* cv);

cute_video_info_t
cute_video_info(cute_video_t* cv);

void
cute_video_render(cute_video_t* cv);

void
cute_video_update(cute_video_t* cv, double elapsed_time);

bool
cute_video_ended(cute_video_t* cv);

#endif
