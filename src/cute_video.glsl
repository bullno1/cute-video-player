layout(set = 2, binding = 1) uniform sampler2D tex_y;
layout(set = 2, binding = 2) uniform sampler2D tex_cr;
layout(set = 2, binding = 3) uniform sampler2D tex_cb;
layout(set = 3, binding = 1) uniform shd_uniforms {
	vec2 u_uv_scale;
};

const mat4 rec601 = mat4(
	1.16438,  0.00000,  1.59603, -0.87079,
	1.16438, -0.39176, -0.81297,  0.52959,
	1.16438,  2.01723,  0.00000, -1.08139,
	0, 0, 0, 1
);

vec4 shader(vec4 color, vec2 pos, vec2 screen_uv, vec4 params)
{
	vec2 uv = screen_uv * u_uv_scale;
	float y = texture(tex_y, uv).r;
	float cb = texture(tex_cb, uv).r;
	float cr = texture(tex_cr, uv).r;
	return vec4(y, cb, cr, 1.0) * rec601;
}
