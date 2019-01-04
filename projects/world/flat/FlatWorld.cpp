#include "FlatWorld.h"

#include "world/terrain/Ground.h"
#include "world/tree/ForestLayer.h"
#include "world/tree/SimpleTreeDecorator.h"

namespace world {

class PFlatWorld {
public:
    PFlatWorld() {}

    std::vector<std::unique_ptr<FlatWorldDecorator>> _chunkDecorators;
};

FlatWorld *FlatWorld::createDemoFlatWorld() {
    FlatWorld *world = new FlatWorld();

    world->addFlatWorldDecorator<ForestLayer>();

    return world;
}

FlatWorld::FlatWorld() : _internal(new PFlatWorld()) {

    Ground &ground = setGround<Ground>();
    ground.setDefaultWorkerSet();
}

FlatWorld::~FlatWorld() { delete _internal; }

IGround &FlatWorld::ground() { return *_ground; }

void FlatWorld::collect(const WorldZone &zone, ICollector &collector,
                        const IResolutionModel &resolutionModel) {
    World::collect(zone, collector, resolutionModel);

    ground().collectZone(zone, collector, resolutionModel);
}

void FlatWorld::onFirstExploration(const WorldZone &chunk) {
    World::onFirstExploration(chunk);

    for (auto &decorator : _internal->_chunkDecorators) {
        decorator->decorate(*this, chunk);
    }
}

void FlatWorld::setGroundInternal(IGround *ground) {
    _ground = std::unique_ptr<IGround>(ground);
}

void FlatWorld::addFlatWorldDecoratorInternal(
    world::FlatWorldDecorator *decorator) {
    _internal->_chunkDecorators.emplace_back(decorator);
}
} // namespace world