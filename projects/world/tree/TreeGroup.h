#ifndef WORLD_FOREST_H
#define WORLD_FOREST_H

#include "world/core/WorldConfig.h"
#include "world/core/World.h"
#include "world/core/WorldNode.h"
#include "world/assets/Mesh.h"
#include "Tree.h"

namespace world {

class PTreeGroup;

/** A TreeGroup enables several trees to be rendered as
 * a single mesh. This is useful when the trees are really
 * far (ie a very low LOD) and you want to limit the number
 * of different objects in the scene. */
class WORLDAPI_EXPORT TreeGroup : public WorldNode {
public:
    TreeGroup();

    ~TreeGroup() override;

    void addTree(const vec3d &pos);

    void collect(ICollector &collector, const IResolutionModel &resolutionModel,
                 const ExplorationContext &ctx) override;

private:
    PTreeGroup *_internal;

    Mesh _trunksMesh;
    Mesh _leavesMesh;


    void configTree(Tree &tree);
};

} // namespace world

#endif // WORLD_FOREST_H
