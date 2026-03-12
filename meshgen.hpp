#pragma once

#include <memory>
#include "mesh.hpp"

// Pouze hlavičky funkcí (bez složených závorek)
std::shared_ptr<Mesh> generateCube();
std::shared_ptr<Mesh> generateSphere(unsigned int sectors, unsigned int rings);