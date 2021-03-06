
#ifndef EXPLORATION_CONTEXT_H
#define EXPLORATION_CONTEXT_H

#include "world/core/WorldConfig.h"

#include "WorldKeys.h"
#include "world/math/Vector.h"
#include "world/assets/SceneNode.h"
#include "IEnvironment.h"

namespace world {

class WORLDAPI_EXPORT ExplorationContext {
public:
    static const ExplorationContext &getDefault();


    ExplorationContext();

    void addOffset(const vec3d &offset);

    void appendPrefix(const NodeKey &prefix);

    void setEnvironment(IEnvironment *environment);

    ItemKey mutateKey(const ItemKey &key) const;

    /// Handy alias for #mutateKey
    ItemKey operator()(const ItemKey &key) const;

    vec3d getOffset() const;

    /** Create a node with the keys given in parameters. Keys are mutated as
     * needed.
     * @param meshKey The relative key of the mesh.
     * @param materialKey The relative key of the material. */
    SceneNode createNode(const ItemKey &meshKey,
                         const ItemKey &materialKey) const;

    bool hasEnvironment() const;

    const IEnvironment &getEnvironment() const;

private:
    ItemKey _keyPrefix;
    vec3d _offset;

    IEnvironment *_environment;
};

} // namespace world

#endif // EXPLORATION_CONTEXT_H
