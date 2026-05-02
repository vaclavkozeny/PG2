#version 460 core

out vec4 FragColor;

// Material properties
uniform vec3 ambient_intensity, diffuse_intensity, specular_intensity;
uniform vec3 ambient_material, diffuse_material, specular_material;
uniform float specular_shinines;

// Texture
uniform sampler2D tex0;
uniform float uAlpha;

// Directional light
uniform bool use_directional_light;
uniform vec3 light_direction;

// Point lights
uniform bool use_point_lights;
uniform vec3 point_light_positions[3];
uniform vec3 point_light_colors[3];
uniform int num_point_lights;

in VS_OUT {
    vec3 N;
    vec3 V;
    vec3 P;
    vec2 texCoord;
} fs_in;

void main(void) {
    vec3 N = normalize(fs_in.N);
    vec3 V = normalize(fs_in.V);
    vec3 albedo = texture(tex0, fs_in.texCoord).rgb;

    vec3 ambient = ambient_material * ambient_intensity;
    vec3 diffuse_sum = vec3(0.0);
    vec3 specular_sum = vec3(0.0);

    // Directional light contribution
    if (use_directional_light) {
        vec3 L = normalize(light_direction);
        vec3 R = reflect(-L, N);

        float diffuse_factor = max(dot(N, L), 0.0);
        float specular_factor = pow(max(dot(R, V), 0.0), specular_shinines);

        diffuse_sum += diffuse_factor * diffuse_material * diffuse_intensity;
        specular_sum += specular_factor * specular_material * specular_intensity;
    }

    // Point lights contribution
    if (use_point_lights) {
        for (int i = 0; i < num_point_lights; ++i) {
            vec3 L = normalize(point_light_positions[i] - fs_in.P);
            vec3 R = reflect(-L, N);

            float diffuse_factor = max(dot(N, L), 0.0);
            float specular_factor = pow(max(dot(R, V), 0.0), specular_shinines);

            diffuse_sum += diffuse_factor * diffuse_material * diffuse_intensity * point_light_colors[i];
            specular_sum += specular_factor * specular_material * specular_intensity * point_light_colors[i];
        }
    }

    FragColor = vec4((ambient + diffuse_sum) * albedo + specular_sum, uAlpha);
}
