#include <NEAT/node.hpp>

namespace neat {

Node::Node(int id, int layer) : id(id), layer(layer) {
    sumInput = 0.0f;
    sumOutput = 0.0f;
}

} // namespace neat
