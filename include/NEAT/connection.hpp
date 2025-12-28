#ifndef NEAT_CONNECTION_HPP
#define NEAT_CONNECTION_HPP

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

#endif // NEAT_CONNECTION_HPP
