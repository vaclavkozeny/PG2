#pragma once

#include <vector>
#include <filesystem>
#include <GL/glew.h>

#include "assets.hpp"

bool loadOBJ(const std::filesystem::path& filename,
	         std::vector <vertex> & vertices,
	         std::vector <GLuint>& indices);

