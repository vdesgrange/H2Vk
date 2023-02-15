#include "vk_env_map.h"
#include "core/vk_window.h"
#include "core/vk_device.h"
#include "core/vk_framebuffers.h"
#include "core/vk_buffer.h"
#include "core/vk_swapchain.h"
#include "core/vk_renderpass.h"
#include "core/vk_pipeline.h"
#include "core/vk_command_pool.h"
#include "core/vk_command_buffer.h"
#include "core/vk_descriptor_allocator.h"
#include "core/vk_descriptor_cache.h"
#include "core/vk_descriptor_builder.h"
#include "core/vk_texture.h"
#include "core/model/vk_poly.h"
#include "core/utilities/vk_helpers.h"
#include "core/utilities/vk_initializers.h"

void EnvMap::init_pipeline() {

}

void EnvMap::clean_up() {

}

void EnvMap::loader(const char* file, Device& device, Texture& texture, UploadContext& uploadContext) {
    // Load image file
    stbi_uc* pixels = stbi_load(file, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        std::cout << "Failed to load texture file " << file << std::endl;
    }

    void* pixel_ptr = pixels;
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    AllocatedBuffer buffer = Buffer::create_buffer(device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void* data;
    vmaMapMemory(device._allocator, buffer._allocation, &data);
    memcpy(data, pixel_ptr, static_cast<size_t>(imageSize));
    vmaUnmapMemory(device._allocator, buffer._allocation);
    stbi_image_free(pixels);

    // Create VkImageView and VkImage for loaded texture
    VkExtent3D imageExtent;
    imageExtent.width = static_cast<uint32_t>(texWidth);
    imageExtent.height = static_cast<uint32_t>(texHeight);
    imageExtent.depth = 1;

    VkImageCreateInfo imgInfo = vkinit::image_create_info(format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imgAllocinfo = {};
    imgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImage(device._allocator, &imgInfo, &imgAllocinfo, &texture._image, &texture._allocation, nullptr);

    CommandBuffer::immediate_submit(device, uploadContext, [&](VkCommandBuffer cmd) {
        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

        VkImageMemoryBarrier imageBarrier_toTransfer = {};
        imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toTransfer.image = texture._image;
        imageBarrier_toTransfer.subresourceRange = range;
        imageBarrier_toTransfer.srcAccessMask = 0;
        imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        //barrier the image into the transfer-receive layout
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);

        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;
        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = imageExtent;

        //copy the buffer into the image
        vkCmdCopyBufferToImage(cmd, buffer._buffer, texture._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
        imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);

        // Change texture image layout to shader read after all mip levels have been copied
        texture._imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);
        vkCreateSampler(device._logicalDevice, &samplerInfo, nullptr, &texture._sampler);

        VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(format, texture._image, VK_IMAGE_ASPECT_COLOR_BIT);
        vkCreateImageView(device._logicalDevice, &imageViewInfo, nullptr, &texture._imageView);

        texture.updateDescriptor();
    });

    vmaDestroyBuffer(device._allocator, buffer._buffer, buffer._allocation);
    std::cout << "Sphere map loaded successfully " << file << std::endl;
};

void EnvMap::cube_map_converter(Window& window, Device& device, UploadContext& uploadContext, Texture& inTexture) {
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

    // === Prepare texture target ===
    Texture outTexture{};

    VkExtent3D imageExtent;
    imageExtent.width = static_cast<uint32_t>(texWidth);
    imageExtent.height = static_cast<uint32_t>(texHeight);
    imageExtent.depth = 1;

    VkImageCreateInfo imgInfo = vkinit::image_create_info(format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
    imgInfo.arrayLayers = 6;
    imgInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imgAllocinfo = {};
    imgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImage(device._allocator, &imgInfo, &imgAllocinfo, &outTexture._image, &outTexture._allocation, nullptr);

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

        outTexture.updateDescriptor(); // update descriptor with sample, imageView, imageLayout
    });


    SwapChain swapchain(window, device); // to remove
    swapchain._depthFormat = VK_FORMAT_D32_SFLOAT;
    swapchain._swapChainImageFormat = VK_FORMAT_UNDEFINED;

    RenderPass renderPass = RenderPass(device, swapchain);
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil.depth = 1.0f;

    std::array<VkFramebuffer, 6> framebuffers {};
    std::array<VkImageView, 6> imagesViews {};
    std::array<VkSampler, 6> samplers {};

//    VkImageCreateInfo imgInfo = vkinit::image_create_info(format,   VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, imageExtent);
//    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//    // Create image
//    VmaAllocationCreateInfo imgAllocinfo = {};
//    imgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
//    vmaCreateImage(device._allocator, &imgInfo, &imgAllocinfo, &outTexture._image, &outTexture._allocation, nullptr);

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext = nullptr;
    framebufferInfo.renderPass = renderPass._renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.width = texWidth;
    framebufferInfo.height = texHeight;
    framebufferInfo.layers = 1;

    for (int face = 0; face < 6; face++) {
        // Create image view
        VkImageViewCreateInfo info = vkinit::imageview_create_info(format, outTexture._image, VK_IMAGE_ASPECT_COLOR_BIT);
        info.subresourceRange.baseArrayLayer = face; // cube map face in the out texture.
        info.subresourceRange.layerCount = 1;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        vkCreateImageView(device._logicalDevice, &info, nullptr, &imagesViews[face]);

        // Create sampler
        VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);
        vkCreateSampler(device._logicalDevice, &samplerInfo, nullptr, &samplers[face]);

        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &imagesViews[face]; // framebuffer used for image modification
        VK_CHECK(vkCreateFramebuffer(device._logicalDevice, &framebufferInfo, nullptr, &framebuffers[face]));
    }

    VkDescriptorSetLayout setLayout;
    VkDescriptorSet descriptor;
    DescriptorLayoutCache layoutCache = DescriptorLayoutCache(device);
    DescriptorAllocator allocator = DescriptorAllocator(device);
    std::vector<VkDescriptorPoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}};
    DescriptorBuilder::begin(layoutCache, allocator) // reference texture image
        .bind_image(inTexture._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0)
        .layout(setLayout)
        .build(descriptor, setLayout, poolSizes);

    PipelineBuilder pipelineBuilder = PipelineBuilder(window, device, renderPass);
    pipelineBuilder._viewport = vkinit::get_viewport((float) texWidth, (float) texHeight);
    pipelineBuilder._scissor = vkinit::get_scissor((float) texWidth, (float) texHeight);

    std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> modules = {
            {VK_SHADER_STAGE_VERTEX_BIT, "../src/shaders/env_map/converter.vert.spv"},
            {VK_SHADER_STAGE_FRAGMENT_BIT, "../src/shaders/env_map/converter.frag.spv"},
    };

    VkPushConstantRange pushCst;
    pushCst.offset = 0;
    pushCst.size = sizeof(glm::mat4);
    pushCst.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;


    std::shared_ptr<ShaderEffect> effect = pipelineBuilder.build_effect({setLayout}, {pushCst}, modules);
    std::shared_ptr<ShaderPass> converterPass = pipelineBuilder.build_pass(effect);
    // pipelineBuilder.create_material("converter", converterPass);

    glm::mat4 projection = glm::perspective(glm::radians(90.0f), texWidth / static_cast<float>(texHeight),  0.1f, 10.0f);
    std::array<glm::mat4, 6> views = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    };

    std::shared_ptr<Model> cube = ModelPOLY::create_cube(&device, {-100.0f, -100.0f, -100.0f},  {100.f, 100.f, 100.0f});

    for (int face = 0; face < 6; face++) {
        VkDeviceSize offset = 0;

        CommandPool commandPool = CommandPool(device);
        CommandBuffer commandBuffer =  CommandBuffer(device, commandPool);

        VkExtent2D extent;
        extent.width = static_cast<uint32_t>(texWidth);
        extent.height = static_cast<uint32_t>(texHeight);

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
            vkCmdBeginRenderPass(commandBuffer._commandBuffer, &renderPassInfo,VK_SUBPASS_CONTENTS_INLINE);
            {
                vkCmdBindPipeline(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, converterPass->pipeline);
                vkCmdPushConstants(commandBuffer._commandBuffer, converterPass->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &viewProj);
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
    for (int face = 0; face < 6; face++) {
        vkDestroyFramebuffer(device._logicalDevice, framebuffers[face], nullptr);
        vkDestroyImageView(device._logicalDevice, imagesViews[face], nullptr);
    }
}

Texture EnvMap::irradiance_mapping(Window& window, Device& device, UploadContext& uploadContext, Texture& inTexture) {
    uint32_t convolveWidth = 32;
    uint32_t convolveHeight = 32;

    // === Prepare texture target ===
    Texture outTexture{};
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

    VkExtent3D imageExtent;
    imageExtent.width = static_cast<uint32_t>(convolveWidth);
    imageExtent.height = static_cast<uint32_t>(convolveHeight);
    imageExtent.depth = 1;

    VkImageCreateInfo imgInfo = vkinit::image_create_info(format,   VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, imageExtent);
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // Create image
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

        outTexture.updateDescriptor(); // update descriptor with sample, imageView, imageLayout
    });

    // ============================
    // createImage, createImageView
    SwapChain swapchain(window, device);
    swapchain._depthFormat = VK_FORMAT_D32_SFLOAT;
    swapchain._swapChainImageFormat = format;

    // Render Pass
    RenderPass renderPass = RenderPass(device, swapchain); // no depth maybe?
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil.depth = 1.0f;

    // Texture to modify
    VkImageView imageView;
    VkImageViewCreateInfo info = vkinit::imageview_create_info(format, outTexture._image, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCreateImageView(device._logicalDevice, &info, nullptr, &imageView);

    std::array<VkImageView, 2> attachments = {imageView, swapchain._depthImageView}; // to do remove depth

//    VkFramebufferCreateInfo framebufferInfo{};
//    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
//    framebufferInfo.pNext = nullptr;
//    framebufferInfo.renderPass = VK_NULL_HANDLE; // renderPass._renderPass;
//    framebufferInfo.attachmentCount = attachments.size();
//    framebufferInfo.width = convolveWidth;
//    framebufferInfo.height = convolveHeight;
//    framebufferInfo.layers = 1;
//    framebufferInfo.pAttachments = attachments.data(); // framebuffer used for image modification
//    framebufferInfo.flags = VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT;
//    VkFramebuffer framebuffer;
//    VK_CHECK(vkCreateFramebuffer(device._logicalDevice, &framebufferInfo, nullptr, &framebuffer));

    // === Create compute pipeline ===

    // Descriptor set
    VkDescriptorSet descriptor{};
    VkDescriptorSetLayout setLayout{};
    DescriptorLayoutCache layoutCache = DescriptorLayoutCache(device);
    DescriptorAllocator allocator = DescriptorAllocator(device);
    std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2}
    };

    DescriptorBuilder::begin(layoutCache, allocator) // reference texture image
            .bind_image(inTexture._descriptor, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0)
            .bind_image(outTexture._descriptor, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1) // VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
            .layout(setLayout)
            .build(descriptor, setLayout, poolSizes);

    // Build pipeline
    PipelineBuilder pipelineBuilder = PipelineBuilder(window, device, renderPass);
    pipelineBuilder._type = PipelineBuilder::Type::compute;
    pipelineBuilder._viewport = vkinit::get_viewport((float) convolveWidth, (float) convolveHeight);
    pipelineBuilder._scissor = vkinit::get_scissor((float) convolveWidth, (float) convolveHeight);

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

//        VkExtent2D extent;
//        extent.width = static_cast<uint32_t>(texWidth);
//        extent.height = static_cast<uint32_t>(texHeight);

//        VkRenderPassBeginInfo renderPassInfo = vkinit::renderpass_begin_info(renderPass._renderPass, extent, framebuffer);
//        renderPassInfo.clearValueCount = clearValues.size();
//        renderPassInfo.pClearValues = clearValues.data();

        VkCommandBufferBeginInfo cmdBeginInfo{};
        cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBeginInfo.pNext = nullptr;
        cmdBeginInfo.pInheritanceInfo = nullptr;
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(commandBuffer._commandBuffer, &cmdBeginInfo));
        {
//            vkCmdBeginRenderPass(commandBuffer._commandBuffer, &renderPassInfo,VK_SUBPASS_CONTENTS_INLINE);
//            {
                vkCmdBindPipeline(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, irradiancePass->pipeline);
                vkCmdBindDescriptorSets(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, irradiancePass->pipelineLayout, 0,1, &descriptor, 0, nullptr);
                vkCmdDispatch(commandBuffer._commandBuffer, 1, 1, 1);
//            }
//            vkCmdEndRenderPass(commandBuffer._commandBuffer);
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
        imageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;;
        imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

        outTexture.updateDescriptor(); // update descriptor with sample, imageView, imageLayout
    });

    for (auto& shader : effect->shaderStages) {
        vkDestroyShaderModule(device._logicalDevice, shader.shaderModule, nullptr);
    }

    // vkDestroyFramebuffer(device._logicalDevice, framebuffer, nullptr);
    vkDestroyImageView(device._logicalDevice, imageView, nullptr);

    return outTexture;
}

void EnvMap::prefilter_mapping() {}

void EnvMap::brdf_convolution() {}

void EnvMap::main() {
    // to do
}