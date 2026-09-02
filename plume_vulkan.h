//
// plume
//
// Copyright (c) 2024 renderbag and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file for details.
//

#pragma once

#include "plume_render_interface.h"

#include <functional>
#include <mutex>
#include <set>
#include <unordered_map>
#include <unordered_set>

#if defined(_WIN64)
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(__linux__) && !defined(PLUME_SDL_VULKAN_ENABLED)
#define VK_USE_PLATFORM_XLIB_KHR
#elif defined(__APPLE__)
#define VK_USE_PLATFORM_METAL_EXT
#include "plume_apple.h"
#endif

// For VK_KHR_portability_subset
#define VK_ENABLE_BETA_EXTENSIONS

#include <volk.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"
#endif

#include <vk_mem_alloc.h>

#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace plume {
    struct VulkanCommandQueue;
    struct VulkanDevice;
    struct VulkanInterface;
    struct VulkanPool;
    struct VulkanQueue;

    struct VulkanBuffer : RenderBuffer {
        VkBuffer vk = VK_NULL_HANDLE;
        VulkanDevice *device = nullptr;
        VulkanPool *pool = nullptr;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VmaAllocationInfo allocationInfo = {};
        RenderBufferDesc desc;
        RenderBarrierStages barrierStages = RenderBarrierStage::NONE;

        VulkanBuffer() = default;
        VulkanBuffer(VulkanDevice *device, VulkanPool *pool, const RenderBufferDesc &desc);
        ~VulkanBuffer() override;
        void *map(uint32_t subresource, const RenderRange *readRange) override;
        void unmap(uint32_t subresource, const RenderRange *writtenRange) override;
        std::unique_ptr<RenderBufferFormattedView> createBufferFormattedView(RenderFormat format) override;
        void setName(const std::string &name) override;
        uint64_t getDeviceAddress() const override;
    };

    struct VulkanBufferFormattedView : RenderBufferFormattedView {
        VkBufferView vk = VK_NULL_HANDLE;
        VulkanBuffer *buffer = nullptr;

        VulkanBufferFormattedView(VulkanBuffer *buffer, RenderFormat format);
        ~VulkanBufferFormattedView() override;
    };

    struct VulkanTexture : RenderTexture {
        VkImage vk = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkFormat imageFormat = VK_FORMAT_UNDEFINED;
        VkImageSubresourceRange imageSubresourceRange = {};
        VulkanDevice *device = nullptr;
        VulkanPool *pool = nullptr;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VmaAllocationInfo allocationInfo = {};
        RenderTextureLayout textureLayout = RenderTextureLayout::UNKNOWN;
        RenderBarrierStages barrierStages = RenderBarrierStage::NONE;
        bool ownership = false;
        RenderTextureDesc desc;

        VulkanTexture() = default;
        VulkanTexture(VulkanDevice *device, VulkanPool *pool, const RenderTextureDesc &desc);
        VulkanTexture(VulkanDevice *device, VkImage image);
        ~VulkanTexture() override;
        void createImageView(VkFormat format);
        std::unique_ptr<RenderTextureView> createTextureView(const RenderTextureViewDesc &desc) const override;
        void setName(const std::string &name) override;
        void fillSubresourceRange();
    };

    struct VulkanTextureView : RenderTextureView {
        VkImageView vk = VK_NULL_HANDLE;
        const VulkanTexture *texture = nullptr;
        RenderTextureViewDesc desc;

        VulkanTextureView(const VulkanTexture *texture, const RenderTextureViewDesc &desc);
        ~VulkanTextureView() override;
    };

    struct VulkanAccelerationStructure : RenderAccelerationStructure {
        VkAccelerationStructureKHR vk = VK_NULL_HANDLE;
        VulkanDevice *device = nullptr;
        RenderAccelerationStructureType type = RenderAccelerationStructureType::UNKNOWN;

        VulkanAccelerationStructure(VulkanDevice *device, const RenderAccelerationStructureDesc &desc);
        ~VulkanAccelerationStructure() override;
    };

    struct VulkanDescriptorSetLayout {
        VkDescriptorSetLayout vk = VK_NULL_HANDLE;
        std::vector<VkDescriptorSetLayoutBinding> setBindings;
        std::vector<uint32_t> descriptorIndexBases;
        std::vector<uint32_t> descriptorBindingIndices;
        VulkanDevice *device = nullptr;

        VulkanDescriptorSetLayout(VulkanDevice *device, const RenderDescriptorSetDesc &descriptorSetDesc);
        ~VulkanDescriptorSetLayout();
    };

    struct VulkanPipelineLayout : RenderPipelineLayout {
        VkPipelineLayout vk = VK_NULL_HANDLE;
        std::vector<VkPushConstantRange> pushConstantRanges;
        std::vector<VulkanDescriptorSetLayout *> descriptorSetLayouts;
        VulkanDevice *device = nullptr;

        VulkanPipelineLayout(VulkanDevice *device, const RenderPipelineLayoutDesc &desc);
        ~VulkanPipelineLayout() override;
    };

    struct VulkanShader : RenderShader {
        VkShaderModule vk = VK_NULL_HANDLE;
        std::string entryPointName;
        VulkanDevice *device = nullptr;
        RenderShaderFormat format = RenderShaderFormat::UNKNOWN;

        VulkanShader(VulkanDevice *device, const void *data, uint64_t size, const char *entryPointName, RenderShaderFormat format);
        ~VulkanShader() override;
        virtual void setName(const std::string &name) override;
    };

    struct VulkanSampler : RenderSampler {
        VkSampler vk = VK_NULL_HANDLE;
        VulkanDevice *device = nullptr;

        VulkanSampler(VulkanDevice *device, const RenderSamplerDesc &desc);
        ~VulkanSampler();
    };

    struct VulkanPipeline : RenderPipeline {
        enum class Type {
            Unknown,
            Compute,
            Graphics,
            Raytracing
        };

        VulkanDevice *device = nullptr;
        Type type = Type::Unknown;

        VulkanPipeline(VulkanDevice *device, Type type);
        virtual ~VulkanPipeline() override;
    };

    struct VulkanComputePipeline : VulkanPipeline {
        VkPipeline vk = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

        VulkanComputePipeline(VulkanDevice *device, const RenderComputePipelineDesc &desc);
        ~VulkanComputePipeline() override;
        void setName(const std::string &name) override;
        RenderPipelineProgram getProgram(const std::string &name) const override;
    };

    struct VulkanGraphicsPipeline : VulkanPipeline {
        VkPipeline vk = VK_NULL_HANDLE;
        VkRenderPass renderPass = VK_NULL_HANDLE;

        VulkanGraphicsPipeline(VulkanDevice *device, const RenderGraphicsPipelineDesc &desc);
        ~VulkanGraphicsPipeline() override;
        void setName(const std::string &name) override;
        RenderPipelineProgram getProgram(const std::string &name) const override;
        static VkRenderPass createRenderPass(VulkanDevice *device, const VkFormat *renderTargetFormat, uint32_t renderTargetCount, VkFormat depthTargetFormat, VkSampleCountFlagBits sampleCount, uint32_t viewMask = 0, bool fragmentDensityMap = false);
    };

    struct VulkanRaytracingPipeline : VulkanPipeline {
        VkPipeline vk = VK_NULL_HANDLE;
        std::unordered_map<std::string, RenderPipelineProgram> nameProgramMap;
        uint32_t groupCount = 0;
        uint32_t descriptorSetCount = 0;

        VulkanRaytracingPipeline(VulkanDevice *device, const RenderRaytracingPipelineDesc &desc, const RenderPipeline *previousPipeline);
        ~VulkanRaytracingPipeline() override;
        void setName(const std::string &name) override;
        RenderPipelineProgram getProgram(const std::string &name) const override;
    };

    struct VulkanDescriptorSet : RenderDescriptorSet {
        VkDescriptorSet vk = VK_NULL_HANDLE;
        VulkanDescriptorSetLayout *setLayout = nullptr;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        VulkanDevice *device = nullptr;

        VulkanDescriptorSet(VulkanDevice *device, const RenderDescriptorSetDesc &desc);
        ~VulkanDescriptorSet() override;
        void setBuffer(uint32_t descriptorIndex, const RenderBuffer *buffer, uint64_t bufferSize, const RenderBufferStructuredView *bufferStructuredView, const RenderBufferFormattedView *bufferFormattedView) override;
        void setTexture(uint32_t descriptorIndex, const RenderTexture *texture, RenderTextureLayout textureLayout, const RenderTextureView *textureView) override;
        void setSampler(uint32_t descriptorIndex, const RenderSampler *sampler) override;
        void setAccelerationStructure(uint32_t descriptorIndex, const RenderAccelerationStructure *accelerationStructure) override;
        void setDescriptor(uint32_t descriptorIndex, const VkDescriptorBufferInfo *bufferInfo, const VkDescriptorImageInfo *imageInfo, const VkBufferView *texelBufferView, void *pNext);
        static VkDescriptorPool createDescriptorPool(VulkanDevice *device, const std::unordered_map<VkDescriptorType, uint32_t> &typeCounts, bool lastRangeIsBoundless);
    };

    struct VulkanSwapChain : RenderSwapChain {
        RenderSwapChainDesc desc;
        VkSwapchainKHR vk = VK_NULL_HANDLE;
        VulkanCommandQueue *commandQueue = nullptr;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
#if defined(__APPLE__)
        std::unique_ptr<CocoaWindow> windowWrapper;
#endif
        uint64_t presentCount = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        VkSwapchainCreateInfoKHR createInfo = {};
        VkSurfaceFormatKHR pickedSurfaceFormat = {};
        VkPresentModeKHR createdPresentMode = VK_PRESENT_MODE_FIFO_KHR;
        VkPresentModeKHR requiredPresentMode = VK_PRESENT_MODE_FIFO_KHR;
        VkCompositeAlphaFlagBitsKHR pickedAlphaFlag = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        std::vector<VulkanTexture> textures;
        uint64_t currentPresentId = 0;
        bool immediatePresentModeSupported = false;
        bool mailboxPresentModeSupported = false;

        VulkanSwapChain(VulkanCommandQueue *commandQueue, const RenderSwapChainDesc &desc);
        ~VulkanSwapChain() override;
        bool present(uint32_t textureIndex, RenderCommandSemaphore **waitSemaphores, uint32_t waitSemaphoreCount) override;
        void wait() override;
        bool resize() override;
        bool needsResize() const override;
        void setVsyncEnabled(bool vsyncEnabled) override;
        bool isVsyncEnabled() const override;
        uint32_t getWidth() const override;
        uint32_t getHeight() const override;
        RenderTexture *getTexture(uint32_t textureIndex) override;
        uint32_t getTextureCount() const override;
        bool acquireTexture(RenderCommandSemaphore *signalSemaphore, uint32_t *textureIndex) override;
        RenderWindow getWindow() const override;
        bool isEmpty() const override;
        uint32_t getRefreshRate() const override;
        void getWindowSize(uint32_t &dstWidth, uint32_t &dstHeight) const;
        void releaseSwapChain();
        void releaseImageViews();
    };

    struct VulkanFramebuffer : RenderFramebuffer {
        VulkanDevice *device = nullptr;
        VkFramebuffer vk = VK_NULL_HANDLE;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        std::vector<const VulkanTexture *> colorAttachments;
        const VulkanTexture *depthAttachment = nullptr;
        std::unique_ptr<VulkanTextureView> depthAttachmentView = nullptr;
        bool depthAttachmentReadOnly = false;
        uint32_t width = 0;
        uint32_t height = 0;

        // Everything needed to rebuild this pass with a different set of load
        // operations. A tile-based GPU pays main-memory bandwidth for every
        // LOAD_OP_LOAD, so a pass whose contents are about to be cleared wants
        // LOAD_OP_CLEAR instead - see getRenderPass.
        std::vector<VkAttachmentDescription> attachmentDescs;
        std::vector<VkAttachmentReference> colorReferences;
        VkAttachmentReference depthReference = {};
        // Foveation, mirrored into every clear-variant pass so they stay
        // compatible with the pipelines drawn into them.
        bool hasFragmentDensityMap = false;
        VkAttachmentReference densityMapReference = {};
        bool hasDepthAttachment = false;
        uint32_t viewMask = 0;
        mutable std::unordered_map<uint32_t, VkRenderPass> clearPassCache;

        VulkanFramebuffer(VulkanDevice *device, const RenderFramebufferDesc &desc);
        ~VulkanFramebuffer() override;
        uint32_t getWidth() const override;
        uint32_t getHeight() const override;
        bool contains(const VulkanTexture *attachment) const;
        // clearMask: bit i selects colour attachment i, bit 8 the depth/stencil
        // attachment. 0 returns the plain LOAD pass. Variants are cached, and
        // all of them are render-pass-compatible with this framebuffer because
        // Vulkan compatibility ignores load and store operations.
        VkRenderPass getRenderPass(uint32_t clearMask) const;
    };

    struct VulkanQueryPool : RenderQueryPool {
        VulkanDevice *device = nullptr;
        std::vector<uint64_t> results;
        VkQueryPool vk = VK_NULL_HANDLE;

        VulkanQueryPool(VulkanDevice *device, uint32_t queryCount);
        virtual ~VulkanQueryPool() override;
        virtual void queryResults(uint32_t count = 0) override;
        virtual const uint64_t *getResults() const override;
        virtual uint32_t getCount() const override;
    };

    struct VulkanCommandList : RenderCommandList {
        VkCommandBuffer vk = VK_NULL_HANDLE;
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VulkanCommandQueue *queue = nullptr;
        const VulkanFramebuffer *targetFramebuffer = nullptr;
        // A clear issued while no render pass is open is deferred, so it can be
        // folded into the pass's load operation rather than costing a full tile
        // load followed by a vkCmdClearAttachments that discards it.
        uint32_t pendingClearMask = 0;
        // Textures the caller has said it does not need the previous contents
        // of. Turned into VK_ATTACHMENT_LOAD_OP_DONT_CARE at pass begin, which
        // on a tiler is the difference between reading the whole target back
        // from memory and not touching it at all. discardTexture used to be a
        // no-op here ("not required in Vulkan"), which is true of the API and
        // false of the hardware.
        std::vector<const RenderTexture *> pendingDiscards;
        VkClearValue pendingClearValues[9] = {};
        const VulkanPipelineLayout *activeComputePipelineLayout = nullptr;
        const VulkanPipelineLayout *activeGraphicsPipelineLayout = nullptr;
        const VulkanPipelineLayout *activeRaytracingPipelineLayout = nullptr;
        VkRenderPass activeRenderPass = VK_NULL_HANDLE;

        VulkanCommandList(VulkanCommandQueue *queue);
        ~VulkanCommandList() override;
        void begin() override;
        void end() override;
        void barriers(RenderBarrierStages stages, const RenderBufferBarrier *bufferBarriers, uint32_t bufferBarriersCount, const RenderTextureBarrier *textureBarriers, uint32_t textureBarriersCount) override;
        void dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) override;
        void traceRays(uint32_t width, uint32_t height, uint32_t depth, RenderBufferReference shaderBindingTable, const RenderShaderBindingGroupsInfo &shaderBindingGroupsInfo) override;
        void drawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation) override;
        void drawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation) override;
        void setPipeline(const RenderPipeline *pipeline) override;
        void setComputePipelineLayout(const RenderPipelineLayout *pipelineLayout) override;
        void setComputePushConstants(uint32_t rangeIndex, const void *data, uint32_t offset = 0, uint32_t size = 0) override;
        void setComputeDescriptorSet(RenderDescriptorSet *descriptorSet, uint32_t setIndex) override;
        void setGraphicsPipelineLayout(const RenderPipelineLayout *pipelineLayout) override;
        void setGraphicsPushConstants(uint32_t rangeIndex, const void *data, uint32_t offset = 0, uint32_t size = 0) override;
        void setGraphicsDescriptorSet(RenderDescriptorSet *descriptorSet, uint32_t setIndex) override;
        void setGraphicsDescriptorSetDynamic(RenderDescriptorSet *descriptorSet, uint32_t setIndex, const uint32_t *dynamicOffsets, uint32_t dynamicOffsetCount) override;
        void setGraphicsRootDescriptor(RenderBufferReference bufferReference, uint32_t rootDescriptorIndex) override;
        void setRaytracingPipelineLayout(const RenderPipelineLayout *pipelineLayout) override;
        void setRaytracingPushConstants(uint32_t rangeIndex, const void *data, uint32_t offset = 0, uint32_t size = 0) override;
        void setRaytracingDescriptorSet(RenderDescriptorSet *descriptorSet, uint32_t setIndex) override;
        void setIndexBuffer(const RenderIndexBufferView *view) override;
        void setVertexBuffers(uint32_t startSlot, const RenderVertexBufferView *views, uint32_t viewCount, const RenderInputSlot *inputSlots) override;
        void setViewports(const RenderViewport *viewports, uint32_t count) override;
        void setScissors(const RenderRect *scissorRects, uint32_t count) override;
        void setFramebuffer(const RenderFramebuffer *framebuffer) override;
        void setDepthBias(float depthBias, float depthBiasClamp, float slopeScaledDepthBias) override;
        void clearColor(uint32_t attachmentIndex, RenderColor colorValue, const RenderRect *clearRects, uint32_t clearRectsCount) override;
        void clearDepthStencil(bool clearDepth, bool clearStencil, float depthValue, uint32_t stencilValue, const RenderRect *clearRects, uint32_t clearRectsCount) override;
        void copyBufferRegion(RenderBufferReference dstBuffer, RenderBufferReference srcBuffer, uint64_t size) override;
        void copyTextureRegion(const RenderTextureCopyLocation &dstLocation, const RenderTextureCopyLocation &srcLocation, uint32_t dstX, uint32_t dstY, uint32_t dstZ, const RenderBox *srcBox) override;
        void copyBuffer(const RenderBuffer *dstBuffer, const RenderBuffer *srcBuffer) override;
        void copyTexture(const RenderTexture *dstTexture, const RenderTexture *srcTexture) override;
        void resolveTexture(const RenderTexture *dstTexture, const RenderTexture *srcTexture) override;
        void resolveTextureRegion(const RenderTexture *dstTexture, uint32_t dstX, uint32_t dstY, const RenderTexture *srcTexture, const RenderRect *srcRect, RenderResolveMode resolveMode) override;
        void buildBottomLevelAS(const RenderAccelerationStructure *dstAccelerationStructure, RenderBufferReference scratchBuffer, const RenderBottomLevelASBuildInfo &buildInfo) override;
        void buildTopLevelAS(const RenderAccelerationStructure *dstAccelerationStructure, RenderBufferReference scratchBuffer, RenderBufferReference instancesBuffer, const RenderTopLevelASBuildInfo &buildInfo) override;
        void discardTexture(const RenderTexture* texture) override;
        void resetQueryPool(const RenderQueryPool *queryPool, uint32_t queryFirstIndex, uint32_t queryCount) override;
        void writeTimestamp(const RenderQueryPool *queryPool, uint32_t queryIndex) override;
        void checkActiveRenderPass();
        void endActiveRenderPass();
        void setDescriptorSet(VkPipelineBindPoint bindPoint, const VulkanPipelineLayout *pipelineLayout, const RenderDescriptorSet *descriptorSet, uint32_t setIndex, const uint32_t *dynamicOffsets = nullptr, uint32_t dynamicOffsetCount = 0);
    };

    struct VulkanCommandFence : RenderCommandFence {
        VkFence vk = VK_NULL_HANDLE;
        VulkanDevice *device = nullptr;

        VulkanCommandFence(VulkanDevice *device);
        ~VulkanCommandFence() override;
    };

    struct VulkanCommandSemaphore : RenderCommandSemaphore {
        VkSemaphore vk = VK_NULL_HANDLE;
        VulkanDevice *device = nullptr;

        VulkanCommandSemaphore(VulkanDevice *device);
        ~VulkanCommandSemaphore() override;
    };

    struct VulkanCommandQueue : RenderCommandQueue {
        VulkanQueue *queue = nullptr;
        VulkanDevice *device = nullptr;
        uint32_t familyIndex = 0;
        uint32_t queueIndex = 0;
        std::unordered_set<VulkanSwapChain *> swapChains;
        RenderCommandListType type = RenderCommandListType::UNKNOWN;

        VulkanCommandQueue(VulkanDevice *device, RenderCommandListType type);
        ~VulkanCommandQueue() override;
        std::unique_ptr<RenderCommandList> createCommandList() override;
        std::unique_ptr<RenderSwapChain> createSwapChain(const RenderSwapChainDesc &desc) override;
        void executeCommandLists(const RenderCommandList **commandLists, uint32_t commandListCount, RenderCommandSemaphore **waitSemaphores, uint32_t waitSemaphoreCount, RenderCommandSemaphore **signalSemaphores, uint32_t signalSemaphoreCount, RenderCommandFence *signalFence) override;
        void waitForCommandFence(RenderCommandFence *fence) override;
    };

    struct VulkanPool : RenderPool {
        VmaPool vk = VK_NULL_HANDLE;
        VulkanDevice *device = nullptr;

        VulkanPool(VulkanDevice *device, const RenderPoolDesc &desc);
        ~VulkanPool() override;
        std::unique_ptr<RenderBuffer> createBuffer(const RenderBufferDesc &desc) override;
        std::unique_ptr<RenderTexture> createTexture(const RenderTextureDesc &desc) override;
    };
    
    struct VulkanQueue {
        VkQueue vk;
        std::unique_ptr<std::mutex> mutex;
        std::unordered_set<const VulkanCommandQueue *> virtualQueues;
    };

    struct VulkanQueueFamily {
        std::vector<VulkanQueue> queues;

        void add(VulkanCommandQueue *virtualQueue);
        void remove(VulkanCommandQueue *virtualQueue);
    };

    struct VulkanDevice : RenderDevice {
        VkDevice vk = VK_NULL_HANDLE;
        VulkanInterface *renderInterface = nullptr;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties physicalDeviceProperties = {};
        VmaAllocator allocator = VK_NULL_HANDLE;
        uint32_t queueFamilyIndices[3] = {};
        std::vector<VulkanQueueFamily> queueFamilies;
        RenderDeviceCapabilities capabilities;
        RenderDeviceDescription description;
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtPipelineProperties = {};
        VkPhysicalDeviceSampleLocationsPropertiesEXT sampleLocationProperties = {};
        std::unique_ptr<RenderBuffer> nullBuffer;
        bool loadStoreOpNoneSupported = false;
        bool nullDescriptorSupported = false;

        VulkanDevice(VulkanInterface *renderInterface, const std::string &preferredDeviceName);
        ~VulkanDevice() override;
        std::unique_ptr<RenderDescriptorSet> createDescriptorSet(const RenderDescriptorSetDesc &desc) override;
        std::unique_ptr<RenderShader> createShader(const void *data, uint64_t size, const char *entryPointName, RenderShaderFormat format) override;
        std::unique_ptr<RenderSampler> createSampler(const RenderSamplerDesc &desc) override;
        std::unique_ptr<RenderPipeline> createComputePipeline(const RenderComputePipelineDesc &desc) override;
        std::unique_ptr<RenderPipeline> createGraphicsPipeline(const RenderGraphicsPipelineDesc &desc) override;
        std::unique_ptr<RenderPipeline> createRaytracingPipeline(const RenderRaytracingPipelineDesc &desc, const RenderPipeline *previousPipeline) override;
        std::unique_ptr<RenderCommandQueue> createCommandQueue(RenderCommandListType type) override;
        std::unique_ptr<RenderBuffer> createBuffer(const RenderBufferDesc &desc) override;
        std::unique_ptr<RenderTexture> createTexture(const RenderTextureDesc &desc) override;
        std::unique_ptr<RenderAccelerationStructure> createAccelerationStructure(const RenderAccelerationStructureDesc &desc) override;
        std::unique_ptr<RenderPool> createPool(const RenderPoolDesc &desc) override;
        std::unique_ptr<RenderPipelineLayout> createPipelineLayout(const RenderPipelineLayoutDesc &desc) override;
        std::unique_ptr<RenderCommandFence> createCommandFence() override;
        std::unique_ptr<RenderCommandSemaphore> createCommandSemaphore() override;
        std::unique_ptr<RenderFramebuffer> createFramebuffer(const RenderFramebufferDesc &desc) override;
        std::unique_ptr<RenderQueryPool> createQueryPool(uint32_t queryCount) override;
        void setBottomLevelASBuildInfo(RenderBottomLevelASBuildInfo &buildInfo, const RenderBottomLevelASMesh *meshes, uint32_t meshCount, bool preferFastBuild, bool preferFastTrace) override;
        void setTopLevelASBuildInfo(RenderTopLevelASBuildInfo &buildInfo, const RenderTopLevelASInstance *instances, uint32_t instanceCount, bool preferFastBuild, bool preferFastTrace) override;
        void setShaderBindingTableInfo(RenderShaderBindingTableInfo &tableInfo, const RenderShaderBindingGroups &groups, const RenderPipeline *pipeline, RenderDescriptorSet **descriptorSets, uint32_t descriptorSetCount) override;
        const RenderDeviceCapabilities &getCapabilities() const override;
        const RenderDeviceDescription &getDescription() const override;
        RenderSampleCounts getSampleCountsSupported(RenderFormat format) const override;
        void release();
        bool isValid() const;
        bool beginCapture() override;
        bool endCapture() override;
    };

    // Lets an OpenXR runtime dictate the parts of Vulkan setup it owns. The
    // runtime names the instance extensions, the device extensions and the
    // physical device it will present from; an app that picks its own gets
    // XR_ERROR_GRAPHICS_DEVICE_INVALID at session creation.
    //
    // Every field is optional. A null options pointer keeps the original
    // behaviour exactly.
    struct VulkanInterfaceOptions {
        // std::function for the two creation callbacks below.
        std::vector<std::string> extraInstanceExtensions;
        std::vector<std::string> extraDeviceExtensions;
        // From xrGetVulkanGraphicsDeviceKHR. Overrides the scoring below,
        // which would otherwise pick the discrete GPU on a laptop whose
        // headset is wired to the integrated one.
        VkPhysicalDevice forcedPhysicalDevice = VK_NULL_HANDLE;
        // From xrGetVulkanGraphicsRequirementsKHR. Only ever raises the
        // version plume asks for, never lowers it.
        uint32_t minApiVersion = 0;
        // A Vulkan ICD to load directly instead of the platform loader: the
        // library's vk_icdGetInstanceProcAddr becomes volk's entry point. For
        // a replacement driver such as Mesa's Turnip on Adreno, whose logging
        // says why a render pass fell back to system-memory rendering where
        // the vendor blob says nothing. Empty means the normal loader.
        std::string icdLibraryPath;
        // When set, these perform the actual vkCreateInstance / vkCreateDevice
        // with the create-info plume built, so an OpenXR runtime can create
        // both through XR_KHR_vulkan_enable2 (xrCreateVulkanInstanceKHR and
        // xrCreateVulkanDeviceKHR take the same Vulkan structures plus the
        // vkGetInstanceProcAddr to use). The runtime then dispatches through
        // that entry point rather than the system loader, which is what lets a
        // directly loaded ICD drive the headset.
        std::function<VkResult(const VkInstanceCreateInfo *, VkInstance *)> createInstance;
        std::function<VkResult(VkPhysicalDevice, const VkDeviceCreateInfo *, VkDevice *)> createDevice;
    };

    struct VulkanInterface : RenderInterface {
        VkInstance instance = VK_NULL_HANDLE;
        VkApplicationInfo appInfo = {};
        RenderInterfaceCapabilities capabilities;
        std::vector<std::string> deviceNames;
        VulkanInterfaceOptions options;

        VulkanInterface(const VulkanInterfaceOptions *options = nullptr);

        ~VulkanInterface() override;
        std::unique_ptr<RenderDevice> createDevice(const std::string &preferredDeviceName) override;
        const RenderInterfaceCapabilities &getCapabilities() const override;
        const std::vector<std::string> &getDeviceNames() const override;
        bool isValid() const;
    };

    std::unique_ptr<RenderInterface> CreateVulkanInterface(const VulkanInterfaceOptions *options);
};
