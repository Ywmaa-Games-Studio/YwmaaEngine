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
    @location(0) ambient: vec4f,
    @location(1) tex_coord: vec2f,
    @location(2) normal: vec3f,
    @location(3) view_position: vec3f,
    @location(4) frag_position: vec3f,
    @location(5) color: vec4f,
    @location(6) tangent: vec3f,
};

struct global_uniform_object {
    projection: mat4x4<f32>,
    view: mat4x4<f32>,
    ambient_color: vec4f,
    view_position: vec3f,
    mode: u32,
};
struct model_uniform_object {
    model: mat4x4<f32>,
};
struct object_uniform_object {
    diffuse_color: vec4f,
    shiness: f32,
};
@group(0) @binding(0) var<uniform> global_ubo: global_uniform_object;
@group(1) @binding(0) var<uniform> object_ubo: object_uniform_object;
@group(2) @binding(0) var diffuse_sampler: sampler;
@group(2) @binding(1) var diffuse_texture: texture_2d<f32>;
@group(2) @binding(2) var specular_sampler: sampler;
@group(2) @binding(3) var specular_texture: texture_2d<f32>;
@group(2) @binding(4) var normal_sampler: sampler;
@group(2) @binding(5) var normal_texture: texture_2d<f32>;

var<push_constant> push_constants: model_uniform_object;

struct directional_light {
    direction: vec3f,
    color: vec4f,
};

struct point_light {
    position: vec3f,
    color: vec4f,
    // Usually 1, make sure denominator never gets smaller than 1
    constant: f32,
    // Reduces light intensity linearly
    linear: f32,
    // Makes the light fall off slower at longer distances.
    quadratic: f32,
};

fn calculate_directional_light(light: directional_light, normal: vec3f, ambient_color: vec4f, tex_coord: vec2f, view_direction: vec3f) -> vec4f {
    var diffuse_factor: f32 = max(dot(normal, -light.direction), 0.0);

    var half_direction: vec3f = normalize(view_direction - light.direction);
    var specular_factor: f32 = pow(max(dot(half_direction, normal), 0.0), object_ubo.shiness);

    var diff_samp: vec4f = textureSample(diffuse_texture, diffuse_sampler, tex_coord);
    var ambient: vec4f = vec4f(vec3f(ambient_color.rgb * object_ubo.diffuse_color.rgb), diff_samp.a);
    var diffuse: vec4f = vec4f(vec3f(light.color.rgb * diffuse_factor), diff_samp.a);
    var specular: vec4f = vec4f(vec3(light.color.rgb * specular_factor), diff_samp.a);
    
    if(global_ubo.mode == 0) {
        var spec_samp: vec4f = textureSample(specular_texture, specular_sampler, tex_coord);
        diffuse *= diff_samp;
        ambient *= diff_samp;
        specular *= vec4f(spec_samp.rgb, diffuse.a);
    }


    return (ambient + diffuse + specular);
}

fn calculate_point_light(light: point_light, normal: vec3f, ambient_color: vec4f, tex_coord: vec2f, frag_position: vec3f, view_direction: vec3f) -> vec4f {
    var light_direction : vec3f =  normalize(light.position - frag_position);
    var diff : f32 = max(dot(normal, light_direction), 0.0);

    var reflect_direction : vec3f = reflect(-light_direction, normal);
    var spec : f32 = pow(max(dot(view_direction, reflect_direction), 0.0), object_ubo.shiness);

    // Calculate attenuation, or light falloff over distance.
    var distance : f32 = length(light.position - frag_position);
    var attenuation : f32 = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    var ambient : vec4f = ambient_color;
    var diffuse : vec4f = light.color * diff;
    var specular: vec4f = light.color * spec;
    
    if(global_ubo.mode == 0) {
        var diff_samp : vec4f = textureSample(diffuse_texture, diffuse_sampler, tex_coord);
        diffuse *= diff_samp;
        ambient *= diff_samp;
        specular *= vec4f(textureSample(specular_texture, specular_sampler, tex_coord).rgb, diffuse.a);
    }

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput; // create the output struct

	out.position = global_ubo.projection * global_ubo.view * push_constants.model * vec4f(in.position, 1.0); // same as what we used to directly return

    out.tex_coord = in.texcoord;
    out.color = in.color;

	// Fragment position in world space.
	out.frag_position = (push_constants.model * vec4f(in.position, 1.0)).rgb;
	// Copy the normal over.
    let m3_model = mat3x3<f32>(
        push_constants.model[0].xyz,
        push_constants.model[1].xyz,
        push_constants.model[2].xyz,
    );
    out.normal = normalize(m3_model * in.normal);
	out.tangent = normalize(m3_model * in.tangent);

	out.ambient = global_ubo.ambient_color;
    out.view_position = global_ubo.view_position;
	return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    var TBN : mat3x3<f32>;
    // TODO: feed in from cpu
    var dir_light: directional_light = directional_light(
        vec3f(-0.57735, -0.57735, 0.57735),
        vec4f(0.6, 0.6, 0.6, 1.0)
    );

    // TODO: feed in from cpu
    var p_light_0: point_light = point_light(
        vec3f(-5.5, 0.0, -5.5),
        vec4f(0.0, 1.0, 0.0, 1.0),
        1.0, // Constant
        0.35, // Linear
        0.44  // Quadratic
    );

    // TODO: feed in from cpu
    var p_light_1: point_light = point_light(
        vec3f(5.5, 0.0, -5.5),
        vec4f(1.0, 0.0, 0.0, 1.0),
        1.0, // Constant
        0.35, // Linear
        0.44  // Quadratic
    );

    var normal : vec3f = in.normal;
    var tangent : vec3f = in.tangent;
    tangent = (tangent - dot(tangent, normal) *  normal);
    var bitangent : vec3f = cross(in.normal, in.tangent);
    TBN = mat3x3<f32>(tangent, bitangent, normal);

    // Update the normal to use a sample from the normal map.
    
    var local_normal : vec3f = 2.0 * textureSample(normal_texture, normal_sampler, in.tex_coord).rgb - 1.0;
    normal = normalize(TBN * local_normal);

    var color = vec4f(1.0);
    if(global_ubo.mode == 0 || global_ubo.mode == 1) {
        var view_direction: vec3f = normalize(in.view_position - in.frag_position);
        color = calculate_directional_light(dir_light, normal, in.ambient, in.tex_coord, view_direction);

        color += calculate_point_light(p_light_0, normal, in.ambient, in.tex_coord, in.frag_position, view_direction);
        color += calculate_point_light(p_light_1, normal, in.ambient, in.tex_coord, in.frag_position, view_direction);
    } else if(global_ubo.mode == 2) {
        color = vec4f(abs(normal), 1.0);
    }
    
    return color;
}
