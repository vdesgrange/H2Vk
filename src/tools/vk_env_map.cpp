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


void EnvMap::loader(const char* file, Device& device, Texture& texture, UploadContext& uploadContext) {
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

void EnvMap::cube_map_converter(const char* file, Window& window, Device& device, SwapChain& swapchain, Texture& texture) {
    RenderPass renderPass = RenderPass(device, swapchain);
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil.depth = 1.0f;

    std::array<FrameBuffers, 6> framebuffers;
    std::array<VkImageView, 6> imagesViews;

    for (int face = 0; face < 6; face++) {
        VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(VK_FORMAT_R8G8B8A8_SRGB, texture._image, VK_IMAGE_ASPECT_COLOR_BIT);
        vkCreateImageView(device._logicalDevice, &imageViewInfo, nullptr, &imagesViews[face]);
    }

    VkDescriptorSetLayout setLayout;
    VkDescriptorSet descriptor;
    DescriptorLayoutCache layoutCache = DescriptorLayoutCache(device);
    DescriptorAllocator allocator = DescriptorAllocator(device);
    std::vector<VkDescriptorPoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}};
    DescriptorBuilder::begin(layoutCache, allocator)
        .bind_image(texture._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0)
        .layout(setLayout)
        .build(descriptor, setLayout, poolSizes);

    PipelineBuilder pipelineBuilder = PipelineBuilder(window, device, renderPass);
    std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> modules = {
            {VK_SHADER_STAGE_VERTEX_BIT, "../src/shaders/env_map/converter.vert.spv"},
            {VK_SHADER_STAGE_FRAGMENT_BIT, "../src/shaders/env_map/converter.frag.spv"},
    };

    std::shared_ptr<ShaderEffect> effect = pipelineBuilder.build_effect({setLayout}, {}, modules);
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

        VkRenderPassBeginInfo renderPassInfo = vkinit::renderpass_begin_info(renderPass._renderPass, extent, framebuffers[face]._frameBuffers[0]);
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
    // _frameBuffers.reset();
    for (int i = 0; i < 6; i++) {
        // destroy frame buffers
        vkDestroyImageView(device._logicalDevice, imagesViews[i], nullptr);
    }
}

void EnvMap::main() {
    // to do
}