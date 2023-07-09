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

Atmosphere::Atmosphere(Device &device, UploadContext& uploadContext) : _device(device), _uploadContext(uploadContext) {
    this->_transmittanceLUT = this->compute_transmittance(_device, _uploadContext);
}

Atmosphere::~Atmosphere() {
    this->_transmittanceLUT.destroy(_device);
    vkDestroyFramebuffer(_device._logicalDevice, _transmittanceFramebuffer, nullptr);
}

void Atmosphere::setup_descriptors(DescriptorLayoutCache &layoutCache, DescriptorAllocator &allocator, VkDescriptorSetLayout &setLayout) {
    std::vector<VkDescriptorPoolSize> offscreenPoolSizes = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
    };
}

Texture Atmosphere::compute_transmittance(Device& device, UploadContext& uploadContext) {
    Texture outTexture{};
    VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
    outTexture._width = Atmosphere::TRANSMITTANCE_WIDTH;
    outTexture._height = Atmosphere::TRANSMITTANCE_HEIGHT;

    VkExtent3D imageExtent { TRANSMITTANCE_WIDTH, TRANSMITTANCE_HEIGHT, 1 };
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
    framebufferInfo.width = TRANSMITTANCE_WIDTH;
    framebufferInfo.height = TRANSMITTANCE_HEIGHT;
    framebufferInfo.layers = 1;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &outTexture._imageView;
    VK_CHECK(vkCreateFramebuffer(device._logicalDevice, &framebufferInfo, nullptr, &_transmittanceFramebuffer));

    std::vector<VkDescriptorPoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}};

    VkDescriptorSet descriptor;
    VkDescriptorSetLayout setLayout;
    DescriptorLayoutCache layoutCache = DescriptorLayoutCache(device);
    DescriptorAllocator allocator = DescriptorAllocator(device);
    DescriptorBuilder::begin(layoutCache, allocator).layout(setLayout).build(descriptor, setLayout, poolSizes);

    // Build pipeline
    GraphicPipeline pipelineBuilder = GraphicPipeline(device, renderPass);

    std::vector<std::pair<ShaderType, const char*>> transmittance_module {
            {ShaderType::VERTEX, "../src/shaders/atmosphere/quad.vert.spv"},
            {ShaderType::FRAGMENT, "../src/shaders/atmosphere/transmittance.frag.spv"},
    };

    MaterialManager materialManager = MaterialManager(&device, &pipelineBuilder);
    std::shared_ptr<ShaderPass> transmittancePass = materialManager.create_material("transmittance", {setLayout}, {}, transmittance_module);

    CommandPool commandPool = CommandPool(device); // can use graphic queue for compute work
    CommandBuffer commandBuffer =  CommandBuffer(device, commandPool);

    VkExtent2D extent;
    extent.width = static_cast<uint32_t>(Atmosphere::TRANSMITTANCE_WIDTH);
    extent.height = static_cast<uint32_t>(Atmosphere::TRANSMITTANCE_HEIGHT);

    VkRenderPassBeginInfo offscreenPassInfo = vkinit::renderpass_begin_info(renderPass._renderPass, extent, _transmittanceFramebuffer);
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
            VkViewport viewport = vkinit::get_viewport((float) Atmosphere::TRANSMITTANCE_WIDTH, (float) Atmosphere::TRANSMITTANCE_HEIGHT);
            vkCmdSetViewport(commandBuffer._commandBuffer, 0, 1, &viewport);

            VkRect2D scissor = vkinit::get_scissor((float) Atmosphere::TRANSMITTANCE_WIDTH, (float) Atmosphere::TRANSMITTANCE_HEIGHT);
            vkCmdSetScissor(commandBuffer._commandBuffer, 0, 1, &scissor);

            // vkCmdBindDescriptorSets(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, transmittancePass->pipelineLayout, 0, 1, &descriptor, 0, nullptr);
            vkCmdBindPipeline(commandBuffer._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, transmittancePass->pipeline);
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

    return outTexture;
}

void Atmosphere::run_debug(FrameData& frame) {
//    vkCmdBindDescriptorSets(frame._commandBuffer->_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_debug_effect->pipelineLayout, 0, 1, &frame.debugDescriptor, 0, nullptr);
//    vkCmdBindPipeline(frame._commandBuffer->_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_debug_effect->pipeline);
//    vkCmdDraw(frame._commandBuffer->_commandBuffer, 3, 1, 0, 0);
}