#pragma once

#include "core/WorldConfig.h"

#include <functional>
#include <random>

#include <armadillo/armadillo>

#include "core/WorldTypes.h"

namespace perlin {
enum class WORLDAPI_EXPORT Direction {
    AXIS_X,
    AXIS_Y,
};
}

namespace world {

struct WORLDAPI_EXPORT PerlinInfo {
    int octaves;
    double persistence;
    bool repeatable;

    // Position information
    int reference;
    double frequency;
    int offsetX;
    int offsetY;
};

class WORLDAPI_EXPORT Perlin {
public:
    typedef std::function<double(double, double, double)> modifier;

    static modifier DEFAULT_MODIFIER;

    Perlin();

    Perlin(long seed);

    void setNormalize(bool normalize);

    /** Give the maximum possible value contained in noise
     * generated by this object with the given parameters.
     * This object can be set to normalize produced values, but
     * sometimes it's needed to customize the normalization
     * (exemple : Terrain LOD generation).
     * This method provide some information about the noise
     * produced by this object, to help the user to perform their
     * custom normalization. */
    double getMaxPossibleValue(const PerlinInfo &info);

    void generatePerlinNoise2D(arma::Mat<double> &output,
                               const PerlinInfo &info);

    void generatePerlinNoise2D(arma::Mat<double> &output,
                               const PerlinInfo &info,
                               const modifier &sourceModifier);

    arma::Mat<double> generatePerlinNoise2D(int size, const PerlinInfo &info);

private:
    // Parameters
    bool _normalize = true;

    // Internal fields
    std::mt19937 _rng;
    u8 _hash[512];

    // Buffer for perlin points
    arma::mat _buffer;

    void growBuffer(arma::uword size);

    void fillBuffer(int octave, const PerlinInfo &info,
                    const modifier &sourceModifier);

    void generatePerlinOctave(arma::Mat<double> &output, int octave,
                              const PerlinInfo &info,
                              const modifier &sourceModifier);
};
} // namespace world
