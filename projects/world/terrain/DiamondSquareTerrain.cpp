#include "DiamondSquareTerrain.h"

#include <iostream>

#include "TerrainOps.h"

namespace world {

DiamondSquareTerrain::DiamondSquareTerrain(double jitter)
        : _rng(time(NULL)), _jitter(-jitter / 2, jitter / 2) {}


int findMaxLevel(int terrainRes) {
    int level = 1;
    while (terrainRes > powi(2, level) + 1) {
        level++;
    }
    return level;
}


void DiamondSquareTerrain::processTile(ITileContext &context) {
    Terrain &terrain = context.getTile().terrain();
    TileCoordinates tc = context.getCoords();
    vec2i c(tc._pos);
    int lod = tc._lod;

    bool left = _storage.has(tc + vec2i{-1, 0});
    bool right = _storage.has(tc + vec2i{1, 0});
    bool top = _storage.has(tc + vec2i{0, -1});
    bool bottom = _storage.has(tc + vec2i{0, 1});

    if (lod == 0) {
        init(terrain);
        TerrainOps::copyNeighbours(terrain, tc, _storage);

        for (int i = findMaxLevel(terrain.getResolution()); i >= 1; --i) {
            compute(terrain, i, left, right, top, bottom);
        }
    } else {
        const Terrain &parent =
            _storage.get(context.getParentCoords())._terrain;
        vec2i offset =
            vec2i(mod(c.x, 2), mod(c.y, 2)) * (parent.getResolution() / 2);

        copyParent(parent, terrain, offset);
        TerrainOps::copyNeighbours(terrain, tc, _storage);
        compute(terrain, 1, left, right, top, bottom);
    }

    _storage.set(tc, terrain);
}

void DiamondSquareTerrain::processTerrain(Terrain &terrain) {
    int maxLevel = findMaxLevel(terrain.getResolution());
    // TODO warn user when terrain is not 2^n + 1

    init(terrain);

    for (int i = maxLevel; i >= 1; --i) {
        compute(terrain, i);
    }
}

void DiamondSquareTerrain::init(Terrain &terrain) {
    std::uniform_real_distribution<double> dist(0, 1);
    int r = terrain.getResolution() - 1;
    terrain(0, 0) = dist(_rng);
    terrain(r, 0) = dist(_rng);
    terrain(0, r) = dist(_rng);
    terrain(r, r) = dist(_rng);
}

double DiamondSquareTerrain::value(double h1, double h2) {
    return h1 + (h2 - h1) * (0.5 + _jitter(_rng));
}

// level 1 (minimum) -> square size of 2.
// level n -> square size of 2^n

void DiamondSquareTerrain::compute(Terrain &terrain, int level, bool left,
                                   bool right, bool top, bool bottom) {
    int res = terrain.getResolution() - 1;
    int n = powi(2, level);
    int hn = n / 2;

    for (int y = 0; y < res; y += n) {
        for (int x = 0; x < res; x += n) {
            // square
            if (x == 0 && !left) {
                terrain(x, y + hn) = value(terrain(x, y), terrain(x, y + n));
            }
            if (y == 0 && !top) {
                terrain(x + hn, y) = value(terrain(x, y), terrain(x + n, y));
            }

            if (y != res - n || !bottom) {
                terrain(x + hn, y + n) =
                    value(terrain(x, y + n), terrain(x + n, y + n));
            }

            if (x != res - n || !right) {
                terrain(x + n, y + hn) =
                    value(terrain(x + n, y), terrain(x + n, y + n));
            }

            // diamond
            double v1 = value(terrain(x, y + hn), terrain(x + n, y + hn));
            double v2 = value(terrain(x + hn, y), terrain(x + hn, y + n));
            terrain(x + hn, y + hn) = (v1 + v2) / 2;
        }
    }
}

void DiamondSquareTerrain::copyParent(const Terrain &parent, Terrain &terrain,
                                      const vec2i &offset) {
    int res = terrain.getResolution();

    for (int y = 0; y < res; y += 2) {
        for (int x = 0; x < res; x += 2) {
            terrain(x, y) = parent(x / 2 + offset.x, y / 2 + offset.y);
        }
    }
}

} // namespace world
