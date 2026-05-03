# Project Presentation: Minecraft Parkour 3D App

This document maps every requirement from the grading sheet to the exact code that satisfies it.

---

## What Is the Project?

A first-person 3D parkour game built in C++ with OpenGL. The player spawns on a Minecraft-style grass platform and must jump across floating blocks to reach a gold end platform. The world is lit by a moving sun, three orbiting colored point lights, and a toggleable camera headlight. Two OBJ models (an orbiting sphere and a spinning bunny trophy) float in the scene. Glass blocks are see-through. Particles burst on jump and on level completion. All settings (window size, VSync, title) are read from a JSON file.

**Controls at runtime:**
- `WASD` — move
- `Space` — jump
- `Mouse` — look around
- `Scroll wheel` — zoom (change FOV)
- `V` — toggle VSync
- `F` — toggle fullscreen
- `H` — toggle headlight spotlight
- `I` — toggle HUD overlay
- `R` — restart
- `Right-click` — release cursor
- `ESC` — quit

---

## ESSENTIALS

Each missing essential costs −25 points. Partial functionality gives partial penalty.

---

### 1. 3D GL Core Profile + Shaders + GL Debug Enabled + JSON Config File

**What it wants:**
The app must use OpenGL's *Core Profile* (no legacy "compatibility" API), GLSL shaders, a debug message callback so OpenGL errors are printed during development, and a JSON file to configure the window instead of hardcoded values.

**Where it is:**
- Core profile & version: [src/app.cpp:85-89](src/app.cpp#L85-L89)
- GL debug: [src/app.cpp:129-133](src/app.cpp#L129-L133)
- JSON config loading: [src/app.cpp:851-873](src/app.cpp#L851-L873)
- JSON config file: [resources/config.json](resources/config.json)
- Debug callback implementation: [src/gl_err_callback.cpp](src/gl_err_callback.cpp)
- Shader version declaration: [shaders/lighting.vert:1](shaders/lighting.vert#L1), [shaders/lighting.frag:1](shaders/lighting.frag#L1)

**Code sample — OpenGL Core Profile setup:**
```cpp
// app.cpp lines 85-89
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
glfwWindowHint(GLFW_OPENGL_PROFILE,        GLFW_OPENGL_CORE_PROFILE);
glfwWindowHint(GLFW_SAMPLES,               4);  // MSAA 4x
```

**Explanation:** `GLFW_OPENGL_CORE_PROFILE` tells the driver to create a Core profile context, removing all deprecated legacy functions. `FORWARD_COMPAT` enforces this on macOS. The version is 4.1 because macOS's hardware limit is OpenGL 4.1 (the assignment eval notes specifically exempt "MAC ~ GL 4.1").

**Code sample — GL debug callback:**
```cpp
// app.cpp lines 129-133
if (GLEW_ARB_debug_output) {
    glDebugMessageCallback(MessageCallback, nullptr);
    glEnable(GL_DEBUG_OUTPUT);
    std::cout << "GL_DEBUG enabled.\n";
}
```

**Explanation:** `glDebugMessageCallback` registers `MessageCallback` (in `gl_err_callback.cpp`). After this, OpenGL automatically calls that function whenever it encounters an error, deprecated behavior, or undefined behavior — the callback prints the source, type, severity, and message to the console.

**Code sample — JSON config:**
```json
// resources/config.json
{
  "window": {
    "width": 1280,
    "height": 720,
    "title": "OpenGL JSON Configured Window",
    "vsync": false,
    "msaa": false
  }
}
```

```cpp
// app.cpp — load_config() function
json j;
file >> j;
config.width  = j["window"].value("width",  800);
config.height = j["window"].value("height", 600);
config.title  = j["window"].value("title",  "OpenGL Window");
config.vsync  = j["window"].value("vsync",  false);
```

**Explanation:** The `nlohmann/json` library parses the JSON file. `value("key", default)` reads the setting, falling back to a sensible default if the key is missing. This lets you change window size or VSync without recompiling.

**Note on DSA (Direct State Access):** The requirement lists "no DSA" as an INSTAFAIL. The `Mesh.hpp` uses traditional bind-to-edit (`glBindVertexArray`, `glBindBuffer`) instead of the newer DSA API (`glCreateVertexArrays`, `glNamedBufferData`) — explicitly because DSA requires OpenGL 4.5+ which is unavailable on macOS. The comment in `Mesh.hpp` reads: *"Setup mesh VAO, VBO, EBO using bind-to-edit (Mac compatible)"*. This is covered by the hardware-limitation clause in the eval sheet.

---

### 2. High Performance ≥ 60 FPS (Display FPS)

**What it wants:**
The application must run at 60+ frames per second and show the current FPS somewhere visible to the user.

**Where it is:**
- FPS in window title: [src/app.cpp:764-770](src/app.cpp#L764-L770)
- FPS in ImGui HUD: [src/app.cpp:638](src/app.cpp#L638)
- `fps_meter` utility class: [src/fps_meter.hpp](src/fps_meter.hpp)

**Code sample — window title FPS:**
```cpp
// app.cpp lines 764-770
if (now - lastTime >= 1.0) {
    const double fps = frameCount / (now - lastTime);
    glfwSetWindowTitle(window,
        ("Minecraft Parkour — FPS: " + std::to_string(int(fps))).c_str());
    frameCount = 0;
    lastTime   = now;
}
```

**Explanation:** Every second, the total frames counted since the last update are divided by the elapsed time to get FPS, then that number is written into the window's title bar. So the title always shows something like "Minecraft Parkour — FPS: 120".

**Code sample — HUD FPS:**
```cpp
// app.cpp line 638 (inside draw_hud)
ImGui::Text("FPS  %.0f", ImGui::GetIO().Framerate);
```

**Explanation:** ImGui tracks its own frame rate internally. `GetIO().Framerate` returns a smoothed FPS value that is displayed in the top-left HUD overlay.

---

### 3. VSync Control, Antialiasing, Fullscreen ↔ Windowed (Restore Position & Size)

**What it wants:**
The user must be able to turn VSync on/off at runtime, the app must support antialiasing (smoother edges), and must support switching between fullscreen and windowed mode while remembering the window's previous position and size.

**Where it is:**
- VSync toggle: [src/callbacks.cpp:35-40](src/callbacks.cpp#L35-L40)
- MSAA enable: [src/app.cpp:89](src/app.cpp#L89) and [src/app.cpp:126](src/app.cpp#L126)
- Fullscreen toggle & position save/restore: [src/app.cpp:800-813](src/app.cpp#L800-L813)
- Saved state fields: [src/app.hpp:43-44](src/app.hpp#L43-L44)

**Code sample — VSync toggle (V key):**
```cpp
// callbacks.cpp lines 35-40
case GLFW_KEY_V:
    if (action == GLFW_PRESS) {
        app->vsync = !app->vsync;
        glfwSwapInterval(app->vsync ? 1 : 0);
        std::cout << "VSync: " << (app->vsync ? "ON" : "OFF") << '\n';
    }
    break;
```

**Explanation:** `glfwSwapInterval(1)` makes the driver wait for the monitor's vertical refresh before swapping buffers (cap at monitor Hz, removes tearing). `glfwSwapInterval(0)` disables this (uncapped FPS). The `bool vsync` tracks current state so V toggles it.

**Code sample — MSAA (Multisampling Antialiasing):**
```cpp
// app.cpp line 89 — request 4 samples per pixel during window creation
glfwWindowHint(GLFW_SAMPLES, 4);

// app.cpp line 126 — enable MSAA in OpenGL state
glEnable(GL_MULTISAMPLE);
```

**Explanation:** MSAA works by rendering each pixel at 4 sub-pixel sample points and averaging the results. This smooths jagged edges on polygon boundaries without requiring a separate full-scene pass.

**Code sample — fullscreen toggle (F key):**
```cpp
// app.cpp lines 800-813
void App::toggle_fullscreen(GLFWwindow* win) {
    if (isFullScreen) {
        // Restore saved windowed position and size
        glfwSetWindowMonitor(win, nullptr,
            savedXPos, savedYPos, savedWidth, savedHeight, GLFW_DONT_CARE);
        isFullScreen = false;
    } else {
        // Save current position and size before going fullscreen
        glfwGetWindowPos (win, &savedXPos,  &savedYPos);
        glfwGetWindowSize(win, &savedWidth, &savedHeight);
        GLFWmonitor*       mon  = GetCurrentMonitor(win);
        const GLFWvidmode* mode = glfwGetVideoMode(mon);
        glfwSetWindowMonitor(win, mon, 0, 0, mode->width, mode->height, mode->refreshRate);
        isFullScreen = true;
    }
}
```

**Explanation:** When going fullscreen, the current `x, y, width, height` are saved into `savedXPos/Y/Width/Height`. `glfwSetWindowMonitor` with a non-null monitor argument switches to fullscreen at the native resolution. When toggling back, passing `nullptr` as monitor returns to windowed mode and the saved values restore the exact previous position and size. `GetCurrentMonitor()` finds which physical monitor the window is mostly on so dual-monitor setups work correctly.

---

### 4. Event Processing: Mouse (Both Axes + Wheel), Keyboard

**What it wants:**
The application must respond to: mouse movement on both horizontal and vertical axes (typically for camera look), mouse scroll wheel, and keyboard input for movement and app control.

**Where it is:**
- Mouse cursor (both axes): [src/callbacks.cpp:93-104](src/callbacks.cpp#L93-L104)
- Mouse scroll wheel: [src/callbacks.cpp:66-70](src/callbacks.cpp#L66-L70)
- Mouse buttons: [src/callbacks.cpp:80-91](src/callbacks.cpp#L80-L91)
- Keyboard: [src/callbacks.cpp:11-64](src/callbacks.cpp#L11-L64)
- WASD movement read in physics: [src/app.cpp:491-494](src/app.cpp#L491-L494)
- Camera mouse processing: [src/Camera.hpp:81-98](src/Camera.hpp#L81-L98)

**Code sample — mouse cursor movement:**
```cpp
// callbacks.cpp lines 93-104
void App::glfw_cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));

    const double dx = xpos - app->cursorLastX;
    const double dy = app->cursorLastY - ypos;  // y grows downward in screen space
    app->cursorLastX = xpos;
    app->cursorLastY = ypos;

    // Only rotate camera when cursor is captured
    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        app->camera.ProcessMouseMovement(static_cast<float>(dx), static_cast<float>(dy));
}
```

**Explanation:** GLFW calls this every time the cursor moves. `dx` is horizontal delta (left/right), `dy` is vertical delta (up/down — the minus sign corrects OpenGL's Y axis being opposite to screen space). Both deltas are forwarded to the camera. The camera only rotates when the cursor is "captured" (hidden) to avoid turning when the player releases the mouse.

**Code sample — scroll wheel (FOV zoom):**
```cpp
// callbacks.cpp lines 66-70
void App::glfw_scroll_callback(GLFWwindow* window, double /*xoffset*/, double yoffset) {
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    app->fov = std::clamp(app->fov - static_cast<float>(yoffset) * 3.0f, 20.0f, 120.0f);
    app->update_projection_matrix();
}
```

**Explanation:** Scrolling up (positive `yoffset`) decreases the field-of-view (zooms in); scrolling down increases it (zooms out). The `std::clamp` keeps FOV in a sensible [20°, 120°] range. The projection matrix is rebuilt immediately so the change takes effect next frame.

---

### 5. Multiple Different Independently Moving 3D Models (At Least Two Loaded from File)

**What it wants:**
There must be multiple 3D objects in the scene that move independently of each other. At least two of those objects must be loaded from external 3D model files (not procedurally generated in code).

**Where it is:**
- Model loading (OBJ): [src/app.cpp:213-232](src/app.cpp#L213-L232)
- Independent animation each frame: [src/app.cpp:712-723](src/app.cpp#L712-L723)
- `Model` class that wraps OBJ files: [src/Model.hpp](src/Model.hpp)
- OBJ loader: [src/OBJloader.cpp](src/OBJloader.cpp), [src/OBJloader.hpp](src/OBJloader.hpp)

**Code sample — loading two OBJ models:**
```cpp
// app.cpp lines 213-232
// Orbiting sphere above the start of the course
deco_orb = std::make_shared<Model>(
    std::filesystem::path("../obj_samples/sphere_tri_vnt.obj"), lit);
deco_orb->setScale(glm::vec3(1.2f));
deco_orb->meshes[0].texture = texture_library.at("box");

// Spinning trophy bunny above the end platform
trophy_bunny = std::make_shared<Model>(
    std::filesystem::path("../obj_samples/bunny_tri_vn.obj"), lit);
trophy_bunny->setScale(glm::vec3(0.07f));
trophy_bunny->setPosition(glm::vec3(38.0f, 10.0f, 0.0f));
trophy_bunny->meshes[0].texture = texture_library.at("white");
```

**Explanation:** `Model::Model(path, shader)` calls `loadOBJ(filename, vertices, indices)` which reads the `.obj` file from disk and populates vertex/index buffers. The sphere is loaded from `sphere_tri_vnt.obj` (triangle mesh with vertices, normals, and texture coords) and the bunny from `bunny_tri_vn.obj` (vertices and normals). They are stored as shared pointers managed separately.

**Code sample — independent movement each frame:**
```cpp
// app.cpp lines 712-723
orb_angle  += dt * 0.4f;
bunny_spin += dt * 90.0f;  // degrees/s

if (deco_orb) {
    // Orbits above the course at height 8, radius 6 around x=19
    glm::vec3 centre{19.0f, 0.0f, 0.0f};
    deco_orb->setPosition(centre + glm::vec3(
        6.0f * std::cos(orb_angle), 8.0f, 6.0f * std::sin(orb_angle)));
}
if (trophy_bunny) {
    trophy_bunny->setEulerAngles(glm::vec3(0.0f, bunny_spin, 0.0f));
}
```

**Explanation:** Every frame `dt` (delta time in seconds) is added to separate angle accumulators. The sphere traces a horizontal circle of radius 6 at height 8 using sine/cosine — `cos(angle)` drives X, `sin(angle)` drives Z, giving circular motion. The bunny simply rotates around its Y axis at 90 degrees per second. These are completely independent of each other.

---

### 6. Custom Shader Effect

**What it wants:**
The project must include at least one shader with a visually interesting or non-trivial custom effect beyond plain texture rendering.

**Where it is:**
- Rainbow animated shader: [shaders/GL_rainbow.frag](shaders/GL_rainbow.frag)
- Particle shader (color + alpha fade): [shaders/particle.frag](shaders/particle.frag), [shaders/particle.vert](shaders/particle.vert)
- Phong shading (full lighting model) in: [shaders/lighting.frag](shaders/lighting.frag)

**Code sample — rainbow shader:**
```glsl
// shaders/GL_rainbow.frag
#version 410 core

uniform float iTime;
out vec4 FragColor;

void main() {
    const float pi    = 3.1415f;
    const float pi3   = pi / 3.0f;
    const float d_pi3 = pi3 * 2.0f;

    FragColor = vec4(
        abs(sin(           gl_FragCoord.y * 0.1f + 3.0f * iTime)),
        abs(sin(pi3   +    gl_FragCoord.y * 0.1f + 3.0f * iTime)),
        abs(sin(d_pi3 +    gl_FragCoord.y * 0.1f + 3.0f * iTime)),
        1.0f
    );
}
```

**Explanation:** This fragment shader computes color entirely on the GPU with no texture. It uses the fragment's screen-space Y coordinate (`gl_FragCoord.y`) to make horizontal bands, and the three RGB channels are driven by sine waves offset by 120° (`pi/3`, `2*pi/3`) from each other — producing a full-spectrum rainbow that scrolls vertically over time (because `iTime` advances each frame). `abs(sin(...))` keeps values in [0,1].

**Code sample — particle alpha fade:**
```glsl
// shaders/particle.frag
#version 410 core
out vec4 FragColor;
uniform vec4 uColor;  // rgb = particle color, a = fade factor

void main() {
    FragColor = uColor;
}
```

```cpp
// Particle.hpp — alpha is set from remaining lifetime ratio
const float alpha = glm::max(p.lifetime / p.maxLifetime, 0.0f);
shader.setUniform("uColor", glm::vec4(p.color, alpha));
```

**Explanation:** Particles are born with `alpha = 1.0` and the alpha decreases linearly as `lifetime` drops toward zero. This makes them fade out smoothly. The fragment shader just outputs whatever the CPU sends as `uColor`, so the fade is fully driven from the C++ side.

---

### 7. At Least Three Different Textures

**What it wants:**
The scene must use at least three distinct textures (or sub-textures from a texture atlas).

**Where it is:**
- Texture loading: [src/app.cpp:177-190](src/app.cpp#L177-L190)
- Texture class: [src/Texture.hpp](src/Texture.hpp), [src/Texture.cpp](src/Texture.cpp)
- Texture files: [resources/textures/tex_256.png](resources/textures/tex_256.png), [resources/textures/box.png](resources/textures/box.png)

**The three textures:**

| Name | File | Used for |
|------|------|----------|
| `atlas` | `tex_256.png` | All Minecraft blocks (grass, dirt, stone, glass, etc.) via UV subtiles |
| `box` | `box.png` | The decorative orbiting sphere model |
| `white` | 1×1 white pixel (generated in code) | Trophy bunny model (diffuse color from material drives appearance) |

**Code sample:**
```cpp
// app.cpp lines 177-190
// Minecraft terrain atlas (16×16 tiles) — one texture for ALL block types
texture_library.emplace("atlas",
    std::make_shared<Texture>(
        std::filesystem::path("../resources/textures/tex_256.png"),
        Texture::Interpolation::nearest));

// Solid white: lets material diffuse color control appearance of OBJ models
texture_library.emplace("white", std::make_shared<Texture>(glm::vec3(1.0f)));

// Box texture for the decorative sphere
texture_library.emplace("box",
    std::make_shared<Texture>(
        std::filesystem::path("../resources/textures/box.png")));
```

**Explanation of the atlas subtexture system:**
```cpp
// Level.hpp — each block face maps to a different tile column/row in the atlas
{ BlockType::Grass, { {0,0}, {0,1}, {2,0}, false, 1.0f } },
//                      top   sides  bottom

inline glm::vec2 atlas_uv_min(AtlasTile t) {
    return { t.col / 16.0f, 1.0f - (t.row + 1) / 16.0f };
}
```

**Explanation:** `tex_256.png` is a 256×256 Minecraft texture atlas organized as a 16×16 grid of 16×16 pixel tiles. Each `BlockDef` specifies which grid cell (column, row) to use for the top, side, and bottom faces. The UV formula converts tile coordinates to normalized [0,1] UV coordinates, so the same single texture serves all block appearances.

---

### 8. Lighting Model: All Basic Light Types (Ambient, Directional, 2× Point, Spotlight; At Least Two Moving)

**What it wants:**
The scene must implement the Phong (or equivalent) lighting model with: ambient light, at least one directional light (like the sun), at least two point lights (omnidirectional, attenuated), and at least one spotlight (reflector/cone light). At least two of the lights must visibly move.

**Where it is:**
- Light structs: [src/Lights.hpp](src/Lights.hpp)
- Light initialization: [src/app.cpp:267-328](src/app.cpp#L267-L328)
- Light animation (every frame): [src/app.cpp:691-709](src/app.cpp#L691-L709)
- Lighting uniforms upload: [src/app.cpp:334-367](src/app.cpp#L334-L367)
- Lighting shader implementation: [shaders/lighting.frag](shaders/lighting.frag)

**Light summary:**

| Light | Type | Moving? |
|-------|------|---------|
| `dirLight` | Directional (sun) | Yes — angle changes over time |
| `pointLights[0]` | Point (red/warm) | Yes — orbits x=10 |
| `pointLights[1]` | Point (blue/cool) | Yes — orbits x=22 |
| `pointLights[2]` | Point (gold/warm) | Yes — orbits x=38 |
| `spotLight` | Spotlight (camera headlight) | Yes — follows camera |

**Code sample — ambient light (embedded in every light):**
```cpp
// app.cpp line 271
dirLight.ambient = glm::vec3(0.06f);  // dim global fill light
```

```glsl
// lighting.frag — ambient component of directional light
vec3 ambient = light.ambient * mat_ambient;
```

**Explanation:** Ambient light is a constant minimum brightness applied everywhere regardless of surface orientation or distance. It simulates indirect bounced light from the environment.

**Code sample — directional light (animated sun):**
```cpp
// app.cpp lines 691-703
const float angle = static_cast<float>(now) * 0.20f;
const float elev  = std::sin(angle);
dirLight.direction = glm::normalize(glm::vec3(
    std::cos(angle), -std::abs(elev) - 0.1f, std::sin(angle)));

const float t = glm::clamp(std::abs(elev), 0.0f, 1.0f);
const glm::vec3 sunColor = glm::mix(
    glm::vec3(1.0f, 0.4f, 0.1f),   // orange at horizon
    glm::vec3(1.0f, 0.97f, 0.88f), // white-yellow at zenith
    t);
dirLight.diffuse  = sunColor * bright * 0.85f;
dirLight.specular = sunColor * bright * 0.95f;
```

**Explanation:** The sun rotates around the scene — its direction is computed from `cos/sin(time)`. The elevation `sin(angle)` also changes the color (orange at low angles like sunrise/sunset, white at noon) and brightness. This gives a day/night-cycle visual effect.

**Code sample — point lights orbiting:**
```cpp
// Lights.hpp — orbit position computed from angle each frame
void updatePositions() {
    for (std::size_t i = 0; i < NUM_POINT_LIGHTS; ++i) {
        position[i] = orbitCenter[i] + glm::vec3{
            orbitRadius[i] * glm::cos(orbitAngle[i]),
            orbitHeight[i],
            orbitRadius[i] * glm::sin(orbitAngle[i]),
        };
    }
}
```

```glsl
// lighting.frag — point light with quadratic attenuation
vec3 calcPointLight(int i, vec3 N, vec3 fragPos, vec3 V, vec3 texColor) {
    vec3 toLight = pointLights.position[i] - fragPos;
    float d = sqrt(dot(toLight, toLight));
    float att = 1.0 / (pointLights.constant[i]
                     + pointLights.linear[i]   * d
                     + pointLights.quadratic[i] * d * d);
    // ... diffuse + specular ...
    return (ambient + diffuse + specular) * texColor * att;
}
```

**Explanation:** Each point light orbits a center point at a given radius and height. The attenuation formula `1 / (c + l*d + q*d²)` makes light intensity fall off with distance — constant term `c` keeps it from dividing by zero at distance 0, linear term `l` gives gentle falloff, quadratic term `q*d²` ensures it reaches zero at long range.

**Code sample — spotlight (camera headlight):**
```cpp
// app.cpp lines 317-328
spotLight.direction   = glm::vec3(0.0f, 0.0f, -1.0f);  // forward in view space
spotLight.cutoff      = std::cos(glm::radians(12.5f));  // inner cone
spotLight.outerCutoff = std::cos(glm::radians(17.5f));  // outer (soft) edge
```

```glsl
// lighting.frag — spotlight cone calculation
float theta     = dot(L, -D);
float epsilon   = light.cutoff - light.outerCutoff;
float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);
```

**Explanation:** The spotlight is attached to the camera — its position is the view-space origin (the camera eye) and direction is `(0,0,-1)` (straight ahead in view space). `theta` is the cosine of the angle between the fragment direction and the light direction. The `clamp(...)` formula creates a smooth falloff zone between the inner cone angle (12.5°) and the outer angle (17.5°), avoiding a hard edge cutoff.

---

### 9. Correct Full Alpha Scale Transparency (At Least Two Transparent Objects)

**What it wants:**
At least two objects in the scene must be genuinely semi-transparent using a continuous [0.0, 1.0] alpha range — not the cheap hack of simply discarding fragments below a threshold (`if(alpha < 0.1) discard;`).

**Where it is:**
- Transparent block definition: [src/Level.hpp:82](src/Level.hpp#L82)
- Transparency render pass (sorting + blend): [src/app.cpp:449-477](src/app.cpp#L449-L477)
- Particle alpha fade: [src/Particle.hpp:107-113](src/Particle.hpp#L107-L113)
- `mat_alpha` uniform used in shader: [shaders/lighting.frag:141](shaders/lighting.frag#L141)

**Two transparent objects:** Glass blocks (alpha = 0.5) and particles (alpha fades from 1.0 to 0.0 over lifetime).

**Code sample — glass block definition:**
```cpp
// Level.hpp line 82
{ BlockType::Glass, { {1,3}, {1,3}, {1,3}, true, 0.5f } },
//                                          ^     ^
//                                     transparent  alpha=0.5
```

**Code sample — correct painter's algorithm (back-to-front sorting + blending):**
```cpp
// app.cpp lines 451-476
// Collect transparent blocks, sort from farthest to nearest
std::vector<const LevelBlock*> glass_blocks;
for (const auto& block : level.blocks)
    if (block_defs().at(block.type).transparent)
        glass_blocks.push_back(&block);

std::sort(glass_blocks.begin(), glass_blocks.end(),
    [&](const LevelBlock* a, const LevelBlock* b) {
        return glm::distance(camera.Position, glm::vec3(a->grid))
             > glm::distance(camera.Position, glm::vec3(b->grid));
    });

glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glDepthMask(GL_FALSE);  // don't write to depth buffer during blend pass

for (const LevelBlock* b : glass_blocks) {
    const float alpha = block_defs().at(b->type).alpha;  // 0.5 for glass
    draw_block(*b, view, alpha);
}
```

**Explanation of why sorting matters:** When rendering semi-transparent objects, the GPU needs to combine the fragment color with whatever is already in the framebuffer behind it. If you draw a near glass block before a far glass block, the far block won't have anything behind it to blend with yet. Sorting farthest-first (painter's algorithm) ensures each transparent object is drawn on top of already-rendered background.

`glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)` computes: `output = src_color * src_alpha + dst_color * (1 - src_alpha)` — the classic semi-transparent mix. `glDepthMask(GL_FALSE)` prevents transparent fragments from blocking each other in the depth buffer.

**Code sample — shader outputs alpha (no discard):**
```glsl
// lighting.frag line 141
FragColor = vec4(result, mat_alpha);
// mat_alpha is 0.5 for glass, 1.0 for opaque blocks — full continuous range
```

**Explanation:** There is no `if(alpha < threshold) discard;` anywhere. The alpha value is passed through in full as the 4th component of `FragColor`, letting the hardware blending equation handle all transparency levels smoothly.

---

### 10. Correct Collisions

**What it wants:**
The player must collide correctly with the 3D geometry — not falling through floors, being pushed out of walls, and bouncing off ceilings.

**Where it is:**
- Spatial grid build: [src/app.cpp:252-261](src/app.cpp#L252-L261)
- Collision resolution: [src/app.cpp:546-605](src/app.cpp#L546-L605)
- Physics integration (calls collision): [src/app.cpp:484-543](src/app.cpp#L484-L543)
- Player bounding box constants: [src/Player.hpp](src/Player.hpp)

**How it works — two-phase physics:**
1. Move vertically, resolve Y collisions → detects landing on top and hitting ceiling
2. Move horizontally, resolve XZ collisions → detects walking into walls

**Code sample — spatial grid (performance optimization):**
```cpp
// app.cpp lines 255-261
for (const auto& block : level.blocks) {
    int cell_x = static_cast<int>(std::floor(block.grid.x / GRID_CELL_SIZE));
    int cell_z = static_cast<int>(std::floor(block.grid.z / GRID_CELL_SIZE));
    long long key = (static_cast<long long>(cell_x) << 32) | (cell_z & 0xFFFFFFFF);
    collision_grid[key].push_back(&block);
}
```

**Explanation:** Instead of checking every block in the level each frame, blocks are grouped into 16×16 grid cells. Only the 4–9 cells the player actually overlaps are checked. This keeps collision detection O(1) regardless of level size.

**Code sample — AABB collision resolution:**
```cpp
// app.cpp lines 574-604
// Quick axis-aligned bounding box (AABB) rejection
if (px2 <= bx1 || px1 >= bx2) continue;
if (py2 <= by1 || py1 >  by2) continue;
if (pz2 <= bz1 || pz1 >= bz2) continue;

// Determine which axis to resolve based on previous position
const bool came_from_above = prev_feet_y        >= by2 - 0.02f;
const bool came_from_below = prev_feet_y + H    <= by1 + 0.02f;

if (came_from_above && player.vel_y <= 0.05f) {
    // Landing on top of block
    player.feet.y   = by2;
    player.vel_y    = 0.0f;
    player.grounded = true;
} else if (came_from_below && player.vel_y > 0.0f) {
    // Head hit ceiling
    player.feet.y = by1 - H;
    player.vel_y  = 0.0f;
} else {
    // Horizontal push-out: find the minimum penetration axis
    const float pen_x = std::min(px2 - bx1, bx2 - px1);
    const float pen_z = std::min(pz2 - bz1, bz2 - pz1);
    if (pen_x < pen_z) {
        player.feet.x = (player.feet.x < bp.x) ? bx1 - R : bx2 + R;
    } else {
        player.feet.z = (player.feet.z < bp.z) ? bz1 - R : bz2 + R;
    }
}
```

**Explanation:** The player bounding box is a capsule approximated as a rectangle: `[feet.x ± R, feet.y to feet.y + H, feet.z ± R]`. For each overlapping block: the "came from above/below" test (using the position from the *previous* frame's Y) decides whether this is a floor or ceiling collision and resolves it vertically. If neither, the minimum-penetration axis on XZ is chosen to push the player out of the wall — this prevents jittering at corners.

---

## EXTRAS

Each working extra = +10 points.

---

### EXTRA: Particle Effects ✓ IMPLEMENTED

**What it wants:**
A particle system that produces visible particle effects during gameplay.

**Where it is:**
- `ParticleSystem` class: [src/Particle.hpp](src/Particle.hpp)
- Jump particles emitted: [src/app.cpp:515-517](src/app.cpp#L515-L517)
- Win particles emitted: [src/app.cpp:740-744](src/app.cpp#L740-L744)
- Particle mesh (tetrahedron): [src/app.cpp:236-245](src/app.cpp#L236-L245)
- Particle shaders: [shaders/particle.vert](shaders/particle.vert), [shaders/particle.frag](shaders/particle.frag)

**Two particle events:**
1. **Dust puff** — 6 particles emitted at the player's feet each time they jump
2. **Firework burst** — 60 particles emitted above the end platform when the player wins

**Code sample — emitting jump particles:**
```cpp
// app.cpp lines 515-517
particle_system.emit(player.feet,
    glm::vec3(0.55f, 0.45f, 0.30f),  // sandy/dust color
    6,    // count
    1.8f, // speed m/s
    0.35f // lifetime seconds
);
```

**Code sample — particle physics update:**
```cpp
// Particle.hpp lines 74-91
void update(float dt) {
    for (auto& p : particles_) {
        p.velocity.y += GRAVITY * dt;   // gravity pulls down
        p.position   += p.velocity * dt; // integrate position

        if (p.position.y < FLOOR_Y) {   // simple floor bounce
            p.position.y  = FLOOR_Y;
            p.velocity.y  = -p.velocity.y * BOUNCE_DAMP;  // lose energy on bounce
            p.velocity.x *= LATERAL_DAMP;
            p.velocity.z *= LATERAL_DAMP;
        }
        p.lifetime -= dt;
    }
    std::erase_if(particles_, [](const Particle& p) { return p.lifetime <= 0.0f; });
}
```

**Explanation:** Each particle has its own position, velocity, color, and remaining lifetime. Gravity is applied as an acceleration (`vel.y += GRAVITY * dt`), then velocity is integrated into position. When a particle hits the ground, it bounces with damping (loses 55% of vertical energy, 12% of horizontal energy per bounce). Dead particles (lifetime ≤ 0) are removed. All particles share one mesh (a tetrahedron), drawn instanced-style with per-particle model matrices and colors uploaded as uniforms.

---

### EXTRA: Height Map — NOT IMPLEMENTED

The level is built from discrete JSON-defined blocks, not a height map. There is no terrain heightmap texture or height-driven player elevation system.

---

### EXTRA: Audio — NOT IMPLEMENTED

There is no audio library (`miniaudio`, `irrKlang`, `FMOD`, etc.) in the project. No sound effects or music are played.

---

### EXTRA: Scripting — NOT IMPLEMENTED

There is no scripting engine (Lua, Python, etc.) integrated into the project.

---

### EXTRA: Other Nice Complicated Effect — PARTIAL

The `GL_rainbow.frag` animated rainbow shader is present. Whether this qualifies as a "nice complicated effect" for extra points is at the teacher's discretion, but it demonstrates GPU-side procedural animation with no texture dependency.

---

## INSTAFAIL Checklist

| Forbidden thing | Status |
|-----------------|--------|
| GLUT library | Not used. GLFW is used instead. |
| GL Compatibility profile | Not used. `GLFW_OPENGL_CORE_PROFILE` is explicitly set. |
| No DSA (Direct State Access) | **Partial exception — see below.** |

**DSA Note:** `Mesh.hpp` and `Texture.cpp` use the traditional bind-to-edit API (`glGenBuffers` + `glBindBuffer`) rather than the newer DSA API (`glCreateBuffers` + `glNamedBufferData`). This is explicitly because macOS only supports OpenGL 4.1, and the DSA-style buffer/array creation functions (`glCreateVertexArrays`, etc.) require OpenGL 4.5. The `Mesh.hpp` comment reads: *"Setup mesh VAO, VBO, EBO using bind-to-edit (Mac compatible)"*. The assignment eval sheet explicitly exempts Mac hardware: *"Hardware limitation might apply (eg. no mouse wheel on notebook, MAC ~ GL 4.1 etc.), in that case the subtask can be ignored."*

---

## File Map Quick Reference

| File | What it does |
|------|-------------|
| [src/main.cpp](src/main.cpp) | Entry point — creates `App`, calls `init()` + `run()` |
| [src/app.cpp](src/app.cpp) | All game logic: init, render loop, physics, lighting, draw calls |
| [src/app.hpp](src/app.hpp) | `App` class definition with all members and method declarations |
| [src/callbacks.cpp](src/callbacks.cpp) | GLFW input callbacks (keyboard, mouse, scroll, resize) |
| [src/Camera.hpp](src/Camera.hpp) | First-person camera: view matrix, mouse/keyboard input |
| [src/Player.hpp](src/Player.hpp) | Player physics state (feet position, velocity, jump flag) |
| [src/Level.hpp](src/Level.hpp) | Level loading from JSON, block type definitions, atlas UV math, mesh generation |
| [src/Lights.hpp](src/Lights.hpp) | `DirectionalLight`, `PointLights`, `SpotLight` structs |
| [src/Particle.hpp](src/Particle.hpp) | Full particle system: emit, update, draw |
| [src/Model.hpp](src/Model.hpp) | OBJ model wrapper with transform methods |
| [src/Mesh.hpp](src/Mesh.hpp) | VAO/VBO/EBO upload and `draw()` call |
| [src/Texture.cpp/hpp](src/Texture.cpp) | Texture loading via OpenCV, GPU upload, mipmap generation |
| [src/ShaderProgram.hpp](src/ShaderProgram.hpp) | GLSL shader compile, link, `setUniform()` helpers |
| [src/OBJloader.cpp/hpp](src/OBJloader.cpp) | Parses `.obj` files into vertex/index arrays |
| [src/fps_meter.hpp](src/fps_meter.hpp) | FPS measurement utility using `std::chrono` |
| [src/gl_err_callback.cpp](src/gl_err_callback.cpp) | OpenGL debug message callback handler |
| [shaders/lighting.vert/frag](shaders/lighting.vert) | Phong lighting shader for all blocks and OBJ models |
| [shaders/particle.vert/frag](shaders/particle.vert) | Unlit colored particle shader |
| [shaders/GL_rainbow.frag](shaders/GL_rainbow.frag) | Custom animated rainbow procedural shader |
| [resources/config.json](resources/config.json) | Window settings (size, title, vsync, msaa) |
| [resources/level.json](resources/level.json) | Level block layout, start/end positions |
| [resources/textures/tex_256.png](resources/textures/tex_256.png) | Minecraft 16×16 texture atlas |
| [resources/textures/box.png](resources/textures/box.png) | Box texture for the decorative sphere |

---

## Points Estimate

| Category | Status | Points |
|----------|--------|--------|
| Start | — | +100 |
| GL Core + shaders + debug + JSON | Fully implemented (GL 4.1 due to Mac) | 0 |
| ≥60 FPS + display | Implemented | 0 |
| VSync + antialiasing + fullscreen | All implemented | 0 |
| Mouse (both axes + wheel) + keyboard | All implemented | 0 |
| 2+ moving models from file | Sphere + bunny OBJ, independent motion | 0 |
| Custom shader effect | Rainbow + particle shaders | 0 |
| 3+ textures | Atlas + box + white | 0 |
| All light types (2+ moving) | Dir + 3 point + spotlight, all animated | 0 |
| Full alpha transparency | Glass + particles, sorted, no discard | 0 |
| Correct collisions | AABB with spatial grid | 0 |
| **EXTRA: Particles** | Jump dust + win firework | **+10** |
| **Estimated total** | | **~110** → capped at 100 → **A** |
