#include "creature_circle.hpp"
#include "game.hpp"

#include <algorithm>

CreatureCircle::CreatureCircle(const b2WorldId &worldId,
                         float position_x,
                         float position_y,
                         float radius,
                         float density,
                         float angle,
                         int generation,
                         int init_mutation_rounds,
                         float init_add_node_thresh,
                         float init_add_connection_thresh,
                         const neat::Genome* base_brain,
                         std::vector<std::vector<int>>* innov_ids,
                         int* last_innov_id,
                         Game* owner) :
    EatableCircle(worldId, position_x, position_y, radius, density, /*toxic=*/false, /*division_pellet=*/false, angle, /*boost_particle=*/false),
    brain(base_brain ? *base_brain : neat::Genome(BRAIN_INPUTS, BRAIN_OUTPUTS, innov_ids, last_innov_id, owner ? owner->get_weight_extremum_init() : 0.001f)) {
    set_kind(CircleKind::Creature);
    neat_innovations = innov_ids;
    neat_last_innov_id = last_innov_id;
    owner_game = owner;
    set_generation(generation);
    initialize_brain(
        init_mutation_rounds,
        init_add_node_thresh,
        init_add_connection_thresh);
    run_brain_cycle_from_touching();
    smooth_display_color(1.0f); // start display at brain-driven color immediately
}

void CreatureCircle::initialize_brain(int mutation_rounds, float add_node_thresh, float add_connection_thresh) {
    // Mutate repeatedly to seed a non-trivial brain topology.
    int rounds = std::max(0, mutation_rounds);
    for (int i = 0; i < rounds; ++i) {
        if (neat_innovations && neat_last_innov_id) {
            float weight_thresh = 0.8f;
            float weight_full = 0.1f;
            float weight_factor = 1.2f;
            float reactivate = 0.25f;
            int add_conn_iters = 20;
            int add_node_iters = 20;
            if (owner_game) {
                weight_thresh = owner_game->get_mutate_weight_thresh();
                weight_full = owner_game->get_mutate_weight_full_change_thresh();
                weight_factor = owner_game->get_mutate_weight_factor();
                reactivate = owner_game->get_reactivate_connection_thresh();
                add_conn_iters = owner_game->get_max_iterations_find_connection_thresh();
                add_node_iters = owner_game->get_max_iterations_find_node_thresh();
            }
            brain.mutate(
                neat_innovations,
                neat_last_innov_id,
                weight_thresh,
                weight_full,
                weight_factor,
                add_connection_thresh,
                add_conn_iters,
                reactivate,
                add_node_thresh,
                add_node_iters);
        }
    }
}
