#ifndef WORLD_INSTANCEDISTRIBUTION_H
#define WORLD_INSTANCEDISTRIBUTION_H

#include "world/core/WorldConfig.h"

#include <random>

#include "world/core/Chunk.h"

namespace world {

/** Habitat features for a given "species" of object. */
struct WORLDAPI_EXPORT HabitatFeatures {
    // in m
    vec2d _altitude{0, 2000};
    // radians
    vec2d _slope{0, M_PI_2};
    // celsius
    vec2d _temperature{0, 40};
    vec2d _humidity{0, 1};
    bool _sea = false;
    /// Density per m^2
    double _density = 0.4;

    // TODO add physical materials
};


// TODO blog post on species generation and seed distribution

class WORLDAPI_EXPORT DistributionBase {
public:
    struct Position {
        vec3d _pos;
        int _genID;
    };


    DistributionBase(IEnvironment *env)
            : _env{env}, _rng{static_cast<u32>(time(NULL))} {}

    void setResolution(double resolution) { _resolution = resolution; }

    void addGenerator(HabitatFeatures habitat) {
        _habitats.emplace_back(std::move(habitat));
    }

    const std::vector<HabitatFeatures> &getHabitatFeatures() const {
        return _habitats;
    }

protected:
    IEnvironment *_env;
    std::mt19937 _rng;

    std::vector<HabitatFeatures> _habitats;
    double _resolution = 20;
};


class WORLDAPI_EXPORT RandomDistribution : public DistributionBase {
public:
    RandomDistribution(IEnvironment *env) : DistributionBase(env) {}

    void setDensity(double density) { _density = density; }

    std::vector<Position> getPositions(Chunk &chunk) {
        std::vector<Position> positions;

        const vec3d chunkPos = chunk.getPosition3D();
        const vec3d chunkDims = chunk.getSize();

        double chunkArea = chunkDims.x * chunkDims.y;
        int instanceCount = static_cast<int>(floor(chunkArea * _density));
        std::uniform_real_distribution<double> posDistrib(0, 1);
        std::uniform_int_distribution<int> genIDDistrib(0,
                                                        _habitats.size() - 1);

        for (int i = 0; i < instanceCount; ++i) {
            double x = posDistrib(_rng) * chunkDims.x;
            double y = posDistrib(_rng) * chunkDims.y;
            vec3d position =
                _env->findNearestFreePoint(chunkPos + vec3d{x, y, -3000},
                                           vec3d{0, 0, 1}, _resolution,
                                           ExplorationContext::getDefault()) -
                chunkPos;

            if (position.z < 0 || position.z >= chunkDims.z) {
                continue;
            }

            positions.push_back({position, genIDDistrib(_rng)});
        }

        return positions;
    }

private:
    // instance count per m^2
    double _density = 0.2;
};

} // namespace world

#endif // WORLD_INSTANCEDISTRIBUTION_H
