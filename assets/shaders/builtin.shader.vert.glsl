/* #version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;

layout(set = 0, binding = 0) uniform global_uniform_object {
    mat4 projection;
	mat4 view;
} global_ubo;

void main() {
    gl_Position = global_ubo.projection * global_ubo.view * vec4(in_position, 1.0);
} */
// hard-coded Shader code for showing a triangle 

//we will be using glsl version 4.5 syntax
#version 450

void main()
{
	//const array of positions for the triangle
	const vec3 positions[3] = vec3[3](
		vec3(0.5f,0.5f, 0.0f),
		vec3(-0.5f,0.5f, 0.0f),
		vec3(0.f,-0.5f, 0.0f)
	);

	//output the position of each vertex
	gl_Position = vec4(positions[gl_VertexIndex], 1.0f);
}