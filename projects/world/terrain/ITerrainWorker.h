#ifndef WORLD_ITERRAINGENERATOR_H
#define WORLD_ITERRAINGENERATOR_H

#include "world/core/WorldConfig.h"

#include "world/core/TileSystem.h"

#include "Terrain.h"
#include "world/core/GridStorage.h"

namespace world {

class WORLDAPI_EXPORT TerrainTile {
public:
    TileCoordinates _key;
    Terrain _terrain;
    Mesh _mesh;


    TerrainTile(TileCoordinates key, int size) : _key(key), _terrain(size) {}

    Terrain &terrain() { return _terrain; }

    Image &texture() { return _terrain.getTexture(); }

    Mesh &mesh() { return _mesh; }
};

class WORLDAPI_EXPORT ITileContext {
public:
    virtual ~ITileContext() = default;

    virtual TerrainTile &getTile() const = 0;

    virtual TileCoordinates getCoords() const = 0;

    virtual TileCoordinates getParentCoords() const = 0;

    /*
        virtual Terrain &getTerrain() const = 0;

        virtual Image &getTexture() const = 0;

        virtual optional<const Terrain &> getNeighbour(int x, int y) const = 0;

        virtual optional<const Terrain &> getParent() const = 0;

        virtual int getParentCount() const = 0;
    */
};

class WORLDAPI_EXPORT ITerrainWorker {
public:
    virtual ~ITerrainWorker() = default;

    /** Get storage associated with this worker. The HeightmapGround may
     * execute some operations on the storage.
     *
     * This method can return null, if the worker has no storage. */
    // This may even not be useful if tile dropping is managed by
    // GridStorage
    virtual GridStorageBase *getStorage() { return nullptr; };

    virtual void processTerrain(Terrain &terrain) = 0;

    virtual void processTile(ITileContext &context) = 0;

    /** This method apply all modifications to the terrains before the next
     * worker starts processing. This may be useful if this ITerrainWorker can
     * run several jobs concurrently. */
    virtual void flush(){};
};
} // namespace world

#endif // WORLD_ITERRAINGENERATOR_H
