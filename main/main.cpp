#include <vulkan/vulkan.hpp>
#include <iostream>
using namespace std;
using namespace vk;

int main(int argc, char** argv)
{
	Instance instance;
	Device logicalDevice;
	PhysicalDevice physicalDevice;
	Queue deviceQueue;
	uint32_t queueFamilyIndex;
	PipelineCache pipelineCache;
	DeviceMemory deviceMemory;

	ApplicationInfo appInfo("TEST", 1, "TEST", 1, VK_MAKE_VERSION(1, 2, 135));
	InstanceCreateInfo instanceCreateInfo({}, &appInfo);

	instance = createInstance(instanceCreateInfo);
	auto physicalDevices = instance.enumeratePhysicalDevices();
	physicalDevice = physicalDevices[0];

    auto queueProps = physicalDevice.getQueueFamilyProperties();

    queueFamilyIndex = -1;
    for (size_t i = 0; i < queueProps.size(); ++i) {
        const auto& queueFamilyProperties = queueProps[i];
        if (queueFamilyProperties.queueFlags & QueueFlagBits::eCompute && queueFamilyProperties.queueCount > 0) {
            queueFamilyIndex = i;
            break;
        }
    }
    if (queueFamilyIndex == -1) {
        std::cerr << "No compatible queues found!" << endl;
        exit(1);
    }
    float priority = 1;
    DeviceQueueCreateInfo queueCreateInfo({}, queueFamilyIndex, 1, &priority); 
    DeviceCreateInfo deviceCreateInfo({}, 1, &queueCreateInfo);
    logicalDevice = physicalDevice.createDevice(deviceCreateInfo);
    deviceQueue = logicalDevice.getQueue(queueFamilyIndex, 0);
    pipelineCache = logicalDevice.createPipelineCache({}); 

    auto deviceMemoryProperties = physicalDevice.getMemoryProperties();





    Extent2D extent2D(128, 128);
    ImageCreateInfo imageCreateInfo(
        {},
        ImageType::e2D,
        Format::eR8G8B8A8Unorm,
        Extent3D(extent2D, 1),
        1,
        1,
        SampleCountFlagBits::e1,
        ImageTiling::eOptimal,
        ImageUsageFlagBits::eSampled | ImageUsageFlagBits::eStorage | ImageUsageFlagBits::eTransferSrc,
        SharingMode::eExclusive,
        1,
        &queueFamilyIndex,
        ImageLayout::eUndefined);

    Image image = logicalDevice.createImage(imageCreateInfo);
    MemoryRequirements memoryRequirements = logicalDevice.getImageMemoryRequirements(image);
    size_t alignment = 256;
    size_t memorySize = memoryRequirements.size;
    if (memorySize % alignment != 0) 
        memorySize =  memorySize + alignment - (memorySize % alignment);

    size_t memoryHeapIndex = 0;
    size_t memoryTypeIndex = 0;
    // Find memory accessible by CPU
    for (size_t i = 0; i < deviceMemoryProperties.memoryTypeCount; ++i) {
        const auto type = deviceMemoryProperties.memoryTypes[i];

        if ((memoryRequirements.memoryTypeBits & (1 << i))) {
            memoryHeapIndex = type.heapIndex;
            memoryTypeIndex = i;
            break;
        }
    }

    MemoryAllocateInfo memoryAllocateInfo(memorySize, memoryTypeIndex);
    deviceMemory = logicalDevice.allocateMemory(memoryAllocateInfo);
    logicalDevice.bindImageMemory(image, deviceMemory, 0);


    CommandPoolCreateInfo commandPoolCreateInfo({}, queueFamilyIndex);
    CommandPool commandPool = logicalDevice.createCommandPool(commandPoolCreateInfo);

    ImageMemoryBarrier imageMemoryBarrier({}, {}, ImageLayout::eUndefined, ImageLayout::eGeneral, queueFamilyIndex, queueFamilyIndex, image, ImageSubresourceRange(ImageAspectFlagBits::eColor, 0, 1, 0, 1));

    CommandBufferAllocateInfo commandBufferAllocateInfo(commandPool, CommandBufferLevel::ePrimary, 1);
    CommandBuffer cb = logicalDevice.allocateCommandBuffers(commandBufferAllocateInfo)[0];
    CommandBufferBeginInfo commandBufferBeginInfo(CommandBufferUsageFlagBits::eOneTimeSubmit);
    cb.begin(commandBufferBeginInfo);
    cb.pipelineBarrier(PipelineStageFlagBits::eTopOfPipe, PipelineStageFlagBits::eBottomOfPipe, {}, 0, nullptr, 0,
        nullptr, 1, &imageMemoryBarrier);

    //This one doesn't work
    ArrayProxy<const SubmitInfo> submitInfo = { { 0, nullptr, nullptr, 1, &cb } };
    deviceQueue.submit(submitInfo, {}); // FAILS HERE

    // This one works 
    //SubmitInfo submitInfo = { 0, nullptr, nullptr, 1, &cb };
    //deviceQueue.submit(1, &submitInfo, {}); // Works
    deviceQueue.waitIdle();
    logicalDevice.free(commandPool, 1, &cb);

	return 0;
}
