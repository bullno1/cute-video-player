add_executable(cute-video-player "main.c" "cute_video.c" "cute_video_shd.h")
target_link_libraries(cute-video-player PRIVATE cute pl_mpeg)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/cute_video_shd.h
	COMMAND cute-shaderc
		-type=draw
		-varname=cute_video_shd
		-oheader=${CMAKE_CURRENT_SOURCE_DIR}/cute_video_shd.h
		${CMAKE_CURRENT_SOURCE_DIR}/cute_video.glsl
	DEPENDS cute_video.glsl
	DEPENDS cute-shaderc
)
