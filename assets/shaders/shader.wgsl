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
//struct model_uniform_object {
//    model: mat4x4<f32>,
//};

const MAX_POINT_LIGHTS = 10;

struct directional_light {
    color: vec4f,
    direction: vec4f,
};

struct point_light {
    color: vec4f,
    position: vec4f,
    // Usually 1, make sure denominator never gets smaller than 1
    constant: f32,
    // Reduces light intensity linearly
    linear: f32,
    // Makes the light fall off slower at longer distances.
    quadratic: f32,
    padding: f32,
};

struct object_uniform_object {
    model: mat4x4<f32>,
    diffuse_color: vec4f,
    dir_light: directional_light,
    p_lights: array<point_light, MAX_POINT_LIGHTS>,
    num_p_lights: i32,
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

//var<push_constant> push_constants: model_uniform_object;


fn calculate_directional_light(light: directional_light, normal: vec3f, ambient_color: vec4f, tex_coord: vec2f, view_direction: vec3f) -> vec4f {
    var diffuse_factor: f32 = max(dot(normal, -light.direction.xyz), 0.0);

    var half_direction: vec3f = normalize(view_direction - light.direction.xyz);
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
    var light_direction : vec3f =  normalize(light.position.xyz - frag_position);
    var diff : f32 = max(dot(normal, light_direction), 0.0);

    var reflect_direction : vec3f = reflect(-light_direction, normal);
    var spec : f32 = pow(max(dot(view_direction, reflect_direction), 0.0), object_ubo.shiness);

    // Calculate attenuation, or light falloff over distance.
    var distance : f32 = length(light.position.xyz - frag_position);
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

	out.position = global_ubo.projection * global_ubo.view * object_ubo.model * vec4f(in.position, 1.0); // same as what we used to directly return

    out.tex_coord = in.texcoord;
    out.color = in.color;

	// Fragment position in world space.
	out.frag_position = (object_ubo.model * vec4f(in.position, 1.0)).rgb;
	// Copy the normal over.
    let m3_model = mat3x3<f32>(
        object_ubo.model[0].xyz,
        object_ubo.model[1].xyz,
        object_ubo.model[2].xyz,
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

        // Directional lighting calculation
        color = calculate_directional_light(object_ubo.dir_light, normal, in.ambient, in.tex_coord, view_direction);

        // point lights calculations

        for(var i = 0; i < object_ubo.num_p_lights; i++) {
            color += calculate_point_light(object_ubo.p_lights[i], normal, in.ambient, in.tex_coord, in.frag_position, view_direction);
        }

        //color += calculate_point_light(p_light_0, normal, in.ambient, in.tex_coord, in.frag_position, view_direction);
    } else if(global_ubo.mode == 2) {
        color = vec4f(abs(normal), 1.0);
    }
    
    return color;
}
