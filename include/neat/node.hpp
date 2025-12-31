#pragma once

namespace neat {

class Node {
public:
    int id = 0;
    int layer = 0;
    float sumInput = 0.0f;
    float sumOutput = 0.0f;
    bool enabled = true;

    Node(int id, int layer);
    Node() = default;
};

} // namespace neat
