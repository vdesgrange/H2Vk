/*
*  H2Vk - Atmospheric scattering
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/


#include "vk_atmosphere.h"
#include "core/utilities/vk_initializers.h"
#include "core/vk_descriptor_allocator.h"
#include "core/vk_descriptor_cache.h"
#include "core/vk_descriptor_builder.h"
#include "core/vk_pipeline.h"
#include "core/manager/vk_material_manager.h"
#include "components/lighting/vk_light.h"
#include "core/vk_command_pool.h"
#include "core/vk_command_buffer.h"
#include "core/utilities/vk_global.h"
#include "components/camera/vk_camera.h"

#include <chrono>
#include <iostream>

Atmosphere::Atmosphere(Device &device, MaterialManager& materialManager, LightingManager& lightingManager, UploadContext& uploadContext) :
_device(device),
_materialManager(materialManager),
_lightingManager(lightingManager),
_uploadContext(uploadContext),
_multipleScatteringRenderPass(RenderPass(device)),
_skyviewRenderPass(RenderPass(device)) {
}

Atmosphere::~Atmosphere() {
    this->_transmittanceLUT.destroy(_device);
    this->_multipleScatteringLUT.destroy(_device);
    this->_skyviewLUT.destroy(_device);
    this->_atmosphereLUT.destroy(_device);

//    if (_multipleScatteringFramebuffer != VK_NULL_HANDLE) {
//        vkDestroyFramebuffer(_device._logicalDevice, _multipleScatteringFramebuffer, nullptr);
//    }

//    if (_skyviewFramebuffer != VK_NULL_HANDLE) {
//        vkDestroyFramebuffer(_device._logicalDevice, _skyviewFramebuffer, nullptr);
//    }

//    if (_atmosphereFramebuffer != VK_NULL_HANDLE) {
//        vkDestroyFramebuffer(_device._logicalDevice, _atmosphereFramebuffer, nullptr);
//    }
}

/**
 * Initialize images, image views, framebuffer, render pass, material, etc. used by atmospheric scattering.
 * @Brief initialize atmospheric scattering resources
 * @param layoutCache
 * @param allocator
 */
void Atmosphere::create_resources(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, RenderPass& renderPass) {
    this->create_transmittance_resource(_device, _uploadContext, layoutCache, allocator);
    this->create_multiple_scattering_resource(_device, _uploadContext, layoutCache, allocator);
    this->create_skyview_resource(_device, _uploadContext, layoutCache, allocator);
    this->create_atmosphere_resource(_device, _uploadContext, layoutCache, allocator, renderPass); // render pass ?
}

/**
 * Pre-compute look-up tables (ie. transmittance) independent of external resources (such as camera data).
 * @note To be call at initialisation
 * @brief pre-compute look-up tables
 */
void Atmosphere::precompute_resources() {
    CommandPool commandPool = CommandPool(_device); // can use graphic queue for compute work
    CommandBuffer commandBuffer = CommandBuffer(_device, commandPool);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.waitSemaphoreCount = 0; // Semaphore to wait before executing the command buffers
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.signalSemaphoreCount = 0; // Number of semaphores to be signaled once the commands
    submitInfo.pSignalSemaphores = nullptr;
    submitInfo.commandBufferCount = 1; // Number of command buffers to execute in the batch
    submitInfo.pCommandBuffers = &commandBuffer._commandBuffer;

    VK_CHECK(vkQueueWaitIdle(_device.get_graphics_queue()));

    this->compute_transmittance(_device, _uploadContext, commandBuffer);

    VK_CHECK(vkQueueSubmit(_device.get_graphics_queue(), 1, &submitInfo, VK_NULL_HANDLE));
    VK_CHECK(vkQueueWaitIdle(_device.get_graphics_queue()));

    this->compute_multiple_scattering(_device, _uploadContext, commandBuffer);

    VK_CHECK(vkQueueSubmit(_device.get_graphics_queue(), 1, &submitInfo, VK_NULL_HANDLE));
    VK_CHECK(vkQueueWaitIdle(_device.get_graphics_queue()));
}

/**
 * Compute resources such (ie. skyview) affected by real time modification (ie. camera data)
 * @note This method must be call after dependencies buffer allocation.
 * @brief compute real-time resources
 */
void Atmosphere::compute_resources(uint32_t frameIndex) {
    glm::vec4 sun_direction = glm::vec4(0.0f, M_PI / 32.0, 0.0f, 0.0f);
    std::shared_ptr<Light> main = std::static_pointer_cast<Light>(_lightingManager.get_entity("sun"));
    if (main != nullptr) {
        sun_direction = main->get_position();
    }

    this->compute_skyview(_device, _uploadContext, frameIndex, sun_direction);
}

/**
 * todo - utiliser uniquement quand swapchain est detruit (ex. fenetre deformee), il ne faut pas tout recreer a chaque frame
 * Destroy real-times resources computed for the latest frame rendered.
 * @brief Destroy real-times resources
 * @note If not performed, the memory allocated will remain.
 */
void Atmosphere::destroy_resources() {
    // this->_skyviewLUT.destroy(_device);

    // vkDestroyFramebuffer(_device._logicalDevice, _skyviewFramebuffer, nullptr);
}

/**
 * Create resources (render pass, framebuffer, pipeline, etc.) used to compute the transmittance look-up table.
 * @brief Create transmittance LUT resources
 * @param device
 * @param uploadContext
 * @param layoutCache
 * @param allocator
 */
void Atmosphere::create_transmittance_resource(Device& device, UploadContext& uploadContext, DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator) {
    VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
    _transmittanceLUT._width = Atmosphere::TRANSMITTANCE_WIDTH;
    _transmittanceLUT._height = Atmosphere::TRANSMITTANCE_HEIGHT;

    VkExtent3D imageExtent { Atmosphere::TRANSMITTANCE_WIDTH, Atmosphere::TRANSMITTANCE_HEIGHT, 1 };
    VkImageCreateInfo imgInfo = vkinit::image_create_info(format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imgAllocinfo = {};
    imgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImage(device._allocator, &imgInfo, &imgAllocinfo, &_transmittanceLUT._image, &_transmittanceLUT._allocation, nullptr);

    VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(format, _transmittanceLUT._image, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCreateImageView(device._logicalDevice, &imageViewInfo, nullptr, &_transmittanceLUT._imageView);

    VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    vkCreateSampler(device._logicalDevice, &samplerInfo, nullptr, &_transmittanceLUT._sampler);

    CommandBuffer::immediate_submit(device, uploadContext, [&](VkCommandBuffer cmd) {
        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

        VkImageMemoryBarrier imageBarrier = {};
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageBarrier.image = _transmittanceLUT._image;
        imageBarrier.subresourceRange = range;
        imageBarrier.srcAccessMask = 0;
        imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

        // Change texture image layout to shader read after all mip levels have been copied
        _transmittanceLUT._imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        _transmittanceLUT.updateDescriptor();
    });


    // === Create compute pipeline ===

    // Descriptor set
    std::vector<VkDescriptorPoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}};
    DescriptorBuilder::begin(layoutCache, allocator) // reference texture image
            .bind_image(_transmittanceLUT._descriptor, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0)
            .layout(_transmittanceDescriptorLayout)
            .build(_transmittanceDescriptor, _transmittanceDescriptorLayout, poolSizes);

    // Build pipeline
    ComputePipeline pipelineBuilder = ComputePipeline(device);
    std::vector<std::pair<ShaderType, const char*>> comp_module {
        {ShaderType::COMPUTE, "../src/shaders/atmosphere/transmittance.comp.spv"},
    };

    _materialManager._pipelineBuilder = &pipelineBuilder;
    _transmittancePass = _materialManager.create_material("transmittance", {_transmittanceDescriptorLayout}, {}, comp_module);
}

/**
 * Create resources  (render pass, framebuffer, pipeline, etc.) used to compute the multiple scattering look-up table.
 * @brief Create multiple scattering LUT resources
 * @param device
 * @param uploadContext
 * @param layoutCache
 * @param allocator
 */
void Atmosphere::create_multiple_scattering_resource(Device& device, UploadContext& uploadContext, DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator) {
    VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
    _multipleScatteringLUT._width = Atmosphere::MULTISCATTERING_WIDTH;
    _multipleScatteringLUT._height = Atmosphere::MULTISCATTERING_HEIGHT;

    VkExtent3D imageExtent { Atmosphere::MULTISCATTERING_WIDTH, Atmosphere::MULTISCATTERING_HEIGHT, 1 };
    VkImageCreateInfo imgInfo = vkinit::image_create_info(format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT  | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imgAllocinfo = {};
    imgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImage(device._allocator, &imgInfo, &imgAllocinfo, &_multipleScatteringLUT._image, &_multipleScatteringLUT._allocation, nullptr);

    VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(format, _multipleScatteringLUT._image, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCreateImageView(device._logicalDevice, &imageViewInfo, nullptr, &_multipleScatteringLUT._imageView);

    VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    vkCreateSampler(device._logicalDevice, &samplerInfo, nullptr, &_multipleScatteringLUT._sampler);

    CommandBuffer::immediate_submit(device, uploadContext, [&](VkCommandBuffer cmd) {
        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

        VkImageMemoryBarrier imageBarrier = {};
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageBarrier.image = _multipleScatteringLUT._image;
        imageBarrier.subresourceRange = range;
        imageBarrier.srcAccessMask = 0;
        imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

        // Change texture image layout to shader read after all mip levels have been copied
        _multipleScatteringLUT._imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        _multipleScatteringLUT.updateDescriptor();
    });

    _multipleScatteringRenderPass = RenderPass(device);
    RenderPass::Attachment color = RenderPass::color(format);
    color.description.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    std::vector<VkAttachmentReference> references = {color.ref};
    std::vector<VkAttachmentDescription> attachments = {color.description};
    std::vector<VkSubpassDependency> dependencies = {color.dependency};
    std::vector<VkSubpassDescription> subpasses = {RenderPass::subpass_description(references, nullptr)};

    _multipleScatteringRenderPass.init(attachments, dependencies, subpasses);

    // Prepare framebuffer
     std::vector<VkImageView> imageviews = {_multipleScatteringLUT._imageView};
     _multipleScatteringFramebuffer = std::make_unique<FrameBuffer>(_multipleScatteringRenderPass, imageviews, Atmosphere::MULTISCATTERING_WIDTH, Atmosphere::MULTISCATTERING_HEIGHT, 1);

    DescriptorBuilder::begin(layoutCache, allocator) // reference texture image
            .bind_image(_transmittanceLUT._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0)
            .layout(_multipleScatteringDescriptorLayout)
            .build(_multipleScatteringDescriptor, _multipleScatteringDescriptorLayout, {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}});

    // Build pipeline
    GraphicPipeline pipelineBuilder = GraphicPipeline(device, _multipleScatteringRenderPass);
    pipelineBuilder._vertexInputInfo =  vkinit::vertex_input_state_create_info();

    std::vector<std::pair<ShaderType, const char*>> transmittance_module {
            {ShaderType::VERTEX, "../src/shaders/atmosphere/quad.vert.spv"},
            {ShaderType::FRAGMENT, "../src/shaders/atmosphere/multiple_scattering.frag.spv"},
    };

    _materialManager._pipelineBuilder = &pipelineBuilder;
    _multiScatteringPass = _materialManager.create_material("multiScattering", {_multipleScatteringDescriptorLayout}, {}, transmittance_module);

}

/**
 * Create resources  (render pass, framebuffer, pipeline, etc.) used to compute the downscaled sky view.
 * @brief Create skyview LUT resources
 * @param device
 * @param uploadContext
 * @param layoutCache
 * @param allocator
 */
void Atmosphere::create_skyview_resource(Device &device, UploadContext &uploadContext, DescriptorLayoutCache &layoutCache, DescriptorAllocator &allocator) {
    _skyviewDescriptor.reserve(FRAME_OVERLAP);

    VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
    _skyviewLUT._width = Atmosphere::SKYVIEW_WIDTH;
    _skyviewLUT._height = Atmosphere::SKYVIEW_HEIGHT;

    VkExtent3D imageExtent { Atmosphere::SKYVIEW_WIDTH, Atmosphere::SKYVIEW_HEIGHT, 1 };
    VkImageCreateInfo imgInfo = vkinit::image_create_info(format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT  | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);

    VmaAllocationCreateInfo imgAllocinfo = {};
    imgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImage(device._allocator, &imgInfo, &imgAllocinfo, &_skyviewLUT._image, &_skyviewLUT._allocation, nullptr);

    VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(format, _skyviewLUT._image, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCreateImageView(device._logicalDevice, &imageViewInfo, nullptr, &_skyviewLUT._imageView);

    VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    vkCreateSampler(device._logicalDevice, &samplerInfo, nullptr, &_skyviewLUT._sampler);

    CommandBuffer::immediate_submit(device, uploadContext, [&](VkCommandBuffer cmd) {
        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

        VkImageMemoryBarrier imageBarrier = {};
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageBarrier.image = _skyviewLUT._image;
        imageBarrier.subresourceRange = range;
        imageBarrier.srcAccessMask = 0;
        imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

        // Change texture image layout to shader read after all mip levels have been copied
        _skyviewLUT._imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        _skyviewLUT.updateDescriptor();
    });

    // === Prepare mapping ===
    _skyviewRenderPass = RenderPass(device);
    RenderPass::Attachment color = RenderPass::color(format);
    color.description.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    std::vector<VkAttachmentReference> references = {color.ref};
    std::vector<VkAttachmentDescription> attachments = {color.description};
    std::vector<VkSubpassDependency> dependencies = {color.dependency};
    std::vector<VkSubpassDescription> subpasses = {RenderPass::subpass_description(references, nullptr)};

    _skyviewRenderPass.init(attachments, dependencies, subpasses);

    // === Prepare framebuffer ===
    std::vector<VkImageView> imageviews = {_skyviewLUT._imageView};
    _skyviewFramebuffer = std::make_unique<FrameBuffer>(_skyviewRenderPass, imageviews, Atmosphere::SKYVIEW_WIDTH, Atmosphere::SKYVIEW_HEIGHT, 1);

    std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2},
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}
    };

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        _skyviewDescriptor.push_back(VkDescriptorSet());

        VkDescriptorBufferInfo camBInfo{};
        camBInfo.buffer = g_frames[i].cameraBuffer._buffer;
        camBInfo.offset = 0;
        camBInfo.range = sizeof(GPUCameraData);

        DescriptorBuilder::begin(layoutCache, allocator)
            .bind_image(_transmittanceLUT._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0)
            .bind_image(_multipleScatteringLUT._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
            .bind_buffer(camBInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2)
            .layout(_skyviewDescriptorLayout)
            .build(_skyviewDescriptor.at(i), _skyviewDescriptorLayout, poolSizes);
    }


    // Build pipeline
    GraphicPipeline pipelineBuilder = GraphicPipeline(device, _skyviewRenderPass);
    pipelineBuilder._vertexInputInfo =  vkinit::vertex_input_state_create_info();

    std::vector<PushConstant> constants {{sizeof(glm::vec4), ShaderType::FRAGMENT}};
    std::vector<std::pair<ShaderType, const char*>> skyview_module {
            {ShaderType::VERTEX, "../src/shaders/atmosphere/quad.vert.spv"},
            {ShaderType::FRAGMENT, "../src/shaders/atmosphere/skyview.frag.spv"},
    };

    _materialManager._pipelineBuilder = &pipelineBuilder;
    _skyviewPass = _materialManager.create_material("skyview", {_skyviewDescriptorLayout}, constants, skyview_module);
}

/**
 * Create resources (render pass, framebuffer, pipeline, etc.) used to render the atmosphere texture.
 * @brief Create atmosphere LUT resources
 * @param device
 * @param uploadContext
 * @param layoutCache
 * @param allocator
 */
void Atmosphere::create_atmosphere_resource(Device &device, UploadContext &uploadContext, DescriptorLayoutCache &layoutCache, DescriptorAllocator &allocator, RenderPass& renderPass) {
    // Atmosphere descriptor
    std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}
    };

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        g_frames[i].atmosphereDescriptor = VkDescriptorSet();

        VkDescriptorBufferInfo camBInfo{};
        camBInfo.buffer = g_frames[i].cameraBuffer._buffer;
        camBInfo.offset = 0;
        camBInfo.range = sizeof(GPUCameraData);

        DescriptorBuilder::begin(layoutCache, allocator)
                .bind_image(_transmittanceLUT._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0)
                .bind_image(_multipleScatteringLUT._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
                .bind_image(_skyviewLUT._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2)
                .bind_buffer(camBInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 3)
                .layout(_atmosphereDescriptorLayout)
                .build(g_frames[i].atmosphereDescriptor, _atmosphereDescriptorLayout, poolSizes);
    }

    std::vector<PushConstant> constants {{sizeof(glm::vec4), ShaderType::FRAGMENT}};
    std::vector<std::pair<ShaderType, const char*>> module {
            {ShaderType::VERTEX, "../src/shaders/atmosphere/quad.vert.spv"},
            {ShaderType::FRAGMENT, "../src/shaders/atmosphere/atmosphere.frag.spv"},
    };

    GraphicPipeline pipeline = GraphicPipeline(_device, renderPass);
    pipeline._vertexInputInfo =  vkinit::vertex_input_state_create_info();

    _materialManager._pipelineBuilder = &pipeline;
    _atmospherePass = _materialManager.create_material("atmosphere", {_atmosphereDescriptorLayout}, constants, module);
}

/**
 * Pre-compute the transmittance look-up table. It only need to be computed once at initialisation.
 * @brief Pre-compute transmittance LUT
 * @param device
 * @param uploadContext
 * @param commandBuffer
 */
void Atmosphere::compute_transmittance(Device& device, UploadContext& uploadContext, CommandBuffer& commandBuffer) {

    VkCommandBufferBeginInfo cmdBeginInfo{};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.pNext = nullptr;
    cmdBeginInfo.pInheritanceInfo = nullptr;
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(commandBuffer._commandBuffer, &cmdBeginInfo));
    {
        vkCmdBindPipeline(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _transmittancePass->pipeline);
        vkCmdBindDescriptorSets(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _transmittancePass->pipelineLayout, 0, 1, &_transmittanceDescriptor, 0, nullptr);
        vkCmdDispatch(commandBuffer._commandBuffer, 8, 2, 1);
    }
    VK_CHECK(vkEndCommandBuffer(commandBuffer._commandBuffer));

    CommandBuffer::immediate_submit(_device, _uploadContext, [&](VkCommandBuffer cmd) {
        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

        VkImageMemoryBarrier imageBarrier = {};
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageBarrier.image = _transmittanceLUT._image;
        imageBarrier.subresourceRange = range;
        imageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        imageBarrier.srcQueueFamilyIndex = _device.get_graphics_queue_family();
        imageBarrier.dstQueueFamilyIndex = _device.get_graphics_queue_family();
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

        _transmittanceLUT._imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        _transmittanceLUT.updateDescriptor();
    });

}

/**
 * Pre-compute the multiple scattering look-up table. It only need to be computed once at initialisation.
 * @brief Pre-compute multiple scattering LUT
 * @param device
 * @param uploadContext
 * @param commandBuffer
 */
void Atmosphere::compute_multiple_scattering(Device& device, UploadContext& uploadContext, CommandBuffer& commandBuffer) {
    VkExtent2D extent;
    extent.width = static_cast<uint32_t>(Atmosphere::MULTISCATTERING_WIDTH);
    extent.height = static_cast<uint32_t>(Atmosphere::MULTISCATTERING_HEIGHT);

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil.depth = 1.0f;

    VkRenderPassBeginInfo offscreenPassInfo = vkinit::renderpass_begin_info(_multipleScatteringRenderPass._renderPass, extent, _multipleScatteringFramebuffer->_frameBuffer);
    offscreenPassInfo.clearValueCount = clearValues.size();
    offscreenPassInfo.pClearValues = clearValues.data();

    VkCommandBufferBeginInfo cmdBeginInfo{};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.pNext = nullptr;
    cmdBeginInfo.pInheritanceInfo = nullptr;
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(commandBuffer._commandBuffer, &cmdBeginInfo));
    {
        vkCmdBeginRenderPass(commandBuffer._commandBuffer, &offscreenPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        {
            VkViewport viewport = vkinit::get_viewport((float) Atmosphere::MULTISCATTERING_WIDTH,(float) Atmosphere::MULTISCATTERING_HEIGHT);
            vkCmdSetViewport(commandBuffer._commandBuffer, 0, 1, &viewport);

            VkRect2D scissor = vkinit::get_scissor((float) Atmosphere::MULTISCATTERING_WIDTH,(float) Atmosphere::MULTISCATTERING_HEIGHT);
            vkCmdSetScissor(commandBuffer._commandBuffer, 0, 1, &scissor);

            vkCmdBindDescriptorSets(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _multiScatteringPass->pipelineLayout, 0, 1, &_multipleScatteringDescriptor, 0, nullptr);
            vkCmdBindPipeline(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _multiScatteringPass->pipeline);
            vkCmdDraw(commandBuffer._commandBuffer, 3, 1, 0, 0);
        }
        vkCmdEndRenderPass(commandBuffer._commandBuffer);
    }
    VK_CHECK(vkEndCommandBuffer(commandBuffer._commandBuffer));

    CommandBuffer::immediate_submit(device, uploadContext, [&](VkCommandBuffer cmd) {
        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

        VkImageMemoryBarrier imageBarrier = {};
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageBarrier.image = _multipleScatteringLUT._image;
        imageBarrier.subresourceRange = range;
        imageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
    });
}

/**
 * Compute the skyview look-up table. Must be computed in real-time.
 * @brief Compute skyview LUT
 * @param device
 * @param uploadContext
 * @param commandBuffer
 */
void Atmosphere::compute_skyview(Device& device, UploadContext& uploadContext, uint32_t frameIndex, glm::vec4 sun_direction) {
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil.depth = 1.0f;

    {
        CommandPool commandPool = CommandPool(device); // can use graphic queue for compute work
        CommandBuffer commandBuffer = CommandBuffer(device, commandPool);

        VkExtent2D extent;
        extent.width = static_cast<uint32_t>(Atmosphere::SKYVIEW_WIDTH);
        extent.height = static_cast<uint32_t>(Atmosphere::SKYVIEW_HEIGHT);

        VkRenderPassBeginInfo offscreenPassInfo = vkinit::renderpass_begin_info(_skyviewRenderPass._renderPass, extent, _skyviewFramebuffer->_frameBuffer);
        offscreenPassInfo.clearValueCount = clearValues.size();
        offscreenPassInfo.pClearValues = clearValues.data();

        VkCommandBufferBeginInfo cmdBeginInfo{};
        cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBeginInfo.pNext = nullptr;
        cmdBeginInfo.pInheritanceInfo = nullptr;
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(commandBuffer._commandBuffer, &cmdBeginInfo));
        {
            vkCmdBeginRenderPass(commandBuffer._commandBuffer, &offscreenPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            {
                VkViewport viewport = vkinit::get_viewport((float) Atmosphere::SKYVIEW_WIDTH,(float) Atmosphere::SKYVIEW_HEIGHT);
                vkCmdSetViewport(commandBuffer._commandBuffer, 0, 1, &viewport);

                VkRect2D scissor = vkinit::get_scissor((float) Atmosphere::SKYVIEW_WIDTH,(float) Atmosphere::SKYVIEW_HEIGHT);
                vkCmdSetScissor(commandBuffer._commandBuffer, 0, 1, &scissor);

                vkCmdBindDescriptorSets(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _skyviewPass->pipelineLayout, 0, 1, &_skyviewDescriptor.at(frameIndex), 0, nullptr);
                vkCmdBindPipeline(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _skyviewPass->pipeline);
                vkCmdPushConstants(commandBuffer._commandBuffer, _skyviewPass->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::vec4), &sun_direction);
                vkCmdDraw(commandBuffer._commandBuffer, 3, 1, 0, 0);
            }
            vkCmdEndRenderPass(commandBuffer._commandBuffer);
        }
        VK_CHECK(vkEndCommandBuffer(commandBuffer._commandBuffer));

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.pWaitDstStageMask = nullptr;
        submitInfo.waitSemaphoreCount = 0; // Semaphore to wait before executing the command buffers
        submitInfo.pWaitSemaphores = nullptr;
        submitInfo.signalSemaphoreCount = 0; // Number of semaphores to be signaled once the commands
        submitInfo.pSignalSemaphores = nullptr;
        submitInfo.commandBufferCount = 1; // Number of command buffers to execute in the batch
        submitInfo.pCommandBuffers = &commandBuffer._commandBuffer;

        VK_CHECK(vkQueueSubmit(device.get_graphics_queue(), 1, &submitInfo, VK_NULL_HANDLE));
        VK_CHECK(vkQueueWaitIdle(device.get_graphics_queue()));
    }

}

/**
 * Draw atmosphere background. Must be computed in real-time.
 * @brief Render atmosphere
 * @param commandBuffer
 * @param descriptor
 * @param framebuffer
 */
void Atmosphere::draw(VkCommandBuffer& cmd, VkDescriptorSet* descriptor) {

    glm::vec4 sun_direction = glm::vec4(0.0f, M_PI / 32.0, 0.0f, 0.0f);
    std::shared_ptr<Light> main = std::static_pointer_cast<Light>(_lightingManager.get_entity("sun"));
    if (main != nullptr) {
        sun_direction = main->get_position();
    }

    // warning : push constants can be written to a share command buffer,
    // but they are stateful. So must be careful to overwrite them properly for each pass.
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_atmospherePass->pipelineLayout, 0, 1, descriptor, 0, nullptr);
    vkCmdPushConstants(cmd, this->_atmospherePass->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::vec4), &sun_direction);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,this->_atmospherePass->pipeline);
    vkCmdDraw(cmd, 3, 1, 0, 0);
}