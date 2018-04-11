#ifndef WORLD_BOUNDINGBOX_H
#define WORLD_BOUNDINGBOX_H

#include "core/WorldConfig.h"

#include "Vector.h"

namespace world {
    class WORLDAPI_EXPORT BoundingBox {
    public:
        BoundingBox(const vec3d& b1, const vec3d& b2);
        void reset(const vec3d& b1, const vec3d& b2);

        vec3d getLowerBound() const;
        vec3d getUpperBound() const;

        vec3d getDimensions() const;
    private:
        vec3d _lower;
        vec3d _upper;
    };
}


#endif //WORLD_BOUNDINGBOX_H