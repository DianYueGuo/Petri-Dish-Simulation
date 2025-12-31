#include <neat/genome.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace neat {

Genome::Genome(int nbInput, int nbOutput, std::vector<std::vector<int>>* innovIds, int* lastInnovId, float weightExtremumInit, bool connectInputsToOutputs)
    : weightExtremumInit(weightExtremumInit), nbInput(nbInput), nbOutput(nbOutput) {
    speciesId = -1;

    // Nodes: bias
    nodes.push_back(Node(0, 0));
    nodes[0].sumInput = 1.0f;
    nodes[0].sumOutput = 1.0f;

    // Inputs
    for (int i = 1; i < nbInput + 1; i++) {
        nodes.push_back(Node(i, 0));
    }
    // Outputs (fully connect input->output initially)
    for (int i = nbInput + 1; i < nbInput + 1 + nbOutput; i++) {
        nodes.push_back(Node(i, 1));
    }

    if (connectInputsToOutputs) {
        for (int inNodeId = 0; inNodeId < nbInput + 1; inNodeId++) {
            for (int outNodeId = nbInput + 1; outNodeId < nbInput + 1 + nbOutput; outNodeId++) {
                int innovId = getInnovId(innovIds, lastInnovId, inNodeId, outNodeId);
                float weight = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f * weightExtremumInit - weightExtremumInit;
                connections.push_back(Connection(innovId, inNodeId, outNodeId, weight, true));
            }
        }
    }
    topoDirty = true;
}

int Genome::getInnovId(std::vector<std::vector<int>>* innovIds, int* lastInnovId, int inNodeId, int outNodeId) {
    if (static_cast<int>(innovIds->size()) < inNodeId + 1) {
        int previousSize = static_cast<int>(innovIds->size());
        for (int k = 0; k < inNodeId + 1 - previousSize; k++) {
            innovIds->push_back({-1});
        }
    }
    if (static_cast<int>((*innovIds)[inNodeId].size()) < outNodeId + 1) {
        int previousSize = static_cast<int>((*innovIds)[inNodeId].size());
        for (int k = 0; k < outNodeId + 1 - previousSize; k++) {
            (*innovIds)[inNodeId].push_back(-1);
        }
    }
    if ((*innovIds)[inNodeId][outNodeId] == -1) {
        *lastInnovId += 1;
        (*innovIds)[inNodeId][outNodeId] = *lastInnovId;
    }

    return (*innovIds)[inNodeId][outNodeId];
}

float Genome::randomUnitExclusive() {
    float randomNb = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    while (randomNb >= 1.0f - 1e-10f) {
        randomNb = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    }
    return randomNb;
}

void Genome::loadInputs(float inputs[]) {
    for (int i = 0; i < nbInput; i++) {
        nodes[i + 1].sumInput = inputs[i];
        nodes[i + 1].sumOutput = inputs[i];
    }
}

void Genome::runNetwork(float activationFn(float input)) {
    if (topoDirty) {
        rebuildTopology();
    }
    for (int i = nbInput + 1; i < static_cast<int>(nodes.size()); i++) {
        nodes[i].sumInput = 0.0f;
        nodes[i].sumOutput = 0.0f;
    }

    for (int nodeId : topoOrder) {
        if (nodeId > nbInput) {
            nodes[nodeId].sumOutput = activationFn(nodes[nodeId].sumInput);
        }
        for (int edgeIdx : forwardAdj[nodeId]) {
            const Connection& conn = connections[edgeIdx];
            nodes[conn.outNodeId].sumInput += nodes[conn.inNodeId].sumOutput * conn.weight;
        }
    }
}

void Genome::getOutputs(float outputs[]) {
    for (int i = 0; i < nbOutput; i++) {
        outputs[i] = nodes[1 + nbInput + i].sumOutput;
    }
}

void Genome::mutate(std::vector<std::vector<int>>* innovIds, int* lastInnovId, float mutateWeightThresh, float mutateWeightFullChangeThresh, float mutateWeightFactor, float addConnectionThresh, int maxIterationsFindConnectionThresh, float reactivateConnectionThresh, float disableConnectionThresh, float addNodeThresh, int maxIterationsFindNodeThresh) {
    float randomNb = randomUnitExclusive();

    mutateWeights(mutateWeightFullChangeThresh, mutateWeightFactor, mutateWeightThresh);

    randomNb = randomUnitExclusive();
    if (randomNb < addConnectionThresh) {
        addConnection(innovIds, lastInnovId, maxIterationsFindConnectionThresh, reactivateConnectionThresh);
    }

    randomNb = randomUnitExclusive();
    if (randomNb < disableConnectionThresh) {
        disableConnection();
    }

    randomNb = randomUnitExclusive();
    if (randomNb < addNodeThresh) {
        addNode(innovIds, lastInnovId, maxIterationsFindNodeThresh);
    }
}

void Genome::mutateWeights(float mutateWeightFullChangeThresh, float mutateWeightFactor, float mutateWeightThresh) {
    for (auto& conn : connections) {
        float randomNb = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        if (randomNb > mutateWeightThresh) {
            continue;
        }

        randomNb = randomUnitExclusive();
        if (randomNb < mutateWeightFullChangeThresh) {
            conn.weight = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f * weightExtremumInit - weightExtremumInit;
        } else {
            float u1 = (static_cast<float>(rand()) + 1.0f) / (static_cast<float>(RAND_MAX) + 2.0f);
            float u2 = (static_cast<float>(rand()) + 1.0f) / (static_cast<float>(RAND_MAX) + 2.0f);
            float mag = std::sqrt(-2.0f * std::log(u1)) * std::cos(2.0f * 3.1415926f * u2);
            conn.weight += mag * mutateWeightFactor;
        }
    }
}

bool Genome::addConnection(std::vector<std::vector<int>>* innovIds, int* lastInnovId, int maxIterationsFindConnectionThresh, float reactivateConnectionThresh) {
    int iterationNb = 0;
    int isValid = 0;
    int inNodeId = rand() % static_cast<int>(nodes.size());
    int outNodeId = rand() % static_cast<int>(nodes.size());
    while (iterationNb < maxIterationsFindConnectionThresh && isValid == 0) {
        inNodeId = rand() % static_cast<int>(nodes.size());
        outNodeId = rand() % static_cast<int>(nodes.size());
        isValid = isValidNewConnection(inNodeId, outNodeId);
        iterationNb++;
    }

    if (iterationNb >= maxIterationsFindConnectionThresh) {
        return false;
    }

    if (isValid == 2) {
        float randomNb = randomUnitExclusive();
        if (randomNb < reactivateConnectionThresh) {
            // Prefer to reactivate a disabled connection; if none, treat as success.
            for (auto& conn : connections) {
                if (conn.inNodeId == inNodeId && conn.outNodeId == outNodeId) {
                    if (!conn.enabled) {
                        conn.enabled = true;
                        topoDirty = true;
                    }
                    return true;
                }
            }
            return true;
        }
        return true;
    }

    int innovId = getInnovId(innovIds, lastInnovId, inNodeId, outNodeId);
    float weight = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f * weightExtremumInit - weightExtremumInit;
    connections.push_back(Connection(innovId, inNodeId, outNodeId, weight, true));
    topoDirty = true;
    return true;
}

bool Genome::disableConnection() {
    std::vector<int> enabledConnections;
    enabledConnections.reserve(connections.size());
    for (int idx = 0; idx < static_cast<int>(connections.size()); ++idx) {
        if (connections[idx].enabled) {
            enabledConnections.push_back(idx);
        }
    }
    if (enabledConnections.empty()) {
        return false;
    }
    int choice = rand() % static_cast<int>(enabledConnections.size());
    int connIdx = enabledConnections[choice];
    connections[connIdx].enabled = false;
    topoDirty = true;
    return true;
}

int Genome::isValidNewConnection(int inNodeId, int outNodeId) {
    if (inNodeId == outNodeId) return 0;

    int inLayer = nodes[inNodeId].layer;
    int outLayer = nodes[outNodeId].layer;
    if (inLayer >= outLayer) return 0;

    for (const auto& connection : connections) {
        if (connection.inNodeId == inNodeId && connection.outNodeId == outNodeId) {
            return 2;
        }
    }

    return 1;
}

bool Genome::addNode(std::vector<std::vector<int>>* innovIds, int* lastInnovId, int maxIterationsFindNodeThresh) {
    if (connections.empty()) {
        return false;
    }
    int iterationNb = 0;
    int connId = rand() % static_cast<int>(connections.size());
    while (iterationNb < maxIterationsFindNodeThresh && !connections[connId].enabled) {
        connId = rand() % static_cast<int>(connections.size());
        iterationNb++;
    }

    if (iterationNb >= maxIterationsFindNodeThresh) {
        return false;
    }

    connections[connId].enabled = false;
    nodes.push_back(Node(static_cast<int>(nodes.size()), nodes[connections[connId].inNodeId].layer + 1));
    int newInNodeId = nodes.back().id;

    int innovId = getInnovId(innovIds, lastInnovId, connections[connId].inNodeId, newInNodeId);
    connections.push_back(Connection(innovId, connections[connId].inNodeId, newInNodeId, 1.0f, true));

    innovId = getInnovId(innovIds, lastInnovId, newInNodeId, connections[connId].outNodeId);
    connections.push_back(Connection(innovId, newInNodeId, connections[connId].outNodeId, connections[connId].weight, true));

    topoDirty = true;
    return true;
}

void Genome::updateLayersRec(int nodeId) {
    for (auto& connection : connections) {
        if (connection.inNodeId == nodeId) {
            if (nodes[connection.outNodeId].layer <= nodes[nodeId].layer) {
                nodes[connection.outNodeId].layer = nodes[nodeId].layer + 1;
                updateLayersRec(connection.outNodeId);
            }
        }
    }
}

void Genome::ensureForwardLayers() {
    for (auto& connection : connections) {
        int inLayer = nodes[connection.inNodeId].layer;
        int outLayer = nodes[connection.outNodeId].layer;
        if (inLayer >= outLayer) {
            nodes[connection.outNodeId].layer = inLayer + 1;
            updateLayersRec(connection.outNodeId);
        }
    }
}

void Genome::rebuildTopology() {
    ensureForwardLayers();

    int maxLayer = 0;
    for (const auto& node : nodes) {
        maxLayer = std::max(maxLayer, node.layer);
    }

    std::vector<std::vector<int>> nodesPerLayer(maxLayer + 1);
    for (const auto& node : nodes) {
        nodesPerLayer[node.layer].push_back(node.id);
    }

    topoOrder.clear();
    forwardAdj.assign(nodes.size(), {});
    for (int layer = 0; layer <= maxLayer; ++layer) {
        for (int nodeId : nodesPerLayer[layer]) {
            topoOrder.push_back(nodeId);
        }
    }

    for (int idx = 0; idx < static_cast<int>(connections.size()); ++idx) {
        if (!connections[idx].enabled) continue;
        forwardAdj[connections[idx].inNodeId].push_back(idx);
    }

    topoDirty = false;
}

void Genome::drawNetwork() {
    // Disabled to avoid SFML dependency in this project build.
}

} // namespace neat
