#pragma once

namespace neat {

class Connection {
public:
    int innovId;
    int inNodeId;
    int outNodeId;
    float weight;
    bool enabled;

    Connection(int innovId, int inNodeId, int outNodeId, float weight, bool enabled);
    Connection() = default;
};

} // namespace neat
