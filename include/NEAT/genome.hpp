#pragma once

#include <vector>

#include <neat/node.hpp>
#include <neat/connection.hpp>

namespace neat {

class Genome {
private:
    float weightExtremumInit;
    bool topoDirty = true;
    std::vector<std::vector<int>> forwardAdj;
    std::vector<int> topoOrder;

    int getInnovId(std::vector<std::vector<int>>* innovIds, int* lastInnovId, int inNodeId, int outNodeId);
    void mutateWeights(float mutateWeightFullChangeThresh, float mutateWeightFactor, float mutateWeightThresh);
    bool addConnection(std::vector<std::vector<int>>* innovIds, int* lastInnovId, int maxIterationsFindConnectionThresh, float reactivateConnectionThresh);
    bool disableConnection();
    void disableOrphanHiddenNodes();
    int isValidNewConnection(int inNodeId, int outNodeId);
    bool addNode(std::vector<std::vector<int>>* innovIds, int* lastInnovId, int maxIterationsFindNodeThresh);
    void updateLayersRec(int nodeId);
    void ensureForwardLayers();
    void rebuildTopology();
    static float randomUnitExclusive();

public:
    int nbInput;
    int nbOutput;
    float fitness;
    int speciesId;

    std::vector<Node> nodes;
    std::vector<Connection> connections;

    Genome(int nbInput, int nbOutput, std::vector<std::vector<int>>* innovIds, int* lastInnovId, float weightExtremumInit = 20.0f, bool connectInputsToOutputs = true);
    void loadInputs(float inputs[]);
    void runNetwork(float activationFn(float input));
    void getOutputs(float outputs[]);
    void mutate(std::vector<std::vector<int>>* innovIds, int* lastInnovId, float mutateWeightThresh = 0.8f, float mutateWeightFullChangeThresh = 0.1f, float mutateWeightFactor = 0.1f, float addConnectionThresh = 0.05f, int maxIterationsFindConnectionThresh = 20, float reactivateConnectionThresh = 0.25f, float disableConnectionThresh = 0.0f, float addNodeThresh = 0.03f, int maxIterationsFindNodeThresh = 20);
    void drawNetwork();
};

} // namespace neat
