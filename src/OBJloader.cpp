#include <string>
#include <algorithm>
#include <GL/glew.h> 
#include <glm/glm.hpp>
#include <iostream>

#include "OBJloader.hpp"

#define MAX_LINE_SIZE 1024

bool loadOBJ(const std::filesystem::path& filename, std::vector<vertex>& vertices, std::vector<GLuint>& indices)
{
	std::cout << "Loading model: " << filename.string() << std::endl;

	std::vector< glm::vec3 > temp_vertices;
	std::vector< glm::vec2 > temp_uvs;
	std::vector< glm::vec3 > temp_normals;

	vertices.clear();
	indices.clear();

	FILE* file = fopen(filename.string().c_str(), "r");
	if (file == NULL) {
		printf("Impossible to open the file !\n");
		return false;
	}

	while (1) {

		char lineHeader[MAX_LINE_SIZE];
		int res = fscanf(file, "%s", lineHeader, MAX_LINE_SIZE);
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
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			if (matches != 9) {
				printf("File can't be read by simple parser :( Try exporting with other options\n");
				return false;
			}

			for (int i = 0; i < 3; i++) {
				GLuint currentIndex;
				vertex currentVertex;
				currentVertex.position = temp_vertices[vertexIndex[i]-1]; // OBJ array start from 1
				currentVertex.normal = temp_normals[normalIndex[i]-1];
				currentVertex.texCoords = temp_uvs[uvIndex[i]-1];

                // avoid duplicit vertices
				auto t = std::find_if(vertices.begin(),
					vertices.end(),
					[&currentVertex]
					(const vertex& v2) -> bool {
						return (currentVertex == v2);
					});
                    
				if (t == vertices.end()) {
					vertices.push_back(currentVertex);
					currentIndex = vertices.size() - 1;
				}
				else {
					currentIndex = t - vertices.begin();
				}
				indices.push_back(currentIndex);
			}

		}
	}
	
	std::cout << "Model loaded: " << filename.string() << std::endl;

	fclose(file);
	return true;
}
