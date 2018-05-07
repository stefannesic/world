#ifdef WORLD_BUILD_VULKAN_MODULES
#include "Vulkan.h"
#include "PVulkan.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>

#include <vulkan.h>

#include "core/WorldTypes.h"
#include "assets/Image.h"

// VULKAN FUNCTIONS

/** debug callback setter */
VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}

// ====

namespace world {

const std::vector<const char*> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData) {

	std::cerr << "validation layer: " << msg << std::endl;

	return VK_FALSE;
}

// ---- INITIALISATION METHODS

bool PVulkanContext::checkValidationLayerSupport() {
	// TODO for each validation layer in validationLayers, check if it's supported and return false if at least one isn't.
	return true;
}

void PVulkanContext::setupDebugCallback() {
	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = debugCallback;

	if (CreateDebugReportCallbackEXT(_instance, &createInfo, nullptr, &_debugCallback) != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] [Initialization] failed to setup debug callback");
	}
}

void PVulkanContext::pickPhysicalDevice() {
	u32 deviceCount = 0;
	vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("[Vulkan] [Initialization] could not find any device that supports Vulkan");
	}
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

	// pick the first device that is suitable
	for (auto &device : devices) {
		// TODO Test suitability
		// Or even better, run a battery of tests to determine which one is the best

		_physicalDevice = device;
	}
}

void PVulkanContext::createLogicalDevice() {
	int familyIndex = findComputeQueueFamily();

	// Information about the queue
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = familyIndex;
	queueCreateInfo.queueCount = 1;

	float queuePriority = 1.0f;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures deviceFeatures = {};

	// Device creation
	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.queueCreateInfoCount = 1;

	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = 0;

	// Validation layers
	if (_enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<u32>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device) != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] [Initialization] failed to create logical device!");
	}
}

void PVulkanContext::createComputeResources() {
	// Get queue
	vkGetDeviceQueue(_device, findComputeQueueFamily(), 0, &_computeQueue);

	// Create descriptor pool
	// TODO Scale descriptor pool size according to needs. Change the system if it's no more sufficient.
	VkDescriptorPoolSize descriptorPoolSizes[2];

	descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorPoolSizes[0].descriptorCount = 10;

	descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorPoolSizes[1].descriptorCount = 10;

	VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.maxSets = 10;
	descriptorPoolInfo.poolSizeCount = 2;
	descriptorPoolInfo.pPoolSizes = descriptorPoolSizes;

	if (vkCreateDescriptorPool(_device, &descriptorPoolInfo, nullptr, &_descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] [Initialization] failed create descriptor pool");
	}

	// Create compute command pool
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = 0;
	commandPoolCreateInfo.queueFamilyIndex = findComputeQueueFamily();

	if (vkCreateCommandPool(_device, &commandPoolCreateInfo, nullptr, &_computeCommandPool) != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] [Initialization] failed create command pool");
	}
}

VkShaderModule PVulkanContext::createShader(const std::vector<char>& shaderCode) {
	VkShaderModuleCreateInfo shaderInfo = {};
	shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderInfo.codeSize = shaderCode.size();
	shaderInfo.pCode = reinterpret_cast<const u32*>(shaderCode.data());

	VkShaderModule shader;
	if (vkCreateShaderModule(_device, &shaderInfo, nullptr, &shader) != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] [Create-Shader] failed to create shader!");
	}

	// Add to the internal register for further deleting
	return shader;
}

int PVulkanContext::findComputeQueueFamily() {
	// Get queues family
	u32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (auto &queueFamily : queueFamilies) {
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
			return i;
		}

		++i;
	}

	return -1;
}

int PVulkanContext::findMemoryType(u32 memorySize) {
	VkPhysicalDeviceMemoryProperties properties;
	vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &properties);

	u32 memoryTypeIndex;
	bool foundMemoryType = false;

	for (u32 i = 0; i < properties.memoryTypeCount; ++i) {
		const VkMemoryType memoryType = properties.memoryTypes[i];

		if ((VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & memoryType.propertyFlags)
			&& (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & memoryType.propertyFlags)
			&& (properties.memoryHeaps[memoryType.heapIndex].size > memorySize)) {

			memoryTypeIndex = i;
			foundMemoryType = true;
		}
	}

	if (!foundMemoryType) {
		throw std::runtime_error("[Vulkan] [Config-Test] did not find suitable memory type");
	}

	return memoryTypeIndex;
}

std::vector<char> PVulkanContext::readFile(const std::string &filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.good()) {
		throw std::runtime_error("[Vulkan] [readFile] File not found");
	}

	size_t filesize = static_cast<size_t>(file.tellg());
	std::vector<char> result(filesize);
	file.seekg(0, file.beg);
	file.read(result.data(), filesize);
	file.close();
	return result;
}

// ---- PUBLIC METHODS

VulkanContext::VulkanContext()
	: _internal(new PVulkanContext()) {

	// Create instance
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "World";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.pNext = nullptr;

	std::vector<const char*> extensions;

	// Validation layers
#ifdef _DEBUG
	_internal->_enableValidationLayers = true;
#else
	_internal->_enableValidationLayers = true; // TODO set it to false when debug will be decently useable
#endif

	if (!_internal->checkValidationLayerSupport() && _internal->_enableValidationLayers) {
		std::cerr << "[Warning] [Vulkan] Validation layers are not supported" << std::endl;
		_internal->_enableValidationLayers = false;
	}

	if (_internal->_enableValidationLayers) {
		// Set layer
		createInfo.enabledLayerCount = static_cast<u32>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		// Activating suitable extension
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	// Setup extensions
	createInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (vkCreateInstance(&createInfo, nullptr, &_internal->_instance) != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] [Initialisation] failed to create instance!");
	}

	// Setup debug callback
	_internal->setupDebugCallback();

	// Initialization
	_internal->pickPhysicalDevice();
	_internal->createLogicalDevice();

	// Compute resources
	_internal->createComputeResources();
}

VulkanContext::~VulkanContext() {
	vkDestroyDescriptorPool(_internal->_device, _internal->_descriptorPool, nullptr);
	vkDestroyCommandPool(_internal->_device, _internal->_computeCommandPool, nullptr);
	vkDestroyDevice(_internal->_device, nullptr);

	if (_internal->_enableValidationLayers) {
		DestroyDebugReportCallbackEXT(_internal->_instance, _internal->_debugCallback, nullptr);
	}

	// It crashes... why ?
	vkDestroyInstance(_internal->_instance, nullptr);

	delete _internal;
}

PVulkanContext &VulkanContext::internal() {
	return *_internal;
}

// http://www.duskborn.com/a-simple-vulkan-compute-example/
// https://github.com/Erkaman/vulkan_minimal_compute
/** Simple perlin noise throught a 1024 * 1024 image */
void VulkanContext::configTest() {
	// TODO Currently this is a kind of toy to play around with Vulkan.
	// Later we should split the differents parts of this function into useable methods for the API.
	
	const VkDeviceSize memorySize = 1024 * 1024 * 4 * sizeof(float);
	
	// Creating buffer
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = memorySize;
	bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer buffer;
	vkCreateBuffer(_internal->_device, &bufferInfo, nullptr, &buffer);

	// Allocate memory
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(_internal->_device, buffer, &memRequirements);

	VkMemoryAllocateInfo memoryInfo = {};
	memoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryInfo.pNext = nullptr;
	memoryInfo.allocationSize = memRequirements.size;
	memoryInfo.memoryTypeIndex = _internal->findMemoryType(memorySize);

	VkDeviceMemory memory;
	if (vkAllocateMemory(_internal->_device, &memoryInfo, nullptr, &memory) != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] [Config-Test] failed allocate memory");
	}

	// Bind memory
	vkBindBufferMemory(_internal->_device, buffer, memory, 0);

	// ==========

	// Create descriptor set layout
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
	descriptorSetLayoutBinding.binding = 0;
	descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorSetLayoutBinding.descriptorCount = 1;
	descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.bindingCount = 1; // only a single binding in this descriptor set layout. 
	descriptorSetLayoutCreateInfo.pBindings = &descriptorSetLayoutBinding;

	VkDescriptorSetLayout descriptorSetLayout;
	if (vkCreateDescriptorSetLayout(_internal->_device, &descriptorSetLayoutCreateInfo, NULL, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] [Config-Test] failed create descriptor set layout");
	}

	// Allocate descriptor set in the pool
	VkDescriptorSetAllocateInfo descriptorSetInfo = {};
	descriptorSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetInfo.descriptorPool = _internal->_descriptorPool;
	descriptorSetInfo.descriptorSetCount = 1;
	descriptorSetInfo.pSetLayouts = &descriptorSetLayout;

	VkDescriptorSet descriptorSet;
	if (vkAllocateDescriptorSets(_internal->_device, &descriptorSetInfo, &descriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] [Config-Test] failed allocate descriptor set");
	}


	// Connect buffer to descriptor
	VkDescriptorBufferInfo descriptorBufferInfo = {};
	descriptorBufferInfo.buffer = buffer;
	descriptorBufferInfo.offset = 0;
	descriptorBufferInfo.range = memorySize;

	VkWriteDescriptorSet writeDescriptorSet = {};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet = descriptorSet; // write to this descriptor set.
	writeDescriptorSet.dstBinding = 0; // write to the first, and only binding.
	writeDescriptorSet.descriptorCount = 1; // update a single descriptor.
	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; // storage buffer.
	writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;

	vkUpdateDescriptorSets(_internal->_device, 1, &writeDescriptorSet, 0, nullptr);

	// ==========

	// Setup the computation pipeline
	// Create shader
	auto shader = _internal->createShader(_internal->readFile("perlin.spv"));

	// Create pipeline stage info
	VkPipelineShaderStageCreateInfo pipelineStageInfo = {};
	pipelineStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	pipelineStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	pipelineStageInfo.module = shader;
	pipelineStageInfo.pName = "main";

	// Create pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	
	VkPipelineLayout pipelineLayout;
	if(vkCreatePipelineLayout(_internal->_device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] [Config-Test] failed create pipeline layout");
	}

	// Create pipeline from layout and stage info
	VkComputePipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stage = pipelineStageInfo;
	pipelineCreateInfo.layout = pipelineLayout;

	VkPipeline pipeline;
	if (vkCreateComputePipelines(_internal->_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] [Config-Test] failed create pipeline");
	}

	// ==========

	// Create command buffer
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = 0;
	commandPoolCreateInfo.queueFamilyIndex = _internal->findComputeQueueFamily();

	VkCommandPool commandPool;
	if (vkCreateCommandPool(_internal->_device, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] [Config-Test] failed create command pool");
	}

	// Allocate a command buffer
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.commandBufferCount = 1;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	
	VkCommandBuffer commandBuffer;
	// TODO Test VK_SUCCESS
	vkAllocateCommandBuffers(_internal->_device, &commandBufferAllocateInfo, &commandBuffer);


	// Writing the commands
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // the buffer is only submitted and used once in this application.

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

	// Dispatching the working groups
	vkCmdDispatch(commandBuffer, 32, 32, 1);
	// End recording
	// TODO Test VK_SUCCESS
	vkEndCommandBuffer(commandBuffer);

	// ==========
	
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1; // submit a single command buffer
	submitInfo.pCommandBuffers = &commandBuffer; // the command buffer to submit.

	// Create fence
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = 0;

	VkFence fence;
	// TODO Test VK_SUCCESS
	vkCreateFence(_internal->_device, &fenceCreateInfo, NULL, &fence);

	// Submid queue
	// TODO Test VK_SUCCESS
	vkQueueSubmit(_internal->_computeQueue, 1, &submitInfo, fence);

	auto start = std::chrono::steady_clock::now();
	// TODO Test VK_SUCCESS
	vkWaitForFences(_internal->_device, 1, &fence, VK_TRUE, 100000000000);
	std::cout << "Exploration terminee en "
		<< std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count()
		<< " us" << std::endl;

	vkDestroyFence(_internal->_device, fence, NULL);

	// ==========
	start = std::chrono::steady_clock::now();

	// Read memory
	float* mappedMemory = nullptr;
	vkMapMemory(_internal->_device, memory, 0, memorySize, 0, reinterpret_cast<void**>(&mappedMemory));

	// Fill and save image
	Image image(1024, 1024, ImageType::RGBA);

	// TODO make a method to fill buffer directly
	for (u32 y = 0; y < 1024; ++y) {
		for (u32 x = 0; x < 1024; ++x) {
			float*ptr = &(mappedMemory[(y * 1024 + x) * 4]);
			image.rgba(x, y).setf(ptr[0], ptr[1], ptr[2], ptr[3]);
		}
	}
	std::cout << "Exploration terminee en "
		<< std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count()
		<< " us" << std::endl;

	image.write("config-test.png");

	vkUnmapMemory(_internal->_device, memory);
}

void VulkanContext::displayAvailableExtensions() {
	// Extensions check
	u32 extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	std::cout << "Extensions :" << std::endl;
	for (auto &ext : extensions) {
		std::cout << "\t- " << ext.extensionName << std::endl;
	}
}


VulkanContext &Vulkan::context() {
	static VulkanContext context;
	return context;
}
}

#endif