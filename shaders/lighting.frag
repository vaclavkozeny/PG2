#version 410 core

out vec4 FragColor;

uniform sampler2D tex0;

// Material
uniform vec3  mat_ambient;
uniform vec3  mat_diffuse;
uniform vec3  mat_specular;
uniform float mat_shininess;
uniform float mat_alpha;      // 1.0 = opaque, <1.0 = transparent

// ---- Light types ----

// Task 1: Directional light (sun), direction in view space
struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform DirLight dirLight;

// Task 2: Point lights (3 independent, orbiting lights)
struct PointLight {
    vec3  position;    // view space
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    float constant;
    float linear;
    float quadratic;
};
#define NUM_POINT_LIGHTS 3
uniform PointLight pointLights[NUM_POINT_LIGHTS];

// Task 3: Spotlight (camera headlight) - view space: pos=origin, dir=(0,0,-1)
struct SpotLight {
    vec3  direction;
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    float cutoff;       // cos of inner half-angle
    float outerCutoff;  // cos of outer half-angle
    float constant;
    float linear;
    float quadratic;
};
uniform SpotLight spotLight;
uniform int       spotLightOn;

// ---- Inputs from vertex shader (all in view space) ----
in vec3 vFragPos;
in vec3 vNormal;
in vec2 vTexCoord;

// ---- Phong helpers ----

vec3 calcDirLight(DirLight light, vec3 N, vec3 V, vec3 texColor) {
    vec3 L = normalize(light.direction);
    vec3 R = reflect(-L, N);
    vec3 ambient  = light.ambient  * mat_ambient  * texColor;
    vec3 diffuse  = max(dot(N, L), 0.0) * light.diffuse * mat_diffuse * texColor;
    vec3 specular = pow(max(dot(R, V), 0.0), mat_shininess) * light.specular * mat_specular;
    return ambient + diffuse + specular;
}

vec3 calcPointLight(PointLight light, vec3 N, vec3 fragPos, vec3 V, vec3 texColor) {
    vec3  L   = normalize(light.position - fragPos);
    vec3  R   = reflect(-L, N);
    float d   = length(light.position - fragPos);
    float att = 1.0 / (light.constant + light.linear * d + light.quadratic * d * d);
    vec3 ambient  = light.ambient  * mat_ambient  * texColor;
    vec3 diffuse  = max(dot(N, L), 0.0) * light.diffuse * mat_diffuse * texColor;
    vec3 specular = pow(max(dot(R, V), 0.0), mat_shininess) * light.specular * mat_specular;
    return (ambient + diffuse + specular) * att;
}

vec3 calcSpotLight(SpotLight light, vec3 N, vec3 fragPos, vec3 V, vec3 texColor) {
    // Camera at view-space origin → L points from fragment toward origin
    vec3  L    = normalize(-fragPos);
    vec3  R    = reflect(-L, N);
    vec3  D    = normalize(light.direction);
    float theta     = dot(L, -D);
    float epsilon   = light.cutoff - light.outerCutoff;
    float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);
    float d   = length(fragPos);
    float att = 1.0 / (light.constant + light.linear * d + light.quadratic * d * d);
    vec3 ambient  = light.ambient  * mat_ambient  * texColor;
    vec3 diffuse  = max(dot(N, L), 0.0) * light.diffuse * mat_diffuse * texColor;
    vec3 specular = pow(max(dot(R, V), 0.0), mat_shininess) * light.specular * mat_specular;
    return ambient * att + (diffuse + specular) * intensity * att;
}

void main() {
    vec3 N = normalize(vNormal);
    // Flip normal on backfaces so transparent objects are lit correctly
    // on both sides (backface culling is disabled for glass objects).
    if (!gl_FrontFacing) N = -N;

    vec3 V        = normalize(-vFragPos);
    vec3 texColor = texture(tex0, vTexCoord).rgb;

    // Directional light (sun)
    vec3 result = calcDirLight(dirLight, N, V, texColor);

    // Three point lights
    for (int i = 0; i < NUM_POINT_LIGHTS; i++) {
        result += calcPointLight(pointLights[i], N, vFragPos, V, texColor);
    }

    // Spotlight (headlight)
    if (spotLightOn != 0) {
        result += calcSpotLight(spotLight, N, vFragPos, V, texColor);
    }

    // Alpha controls opacity: 1.0 = opaque, <1.0 = transparent
    FragColor = vec4(result, mat_alpha);
}
