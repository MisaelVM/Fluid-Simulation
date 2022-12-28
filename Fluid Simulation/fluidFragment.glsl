#version 430 core

out vec4 color;

uniform vec2 u_Resolution;
uniform vec2 u_GridDim;

layout (std430, binding = 0) buffer gridBufferLayout {
	vec4 gridColours[];
};

void main()
{
	ivec2 res = ivec2(int(u_Resolution.x), int(u_Resolution.y));
	ivec2 pos = ivec2(int(gl_FragCoord.x), int(gl_FragCoord.y));
	ivec2 tile = ivec2(int(u_Resolution.x / u_GridDim.x), int(u_Resolution.y / u_GridDim.y));
	ivec2 gridpos = ivec2(int(pos.x / tile.x), int(pos.y / tile.y));
	int gridindex = gridpos.x + gridpos.y * int(u_GridDim.y);
	color = gridColours[gridindex];
}
