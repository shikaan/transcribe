@vs vs
in vec2 xy;
in vec2 uv;
in vec4 color;
out vec4 v_color;
out vec2 v_uv;

void main() {
    gl_Position = vec4(xy, 0.0, 1.0);
    v_color = color;
    v_uv = uv;
}
@end

@fs fs
layout(binding=0) uniform texture2D atlas;
layout(binding=0) uniform sampler smp;
in vec4 v_color;
in vec2 v_uv;
out vec4 frag_color;

void main() {
    float alpha = texture(sampler2D(atlas, smp), v_uv).r;
    frag_color = vec4(v_color.rgb, v_color.a * alpha);
}
@end

@program shader vs fs
