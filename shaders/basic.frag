#version 460

layout(set = 0, binding = 0) uniform UniformBufferObject0 {
    vec3 color;
};

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.0, 1.0, 1.0, 1.0);
}