#include <neat/connection.hpp>

namespace neat {

Connection::Connection(int innovId, int inNodeId, int outNodeId, float weight, bool enabled)
    : innovId(innovId), inNodeId(inNodeId), outNodeId(outNodeId), weight(weight), enabled(enabled) {
}

} // namespace neat
