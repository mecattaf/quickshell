#version 440

layout(location = 0) in vec2 position;
layout(location = 0) out vec2 v_texCoord;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    vec4 tintColor;
    vec4 params;    // x=cornerRadius, y=materialLevel, z=opacity, w=unused
    vec2 size;      // width, height in pixels
    vec2 padding;
};

void main() {
    // Map from [-1,1] to [0,1] for texture coordinates
    v_texCoord = position * 0.5 + 0.5;
    gl_Position = mvp * vec4(position, 0.0, 1.0);
}
