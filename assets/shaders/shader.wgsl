struct VertexInput {
    @builtin(vertex_index) vertex_index: u32,
    @location(0) position: vec3f,
    @location(1) texcoord: vec2f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) color: vec3f,
    @location(1) tex_coord: vec2f,
};

struct global_uniform_object {
    projection: mat4x4<f32>,
    view: mat4x4<f32>,
};
struct model_uniform_object {
    model: mat4x4<f32>,
};
struct object_uniform_object {
    diffuse_color: vec4f,
};
@group(0) @binding(0) var<uniform> global_ubo: global_uniform_object;
@group(1) @binding(0) var<uniform> object_ubo: object_uniform_object;
@group(1) @binding(1) var diffuse_sampler: sampler;
@group(1) @binding(2) var diffuse_texture: texture_2d<f32>;
var<push_constant> model_ubo: model_uniform_object;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput; // create the output struct

	out.position = global_ubo.projection * global_ubo.view * model_ubo.model * vec4f(in.position, 1.0); // same as what we used to directly return
    

	out.color = vec3f(1.0); // forward the color attribute to the fragment shader
    out.tex_coord = in.texcoord;
	return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let color = textureLoad(diffuse_texture, vec2<i32>(in.tex_coord), 0).rgb;
    return object_ubo.diffuse_color * textureSample(diffuse_texture, diffuse_sampler, in.tex_coord);
}
