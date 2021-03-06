#pragma once

#include <map>
#include <chrono>
#include <memory>

#include <irrlicht.h>

#include <world/core.h>
#include <world/flat.h>

#include "RenderingModule.h"

class ObjectsManager;

class ObjectNodeHandler {
public:
    bool removeTag = false;


    ObjectNodeHandler(ObjectsManager &objectsManager,
                      const world::SceneNode &obj, world::Collector &collector);
    virtual ~ObjectNodeHandler();

    /// return true if texture was found
    bool setTexture(int id, const std::string &path,
                    world::Collector &collector);
    void setMaterial(const world::Material &material,
                     world::Collector &collector);

private:
    ObjectsManager &_objManager;

    irr::scene::IMeshSceneNode *_meshNode;
    std::vector<std::string> _usedTextures;

    void updateObject3D(const world::SceneNode &object,
                        world::Collector &collector);
};

class ObjectsManager : public RenderingModule {
public:
    std::map<std::string, irr::video::E_MATERIAL_TYPE> _loadedShaders;


    ObjectsManager(Application &app, irr::IrrlichtDevice *device);
    ~ObjectsManager() override;

    void initialize(world::Collector &collector) override;
    void update(world::Collector &collector) override;

    irr::video::ITexture *getOrLoadTexture(const std::string &texture,
                                           world::Collector &collector);
    void addTextureUser(const std::string &texId);
    void removeTextureUser(const std::string &texId);

    static irr::scene::SMesh *convertToIrrlichtMesh(
        const world::Mesh &mesh, irr::video::IVideoDriver *driver);

private:
    std::map<world::ItemKey, std::unique_ptr<ObjectNodeHandler>> _objects;
    std::map<std::string, int> _loadedTextures;

    /** If true, on each update, objects that were already present
     * on the last update are not updated to save time and avoid lags.*/
    bool _partialUpdate = true;

    // Debug variables
    bool _dbgOn = false;
    int _dbgAdded = 0;
    int _dbgRemoved = 0;
    long _elapsedTime = 0;
};
