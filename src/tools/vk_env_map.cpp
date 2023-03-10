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
#include "core/model/vk_poly.h"
#include "core/manager/vk_mesh_manager.h"
#include "core/utilities/vk_helpers.h"
#include "core/utilities/vk_initializers.h"


Texture EnvMap::cube_map_converter(Device& device, UploadContext& uploadContext, MeshManager& meshManager, Texture& inTexture) {
    uint32_t count = 6;
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB; // VK_FORMAT_R8G8B8A8_SRGB; voir aussi VK_IMAGE_CREATE_MUTABLE_FORMAT a creation d'image

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
    RenderPass::Attachment color = renderPass.color(format);
    color.description.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    VkSubpassDescription subpass = renderPass.subpass_description(&color.ref, nullptr);
    std::vector<VkAttachmentDescription> attachments = {color.description};
    std::vector<VkSubpassDependency> dependencies = {color.dependency};

    renderPass.init(attachments, dependencies, subpass);

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil.depth = 1.0f;

    std::array<VkFramebuffer, 6> framebuffers {};
    std::array<VkImageView, 6> imagesViews {};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext = nullptr;
    framebufferInfo.renderPass = renderPass._renderPass;
    framebufferInfo.width = ENV_WIDTH;
    framebufferInfo.height = ENV_HEIGHT;
    framebufferInfo.layers = 1;

    for (int face = 0; face < count; face++) {
        // Create image view
        VkImageViewCreateInfo info = vkinit::imageview_create_info(format, outTexture._image, VK_IMAGE_ASPECT_COLOR_BIT);
        info.subresourceRange.baseArrayLayer = face; // cube map face in the out texture.
        info.subresourceRange.layerCount = 1;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        vkCreateImageView(device._logicalDevice, &info, nullptr, &imagesViews[face]);

        std::array<VkImageView, 1> attachments = {imagesViews[face]};
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();  // framebuffer used for image modification
        VK_CHECK(vkCreateFramebuffer(device._logicalDevice, &framebufferInfo, nullptr, &framebuffers[face]));
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
    PipelineBuilder pipelineBuilder = PipelineBuilder(device, renderPass);
    pipelineBuilder._type = PipelineBuilder::Type::graphic;
    pipelineBuilder._viewport = vkinit::get_viewport((float) ENV_WIDTH, (float) ENV_HEIGHT);
    pipelineBuilder._scissor = vkinit::get_scissor((float) ENV_WIDTH, (float) ENV_HEIGHT);

    std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> modules = {
            {VK_SHADER_STAGE_VERTEX_BIT, "../src/shaders/env_map/cube_map.vert.spv"},
            {VK_SHADER_STAGE_FRAGMENT_BIT, "../src/shaders/env_map/converter.frag.spv"},
    };

    VkPushConstantRange pushCst;
    pushCst.offset = 0;
    pushCst.size = 2 * sizeof(glm::mat4);
    pushCst.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::shared_ptr<ShaderEffect> effect = pipelineBuilder.build_effect({setLayout}, {pushCst}, modules);
    std::shared_ptr<ShaderPass> converterPass = pipelineBuilder.build_pass(effect);
    pipelineBuilder.create_material("converter", converterPass);

    glm::mat4 projection = glm::perspective(glm::radians(90.0f), ENV_WIDTH / static_cast<float>(ENV_HEIGHT),  0.1f, 10.0f);
    std::array<glm::mat4, 6> views = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    };

    std::shared_ptr<Model> cube = ModelPOLY::create_cube(&device, {-1.0f, -1.0f, -1.0f},  {1.0f, 1.0f, 1.0f});
    meshManager.upload_mesh(*cube);

    for (int face = 0; face < count; face++) {
        CommandPool commandPool = CommandPool(device);
        CommandBuffer commandBuffer =  CommandBuffer(device, commandPool);

        VkExtent2D extent;
        extent.width = static_cast<uint32_t>(ENV_WIDTH);
        extent.height = static_cast<uint32_t>(ENV_HEIGHT);

        VkRenderPassBeginInfo renderPassInfo = vkinit::renderpass_begin_info(renderPass._renderPass, extent, framebuffers[face]);
        renderPassInfo.clearValueCount = clearValues.size();
        renderPassInfo.pClearValues = clearValues.data();

        VkCommandBufferBeginInfo cmdBeginInfo{};
        cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBeginInfo.pNext = nullptr;
        cmdBeginInfo.pInheritanceInfo = nullptr;
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        glm::mat4 viewProj = projection * views[face];
        // glm::mat4 viewProj = -1 * glm::perspective((float)(M_PI / 2.0), 1.0f, 0.1f, 512.0f) * matrices[face];

        VK_CHECK(vkBeginCommandBuffer(commandBuffer._commandBuffer, &cmdBeginInfo));
        {
            vkCmdBeginRenderPass(commandBuffer._commandBuffer, &renderPassInfo,VK_SUBPASS_CONTENTS_INLINE);
            {
                VkViewport viewport = vkinit::get_viewport((float) ENV_WIDTH, (float) ENV_HEIGHT);
                vkCmdSetViewport(commandBuffer._commandBuffer, 0, 1, &viewport);

                VkRect2D scissor = vkinit::get_scissor((float) inTexture._width, (float) inTexture._height);
                vkCmdSetScissor(commandBuffer._commandBuffer, 0, 1, &scissor);

                vkCmdBindPipeline(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, converterPass->pipeline);
                vkCmdPushConstants(commandBuffer._commandBuffer, converterPass->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), sizeof(glm::mat4), &viewProj);
                vkCmdBindDescriptorSets(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, converterPass->pipelineLayout, 0,1, &descriptor, 0, nullptr);
                cube->draw(commandBuffer._commandBuffer, converterPass->pipelineLayout, 0, true);
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
        // destroy commandPool & commandBuffer
    }

    for (auto& shader : effect->shaderStages) {
        vkDestroyShaderModule(device._logicalDevice, shader.shaderModule, nullptr);
    }

    // destroy renderPass
    // destroy pipeline
    for (int face = 0; face < count; face++) {
        vkDestroyFramebuffer(device._logicalDevice, framebuffers[face], nullptr);
        vkDestroyImageView(device._logicalDevice, imagesViews[face], nullptr);
    }

    return outTexture;
}

Texture EnvMap::irradiance_mapping(Device& device, UploadContext& uploadContext, Texture& inTexture) {
    // === Prepare texture target ===
    Texture outTexture{};
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    outTexture._width = CONVOLVE_WIDTH;
    outTexture._height = CONVOLVE_HEIGHT;

    VkExtent3D imageExtent;
    imageExtent.width = CONVOLVE_WIDTH;
    imageExtent.height = CONVOLVE_HEIGHT;
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
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

        outTexture._imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        outTexture.updateDescriptor();
    });

    // ============================

    // === Prepare mapping ===
    RenderPass renderPass = RenderPass(device);
    RenderPass::Attachment color = renderPass.color(format);
    color.description.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    VkSubpassDescription subpass = renderPass.subpass_description(&color.ref, nullptr);
    std::vector<VkAttachmentDescription> attachments = {color.description};
    std::vector<VkSubpassDependency> dependencies = {color.dependency};

    renderPass.init(attachments, dependencies, subpass);

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil.depth = 1.0f;

    // === Create compute pipeline ===
    std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2}
    };

    // Descriptor set
    VkDescriptorSet descriptor{};
    VkDescriptorSetLayout setLayout{};
    DescriptorLayoutCache layoutCache = DescriptorLayoutCache(device);
    DescriptorAllocator allocator = DescriptorAllocator(device);
    DescriptorBuilder::begin(layoutCache, allocator) // reference texture image
            .bind_image(inTexture._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT, 0)
            .bind_image(outTexture._descriptor, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1)
            .layout(setLayout)
            .build(descriptor, setLayout, poolSizes);

    // Build pipeline
    PipelineBuilder pipelineBuilder = PipelineBuilder(device, renderPass); // renderPass useless
    pipelineBuilder._type = PipelineBuilder::Type::compute;
    pipelineBuilder._viewport = vkinit::get_viewport((float) CONVOLVE_WIDTH, (float) CONVOLVE_HEIGHT);
    pipelineBuilder._scissor = vkinit::get_scissor((float) CONVOLVE_WIDTH, (float) CONVOLVE_HEIGHT);

    std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> modules = {
            {VK_SHADER_STAGE_COMPUTE_BIT, "../src/shaders/env_map/irradiance.comp.spv"},
    };

    std::shared_ptr<ShaderEffect> effect = pipelineBuilder.build_effect({setLayout}, {}, modules);
    std::shared_ptr<ShaderPass> irradiancePass = pipelineBuilder.build_pass(effect);
    pipelineBuilder.create_material("irradiance", irradiancePass);

    // Command pool + command buffer for compute operations
    {
        CommandPool commandPool = CommandPool(device); // can use graphic queue for compute work
        CommandBuffer commandBuffer =  CommandBuffer(device, commandPool);

        VkCommandBufferBeginInfo cmdBeginInfo{};
        cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBeginInfo.pNext = nullptr;
        cmdBeginInfo.pInheritanceInfo = nullptr;
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkQueueWaitIdle(device.get_graphics_queue()));

        VK_CHECK(vkBeginCommandBuffer(commandBuffer._commandBuffer, &cmdBeginInfo));
        {
            vkCmdBindPipeline(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, irradiancePass->pipeline);
            vkCmdBindDescriptorSets(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, irradiancePass->pipelineLayout, 0,1, &descriptor, 0, nullptr);
            vkCmdDispatch(commandBuffer._commandBuffer, inTexture._width / 32, inTexture._height / 32, 1);
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

        VK_CHECK(vkQueueSubmit(device.get_graphics_queue(), 1, &submitInfo, VK_NULL_HANDLE));
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

        outTexture.updateDescriptor(); // update descriptor with sample, imageView, imageLayout
    });

    for (auto& shader : effect->shaderStages) {
        vkDestroyShaderModule(device._logicalDevice, shader.shaderModule, nullptr);
    }

    // vkDestroyFramebuffer(device._logicalDevice, framebuffer, nullptr);
    // vkDestroyImageView(device._logicalDevice, imageView, nullptr);

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
    RenderPass::Attachment color = renderPass.color(format);
    color.description.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    color.ref.layout = VK_IMAGE_LAYOUT_GENERAL;
    VkSubpassDescription subpass = renderPass.subpass_description(&color.ref, nullptr);
    std::vector<VkAttachmentDescription> attachments = {color.description};
    std::vector<VkSubpassDependency> dependencies = {color.dependency};

    renderPass.init(attachments, dependencies, subpass);

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil.depth = 1.0f;

    std::array<VkFramebuffer, 6> framebuffers {};
    std::array<VkImageView, 6> imagesViews {};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext = nullptr;
    framebufferInfo.renderPass = renderPass._renderPass;
    framebufferInfo.width = CONVOLVE_WIDTH;
    framebufferInfo.height = CONVOLVE_HEIGHT;
    framebufferInfo.layers = 1;

    for (int face = 0; face < count; face++) {
        // Create image view
        VkImageViewCreateInfo info = vkinit::imageview_create_info(format, outTexture._image, VK_IMAGE_ASPECT_COLOR_BIT);
        info.subresourceRange.baseArrayLayer = face; // cube map face in the out texture.
        info.subresourceRange.layerCount = 1;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        vkCreateImageView(device._logicalDevice, &info, nullptr, &imagesViews[face]);

        std::array<VkImageView, 1> attachments = {imagesViews[face]};
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();  // framebuffer used for image modification
        VK_CHECK(vkCreateFramebuffer(device._logicalDevice, &framebufferInfo, nullptr, &framebuffers[face]));
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
    PipelineBuilder pipelineBuilder = PipelineBuilder(device, renderPass); // renderPass useless
    pipelineBuilder._type = PipelineBuilder::Type::graphic;
    pipelineBuilder._viewport = vkinit::get_viewport((float) CONVOLVE_WIDTH, (float) CONVOLVE_HEIGHT);
    pipelineBuilder._scissor = vkinit::get_scissor((float) CONVOLVE_WIDTH, (float) CONVOLVE_HEIGHT);

    std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> modules = {
            {VK_SHADER_STAGE_VERTEX_BIT, "../src/shaders/env_map/cube_map.vert.spv"},
            {VK_SHADER_STAGE_FRAGMENT_BIT, "../src/shaders/env_map/irradiance.frag.spv"},
    };

    VkPushConstantRange pushCst;
    pushCst.offset = 0;
    pushCst.size = 2 * sizeof(glm::mat4);
    pushCst.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::shared_ptr<ShaderEffect> effect = pipelineBuilder.build_effect({setLayout}, {pushCst}, modules);
    std::shared_ptr<ShaderPass> irradiancePass = pipelineBuilder.build_pass(effect);
    pipelineBuilder.create_material("irradiance", irradiancePass);

    glm::mat4 projection = glm::perspective(glm::radians(90.0f), ENV_WIDTH / static_cast<float>(ENV_HEIGHT),  0.1f, 10.0f);
    std::array<glm::mat4, 6> views = {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    };

    std::shared_ptr<Model> cube = ModelPOLY::create_cube(&device, {-1.0f, -1.0f, -1.0f},  {1.0f, 1.0f, 1.0f});
    meshManager.upload_mesh(*cube);


    // Command pool + command buffer for compute operations
    for (int face = 0; face < count; face++) {
        CommandPool commandPool = CommandPool(device); // can use graphic queue for compute work
        CommandBuffer commandBuffer =  CommandBuffer(device, commandPool);

        VkExtent2D extent;
        extent.width = static_cast<uint32_t>(CONVOLVE_WIDTH);
        extent.height = static_cast<uint32_t>(CONVOLVE_HEIGHT);

        VkRenderPassBeginInfo renderPassInfo = vkinit::renderpass_begin_info(renderPass._renderPass, extent, framebuffers[face]);
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
                VkViewport viewport = vkinit::get_viewport((float) CONVOLVE_WIDTH, (float) CONVOLVE_HEIGHT);
                vkCmdSetViewport(commandBuffer._commandBuffer, 0, 1, &viewport);

                VkRect2D scissor = vkinit::get_scissor((float) inTexture._width, (float) inTexture._height);
                vkCmdSetScissor(commandBuffer._commandBuffer, 0, 1, &scissor);

                vkCmdBindPipeline(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, irradiancePass->pipeline);
                vkCmdPushConstants(commandBuffer._commandBuffer, irradiancePass->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                                   sizeof(glm::mat4), sizeof(glm::mat4), &viewProj);
                vkCmdBindDescriptorSets(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        irradiancePass->pipelineLayout, 0, 1, &descriptor, 0, nullptr);
                cube->draw(commandBuffer._commandBuffer, irradiancePass->pipelineLayout, 0, true);
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

    for (auto& shader : effect->shaderStages) {
        vkDestroyShaderModule(device._logicalDevice, shader.shaderModule, nullptr);
    }

    for (int face = 0; face < count; face++) {
        vkDestroyFramebuffer(device._logicalDevice, framebuffers[face], nullptr);
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
    RenderPass::Attachment color = renderPass.color(format);
    color.description.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    color.ref.layout = VK_IMAGE_LAYOUT_GENERAL;
    VkSubpassDescription subpass = renderPass.subpass_description(&color.ref, nullptr);
    std::vector<VkAttachmentDescription> attachments = {color.description};
    std::vector<VkSubpassDependency> dependencies = {color.dependency};

    renderPass.init(attachments, dependencies, subpass);

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil.depth = 1.0f;

    std::array<VkFramebuffer, 24> framebuffers {}; // 6 * PRE_FILTER_MIP_LEVEL
    std::array<VkImageView, 24> imagesViews {};

    std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> modules = {
            {VK_SHADER_STAGE_VERTEX_BIT, "../src/shaders/env_map/cube_map.vert.spv"},
            {VK_SHADER_STAGE_FRAGMENT_BIT, "../src/shaders/env_map/prefilter.frag.spv"},
    };

    std::array<glm::mat4, 6> views = {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    };

    std::shared_ptr<Model> cube = ModelPOLY::create_cube(&device, {-1.0f, -1.0f, -1.0f},  {1.0f, 1.0f, 1.0f});
    meshManager.upload_mesh(*cube);

    for (int mipLevel = 0; mipLevel < PRE_FILTER_MIP_LEVEL; mipLevel++) {
        uint32_t mipWidth = static_cast<uint32_t>(PRE_FILTER_WIDTH * std::pow(0.5f, mipLevel));
        uint32_t mipHeight = static_cast<uint32_t>(PRE_FILTER_HEIGHT * std::pow(0.5f, mipLevel));
        glm::mat4 projection = glm::perspective(glm::radians(90.0f), static_cast<float>(mipWidth) / static_cast<float>(mipHeight),  0.1f, 10.0f);

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.pNext = nullptr;
        framebufferInfo.renderPass = renderPass._renderPass;
        framebufferInfo.width = mipWidth;
        framebufferInfo.height = mipHeight;
        framebufferInfo.layers = 1;

        for (int face = 0; face < count; face++) {
            // Create image view
            VkImageViewCreateInfo info = vkinit::imageview_create_info(format, outTexture._image, VK_IMAGE_ASPECT_COLOR_BIT);
            info.subresourceRange.baseArrayLayer = face; // cube map face in the out texture.
            info.subresourceRange.layerCount = 1;
            info.subresourceRange.baseMipLevel = mipLevel;
            info.subresourceRange.levelCount = 1;
            vkCreateImageView(device._logicalDevice, &info, nullptr, &imagesViews[mipLevel * 6 + face]);

            std::array<VkImageView, 1> attachments = {imagesViews[mipLevel * 6 + face]};
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();  // framebuffer used for image modification
            VK_CHECK(vkCreateFramebuffer(device._logicalDevice, &framebufferInfo, nullptr, &framebuffers[mipLevel * 6 + face]));
        }

        std::vector<VkDescriptorPoolSize> poolSizes = {
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6}
        };

//        struct GPURoughnessData {
//            alignas(sizeof(float)) float roughness;
//        };
//
//        AllocatedBuffer roughnessBuffer = Buffer::create_buffer(device, sizeof(GPURoughnessData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
//        VkDescriptorBufferInfo roughnessInfo{};
//        roughnessInfo.buffer = roughnessBuffer._buffer;
//        roughnessInfo.offset = 0;
//        roughnessInfo.range = sizeof(GPURoughnessData);

        VkDescriptorSet descriptor;
        VkDescriptorSetLayout setLayout;
        DescriptorLayoutCache layoutCache = DescriptorLayoutCache(device);
        DescriptorAllocator allocator = DescriptorAllocator(device);
        DescriptorBuilder::begin(layoutCache, allocator) // reference texture image
                // .bind_buffer(roughnessInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 0)
                .bind_image(inTexture._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0)
                .layout(setLayout)
                .build(descriptor, setLayout, poolSizes);

        // Build pipeline
        PipelineBuilder pipelineBuilder = PipelineBuilder(device, renderPass); // renderPass useless
        pipelineBuilder._type = PipelineBuilder::Type::graphic;
        pipelineBuilder._viewport = vkinit::get_viewport((float) mipWidth, (float) mipHeight);
        pipelineBuilder._scissor = vkinit::get_scissor((float) mipWidth, (float) mipHeight);

        VkPushConstantRange push_vert;
        push_vert.offset = 0;
        push_vert.size = 2 * sizeof(glm::mat4);
        push_vert.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkPushConstantRange push_frag;
        push_frag.offset = 2 * sizeof(glm::mat4);
        push_frag.size = sizeof(float);
        push_frag.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::shared_ptr<ShaderEffect> effect = pipelineBuilder.build_effect({setLayout}, {push_vert, push_frag}, modules);
        std::shared_ptr<ShaderPass> irradiancePass = pipelineBuilder.build_pass(effect);
        pipelineBuilder.create_material("prefilter", irradiancePass);

        // Command pool + command buffer for compute operations
        float roughness = static_cast<float>(mipLevel) / float(PRE_FILTER_MIP_LEVEL);

//        void *data;
//        vmaMapMemory(device._allocator, roughnessBuffer._allocation, &data);
//        memcpy(data, &roughness, sizeof(float));
//        vmaUnmapMemory(device._allocator, roughnessBuffer._allocation);

        for (int face = 0; face < count; face++) {
            CommandPool commandPool = CommandPool(device); // can use graphic queue for compute work
            CommandBuffer commandBuffer =  CommandBuffer(device, commandPool);

            VkExtent2D extent;
            extent.width = static_cast<uint32_t>(mipWidth);
            extent.height = static_cast<uint32_t>(mipHeight);

            VkRenderPassBeginInfo renderPassInfo = vkinit::renderpass_begin_info(renderPass._renderPass, extent, framebuffers[mipLevel * 6 + face]);
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
                    VkViewport viewport = vkinit::get_viewport((float) mipWidth, (float) mipHeight);
                    vkCmdSetViewport(commandBuffer._commandBuffer, 0, 1, &viewport);

                    VkRect2D scissor = vkinit::get_scissor((float) inTexture._width, (float) inTexture._height);
                    vkCmdSetScissor(commandBuffer._commandBuffer, 0, 1, &scissor);

                    vkCmdBindPipeline(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, irradiancePass->pipeline);
                    vkCmdPushConstants(commandBuffer._commandBuffer, irradiancePass->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,sizeof(glm::mat4), sizeof(glm::mat4), &viewProj);
                    vkCmdPushConstants(commandBuffer._commandBuffer, irradiancePass->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 2 * sizeof(glm::mat4), sizeof(float), &roughness);
                    vkCmdBindDescriptorSets(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,irradiancePass->pipelineLayout, 0, 1, &descriptor, 0, nullptr);
                    cube->draw(commandBuffer._commandBuffer, irradiancePass->pipelineLayout, 0, true);
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

        for (auto& shader : effect->shaderStages) {
            vkDestroyShaderModule(device._logicalDevice, shader.shaderModule, nullptr);
        }
    }

    for (int i = 0; i < PRE_FILTER_MIP_LEVEL * 6; i++) {
        vkDestroyFramebuffer(device._logicalDevice, framebuffers[i], nullptr);
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
    RenderPass renderPass = RenderPass(device);
    PipelineBuilder pipelineBuilder = PipelineBuilder(device, renderPass); // renderPass useless
    pipelineBuilder._type = PipelineBuilder::Type::compute;
    pipelineBuilder._viewport = vkinit::get_viewport((float) BRDF_WIDTH, (float) BRDF_HEIGHT);
    pipelineBuilder._scissor = vkinit::get_scissor((float) BRDF_WIDTH, (float) BRDF_HEIGHT);

    std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> modules = {
            {VK_SHADER_STAGE_COMPUTE_BIT, "../src/shaders/env_map/brdf.comp.spv"},
    };

    std::shared_ptr<ShaderEffect> effect = pipelineBuilder.build_effect({setLayout}, {}, modules);
    std::shared_ptr<ShaderPass> brdfPass = pipelineBuilder.build_pass(effect);
    pipelineBuilder.create_material("brdf", brdfPass);

    // Command pool + command buffer for compute operations
    {
        CommandPool commandPool = CommandPool(device); // can use graphic queue for compute work
        CommandBuffer commandBuffer =  CommandBuffer(device, commandPool);

        VkCommandBufferBeginInfo cmdBeginInfo{};
        cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBeginInfo.pNext = nullptr;
        cmdBeginInfo.pInheritanceInfo = nullptr;
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkQueueWaitIdle(device.get_graphics_queue()));

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

    for (auto& shader : effect->shaderStages) {
        vkDestroyShaderModule(device._logicalDevice, shader.shaderModule, nullptr);
    }


    return outTexture;
}
