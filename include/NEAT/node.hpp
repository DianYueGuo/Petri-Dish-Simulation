#pragma once

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
