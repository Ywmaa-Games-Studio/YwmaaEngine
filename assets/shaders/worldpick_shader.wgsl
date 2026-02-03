struct VertexInput {
    @builtin(vertex_index) vertex_index: u32,
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) texcoord: vec2f,
    @location(3) color: vec4f,
    @location(4) tangent: vec3f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) color: vec3f,
};

struct global_uniform_object {
    projection: mat4x4<f32>,
    view: mat4x4<f32>,
};
//struct model_uniform_object {
//    model: mat4x4<f32>,
//};
struct object_uniform_object {
    model: mat4x4<f32>,
    id_color: vec3f,
};
@group(0) @binding(0) var<uniform> global_ubo: global_uniform_object;
@group(1) @binding(0) var<uniform> object_ubo: object_uniform_object;
//var<push_constant> model_ubo: model_uniform_object;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput; // create the output struct

    out.color = vec3f(1.0); // forward the color attribute to the fragment shader
	
    // Transform position to clip space
	let pos = global_ubo.projection * global_ubo.view * object_ubo.model * vec4f(in.position, 1.0);
	out.position = pos;
    
	return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let color = vec4f(object_ubo.id_color, 1.0);
    return color;
}
