#include "vk_atmosphere.h"
#include "core/utilities/vk_initializers.h"
#include "core/vk_descriptor_allocator.h"
#include "core/vk_descriptor_cache.h"
#include "core/vk_descriptor_builder.h"
#include "core/vk_pipeline.h"
#include "core/vk_renderpass.h"
#include "core/vk_texture.h"
#include "core/manager/vk_material_manager.h"
#include "core/vk_command_pool.h"
#include "core/vk_command_buffer.h"
#include "core/utilities/vk_global.h"

#include <chrono>
#include <iostream>

Atmosphere::Atmosphere(Device &device, UploadContext& uploadContext) : _device(device), _uploadContext(uploadContext) {
    this->_transmittanceLUT = this->compute_transmittance(_device, _uploadContext);
    // this->_multipleScatteringLUT = this->compute_multiple_scattering(_device, _uploadContext);
    this->_multipleScatteringLUT = this->compute_multiple_scattering_2(_device, _uploadContext);
    this->_skyviewLUT = this->compute_skyview(_device, _uploadContext);
    this->_atmosphereLUT = this->render_atmosphere(_device, _uploadContext);
}

Atmosphere::~Atmosphere() {
    this->_transmittanceLUT.destroy(_device);
    this->_multipleScatteringLUT.destroy(_device);
    this->_skyviewLUT.destroy(_device);
    this->_atmosphereLUT.destroy(_device);

    vkDestroyFramebuffer(_device._logicalDevice, _multipleScatteringFramebuffer, nullptr);
    vkDestroyFramebuffer(_device._logicalDevice, _skyviewFramebuffer, nullptr);
    vkDestroyFramebuffer(_device._logicalDevice, _atmosphereFramebuffer, nullptr);

}

Texture Atmosphere::compute_transmittance(Device& device, UploadContext& uploadContext) {
    Texture outTexture{};
    VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
    outTexture._width = Atmosphere::TRANSMITTANCE_WIDTH;
    outTexture._height = Atmosphere::TRANSMITTANCE_HEIGHT;

    VkExtent3D imageExtent { Atmosphere::TRANSMITTANCE_WIDTH, Atmosphere::TRANSMITTANCE_HEIGHT, 1 };
    VkImageCreateInfo imgInfo = vkinit::image_create_info(format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imgAllocinfo = {};
    imgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImage(device._allocator, &imgInfo, &imgAllocinfo, &outTexture._image, &outTexture._allocation, nullptr);

    VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(format, outTexture._image, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCreateImageView(device._logicalDevice, &imageViewInfo, nullptr, &outTexture._imageView);

    VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    vkCreateSampler(device._logicalDevice, &samplerInfo, nullptr, &outTexture._sampler);

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
        imageBarrier.image = outTexture._image;
        imageBarrier.subresourceRange = range;
        imageBarrier.srcAccessMask = 0;
        imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

        // Change texture image layout to shader read after all mip levels have been copied
        outTexture._imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        outTexture.updateDescriptor();
    });

    // === Create compute pipeline ===

    // Descriptor set
    std::vector<VkDescriptorPoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}};
    VkDescriptorSet descriptor{};
    VkDescriptorSetLayout setLayout{};
    DescriptorLayoutCache layoutCache = DescriptorLayoutCache(device);
    DescriptorAllocator allocator = DescriptorAllocator(device);
    DescriptorBuilder::begin(layoutCache, allocator) // reference texture image
            .bind_image(outTexture._descriptor, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0)
            .layout(setLayout)
            .build(descriptor, setLayout, poolSizes);

    // Build pipeline
    ComputePipeline pipelineBuilder = ComputePipeline(device);

    std::vector<std::pair<ShaderType, const char*>> comp_module {
            {ShaderType::COMPUTE, "../src/shaders/atmosphere/transmittance.comp.spv"},
    };

    MaterialManager materialManager = MaterialManager(&device, &pipelineBuilder);
    std::shared_ptr<ShaderPass> transmittancePass = materialManager.create_material("transmittance", {setLayout}, {}, comp_module);

    {
        CommandPool commandPool = CommandPool(device); // can use graphic queue for compute work
        CommandBuffer commandBuffer = CommandBuffer(device, commandPool);

        VkCommandBufferBeginInfo cmdBeginInfo{};
        cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBeginInfo.pNext = nullptr;
        cmdBeginInfo.pInheritanceInfo = nullptr;
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkQueueWaitIdle(device.get_graphics_queue()));

        VK_CHECK(vkBeginCommandBuffer(commandBuffer._commandBuffer, &cmdBeginInfo));
        {
            vkCmdBindPipeline(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, transmittancePass->pipeline);
            vkCmdBindDescriptorSets(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, transmittancePass->pipelineLayout, 0,1, &descriptor, 0, nullptr);
            vkCmdDispatch(commandBuffer._commandBuffer,   8, 2, 1);
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
        imageBarrier.image = outTexture._image;
        imageBarrier.subresourceRange = range;
        imageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
    });

    return outTexture;
}

Texture Atmosphere::compute_multiple_scattering(Device &device, UploadContext &uploadContext) {
    Texture outTexture{};
    VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
    outTexture._width = Atmosphere::MULTISCATTERING_WIDTH;
    outTexture._height = Atmosphere::MULTISCATTERING_HEIGHT;

    VkExtent3D imageExtent { Atmosphere::MULTISCATTERING_WIDTH, Atmosphere::MULTISCATTERING_HEIGHT, 1 };
    VkImageCreateInfo imgInfo = vkinit::image_create_info(format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imgAllocinfo = {};
    imgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImage(device._allocator, &imgInfo, &imgAllocinfo, &outTexture._image, &outTexture._allocation, nullptr);

    VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(format, outTexture._image, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCreateImageView(device._logicalDevice, &imageViewInfo, nullptr, &outTexture._imageView);

    VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    vkCreateSampler(device._logicalDevice, &samplerInfo, nullptr, &outTexture._sampler);

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
        imageBarrier.image = outTexture._image;
        imageBarrier.subresourceRange = range;
        imageBarrier.srcAccessMask = 0;
        imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

        // Change texture image layout to shader read after all mip levels have been copied
        outTexture._imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        outTexture.updateDescriptor();
    });

    // === Create compute pipeline ===

    // Descriptor set
    std::vector<VkDescriptorPoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}, {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2}};
    VkDescriptorSet descriptor{};
    VkDescriptorSetLayout setLayout{};
    DescriptorLayoutCache layoutCache = DescriptorLayoutCache(device);
    DescriptorAllocator allocator = DescriptorAllocator(device);
    DescriptorBuilder::begin(layoutCache, allocator) // reference texture image
            .bind_image(_transmittanceLUT._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT, 0)
            .bind_image(outTexture._descriptor, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1)
            .layout(setLayout)
            .build(descriptor, setLayout, poolSizes);

    // Build pipeline
    ComputePipeline pipelineBuilder = ComputePipeline(device);

    std::vector<std::pair<ShaderType, const char*>> comp_module {
            {ShaderType::COMPUTE, "../src/shaders/atmosphere/multiple_scattering.comp.spv"},
    };

    MaterialManager materialManager = MaterialManager(&device, &pipelineBuilder);
    std::shared_ptr<ShaderPass> multiScatteringPass = materialManager.create_material("multipleScattering", {setLayout}, {}, comp_module);


    {
        CommandPool commandPool = CommandPool(device); // can use graphic queue for compute work
        CommandBuffer commandBuffer = CommandBuffer(device, commandPool);

        VkCommandBufferBeginInfo cmdBeginInfo{};
        cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBeginInfo.pNext = nullptr;
        cmdBeginInfo.pInheritanceInfo = nullptr;
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkQueueWaitIdle(device.get_graphics_queue()));

        VK_CHECK(vkBeginCommandBuffer(commandBuffer._commandBuffer, &cmdBeginInfo));
        {
            vkCmdBindPipeline(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, multiScatteringPass->pipeline);
            vkCmdBindDescriptorSets(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, multiScatteringPass->pipelineLayout, 0,1, &descriptor, 0, nullptr);
            vkCmdDispatch(commandBuffer._commandBuffer,   1, 1, 1);
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
        imageBarrier.image = outTexture._image;
        imageBarrier.subresourceRange = range;
        imageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
    });

    return outTexture;
}

Texture Atmosphere::compute_multiple_scattering_2(Device& device, UploadContext& uploadContext) {
    Texture outTexture{};
    VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
    outTexture._width = Atmosphere::MULTISCATTERING_WIDTH;
    outTexture._height = Atmosphere::MULTISCATTERING_HEIGHT;

    VkExtent3D imageExtent { Atmosphere::MULTISCATTERING_WIDTH, Atmosphere::MULTISCATTERING_HEIGHT, 1 };
    VkImageCreateInfo imgInfo = vkinit::image_create_info(format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT  | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imgAllocinfo = {};
    imgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImage(device._allocator, &imgInfo, &imgAllocinfo, &outTexture._image, &outTexture._allocation, nullptr);

    VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(format, outTexture._image, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCreateImageView(device._logicalDevice, &imageViewInfo, nullptr, &outTexture._imageView);

    VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    vkCreateSampler(device._logicalDevice, &samplerInfo, nullptr, &outTexture._sampler);

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
        imageBarrier.image = outTexture._image;
        imageBarrier.subresourceRange = range;
        imageBarrier.srcAccessMask = 0;
        imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

        // Change texture image layout to shader read after all mip levels have been copied
        outTexture._imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        outTexture.updateDescriptor();
    });

    // === Prepare mapping ===
    RenderPass renderPass = RenderPass(device);
    RenderPass::Attachment color = renderPass.color(format);
    color.description.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    std::vector<VkAttachmentReference> references = {color.ref};
    std::vector<VkAttachmentDescription> attachments = {color.description};
    std::vector<VkSubpassDependency> dependencies = {color.dependency};
    VkSubpassDescription subpass = renderPass.subpass_description(references, nullptr);

    renderPass.init(attachments, dependencies, subpass);

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil.depth = 1.0f;

    // Prepare framebuffer
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext = nullptr;
    framebufferInfo.renderPass = renderPass._renderPass;
    framebufferInfo.width = Atmosphere::MULTISCATTERING_WIDTH;
    framebufferInfo.height = Atmosphere::MULTISCATTERING_HEIGHT;
    framebufferInfo.layers = 1;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &outTexture._imageView;
    VK_CHECK(vkCreateFramebuffer(device._logicalDevice, &framebufferInfo, nullptr, &_multipleScatteringFramebuffer));

    std::vector<VkDescriptorPoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}};

    VkDescriptorSet descriptor;
    VkDescriptorSetLayout setLayout;
    DescriptorLayoutCache layoutCache = DescriptorLayoutCache(device);
    DescriptorAllocator allocator = DescriptorAllocator(device);

    DescriptorBuilder::begin(layoutCache, allocator) // reference texture image
            .bind_image(_transmittanceLUT._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0)
            .layout(setLayout)
            .build(descriptor, setLayout, poolSizes);

    // Build pipeline
    GraphicPipeline pipelineBuilder = GraphicPipeline(device, renderPass);

    std::vector<std::pair<ShaderType, const char*>> transmittance_module {
            {ShaderType::VERTEX, "../src/shaders/atmosphere/quad.vert.spv"},
            {ShaderType::FRAGMENT, "../src/shaders/atmosphere/multiple_scattering.frag.spv"},
    };

    MaterialManager materialManager = MaterialManager(&device, &pipelineBuilder);
    std::shared_ptr<ShaderPass> multiScatteringPass = materialManager.create_material("multiScattering", {setLayout}, {}, transmittance_module);

    {
        CommandPool commandPool = CommandPool(device); // can use graphic queue for compute work
        CommandBuffer commandBuffer = CommandBuffer(device, commandPool);

        VkExtent2D extent;
        extent.width = static_cast<uint32_t>(Atmosphere::MULTISCATTERING_WIDTH);
        extent.height = static_cast<uint32_t>(Atmosphere::MULTISCATTERING_HEIGHT);

        VkRenderPassBeginInfo offscreenPassInfo = vkinit::renderpass_begin_info(renderPass._renderPass, extent,
                                                                                _multipleScatteringFramebuffer);
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

                vkCmdBindDescriptorSets(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, multiScatteringPass->pipelineLayout, 0, 1, &descriptor, 0, nullptr);
                vkCmdBindPipeline(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,multiScatteringPass->pipeline);
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

    return outTexture;
}

Texture Atmosphere::compute_skyview(Device& device, UploadContext& uploadContext) {
    Texture outTexture{};
    VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
    outTexture._width = Atmosphere::SKYVIEW_WIDTH;
    outTexture._height = Atmosphere::SKYVIEW_HEIGHT;

    VkExtent3D imageExtent { Atmosphere::SKYVIEW_WIDTH, Atmosphere::SKYVIEW_HEIGHT, 1 };
    VkImageCreateInfo imgInfo = vkinit::image_create_info(format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT  | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imgAllocinfo = {};
    imgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImage(device._allocator, &imgInfo, &imgAllocinfo, &outTexture._image, &outTexture._allocation, nullptr);

    VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(format, outTexture._image, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCreateImageView(device._logicalDevice, &imageViewInfo, nullptr, &outTexture._imageView);

    VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    vkCreateSampler(device._logicalDevice, &samplerInfo, nullptr, &outTexture._sampler);

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
        imageBarrier.image = outTexture._image;
        imageBarrier.subresourceRange = range;
        imageBarrier.srcAccessMask = 0;
        imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

        // Change texture image layout to shader read after all mip levels have been copied
        outTexture._imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        outTexture.updateDescriptor();
    });

    // === Prepare mapping ===
    RenderPass renderPass = RenderPass(device);
    RenderPass::Attachment color = renderPass.color(format);
    color.description.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    std::vector<VkAttachmentReference> references = {color.ref};
    std::vector<VkAttachmentDescription> attachments = {color.description};
    std::vector<VkSubpassDependency> dependencies = {color.dependency};
    VkSubpassDescription subpass = renderPass.subpass_description(references, nullptr);

    renderPass.init(attachments, dependencies, subpass);

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil.depth = 1.0f;

    // Prepare framebuffer
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext = nullptr;
    framebufferInfo.renderPass = renderPass._renderPass;
    framebufferInfo.width = Atmosphere::SKYVIEW_WIDTH;
    framebufferInfo.height = Atmosphere::SKYVIEW_HEIGHT;
    framebufferInfo.layers = 1;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &outTexture._imageView;
    VK_CHECK(vkCreateFramebuffer(device._logicalDevice, &framebufferInfo, nullptr, &_skyviewFramebuffer));

    std::vector<VkDescriptorPoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2}};

    VkDescriptorSet descriptor;
    VkDescriptorSetLayout setLayout;
    DescriptorLayoutCache layoutCache = DescriptorLayoutCache(device);
    DescriptorAllocator allocator = DescriptorAllocator(device);

    DescriptorBuilder::begin(layoutCache, allocator) // reference texture image
            .bind_image(_transmittanceLUT._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0)
            .bind_image(_multipleScatteringLUT._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
            .layout(setLayout)
            .build(descriptor, setLayout, poolSizes);

    // Build pipeline
    GraphicPipeline pipelineBuilder = GraphicPipeline(device, renderPass);

    std::vector<std::pair<ShaderType, const char*>> skyview_module {
            {ShaderType::VERTEX, "../src/shaders/atmosphere/quad.vert.spv"},
            {ShaderType::FRAGMENT, "../src/shaders/atmosphere/skyview.frag.spv"},
    };

    MaterialManager materialManager = MaterialManager(&device, &pipelineBuilder);
    std::shared_ptr<ShaderPass> skyviewPass = materialManager.create_material("skyview", {setLayout}, {}, skyview_module);

    {
        CommandPool commandPool = CommandPool(device); // can use graphic queue for compute work
        CommandBuffer commandBuffer = CommandBuffer(device, commandPool);

        VkExtent2D extent;
        extent.width = static_cast<uint32_t>(Atmosphere::SKYVIEW_WIDTH);
        extent.height = static_cast<uint32_t>(Atmosphere::SKYVIEW_HEIGHT);

        VkRenderPassBeginInfo offscreenPassInfo = vkinit::renderpass_begin_info(renderPass._renderPass, extent,_skyviewFramebuffer);
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

                vkCmdBindDescriptorSets(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyviewPass->pipelineLayout, 0, 1, &descriptor, 0, nullptr);
                vkCmdBindPipeline(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,skyviewPass->pipeline);
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

    return outTexture;
}

Texture Atmosphere::render_atmosphere(Device &device, UploadContext &uploadContext) {
    Texture outTexture{};
    VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
    outTexture._width = Atmosphere::ATMOSPHERE_WIDTH;
    outTexture._height = Atmosphere::ATMOSPHERE_HEIGHT;

    VkExtent3D imageExtent { Atmosphere::ATMOSPHERE_WIDTH, Atmosphere::ATMOSPHERE_HEIGHT, 1 };
    VkImageCreateInfo imgInfo = vkinit::image_create_info(format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT  | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imgAllocinfo = {};
    imgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImage(device._allocator, &imgInfo, &imgAllocinfo, &outTexture._image, &outTexture._allocation, nullptr);

    VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(format, outTexture._image, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCreateImageView(device._logicalDevice, &imageViewInfo, nullptr, &outTexture._imageView);

    VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    vkCreateSampler(device._logicalDevice, &samplerInfo, nullptr, &outTexture._sampler);

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
        imageBarrier.image = outTexture._image;
        imageBarrier.subresourceRange = range;
        imageBarrier.srcAccessMask = 0;
        imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

        // Change texture image layout to shader read after all mip levels have been copied
        outTexture._imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        outTexture.updateDescriptor();
    });


    // === Prepare mapping ===
    RenderPass renderPass = RenderPass(device);
    RenderPass::Attachment color = renderPass.color(format);
    color.description.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    std::vector<VkAttachmentReference> references = {color.ref};
    std::vector<VkAttachmentDescription> attachments = {color.description};
    std::vector<VkSubpassDependency> dependencies = {color.dependency};
    VkSubpassDescription subpass = renderPass.subpass_description(references, nullptr);

    renderPass.init(attachments, dependencies, subpass);

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil.depth = 1.0f;

    // Prepare framebuffer
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext = nullptr;
    framebufferInfo.renderPass = renderPass._renderPass;
    framebufferInfo.width = Atmosphere::ATMOSPHERE_WIDTH;
    framebufferInfo.height = Atmosphere::ATMOSPHERE_HEIGHT;
    framebufferInfo.layers = 1;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &outTexture._imageView;
    VK_CHECK(vkCreateFramebuffer(device._logicalDevice, &framebufferInfo, nullptr, &_atmosphereFramebuffer));

    std::vector<VkDescriptorPoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3}};

    VkDescriptorSet descriptor;
    VkDescriptorSetLayout setLayout;
    DescriptorLayoutCache layoutCache = DescriptorLayoutCache(device);
    DescriptorAllocator allocator = DescriptorAllocator(device);

    DescriptorBuilder::begin(layoutCache, allocator) // reference texture image
            .bind_image(_transmittanceLUT._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0)
            .bind_image(_multipleScatteringLUT._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
            .bind_image(_skyviewLUT._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2)
            .layout(setLayout)
            .build(descriptor, setLayout, poolSizes);

    // Build pipeline
    GraphicPipeline pipelineBuilder = GraphicPipeline(device, renderPass);

    std::vector<std::pair<ShaderType, const char*>> atmosphere_module {
            {ShaderType::VERTEX, "../src/shaders/atmosphere/quad.vert.spv"},
            {ShaderType::FRAGMENT, "../src/shaders/atmosphere/atmosphere.frag.spv"},
    };

    MaterialManager materialManager = MaterialManager(&device, &pipelineBuilder);
    std::shared_ptr<ShaderPass> atmospherePass = materialManager.create_material("atmosphere", {setLayout}, {}, atmosphere_module);

    {
        CommandPool commandPool = CommandPool(device);
        CommandBuffer commandBuffer = CommandBuffer(device, commandPool);

        VkExtent2D extent;
        extent.width = static_cast<uint32_t>(Atmosphere::ATMOSPHERE_WIDTH);
        extent.height = static_cast<uint32_t>(Atmosphere::ATMOSPHERE_HEIGHT);

        VkRenderPassBeginInfo offscreenPassInfo = vkinit::renderpass_begin_info(renderPass._renderPass, extent,_atmosphereFramebuffer);
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
                VkViewport viewport = vkinit::get_viewport((float) Atmosphere::ATMOSPHERE_WIDTH,(float) Atmosphere::ATMOSPHERE_HEIGHT);
                vkCmdSetViewport(commandBuffer._commandBuffer, 0, 1, &viewport);

                VkRect2D scissor = vkinit::get_scissor((float) Atmosphere::ATMOSPHERE_WIDTH,(float) Atmosphere::ATMOSPHERE_HEIGHT);
                vkCmdSetScissor(commandBuffer._commandBuffer, 0, 1, &scissor);

                vkCmdBindDescriptorSets(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, atmospherePass->pipelineLayout, 0, 1, &descriptor, 0, nullptr);
                vkCmdBindPipeline(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,atmospherePass->pipeline);
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

    return outTexture;
}



void Atmosphere::run_debug(FrameData& frame) {
//    vkCmdBindDescriptorSets(frame._commandBuffer->_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_debug_effect->pipelineLayout, 0, 1, &frame.debugDescriptor, 0, nullptr);
//    vkCmdBindPipeline(frame._commandBuffer->_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_debug_effect->pipeline);
//    vkCmdDraw(frame._commandBuffer->_commandBuffer, 3, 1, 0, 0);
}