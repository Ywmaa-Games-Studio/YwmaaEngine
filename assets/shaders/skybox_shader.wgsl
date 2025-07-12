struct VertexInput {
    @builtin(vertex_index) vertex_index: u32,
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) texcoord: vec2f,
    @location(3) color: vec4f,
    @location(4) tangent: vec4f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) tex_coord: vec3f,
};

struct global_uniform_object {
    projection: mat4x4<f32>,
    view: mat4x4<f32>,
};

@group(0) @binding(0) var<uniform> global_ubo: global_uniform_object;
@group(1) @binding(0) var cube_sampler: sampler;
@group(1) @binding(1) var cube_texture: texture_cube<f32>;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput; // create the output struct

	out.tex_coord = in.position;
	out.position = global_ubo.projection * global_ubo.view * vec4(in.position, 1.0);

	return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    return textureSample(cube_texture, cube_sampler, in.tex_coord);
}
