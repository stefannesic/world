#ifndef WORLD_ITREEWORKER_H
#define WORLD_ITREEWORKER_H

#include "world/core/WorldConfig.h"
#include "world/core/ICloneable.h"

namespace world {

class Tree;

class ITreeWorker : public ICloneable<ITreeWorker> {
public:
    virtual void process(Tree &tree) = 0;
};
} // namespace world

#endif // WORLD_ITREEWORKER_H
