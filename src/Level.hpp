#pragma once

#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include "Mesh.hpp"
#include "assets.hpp"

struct AtlasTile { int col, row; };

// Block types supported by the level loader.
// Related to entries in BLOCK_DEFS.
enum class BlockType {
    Grass,
    Dirt,
    Stone,
    Cobblestone,
    Planks,
    Sand,
    Log,
    Gold,
    Glass,   // transparent
};

// UV helpers — tex_256.png is flipped vertically by Texture.cpp
// (cv::flip before upload), so PNG row 0 maps to GL V≈1.
//
// Formula for tile at (col, row) in PNG:
//   U  = [col/16,   (col+1)/16]
//   V  = [1-(row+1)/16, 1-row/16]
inline glm::vec2 atlas_uv_min(AtlasTile t) {
    return { t.col / 16.0f, 1.0f - (t.row + 1) / 16.0f };
}
inline glm::vec2 atlas_uv_max(AtlasTile t) {
    return { (t.col + 1) / 16.0f, 1.0f - t.row / 16.0f };
}

// BlockDef — UV tiles for each face of a cube block.
// + Transparent material settings
struct BlockDef {
    AtlasTile top;
    AtlasTile sides;
    AtlasTile bottom;
    bool  transparent{false};
    float alpha      {1.0f};
};

inline const std::unordered_map<BlockType, BlockDef>& block_defs() {
    // Map UV tiles to defs
    static const std::unordered_map<BlockType, BlockDef> defs {
        { BlockType::Grass,       { {0,0}, {4,0}, {2,0}, false, 1.0f } },
        { BlockType::Dirt,        { {2,0}, {2,0}, {2,0}, false, 1.0f } },
        { BlockType::Stone,       { {1,0}, {1,0}, {1,0}, false, 1.0f } },
        { BlockType::Cobblestone, { {5,0}, {5,0}, {5,0}, false, 1.0f } },
        { BlockType::Planks,      { {4,0}, {4,0}, {4,0}, false, 1.0f } },
        { BlockType::Sand,        { {2,1}, {2,1}, {2,1}, false, 1.0f } },
        { BlockType::Log,         { {5,1}, {4,1}, {5,1}, false, 1.0f } },
        { BlockType::Gold,        { {7,1}, {7,1}, {7,1}, false, 1.0f } },
        { BlockType::Glass,       { {1,3}, {1,3}, {1,3}, true,  0.5f } },
    };
    return defs;
}

// helper fn 
inline BlockType block_type_from_string(const std::string& s) {
    if (s == "grass")       return BlockType::Grass;
    if (s == "dirt")        return BlockType::Dirt;
    if (s == "stone")       return BlockType::Stone;
    if (s == "cobblestone") return BlockType::Cobblestone;
    if (s == "planks")      return BlockType::Planks;
    if (s == "sand")        return BlockType::Sand;
    if (s == "log")         return BlockType::Log;
    if (s == "gold")        return BlockType::Gold;
    if (s == "glass")       return BlockType::Glass;
    return BlockType::Stone;
}

// One block instance in the level.
struct LevelBlock {
    glm::ivec3 grid;
    BlockType  type;
};

// Level — all blocks + start/end meta.
struct Level {
    std::vector<LevelBlock> blocks;
    glm::vec3 start_eye_pos{0.0f, 2.1f, 0.0f};
    glm::vec3 end_pos      {38.0f, 7.5f, 0.5f};
    float     end_radius   {2.5f};

    void load(const std::filesystem::path& path) {
        std::ifstream f(path);
        if (!f.is_open())
            throw std::runtime_error("Cannot open level: " + path.string());

        nlohmann::json j = nlohmann::json::parse(f);

        auto to_vec3 = [](const nlohmann::json& a) {
            return glm::vec3{ a[0].get<float>(), a[1].get<float>(), a[2].get<float>() };
        };

        start_eye_pos = to_vec3(j["start_eye_pos"]);
        end_pos       = to_vec3(j["end_pos"]);
        end_radius    = j.value("end_radius", 2.5f);

        for (const auto& b : j["blocks"]) {
            if (!b.contains("x") || !b.contains("y") || !b.contains("z")) {
                continue;
            }
            LevelBlock lb;
            lb.grid = { b["x"].get<int>(), b["y"].get<int>(), b["z"].get<int>() };
            lb.type = block_type_from_string(b.value("type", "stone"));
            blocks.push_back(lb);
        }
    }

    bool is_on_block_type(const glm::vec3& feet_pos, float radius, BlockType type) const {
        constexpr float kTopEps = 0.08f;
        for (const auto& block : blocks) {
            if (block.type != type) continue;
            const float bx1 = block.grid.x - 0.5f;
            const float bx2 = block.grid.x + 0.5f;
            const float bz1 = block.grid.z - 0.5f;
            const float bz2 = block.grid.z + 0.5f;
            const float top = block.grid.y + 0.5f;

            if (feet_pos.x + radius <= bx1 || feet_pos.x - radius >= bx2) continue;
            if (feet_pos.z + radius <= bz1 || feet_pos.z - radius >= bz2) continue;
            if (std::abs(feet_pos.y - top) > kTopEps) continue;
            return true;
        }
        return false;
    }

    // True when the player is standing on a gold block.
    bool at_end(const glm::vec3& feet_pos, float radius) const {
        return is_on_block_type(feet_pos, radius, BlockType::Gold);
    }
};

// make_block_mesh — unit cube centred at origin with correct
// atlas UV coordinates for the given BlockDef.
//
// 6 faces × 4 vertices = 24 verts, 6 faces × 6 indices = 36.
// CCW winding viewed from outside.
inline std::shared_ptr<Mesh> make_block_mesh(const BlockDef& def) {
    std::vector<Vertex>  verts;
    std::vector<GLuint>  idxs;
    verts.reserve(24);
    idxs.reserve(36);

    // Adds one quad face.  p[0..3] are CCW vertices of the face
    // as seen from outside (bottom-left, bottom-right, top-right, top-left).
    auto add_face = [&](const glm::vec3 p[4], const glm::vec3& n, AtlasTile tile) {
        auto base = static_cast<GLuint>(verts.size());
        glm::vec2 mn = atlas_uv_min(tile);
        glm::vec2 mx = atlas_uv_max(tile);
        verts.push_back({ p[0], n, {mn.x, mn.y} });
        verts.push_back({ p[1], n, {mx.x, mn.y} });
        verts.push_back({ p[2], n, {mx.x, mx.y} });
        verts.push_back({ p[3], n, {mn.x, mx.y} });
        idxs.insert(idxs.end(), {base, base+1, base+2, base, base+2, base+3});
    };

    // Top (+Y)
    { glm::vec3 p[4] = {{-0.5f,0.5f, 0.5f},{0.5f,0.5f, 0.5f},{0.5f,0.5f,-0.5f},{-0.5f,0.5f,-0.5f}};
      add_face(p, {0,1,0}, def.top); }
    // Bottom (-Y)
    { glm::vec3 p[4] = {{-0.5f,-0.5f,-0.5f},{0.5f,-0.5f,-0.5f},{0.5f,-0.5f,0.5f},{-0.5f,-0.5f,0.5f}};
      add_face(p, {0,-1,0}, def.bottom); }
    // Front (-Z)
    { glm::vec3 p[4] = {{0.5f,-0.5f,-0.5f},{-0.5f,-0.5f,-0.5f},{-0.5f,0.5f,-0.5f},{0.5f,0.5f,-0.5f}};
      add_face(p, {0,0,-1}, def.sides); }
    // Back (+Z)
    { glm::vec3 p[4] = {{-0.5f,-0.5f,0.5f},{0.5f,-0.5f,0.5f},{0.5f,0.5f,0.5f},{-0.5f,0.5f,0.5f}};
      add_face(p, {0,0,1}, def.sides); }
    // Left (-X)
    { glm::vec3 p[4] = {{-0.5f,-0.5f,0.5f},{-0.5f,-0.5f,-0.5f},{-0.5f,0.5f,-0.5f},{-0.5f,0.5f,0.5f}};
      add_face(p, {-1,0,0}, def.sides); }
    // Right (+X)
    { glm::vec3 p[4] = {{0.5f,-0.5f,-0.5f},{0.5f,-0.5f,0.5f},{0.5f,0.5f,0.5f},{0.5f,0.5f,-0.5f}};
      add_face(p, {1,0,0}, def.sides); }

    return std::make_shared<Mesh>(verts, idxs, GL_TRIANGLES);
}
