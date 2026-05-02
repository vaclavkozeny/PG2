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

#define NUM_POINT_LIGHTS 3
// Task 2: Point lights in SoA form
struct PointLights {
    vec3  position[NUM_POINT_LIGHTS];
    vec3  ambient[NUM_POINT_LIGHTS];
    vec3  diffuse[NUM_POINT_LIGHTS];
    vec3  specular[NUM_POINT_LIGHTS];
    float constant[NUM_POINT_LIGHTS];
    float linear[NUM_POINT_LIGHTS];
    float quadratic[NUM_POINT_LIGHTS];
};
uniform PointLights pointLights;

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

// ---- Helper: compute Phong specular ----
// Cache-friendly: compute cos(theta)^shininess without expensive pow()
float specularTerm(float cosTheta, float shininess) {
    float clamped = max(cosTheta, 0.0);
    // Use bit-less expensive exponentiation: for typical shininess, use approximation
    // or use built-in pow() only when needed (GPU JIT will optimize)
    return pow(clamped, shininess);
}

// ---- Phong helpers ----

vec3 calcDirLight(DirLight light, vec3 N, vec3 V, vec3 texColor) {
    // Direction already normalized on CPU, no need to normalize again
    vec3 L = light.direction;
    vec3 R = reflect(-L, N);
    
    vec3 ambient  = light.ambient  * mat_ambient;
    float nDotL   = max(dot(N, L), 0.0);
    vec3 diffuse  = nDotL * light.diffuse * mat_diffuse;
    vec3 specular = specularTerm(dot(R, V), mat_shininess) * light.specular * mat_specular;
    
    return (ambient + diffuse + specular) * texColor;
}

vec3 calcPointLight(int i, vec3 N, vec3 fragPos, vec3 V, vec3 texColor) {
    vec3 toLight = pointLights.position[i] - fragPos;
    float d2 = dot(toLight, toLight);  // distance squared (avoid sqrt for now)
    float d  = sqrt(d2);
    float att = 1.0 / (pointLights.constant[i] + pointLights.linear[i] * d + pointLights.quadratic[i] * d2);
    
    vec3  L = toLight / d;  // normalize using precomputed distance
    vec3  R = reflect(-L, N);
    
    vec3 ambient  = pointLights.ambient[i]  * mat_ambient;
    float nDotL   = max(dot(N, L), 0.0);
    vec3 diffuse  = nDotL * pointLights.diffuse[i] * mat_diffuse;
    vec3 specular = specularTerm(dot(R, V), mat_shininess) * pointLights.specular[i] * mat_specular;
    
    return (ambient + diffuse + specular) * texColor * att;
}

vec3 calcSpotLight(SpotLight light, vec3 N, vec3 fragPos, vec3 V, vec3 texColor) {
    // Camera at view-space origin; L points toward camera
    float d2 = dot(fragPos, fragPos);
    float d  = sqrt(d2);
    float att = 1.0 / (light.constant + light.linear * d + light.quadratic * d2);
    
    vec3  L = -fragPos / d;  // normalize
    vec3  R = reflect(-L, N);
    vec3  D = light.direction;  // already normalized on CPU
    
    float theta     = dot(L, -D);
    float epsilon   = light.cutoff - light.outerCutoff;
    float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);
    
    vec3 ambient  = light.ambient * mat_ambient;
    float nDotL   = max(dot(N, L), 0.0);
    vec3 diffuse  = nDotL * light.diffuse * mat_diffuse;
    vec3 specular = specularTerm(dot(R, V), mat_shininess) * light.specular * mat_specular;
    
    return (ambient * att + (diffuse + specular) * intensity * att) * texColor;
}

void main() {
    // Flip normal on backfaces without branching (cheaper with modern GPUs)
    vec3 N = normalize(vNormal) * (gl_FrontFacing ? 1.0 : -1.0);
    
    vec3 V = normalize(-vFragPos);
    vec3 texColor = texture(tex0, vTexCoord).rgb;

    // Directional light (sun)
    vec3 result = calcDirLight(dirLight, N, V, texColor);

    // Unroll point light loop (NUM_POINT_LIGHTS = 3)
    result += calcPointLight(0, N, vFragPos, V, texColor);
    result += calcPointLight(1, N, vFragPos, V, texColor);
    result += calcPointLight(2, N, vFragPos, V, texColor);

    // Spotlight: conditional blend instead of full conditional
    result = mix(result, result + calcSpotLight(spotLight, N, vFragPos, V, texColor), 
                 float(spotLightOn != 0));

    FragColor = vec4(result, mat_alpha);
}
