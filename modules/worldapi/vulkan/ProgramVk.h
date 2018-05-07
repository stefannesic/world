#ifndef WORLDAPI_PROGRAMVK_H
#define WORLDAPI_PROGRAMVK_H

#include "core/WorldConfig.h"
#include "core/WorldTypes.h"

#include <memory>
#include <string>

#include "Vulkan.h"
#include "BufferVk.h"

namespace world {

class PProgramVk;

class WORLDAPI_EXPORT ProgramVk {
public:
	ProgramVk(VulkanContext &context);

	void setBuffer(u32 id, BufferVk buffer);

	void setUniform(u32 id, BufferVk buffer);

	void setEmbeddedShader(const std::string &name);

	void setDispatch(u32 x, u32 y, u32 z);

	void run();

private:
	std::shared_ptr<PProgramVk> _internal;
};

}

#endif // WORLDAPI_PROGRAMVK_H