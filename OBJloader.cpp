#include <string>
#include <algorithm>
#include <cstring>
#include <GL/glew.h> 
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "OBJloader.hpp"

#define MAX_LINE_SIZE 1024

// Helper function to parse face vertex indices
// Handles formats: v, v/vt, v//vn, v/vt/vn
struct FaceVertex {
    int vertex_idx;
    int uv_idx;
    int normal_idx;
};

bool parseFaceVertex(const char* str, FaceVertex& fv) {
    fv.vertex_idx = 0;
    fv.uv_idx = 0;
    fv.normal_idx = 0;
    
    int slash_count = 0;
    const char* start = str;
    const char* p = str;
    
    // Count slashes to determine format
    while (*p && *p != ' ' && *p != '\n') {
        if (*p == '/') slash_count++;
        p++;
    }
    
    // Parse based on format
    if (slash_count == 0) {
        // Format: v
        sscanf(str, "%d", &fv.vertex_idx);
    } else if (slash_count == 1) {
        // Format: v/vt
        sscanf(str, "%d/%d", &fv.vertex_idx, &fv.uv_idx);
    } else if (slash_count == 2) {
        // Check if it's v//vn or v/vt/vn
        if (strchr(str, '/')[1] == '/') {
            // Format: v//vn
            sscanf(str, "%d//%d", &fv.vertex_idx, &fv.normal_idx);
        } else {
            // Format: v/vt/vn
            sscanf(str, "%d/%d/%d", &fv.vertex_idx, &fv.uv_idx, &fv.normal_idx);
        }
    }
    
    return true;
}

bool loadOBJ(const std::filesystem::path& filename, std::vector<Vertex>& vertices, std::vector<GLuint>& indices)
{
	std::cout << "Loading model: " << filename.string() << std::endl;

	std::vector< glm::vec3 > temp_vertices;
	std::vector< glm::vec2 > temp_uvs;
	std::vector< glm::vec3 > temp_normals;
	std::vector< std::tuple<unsigned int, unsigned int, unsigned int, unsigned int> > faces; // stores face info for normal calculation

	vertices.clear();
	indices.clear();

	FILE * file = nullptr;
	file = fopen(filename.string().c_str(), "r");
	if (file == NULL) {
		printf("Impossible to open the file !\n");
		return false;
	}

	bool has_normals = false;
	bool has_uvs = false;

	while (1) {

		char lineHeader[MAX_LINE_SIZE];
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF) {
			break;
		}

		if (strcmp(lineHeader, "v") == 0) {
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			temp_uvs.push_back(uv);
			has_uvs = true;
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
			has_normals = true;
		}
		else if (strcmp(lineHeader, "f") == 0) {
			char line[MAX_LINE_SIZE];
			fgets(line, MAX_LINE_SIZE, file);
			
			// Parse face vertices
			std::vector<FaceVertex> face_verts;
			char* token = strtok(line, " \n");
			while (token) {
				FaceVertex fv;
				parseFaceVertex(token, fv);
				face_verts.push_back(fv);
				token = strtok(nullptr, " \n");
			}
			
			// Handle triangles
			if (face_verts.size() == 3) {
				for (const auto& fv : face_verts) {
					GLuint currentIndex;
					Vertex currentVertex;
					
					// Position (mandatory)
					currentVertex.position = temp_vertices[fv.vertex_idx - 1]; // OBJ uses 1-based indexing
					
					// Normal (use provided or default)
					if (has_normals && fv.normal_idx > 0) {
						currentVertex.normal = temp_normals[fv.normal_idx - 1];
					} else {
						currentVertex.normal = glm::vec3(0.0f, 0.0f, 1.0f); // default normal
					}
					
					// TexCoords (use provided or default)
					if (has_uvs && fv.uv_idx > 0) {
						currentVertex.texCoords = temp_uvs[fv.uv_idx - 1];
					} else {
						currentVertex.texCoords = glm::vec2(0.0f, 0.0f); // default UV
					}
					
					// Avoid duplicate vertices
					auto t = std::find_if(vertices.begin(),
						vertices.end(),
						[&currentVertex](const Vertex& v2) -> bool {
							return (currentVertex == v2);
						});
					
					if (t == vertices.end()) {
						vertices.push_back(currentVertex);
						currentIndex = vertices.size() - 1;
					} else {
						currentIndex = t - vertices.begin();
					}
					indices.push_back(currentIndex);
				}
				
				// Store face info for normal calculation if needed
				if (!has_normals && face_verts.size() >= 3) {
					faces.push_back(std::make_tuple(
						face_verts[0].vertex_idx - 1,
						face_verts[1].vertex_idx - 1,
						face_verts[2].vertex_idx - 1,
						0 // placeholder
					));
				}
			}
			// Handle quads by breaking into 2 triangles
			else if (face_verts.size() == 4) {
				// Triangle 1: vertices 0, 1, 2
				for (int i = 0; i < 3; i++) {
					const auto& fv = face_verts[i];
					GLuint currentIndex;
					Vertex currentVertex;
					
					currentVertex.position = temp_vertices[fv.vertex_idx - 1];
					currentVertex.normal = (has_normals && fv.normal_idx > 0) ? 
						temp_normals[fv.normal_idx - 1] : glm::vec3(0.0f, 0.0f, 1.0f);
					currentVertex.texCoords = (has_uvs && fv.uv_idx > 0) ? 
						temp_uvs[fv.uv_idx - 1] : glm::vec2(0.0f, 0.0f);
					
					auto t = std::find_if(vertices.begin(),
						vertices.end(),
						[&currentVertex](const Vertex& v2) -> bool {
							return (currentVertex == v2);
						});
					
					if (t == vertices.end()) {
						vertices.push_back(currentVertex);
						currentIndex = vertices.size() - 1;
					} else {
						currentIndex = t - vertices.begin();
					}
					indices.push_back(currentIndex);
				}
				
				// Triangle 2: vertices 0, 2, 3
				for (int i : {0, 2, 3}) {
					const auto& fv = face_verts[i];
					GLuint currentIndex;
					Vertex currentVertex;
					
					currentVertex.position = temp_vertices[fv.vertex_idx - 1];
					currentVertex.normal = (has_normals && fv.normal_idx > 0) ? 
						temp_normals[fv.normal_idx - 1] : glm::vec3(0.0f, 0.0f, 1.0f);
					currentVertex.texCoords = (has_uvs && fv.uv_idx > 0) ? 
						temp_uvs[fv.uv_idx - 1] : glm::vec2(0.0f, 0.0f);
					
					auto t = std::find_if(vertices.begin(),
						vertices.end(),
						[&currentVertex](const Vertex& v2) -> bool {
							return (currentVertex == v2);
						});
					
					if (t == vertices.end()) {
						vertices.push_back(currentVertex);
						currentIndex = vertices.size() - 1;
					} else {
						currentIndex = t - vertices.begin();
					}
					indices.push_back(currentIndex);
				}
				
				if (!has_normals && face_verts.size() >= 3) {
					faces.push_back(std::make_tuple(
						face_verts[0].vertex_idx - 1,
						face_verts[1].vertex_idx - 1,
						face_verts[2].vertex_idx - 1,
						1 // first triangle of quad
					));
					faces.push_back(std::make_tuple(
						face_verts[0].vertex_idx - 1,
						face_verts[2].vertex_idx - 1,
						face_verts[3].vertex_idx - 1,
						1 // second triangle of quad
					));
				}
			}
		}
	}
	
	// Calculate normals if not present in file
	if (!has_normals) {
		std::cout << "  No normals found, calculating from geometry...\n";
		
		// Initialize normals to zero
		for (auto& v : vertices) {
			v.normal = glm::vec3(0.0f, 0.0f, 0.0f);
		}
		
		// Accumulate normals from triangles
		for (size_t i = 0; i < indices.size(); i += 3) {
			glm::vec3& p0 = vertices[indices[i]].position;
			glm::vec3& p1 = vertices[indices[i + 1]].position;
			glm::vec3& p2 = vertices[indices[i + 2]].position;
			
			glm::vec3 normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));
			
			vertices[indices[i]].normal += normal;
			vertices[indices[i + 1]].normal += normal;
			vertices[indices[i + 2]].normal += normal;
		}
		
		// Normalize all normals
		for (auto& v : vertices) {
			v.normal = glm::normalize(v.normal);
		}
	}
	
	std::cout << "Model loaded: " << filename.string() << std::endl;
	std::cout << "  Vertices: " << vertices.size() << ", Indices: " << indices.size() << std::endl;
	std::cout << "  Has normals: " << (has_normals ? "yes" : "no (calculated)") << std::endl;
	std::cout << "  Has UVs: " << (has_uvs ? "yes" : "no (using defaults)") << std::endl;

	fclose(file);
	return true;
}
