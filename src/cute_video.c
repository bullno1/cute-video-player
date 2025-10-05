#include "cute_video.h"
#include <pl_mpeg.h>
#include <cute_alloc.h>
#include <cute_draw.h>
#include "cute_video_shd.h"
#include <SDL3/SDL_audio.h>

#define CUTE_VIDEO_READ_SIZE 4096

struct cute_video_s {
	CF_File* file;
	plm_t* plm;
	plm_buffer_t* buffer;
	int width;
	int height;
	CF_V2 uv_scale;
	CF_Aabb box;
	SDL_AudioStream* audio_stream;

	CF_Texture tex_y;
	CF_Texture tex_cr;
	CF_Texture tex_cb;
	CF_Shader shader;
};

static void
cute_video_load(plm_buffer_t* buffer, void* ctx) {
	CF_File* file = ctx;

	uint8_t read_buf[CUTE_VIDEO_READ_SIZE];

	size_t bytes_read = cf_fs_read(file, read_buf, sizeof(read_buf));
	if (bytes_read > sizeof(read_buf)) {  // Error
		plm_buffer_signal_end(buffer);
		return;
	}

	plm_buffer_write(buffer, read_buf, bytes_read);
	if (cf_fs_eof(file).code) {
		plm_buffer_signal_end(buffer);
	}
}

static void
cute_video_seek(plm_buffer_t* buffer, size_t offset, void* ctx) {
	CF_File* file = ctx;
	cf_fs_seek(file, offset);
}

static size_t
cute_video_tell(plm_buffer_t* buffer, void* ctx) {
	CF_File* file = ctx;
	return cf_fs_tell(file);
}

static CF_TextureParams
cute_video_texture_params(int width, int height) {
	CF_TextureParams texture_params = cf_texture_defaults(width, height);
	texture_params.stream = true;
	texture_params.pixel_format = CF_PIXEL_FORMAT_R8_UNORM;

	return texture_params;
}

static void
cute_video_on_video(plm_t* plm, plm_frame_t* frame, void* ctx) {
	cute_video_t* cv = ctx;
	cf_texture_update(cv->tex_y, frame->y.data, frame->y.width * frame->y.height);
	cf_texture_update(cv->tex_cr, frame->cr.data, frame->cr.width * frame->cr.height);
	cf_texture_update(cv->tex_cb, frame->cb.data, frame->cb.width * frame->cb.height);
}

static void
cute_video_on_audio(plm_t* plm, plm_samples_t* samples, void* ctx) {
	cute_video_t* cv = ctx;
	int size = sizeof(float) * samples->count * 2;
	SDL_PutAudioStreamData(cv->audio_stream, samples->interleaved, size);
}

cute_video_t*
cute_video_init(CF_File* source, cute_video_options_t* options) {
	cute_video_options_t default_options = {
		.buffer_size = 1024 * 1024,
	};
	if (options == NULL) { options = &default_options; }

	cute_video_t* cv = cf_alloc(sizeof(cute_video_t));

	plm_buffer_t* buffer = plm_buffer_create_with_callbacks(
		cute_video_load,
		cute_video_seek,
		cute_video_tell,
		options->buffer_size,
		source
	);
	plm_t* plm = plm_create_with_buffer(buffer, true);
	if (!plm_has_headers(plm)) {
		plm_destroy(plm);
		cf_free(cv);
		return NULL;
	}

	plm_set_video_decode_callback(plm, cute_video_on_video, cv);

	SDL_AudioStream* audio_stream = NULL;
	if (options->audio_stream == -1) {
		plm_set_audio_enabled(plm, false);
	} else if (plm_get_num_audio_streams(plm) > 0) {
		plm_set_audio_stream(plm, options->audio_stream);
		plm_set_audio_decode_callback(plm, cute_video_on_audio, cv);
		SDL_AudioSpec audio_spec = {
			.channels = 2,
			.format = SDL_AUDIO_F32,
			.freq = plm_get_samplerate(plm),
		};
		audio_stream = SDL_OpenAudioDeviceStream(
			SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
			&audio_spec,
			NULL, NULL
		);
		plm_set_audio_lead_time(plm, (double)4096 / plm_get_samplerate(plm));
		SDL_ResumeAudioStreamDevice(audio_stream);
	}

	// pl_mpeg does not expose this size calculation
	// We want to create the textures upfront
	int width = plm_get_width(plm);
	int height = plm_get_height(plm);
	int mb_width = (width + 15) >> 4;
	int mb_height = (height + 15) >> 4;
	int luma_width = mb_width << 4;
	int luma_height = mb_height << 4;
	int chroma_width = mb_width << 3;
	int chroma_height = mb_height << 3;

	float half_width = (float)width * 0.5f;
	float half_height = (float)height * 0.5f;

	*cv = (cute_video_t){
		.plm = plm,
		.file = source,
		.audio_stream = audio_stream,
		.buffer = buffer,
		.width = width,
		.height = height,
		.uv_scale = cf_v2((float)width / (float)luma_width, (float)height / (float)luma_height),
		.tex_y = cf_make_texture(cute_video_texture_params(luma_width, luma_height)),
		.tex_cr = cf_make_texture(cute_video_texture_params(chroma_width, chroma_height)),
		.tex_cb = cf_make_texture(cute_video_texture_params(chroma_width, chroma_height)),
		.shader = cf_make_draw_shader_from_bytecode(cute_video_shd),
		.box = cf_make_aabb(
			cf_v2(-half_width, -half_height),
			cf_v2(half_width, half_height)
		),
	};

	return cv;
}

void
cute_video_cleanup(cute_video_t* cv) {
	if (cv == NULL) { return; }
	plm_destroy(cv->plm);
	cf_destroy_shader(cv->shader);
	cf_destroy_texture(cv->tex_y);
	cf_destroy_texture(cv->tex_cr);
	cf_destroy_texture(cv->tex_cb);
	if (cv->audio_stream != 0) {
		SDL_DestroyAudioStream(cv->audio_stream);
	}
	cf_free(cv);
}

cute_video_info_t
cute_video_info(cute_video_t* cv) {
	return (cute_video_info_t){
		.plm = cv->plm,
		.width = cv->width,
		.height = cv->height,
	};
}

void
cute_video_render(cute_video_t* cv) {
	cf_draw_push_shader(cv->shader);
	cf_draw_set_texture("tex_y", cv->tex_y);
	cf_draw_set_texture("tex_cr", cv->tex_cr);
	cf_draw_set_texture("tex_cb", cv->tex_cb);
	cf_draw_set_uniform_v2("u_uv_scale", cv->uv_scale);
	cf_draw_box(cv->box, 1.f, 0.f);
	cf_draw_pop_shader();
}

void
cute_video_update(cute_video_t* cv, double elapsed_time) {
	plm_decode(cv->plm, elapsed_time);
	if (plm_buffer_get_remaining(cv->buffer) >= CUTE_VIDEO_READ_SIZE) {
		cute_video_load(cv->buffer, cv->file);
	}
}

bool
cute_video_ended(cute_video_t* cv) {
	return plm_has_ended(cv->plm);
}

#define PL_MPEG_IMPLEMENTATION
#include <pl_mpeg.h>
