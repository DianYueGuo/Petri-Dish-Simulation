#ifndef NEAT_NODE_HPP
#define NEAT_NODE_HPP

namespace neat {

class Node {
public:
    int id;
    int layer;
    float sumInput;
    float sumOutput;

    Node(int id, int layer);
    Node() = default;
};

} // namespace neat

#endif // NEAT_NODE_HPP
