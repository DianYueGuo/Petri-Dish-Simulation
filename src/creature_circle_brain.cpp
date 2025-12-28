#include "creature_circle.hpp"
#include "drawable_circle.hpp"
#include "game.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <vector>

namespace {
constexpr float PI = 3.14159f;
constexpr float TWO_PI = PI * 2.0f;
constexpr int SENSOR_COUNT = kColorSensorCount;
static_assert(SENSOR_COUNT >= kMinColorSensorCount && SENSOR_COUNT <= kMaxColorSensorCount, "Color sensor count out of supported range.");
constexpr float SECTOR_WIDTH = TWO_PI / static_cast<float>(SENSOR_COUNT);
constexpr float SECTOR_HALF = SECTOR_WIDTH * 0.5f;

using SectorSegment = std::pair<float, float>;
struct SpanSegments {
    std::array<SectorSegment, 2> segments{};
    int count = 0;
};
using SectorSegments = std::array<SpanSegments, SENSOR_COUNT>;
using SensorColors = std::array<std::array<float, 3>, SENSOR_COUNT>;
using SensorWeights = std::array<float, SENSOR_COUNT>;

float neat_activation(float x) {
    return 1.0f / (1.0f + std::exp(-x));
}

float normalize_angle(float angle) {
    angle = std::fmod(angle, TWO_PI);
    if (angle > PI) {
        angle -= TWO_PI;
    } else if (angle < -PI) {
        angle += TWO_PI;
    }
    return angle;
}

SpanSegments split_interval(float start, float end) {
    SpanSegments segments{};
    start = normalize_angle(start);
    end = normalize_angle(end);
    if (end < start) {
        segments.segments[0] = {start, PI};
        segments.segments[1] = {-PI, end};
        segments.count = 2;
    } else {
        segments.segments[0] = {start, end};
        segments.count = 1;
    }
    return segments;
}

const SectorSegments& get_sector_segments() {
    static const SectorSegments sector_segments = []() {
        SectorSegments result{};
        for (int i = 0; i < SENSOR_COUNT; ++i) {
            float s_start = -SECTOR_HALF + i * SECTOR_WIDTH;
            float s_end = s_start + SECTOR_WIDTH;
            result[i] = split_interval(s_start, s_end);
        }
        return result;
    }();
    return sector_segments;
}

float cross(const b2Vec2& a, const b2Vec2& b) {
    return a.x * b.y - a.y * b.x;
}

float dot(const b2Vec2& a, const b2Vec2& b) {
    return a.x * b.x + a.y * b.y;
}

float triangle_circle_intersection_area(const b2Vec2& a, const b2Vec2& b, float radius) {
    // Circle is centered at the origin in this helper.
    const float r2 = radius * radius;
    const float len_a2 = dot(a, a);
    const float EPS = 1e-6f;

    // If both vertices are effectively at the origin, there is no area.
    if (len_a2 < EPS && dot(b, b) < EPS) {
        return 0.0f;
    }

    struct ParamPoint {
        float t;
        b2Vec2 p;
    };
    std::array<ParamPoint, 4> pts{};
    int count = 0;
    pts[count++] = {0.0f, a};

    // Solve for intersections of segment ab with the circle.
    b2Vec2 d{b.x - a.x, b.y - a.y};
    float A = dot(d, d);
    float B = 2.0f * dot(a, d);
    float C = len_a2 - r2;
    float disc = B * B - 4.0f * A * C;
    if (disc >= 0.0f && A > EPS) {
        float sqrt_disc = std::sqrt(disc);
        float inv_denom = 0.5f / A;
        float t1 = (-B - sqrt_disc) * inv_denom;
        float t2 = (-B + sqrt_disc) * inv_denom;
        if (t1 > t2) std::swap(t1, t2);
        if (t1 > EPS && t1 < 1.0f - EPS) {
            pts[count++] = {t1, b2Vec2{a.x + d.x * t1, a.y + d.y * t1}};
        }
        if (t2 > EPS && t2 < 1.0f - EPS && std::fabs(t2 - t1) > EPS) {
            if (count < static_cast<int>(pts.size())) {
                pts[count++] = {t2, b2Vec2{a.x + d.x * t2, a.y + d.y * t2}};
            }
        }
    }

    pts[count++] = {1.0f, b};
    std::sort(pts.begin(), pts.begin() + count, [&](const ParamPoint& p1, const ParamPoint& p2) {
        return p1.t < p2.t;
    });

    float area = 0.0f;
    for (int i = 0; i + 1 < count; ++i) {
        const b2Vec2& p = pts[i].p;
        const b2Vec2& q = pts[i + 1].p;
        b2Vec2 mid{0.5f * (p.x + q.x), 0.5f * (p.y + q.y)};
        const float mid_len2 = dot(mid, mid);
        if (mid_len2 <= r2 + EPS) {
            area += 0.5f * cross(p, q);
        } else {
            float ang = std::atan2(cross(p, q), dot(p, q));
            area += 0.5f * r2 * ang;
        }
    }

    return area;
}

float circle_triangle_intersection_area(const std::array<b2Vec2, 3>& poly, const b2Vec2& center, float radius) {
    // Translate polygon so circle center is at the origin.
    float area = 0.0f;
    for (int i = 0; i < 3; ++i) {
        b2Vec2 a{poly[i].x - center.x, poly[i].y - center.y};
        const auto& next = poly[(i + 1) % 3];
        b2Vec2 b{next.x - center.x, next.y - center.y};
        area += triangle_circle_intersection_area(a, b, radius);
    }
    return area;
}

float circle_wedge_overlap_area(const b2Vec2& circle_center_local, float radius, float start_angle, float end_angle) {
    // Build a large triangle that represents the wedge; large enough to fully contain the circle footprint.
    float dist_to_origin = std::sqrt(circle_center_local.x * circle_center_local.x + circle_center_local.y * circle_center_local.y);
    float ray_length = dist_to_origin + radius + 1.0f; // add slack to guarantee containment

    b2Vec2 p1{std::cos(start_angle) * ray_length, std::sin(start_angle) * ray_length};
    b2Vec2 p2{std::cos(end_angle) * ray_length, std::sin(end_angle) * ray_length};
    std::array<b2Vec2, 3> triangle{{b2Vec2{0.0f, 0.0f}, p1, p2}};

    float area = circle_triangle_intersection_area(triangle, circle_center_local, radius);
    return std::max(0.0f, area);
}

float normalize_angle_positive(float angle) {
    float a = std::fmod(angle, TWO_PI);
    if (a < 0.0f) {
        a += TWO_PI;
    }
    return a;
}

void accumulate_touching_circle(const CirclePhysics& circle,
                                const DrawableCircle& drawable,
                                const b2Vec2& self_pos,
                                float cos_h,
                                float sin_h,
                                const SectorSegments& sector_segments,
                                SensorColors& summed_colors,
                                SensorWeights& weights) {
    const b2Vec2 other_pos = circle.getPosition();
    b2Vec2 rel_world{other_pos.x - self_pos.x, other_pos.y - self_pos.y};
    b2Vec2 rel_local{
        cos_h * rel_world.x + sin_h * rel_world.y,
        -sin_h * rel_world.x + cos_h * rel_world.y
    };

    const float other_r = circle.getRadius();
    const float dist2 = rel_local.x * rel_local.x + rel_local.y * rel_local.y;
    const float other_r2 = other_r * other_r;
    const auto& color = drawable.get_color_rgb();

    const auto clamp_sector_index = [](int idx) {
        return std::clamp(idx, 0, SENSOR_COUNT - 1);
    };

    auto accumulate_sector = [&](int sector) {
        const int clamped_sector = clamp_sector_index(sector);
        float area_in_sector = 0.0f;
        const auto& segs = sector_segments[clamped_sector];
        for (int idx = 0; idx < segs.count; ++idx) {
            const auto& seg = segs.segments[idx];
            area_in_sector += circle_wedge_overlap_area(rel_local, other_r, seg.first, seg.second);
        }

        if (area_in_sector <= 0.0f) {
            return;
        }

        summed_colors[clamped_sector][0] += color[0] * area_in_sector;
        summed_colors[clamped_sector][1] += color[1] * area_in_sector;
        summed_colors[clamped_sector][2] += color[2] * area_in_sector;
        weights[clamped_sector] += area_in_sector;
    };

    if (dist2 <= other_r2) {
        // Circle encompasses the origin; intersects all sectors.
        for (int sector = 0; sector < SENSOR_COUNT; ++sector) {
            accumulate_sector(sector);
        }
        return;
    }

    const float dist = std::sqrt(dist2);
    const float half_span = std::asin(std::clamp(other_r / dist, 0.0f, 1.0f));
    const float center_angle = std::atan2(rel_local.y, rel_local.x);
    constexpr float pad = 1e-4f; // avoid missing boundary-touching sectors
    float start = normalize_angle_positive(center_angle - half_span - pad);
    float end = normalize_angle_positive(center_angle + half_span + pad);

    auto angle_to_index = [&](float angle) {
        int idx = static_cast<int>(std::floor(angle / SECTOR_WIDTH));
        return clamp_sector_index(idx);
    };

    int start_idx = angle_to_index(start);
    int end_idx = angle_to_index(end);

    auto process_range = [&](int from, int to) {
        for (int s = from; ; ++s) {
            accumulate_sector(s);
            if (s == to) break;
        }
    };

    if (start <= end) {
        process_range(start_idx, end_idx);
    } else {
        process_range(start_idx, SENSOR_COUNT - 1);
        process_range(0, end_idx);
    }
}

void accumulate_outside_petri(const b2Vec2& self_pos,
                              float self_radius,
                              float cos_h,
                              float sin_h,
                              float petri_radius,
                              const SectorSegments& sector_segments,
                              SensorColors& summed_colors,
                              SensorWeights& weights) {
    if (petri_radius <= 0.0f || self_radius <= 0.0f) {
        return;
    }

    // Petri dish is centered at the world origin.
    b2Vec2 rel_world{-self_pos.x, -self_pos.y};
    b2Vec2 dish_local{
        cos_h * rel_world.x + sin_h * rel_world.y,
        -sin_h * rel_world.x + cos_h * rel_world.y
    };

    constexpr float epsilon = 1e-6f;
    for (int sector = 0; sector < SENSOR_COUNT; ++sector) {
        float outside_area = 0.0f;
        const auto& segs = sector_segments[sector];
        for (int idx = 0; idx < segs.count; ++idx) {
            const auto& seg = segs.segments[idx];
            float span = seg.second - seg.first;
            if (span <= 0.0f) {
                continue;
            }

            // Scale the ray length so the triangle area matches the circular sector area.
            float sin_span = std::sin(span);
            float ray_length = self_radius;
            if (std::fabs(sin_span) > epsilon) {
                ray_length = self_radius * std::sqrt(span / sin_span);
            }

            b2Vec2 p1{std::cos(seg.first) * ray_length, std::sin(seg.first) * ray_length};
            b2Vec2 p2{std::cos(seg.second) * ray_length, std::sin(seg.second) * ray_length};
            std::array<b2Vec2, 3> triangle{{b2Vec2{0.0f, 0.0f}, p1, p2}};

            float inside_area = circle_triangle_intersection_area(triangle, dish_local, petri_radius);
            float segment_area = 0.5f * self_radius * self_radius * span;
            inside_area = std::clamp(inside_area, 0.0f, segment_area);

            outside_area += segment_area - inside_area;
        }

        if (outside_area > 0.0f) {
            summed_colors[sector][0] += outside_area; // Sense outside as red.
            weights[sector] += outside_area;
        }
    }
}
} // namespace

void CreatureCircle::run_brain_cycle_from_touching() {
    update_brain_inputs_from_touching();
    brain.loadInputs(brain_inputs.data());
    brain.runNetwork(neat_activation);
    brain.getOutputs(brain_outputs.data());
    update_color_from_brain();
}

void CreatureCircle::update_brain_inputs_from_touching() {
    SensorColors summed_colors{};
    SensorWeights weights{};

    const b2Vec2 self_pos = this->getPosition();
    const float heading = this->getAngle();
    const float cos_h = std::cos(heading);
    const float sin_h = std::sin(heading);
    const auto& sector_segments = get_sector_segments();

    if (owner_game) {
        auto& graph = owner_game->get_contact_graph();
        auto& registry = owner_game->get_circle_registry();
        graph.for_each_neighbor(get_id(), [&](CircleId neighbor) {
            const auto* senseable = registry.get_senseable(neighbor);
            const auto* physics = registry.get_physics(neighbor);
            if (!senseable || !physics) {
                return;
            }
            auto* drawable = dynamic_cast<const DrawableCircle*>(physics);
            if (!drawable) {
                return;
            }
            accumulate_touching_circle(*physics,
                                       *drawable,
                                       self_pos,
                                       cos_h,
                                       sin_h,
                                       sector_segments,
                                       summed_colors,
                                       weights);
        });
    }

    if (owner_game) {
        float petri_radius = owner_game->get_petri_radius();
        accumulate_outside_petri(self_pos, getRadius(), cos_h, sin_h, petri_radius, sector_segments, summed_colors, weights);
    }

    apply_sensor_inputs(summed_colors, weights);
    write_size_and_memory_inputs();
}

void CreatureCircle::apply_sensor_inputs(const std::array<std::array<float, 3>, SENSOR_COUNT>& summed_colors, const std::array<float, SENSOR_COUNT>& weights) {
    // Order sensors clockwise starting from the forward-facing sector.
    const float sector_area = (PI * getRadius() * getRadius()) / static_cast<float>(SENSOR_COUNT);
    for (int i = 0; i < SENSOR_COUNT; ++i) {
        int base_index = i * 3;
        float weight = weights[i];
        if (weight > 0.0f) {
            float r_sum = summed_colors[i][0];
            float g_sum = summed_colors[i][1];
            float b_sum = summed_colors[i][2];
            float denom_r = r_sum + sector_area;
            float denom_g = g_sum + sector_area;
            float denom_b = b_sum + sector_area;
            brain_inputs[base_index]     = denom_r > 0.0f ? (r_sum / denom_r) : 0.0f;
            brain_inputs[base_index + 1] = denom_g > 0.0f ? (g_sum / denom_g) : 0.0f;
            brain_inputs[base_index + 2] = denom_b > 0.0f ? (b_sum / denom_b) : 0.0f;
        } else {
            brain_inputs[base_index]     = 0.0f;
            brain_inputs[base_index + 1] = 0.0f;
            brain_inputs[base_index + 2] = 0.0f;
        }
    }
}

void CreatureCircle::write_size_and_memory_inputs() {
    float area = this->getArea();
    float normalized = area / (area + 10.0f); // gentler saturation for larger sizes
    brain_inputs[SIZE_INPUT_INDEX] = normalized;

    for (int i = 0; i < MEMORY_SLOTS; ++i) {
        brain_inputs[MEMORY_INPUT_START + i] = memory_state[i];
    }
}

void CreatureCircle::update_color_from_brain() {
    float target_r = std::clamp(brain_outputs[3], 0.0f, 1.0f);
    float target_g = std::clamp(brain_outputs[4], 0.0f, 1.0f);
    float target_b = std::clamp(brain_outputs[5], 0.0f, 1.0f);
    set_color_rgb(target_r, target_g, target_b); // keep the signal exact
    constexpr float smoothing = 0.1f; // exponential smoothing factor for display only
    smooth_display_color(smoothing);
}
