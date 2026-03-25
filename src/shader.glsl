@vs vs
in vec2 pos;
in vec4 color;
out vec4 v_color;

void main() {
    gl_Position = vec4(pos, 0.0, 1.0);
    v_color = color;
}
@end

@fs fs
in vec4 v_color;
out vec4 frag_color;

void main() {
    frag_color = v_color;
}
@end

@program shader vs fs
