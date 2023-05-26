#version 330

in vec2 fragmentTexCoord;

out vec4 FragColor;

uniform sampler2D sampler;

void main() {
    FragColor = texture2D(sampler, fragmentTexCoord.st);
}