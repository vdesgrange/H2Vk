/*
*  H2Vk - Environment mapping
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_env_map.h"
#include "core/vk_device.h"
#include "core/vk_renderpass.h"
#include "core/vk_pipeline.h"
#include "core/vk_command_pool.h"
#include "core/vk_command_buffer.h"
#include "core/vk_descriptor_allocator.h"
#include "core/vk_descriptor_cache.h"
#include "core/vk_descriptor_builder.h"
#include "core/vk_texture.h"
#include "core/vk_framebuffers.h"
#include "core/manager/vk_mesh_manager.h"
#include "core/manager/vk_material_manager.h"
#include "core/utilities/vk_helpers.h"
#include "core/utilities/vk_initializers.h"
#include "components/model/vk_poly.h"

Texture EnvMap::cube_map_converter(Device& device, UploadContext& uploadContext, MeshManager& meshManager, Texture& inTexture) {
    uint32_t count = 6;
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

    // === Prepare texture target ===
    Texture outTexture{};
    outTexture._width = ENV_WIDTH;
    outTexture._height = ENV_HEIGHT;

    VkExtent3D imageExtent;
    imageExtent.width = ENV_WIDTH;
    imageExtent.height = ENV_HEIGHT;
    imageExtent.depth = 1;

    VkImageCreateInfo imgInfo = vkinit::image_create_info(format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
    imgInfo.arrayLayers = count;
    imgInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imgAllocinfo = {};
    imgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImage(device._allocator, &imgInfo, &imgAllocinfo, &outTexture._image, &outTexture._allocation, nullptr);

    VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(format, outTexture._image, VK_IMAGE_ASPECT_COLOR_BIT);
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    imageViewInfo.subresourceRange.layerCount = count;
    vkCreateImageView(device._logicalDevice, &imageViewInfo, nullptr, &outTexture._imageView);

    VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    vkCreateSampler(device._logicalDevice, &samplerInfo, nullptr, &outTexture._sampler);

    CommandBuffer::immediate_submit(device, uploadContext, [&](VkCommandBuffer cmd) {
        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 6;

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
    RenderPass::Attachment color = RenderPass::color(format);
    color.description.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    std::vector<VkAttachmentReference> references = {color.ref};
    std::vector<VkAttachmentDescription> attachments = {color.description};
    std::vector<VkSubpassDependency> dependencies = {color.dependency};
    std::vector<VkSubpassDescription> subpasses = {RenderPass::subpass_description(references, nullptr)};
    
    renderPass.init(attachments, dependencies, subpasses);

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil.depth = 1.0f;

    std::array<VkImageView, 6> imagesViews {};
    std::vector<FrameBuffer> framebuffers;
    framebuffers.reserve(count);

    for (int face = 0; face < count; face++) {
        // Create image view
        VkImageViewCreateInfo info = vkinit::imageview_create_info(format, outTexture._image, VK_IMAGE_ASPECT_COLOR_BIT);
        info.subresourceRange.baseArrayLayer = face; // cube map face in the out texture.
        info.subresourceRange.layerCount = 1;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        vkCreateImageView(device._logicalDevice, &info, nullptr, &imagesViews[face]);

        std::vector<VkImageView> attachments = {imagesViews[face]};
        framebuffers.emplace_back(FrameBuffer(renderPass, attachments, ENV_WIDTH, ENV_HEIGHT, 1));
    }

    std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
    };

    VkDescriptorSet descriptor;
    VkDescriptorSetLayout setLayout;
    DescriptorLayoutCache layoutCache = DescriptorLayoutCache(device);
    DescriptorAllocator allocator = DescriptorAllocator(device);
    DescriptorBuilder::begin(layoutCache, allocator) // reference texture image
        .bind_image(inTexture._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0)
        .layout(setLayout)
        .build(descriptor, setLayout, poolSizes);

    // Build pipeline
    GraphicPipeline pipelineBuilder = GraphicPipeline(device, renderPass);

    std::vector<std::pair<ShaderType, const char*>> modules {
            {ShaderType::VERTEX, "../src/shaders/env_map/cube_map.vert.spv"},
            {ShaderType::FRAGMENT, "../src/shaders/env_map/converter.frag.spv"},
    };

    std::vector<PushConstant> constants {
            {2 * sizeof(glm::mat4), ShaderType::VERTEX},
    };

    MaterialManager materialManager = MaterialManager(&device, &pipelineBuilder);
    std::shared_ptr<ShaderPass> converterPass = materialManager.create_material("converter", {setLayout}, constants, modules);

    glm::mat4 projection = glm::perspective(glm::radians(90.0f), ENV_WIDTH / static_cast<float>(ENV_HEIGHT),  0.1f, 10.0f);
    std::array<glm::mat4, 6> views = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    };

    std::shared_ptr<Model> cube = ModelPOLY::create_cube(&device, uploadContext, {-1.0f, -1.0f, -1.0f},  {1.0f, 1.0f, 1.0f});
    meshManager.upload_mesh(*cube);

    for (int face = 0; face < count; face++) {
        CommandPool commandPool = CommandPool(device);
        CommandBuffer commandBuffer =  CommandBuffer(device, commandPool);

        VkExtent2D extent;
        extent.width = static_cast<uint32_t>(ENV_WIDTH);
        extent.height = static_cast<uint32_t>(ENV_HEIGHT);

        VkRenderPassBeginInfo renderPassInfo = vkinit::renderpass_begin_info(renderPass._renderPass, extent, framebuffers.at(face)._frameBuffer);
        renderPassInfo.clearValueCount = clearValues.size();
        renderPassInfo.pClearValues = clearValues.data();

        VkCommandBufferBeginInfo cmdBeginInfo{};
        cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBeginInfo.pNext = nullptr;
        cmdBeginInfo.pInheritanceInfo = nullptr;
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        glm::mat4 viewProj = projection * views[face];

        VK_CHECK(vkBeginCommandBuffer(commandBuffer._commandBuffer, &cmdBeginInfo));
        {
            vkCmdBeginRenderPass(commandBuffer._commandBuffer, &renderPassInfo,VK_SUBPASS_CONTENTS_INLINE);
            {
                VkViewport viewport = vkinit::get_viewport(static_cast<float>(ENV_WIDTH),static_cast<float>(ENV_HEIGHT));
                vkCmdSetViewport(commandBuffer._commandBuffer, 0, 1, &viewport);

                VkRect2D scissor = vkinit::get_scissor(static_cast<float>(inTexture._width), static_cast<float>(inTexture._height));
                vkCmdSetScissor(commandBuffer._commandBuffer, 0, 1, &scissor);

                vkCmdBindPipeline(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, converterPass->pipeline);
                vkCmdPushConstants(commandBuffer._commandBuffer, converterPass->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), sizeof(glm::mat4), &viewProj);
                vkCmdBindDescriptorSets(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, converterPass->pipelineLayout, 0,1, &descriptor, 0, nullptr);
                cube->draw(commandBuffer._commandBuffer, converterPass->pipelineLayout, sizeof(glm::mat4), 0, true);
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

        device._queue->queue_submit({submitInfo}, VK_NULL_HANDLE);
        device._queue->queue_wait();
    }

    for (int face = 0; face < count; face++) {
        vkDestroyImageView(device._logicalDevice, imagesViews[face], nullptr);
    }

    return outTexture;
}

Texture EnvMap::irradiance_cube_mapping(Device& device, UploadContext& uploadContext, MeshManager& meshManager, Texture& inTexture) {
    uint32_t count = 6;
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

    // === Prepare texture target ===
    Texture outTexture{};
    outTexture._width = CONVOLVE_WIDTH;
    outTexture._height = CONVOLVE_HEIGHT;

    VkExtent3D imageExtent;
    imageExtent.width = CONVOLVE_WIDTH;
    imageExtent.height = CONVOLVE_HEIGHT;
    imageExtent.depth = 1;

    VkImageCreateInfo imgInfo = vkinit::image_create_info(format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
    imgInfo.arrayLayers = count;
    imgInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imgAllocinfo = {};
    imgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImage(device._allocator, &imgInfo, &imgAllocinfo, &outTexture._image, &outTexture._allocation, nullptr);

    // Create image view
    VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(format, outTexture._image, VK_IMAGE_ASPECT_COLOR_BIT);
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    imageViewInfo.subresourceRange.layerCount = count;
    vkCreateImageView(device._logicalDevice, &imageViewInfo, nullptr, &outTexture._imageView);

    // Create sampler
    VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    vkCreateSampler(device._logicalDevice, &samplerInfo, nullptr, &outTexture._sampler);

    CommandBuffer::immediate_submit(device, uploadContext, [&](VkCommandBuffer cmd) {
        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 6;

        VkImageMemoryBarrier imageBarrier = {};
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageBarrier.image = outTexture._image;
        imageBarrier.subresourceRange = range;
        imageBarrier.srcAccessMask = 0;
        imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

        outTexture._imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        outTexture.updateDescriptor();
    });

    // ============================

    // === Prepare mapping ===
    RenderPass renderPass = RenderPass(device);
    RenderPass::Attachment color = RenderPass::color(format);
    color.description.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    color.ref.layout = VK_IMAGE_LAYOUT_GENERAL;
    std::vector<VkAttachmentReference> references = {color.ref};
    std::vector<VkAttachmentDescription> attachments = {color.description};
    std::vector<VkSubpassDependency> dependencies = {color.dependency};
    std::vector<VkSubpassDescription> subpasses = {RenderPass::subpass_description(references, nullptr)};

    renderPass.init(attachments, dependencies, subpasses);

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil.depth = 1.0f;

    std::array<VkImageView, 6> imagesViews {};
    std::vector<FrameBuffer> framebuffers;
    framebuffers.reserve(count);

    for (int face = 0; face < count; face++) {
        // Create image view
        VkImageViewCreateInfo info = vkinit::imageview_create_info(format, outTexture._image, VK_IMAGE_ASPECT_COLOR_BIT);
        info.subresourceRange.baseArrayLayer = face; // cube map face in the out texture.
        info.subresourceRange.layerCount = 1;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        vkCreateImageView(device._logicalDevice, &info, nullptr, &imagesViews[face]);

        std::vector<VkImageView> attachments = {imagesViews[face]};
        framebuffers.emplace_back(FrameBuffer(renderPass, attachments, CONVOLVE_WIDTH, CONVOLVE_HEIGHT, 1));
    }

    std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6}
    };

    VkDescriptorSet descriptor;
    VkDescriptorSetLayout setLayout;
    DescriptorLayoutCache layoutCache = DescriptorLayoutCache(device);
    DescriptorAllocator allocator = DescriptorAllocator(device);
    DescriptorBuilder::begin(layoutCache, allocator) // reference texture image
            .bind_image(inTexture._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0)
            .layout(setLayout)
            .build(descriptor, setLayout, poolSizes);

    // Build pipeline
    GraphicPipeline pipelineBuilder = GraphicPipeline(device, renderPass);

    std::vector<std::pair<ShaderType, const char*>> modules {
            {ShaderType::VERTEX, "../src/shaders/env_map/cube_map.vert.spv"},
            {ShaderType::FRAGMENT, "../src/shaders/env_map/irradiance.frag.spv"},
    };

    std::vector<PushConstant> constants {
            {2 * sizeof(glm::mat4), ShaderType::VERTEX},
    };

    MaterialManager materialManager = MaterialManager(&device, &pipelineBuilder);
    std::shared_ptr<ShaderPass> irradiancePass = materialManager.create_material("irradiance", {setLayout}, constants, modules);

    glm::mat4 projection = glm::perspective(glm::radians(90.0f), ENV_WIDTH / static_cast<float>(ENV_HEIGHT),  0.1f, 10.0f);
    std::array<glm::mat4, 6> views = {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    };

    std::shared_ptr<Model> cube = ModelPOLY::create_cube(&device, uploadContext, {-1.0f, -1.0f, -1.0f},  {1.0f, 1.0f, 1.0f});
    meshManager.upload_mesh(*cube);

    // Command pool + command buffer for compute operations
    for (int face = 0; face < count; face++) {
        CommandPool commandPool = CommandPool(device); // can use graphic queue for compute work
        CommandBuffer commandBuffer =  CommandBuffer(device, commandPool);

        VkExtent2D extent;
        extent.width = static_cast<uint32_t>(CONVOLVE_WIDTH);
        extent.height = static_cast<uint32_t>(CONVOLVE_HEIGHT);

        VkRenderPassBeginInfo renderPassInfo = vkinit::renderpass_begin_info(renderPass._renderPass, extent, framebuffers.at(face)._frameBuffer);
        renderPassInfo.clearValueCount = clearValues.size();
        renderPassInfo.pClearValues = clearValues.data();

        VkCommandBufferBeginInfo cmdBeginInfo{};
        cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBeginInfo.pNext = nullptr;
        cmdBeginInfo.pInheritanceInfo = nullptr;
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        glm::mat4 viewProj = projection * views[face];

        VK_CHECK(vkBeginCommandBuffer(commandBuffer._commandBuffer, &cmdBeginInfo));
        {
            vkCmdBeginRenderPass(commandBuffer._commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            {
                VkViewport viewport = vkinit::get_viewport(static_cast<float>(CONVOLVE_WIDTH), static_cast<float>(CONVOLVE_HEIGHT));
                vkCmdSetViewport(commandBuffer._commandBuffer, 0, 1, &viewport);

                VkRect2D scissor = vkinit::get_scissor(static_cast<float>(inTexture._width), static_cast<float>(inTexture._height));
                vkCmdSetScissor(commandBuffer._commandBuffer, 0, 1, &scissor);

                vkCmdBindPipeline(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, irradiancePass->pipeline);
                vkCmdPushConstants(commandBuffer._commandBuffer, irradiancePass->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                                   sizeof(glm::mat4), sizeof(glm::mat4), &viewProj);
                vkCmdBindDescriptorSets(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        irradiancePass->pipelineLayout, 0, 1, &descriptor, 0, nullptr);
                cube->draw(commandBuffer._commandBuffer, irradiancePass->pipelineLayout, sizeof(glm::mat4), 0, true);
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

        device._queue->queue_submit({submitInfo}, VK_NULL_HANDLE);
        device._queue->queue_wait();
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
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

        outTexture.updateDescriptor(); // update descriptor with sample, imageView, imageLayout
    });

    for (int face = 0; face < count; face++) {
        vkDestroyImageView(device._logicalDevice, imagesViews[face], nullptr);
    }

    return outTexture;
}

Texture EnvMap::prefilter_cube_mapping(Device& device, UploadContext& uploadContext, MeshManager& meshManager, Texture& inTexture) {
    uint32_t count = 6;
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

    // === Prepare texture target ===
    Texture outTexture{};
    outTexture._width = PRE_FILTER_WIDTH;
    outTexture._height = PRE_FILTER_HEIGHT;

    VkExtent3D imageExtent;
    imageExtent.width = PRE_FILTER_WIDTH;
    imageExtent.height = PRE_FILTER_HEIGHT;
    imageExtent.depth = 1;

    VkImageCreateInfo imgInfo = vkinit::image_create_info(format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
    imgInfo.arrayLayers = count;
    imgInfo.mipLevels = PRE_FILTER_MIP_LEVEL;
    imgInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imgAllocinfo = {};
    imgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImage(device._allocator, &imgInfo, &imgAllocinfo, &outTexture._image, &outTexture._allocation, nullptr);

    // Create image view
    VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(format, outTexture._image, VK_IMAGE_ASPECT_COLOR_BIT);
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    imageViewInfo.subresourceRange.layerCount = count;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = PRE_FILTER_MIP_LEVEL;
    vkCreateImageView(device._logicalDevice, &imageViewInfo, nullptr, &outTexture._imageView);

    // Create sampler
    VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(PRE_FILTER_MIP_LEVEL);
    samplerInfo.mipLodBias = 0.0f;
    vkCreateSampler(device._logicalDevice, &samplerInfo, nullptr, &outTexture._sampler);

    CommandBuffer::immediate_submit(device, uploadContext, [&](VkCommandBuffer cmd) {
        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = PRE_FILTER_MIP_LEVEL;
        range.baseArrayLayer = 0;
        range.layerCount = 6;

        VkImageMemoryBarrier imageBarrier = {};
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageBarrier.image = outTexture._image;
        imageBarrier.subresourceRange = range;
        imageBarrier.srcAccessMask = 0;
        imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

        outTexture._imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        outTexture.updateDescriptor();
    });

    // ============================

    // === Prepare mapping ===
    RenderPass renderPass = RenderPass(device);
    RenderPass::Attachment color = RenderPass::color(format);
    color.description.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    color.ref.layout = VK_IMAGE_LAYOUT_GENERAL;
    std::vector<VkAttachmentReference> references = {color.ref};
    std::vector<VkAttachmentDescription> attachments = {color.description};
    std::vector<VkSubpassDependency> dependencies = {color.dependency};
    std::vector<VkSubpassDescription> subpasses = {RenderPass::subpass_description(references, nullptr)};
    
    renderPass.init(attachments, dependencies, subpasses);

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil.depth = 1.0f;

    std::vector<FrameBuffer> framebuffers;
    framebuffers.reserve(6 * PRE_FILTER_MIP_LEVEL);
    std::array<VkImageView, 24> imagesViews {};

    std::vector<std::pair<ShaderType, const char*>> modules {
            {ShaderType::VERTEX, "../src/shaders/env_map/cube_map.vert.spv"},
            {ShaderType::FRAGMENT, "../src/shaders/env_map/prefilter.frag.spv"},
    };

    std::array<glm::mat4, 6> views = {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    };

    std::shared_ptr<Model> cube = ModelPOLY::create_cube(&device, uploadContext, {-1.0f, -1.0f, -1.0f},  {1.0f, 1.0f, 1.0f});
    meshManager.upload_mesh(*cube);

    for (int mipLevel = 0; mipLevel < PRE_FILTER_MIP_LEVEL; mipLevel++) {
        uint32_t mipWidth = static_cast<uint32_t>(PRE_FILTER_WIDTH * std::pow(0.5f, mipLevel));
        uint32_t mipHeight = static_cast<uint32_t>(PRE_FILTER_HEIGHT * std::pow(0.5f, mipLevel));
        glm::mat4 projection = glm::perspective(glm::radians(90.0f), static_cast<float>(mipWidth) / static_cast<float>(mipHeight),  0.1f, 10.0f);

        for (int face = 0; face < count; face++) {
            // Create image view
            VkImageViewCreateInfo info = vkinit::imageview_create_info(format, outTexture._image, VK_IMAGE_ASPECT_COLOR_BIT);
            info.subresourceRange.baseArrayLayer = face; // cube map face in the out texture.
            info.subresourceRange.layerCount = 1;
            info.subresourceRange.baseMipLevel = mipLevel;
            info.subresourceRange.levelCount = 1;
            vkCreateImageView(device._logicalDevice, &info, nullptr, &imagesViews[mipLevel * 6 + face]);

            std::vector<VkImageView> attachments = {imagesViews[mipLevel * 6 + face]};
            framebuffers.emplace_back(FrameBuffer(renderPass, attachments, mipWidth, mipHeight, 1));
        }

        std::vector<VkDescriptorPoolSize> poolSizes = {
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6}
        };

        VkDescriptorSet descriptor;
        VkDescriptorSetLayout setLayout;
        DescriptorLayoutCache layoutCache = DescriptorLayoutCache(device);
        DescriptorAllocator allocator = DescriptorAllocator(device);
        DescriptorBuilder::begin(layoutCache, allocator) // reference texture image
                .bind_image(inTexture._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0)
                .layout(setLayout)
                .build(descriptor, setLayout, poolSizes);

        // Build pipeline
        GraphicPipeline pipelineBuilder = GraphicPipeline(device, renderPass);

        std::vector<PushConstant> constants {
                {2 * sizeof(glm::mat4), ShaderType::VERTEX},
                {sizeof(float), ShaderType::FRAGMENT},
        };

        MaterialManager materialManager = MaterialManager(&device, &pipelineBuilder);
        std::shared_ptr<ShaderPass> prefilterPass = materialManager.create_material("prefilter", {setLayout}, constants, modules);

        // Command pool + command buffer for compute operations
        float roughness = static_cast<float>(mipLevel) / static_cast<float>(PRE_FILTER_MIP_LEVEL);

        for (int face = 0; face < count; face++) {
            CommandPool commandPool = CommandPool(device); // can use graphic queue for compute work
            CommandBuffer commandBuffer =  CommandBuffer(device, commandPool);

            VkExtent2D extent;
            extent.width = static_cast<uint32_t>(mipWidth);
            extent.height = static_cast<uint32_t>(mipHeight);

            VkRenderPassBeginInfo renderPassInfo = vkinit::renderpass_begin_info(renderPass._renderPass, extent, framebuffers.at(mipLevel * 6 + face)._frameBuffer);
            renderPassInfo.clearValueCount = clearValues.size();
            renderPassInfo.pClearValues = clearValues.data();

            VkCommandBufferBeginInfo cmdBeginInfo{};
            cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cmdBeginInfo.pNext = nullptr;
            cmdBeginInfo.pInheritanceInfo = nullptr;
            cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            glm::mat4 viewProj = projection * views[face];

            VK_CHECK(vkBeginCommandBuffer(commandBuffer._commandBuffer, &cmdBeginInfo));
            {
                vkCmdBeginRenderPass(commandBuffer._commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                {
                    VkViewport viewport = vkinit::get_viewport(static_cast<float>(mipWidth),static_cast<float>(mipHeight));
                    vkCmdSetViewport(commandBuffer._commandBuffer, 0, 1, &viewport);

                    VkRect2D scissor = vkinit::get_scissor(static_cast<float>(inTexture._width), static_cast<float>(inTexture._height));
                    vkCmdSetScissor(commandBuffer._commandBuffer, 0, 1, &scissor);

                    vkCmdBindPipeline(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, prefilterPass->pipeline);
                    vkCmdPushConstants(commandBuffer._commandBuffer, prefilterPass->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,sizeof(glm::mat4), sizeof(glm::mat4), &viewProj);
                    vkCmdPushConstants(commandBuffer._commandBuffer, prefilterPass->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 2 * sizeof(glm::mat4), sizeof(float), &roughness);
                    vkCmdBindDescriptorSets(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,prefilterPass->pipelineLayout, 0, 1, &descriptor, 0, nullptr);
                    cube->draw(commandBuffer._commandBuffer, prefilterPass->pipelineLayout, sizeof(glm::mat4), 0, true);
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

            device._queue->queue_submit({submitInfo}, VK_NULL_HANDLE);
            device._queue->queue_wait();
        }

    }

    for (int i = 0; i < PRE_FILTER_MIP_LEVEL * 6; i++) {
        // vkDestroyFramebuffer(device._logicalDevice, framebuffers.at(i)._frameBuffer, nullptr);
        vkDestroyImageView(device._logicalDevice, imagesViews[i], nullptr);
    }

    return outTexture;
}

Texture EnvMap::brdf_convolution(Device& device, UploadContext& uploadContext) {
    // === Prepare texture target ===
    Texture outTexture{};
    VkFormat format = VK_FORMAT_R16G16_SFLOAT;
    outTexture._width = BRDF_WIDTH;
    outTexture._height = BRDF_HEIGHT;

    VkExtent3D imageExtent;
    imageExtent.width = BRDF_WIDTH;
    imageExtent.height = BRDF_HEIGHT;
    imageExtent.depth = 1;

    VkImageCreateInfo imgInfo = vkinit::image_create_info(format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imgAllocinfo = {};
    imgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImage(device._allocator, &imgInfo, &imgAllocinfo, &outTexture._image, &outTexture._allocation, nullptr);

    // Create image view
    VkImageViewCreateInfo imageinfo = vkinit::imageview_create_info(format, outTexture._image, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCreateImageView(device._logicalDevice, &imageinfo, nullptr, &outTexture._imageView);

    // Create sampler
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

        outTexture._imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        outTexture.updateDescriptor();
    });

    // === Create compute pipeline ===
    std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}
    };

    // Descriptor set
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

    std::vector<std::pair<ShaderType, const char*>> modules = {
            {ShaderType::COMPUTE, "../src/shaders/env_map/brdf.comp.spv"},
    };
    MaterialManager materialManager = MaterialManager(&device, &pipelineBuilder);
    std::shared_ptr<ShaderPass> brdfPass = materialManager.create_material("brdf", {setLayout}, {}, modules);

    // Command pool + command buffer for compute operations
    {
        CommandPool commandPool = CommandPool(device); // can use graphic queue for compute work
        CommandBuffer commandBuffer =  CommandBuffer(device, commandPool);

        VkCommandBufferBeginInfo cmdBeginInfo{};
        cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBeginInfo.pNext = nullptr;
        cmdBeginInfo.pInheritanceInfo = nullptr;
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        device._queue->queue_wait();

        VK_CHECK(vkBeginCommandBuffer(commandBuffer._commandBuffer, &cmdBeginInfo));
        {
            vkCmdBindPipeline(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, brdfPass->pipeline);
            vkCmdBindDescriptorSets(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, brdfPass->pipelineLayout, 0,1, &descriptor, 0, nullptr);
            vkCmdDispatch(commandBuffer._commandBuffer,   16, 16, 1);
        }
        VK_CHECK(vkEndCommandBuffer(commandBuffer._commandBuffer));

        VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.pWaitDstStageMask = &waitStageMask;
        submitInfo.waitSemaphoreCount = 0; // Semaphore to wait before executing the command buffers
        submitInfo.pWaitSemaphores = nullptr;
        submitInfo.signalSemaphoreCount = 0; // Number of semaphores to be signaled once the commands
        submitInfo.pSignalSemaphores = nullptr;
        submitInfo.commandBufferCount = 1; // Number of command buffers to execute in the batch
        submitInfo.pCommandBuffers = &commandBuffer._commandBuffer;

        device._queue->queue_submit({submitInfo}, VK_NULL_HANDLE);
        device._queue->queue_wait();
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
