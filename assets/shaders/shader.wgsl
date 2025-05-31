struct VertexInput {
    @builtin(vertex_index) vertex_index: u32,
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) texcoord: vec2f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) color: vec3f,
    @location(1) ambient: vec4f,
    @location(2) tex_coord: vec2f,
    @location(3) normal: vec3f,
};

struct global_uniform_object {
    projection: mat4x4<f32>,
    view: mat4x4<f32>,
    ambient_color: vec4f,
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
var<push_constant> push_constants: model_uniform_object;

struct directional_light {
    direction: vec3f,
    color: vec4f,
};

fn calculate_directional_light(light: directional_light, normal: vec3f, ambient_color: vec4f, tex_coord: vec2f) -> vec4f {
    var diffuse_factor: f32 = max(dot(normal, -light.direction), 0.0);

    var diff_samp: vec4f = textureSample(diffuse_texture, diffuse_sampler, tex_coord);
    var ambient: vec4f = vec4f(vec3f(ambient_color.rgb * object_ubo.diffuse_color.rgb), diff_samp.a);
    var diffuse: vec4f = vec4f(vec3f(light.color.rgb * diffuse_factor), diff_samp.a);
    
    diffuse *= diff_samp;
    ambient *= diff_samp;

    return (ambient + diffuse);
}

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput; // create the output struct

	out.position = global_ubo.projection * global_ubo.view * push_constants.model * vec4f(in.position, 1.0); // same as what we used to directly return


	out.color = vec3f(1.0); // forward the color attribute to the fragment shader
    out.tex_coord = in.texcoord;
    //out.normal = in.normal;
    let model_normal_matrix = mat3x3<f32>(
        push_constants.model[0].xyz,
        push_constants.model[1].xyz,
        push_constants.model[2].xyz,
    );
    out.normal = normalize(in.normal * model_normal_matrix);
	out.ambient = global_ubo.ambient_color;
	return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    // TODO: feed in from cpu
    var dir_light: directional_light = directional_light(
        vec3f(-0.57735, -0.57735, 0.57735),
        vec4f(0.8, 0.8, 0.8, 1.0)
    );
    let color = calculate_directional_light(dir_light, in.normal, in.ambient, in.tex_coord);
    return color;
}
