#include "vk_shadow_map.h"
#include "core/vk_renderpass.h"
#include "core/vk_descriptor_allocator.h"
#include "core/vk_descriptor_cache.h"
#include "core/vk_descriptor_builder.h"
#include "core/vk_pipeline.h"
#include "core/manager/vk_material_manager.h"
#include "core/utilities/vk_initializers.h"
#include "core/utilities/vk_global.h"
#include <array>

ShadowMapping::ShadowMapping(Device &device) : _device(device), _offscreen_pass(RenderPass(device)) {
    this->_offscreen_pass = RenderPass(device);
    this->_offscreen_shadow = Texture();
}

ShadowMapping::~ShadowMapping() {
    this->_offscreen_shadow.destroy(_device);
    this->_offscreen_effect.reset();
    vkDestroyFramebuffer(_device._logicalDevice, _offscreen_framebuffer, nullptr); // todo use FrameBuffer class
}

void ShadowMapping::prepare_depth_map(Device& device, UploadContext& uploadContext) {
    VkFormat format = VK_FORMAT_D16_UNORM;
    // 1 - Render to depth map
    // Prepare image
    Texture outTexture{};
    outTexture._width = SHADOW_WIDTH;
    outTexture._height = SHADOW_HEIGHT;

    VkExtent3D imageExtent;
    imageExtent.width = SHADOW_WIDTH;
    imageExtent.height = SHADOW_HEIGHT;
    imageExtent.depth = 1;

    VkImageCreateInfo imgInfo = vkinit::image_create_info(format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, imageExtent);
    imgInfo.arrayLayers = 1;
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imgAllocinfo = {};
    imgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    // imgAllocinfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; // VMA_MEMORY_USAGE_GPU_ONLY (deprecated);
    vmaCreateImage(device._allocator, &imgInfo, &imgAllocinfo, &outTexture._image, &outTexture._allocation, nullptr);

    VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(format, outTexture._image, VK_IMAGE_ASPECT_DEPTH_BIT);
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vkCreateImageView(device._logicalDevice, &imageViewInfo, nullptr, &outTexture._imageView);

    VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    vkCreateSampler(device._logicalDevice, &samplerInfo, nullptr, &outTexture._sampler);

//    CommandBuffer::immediate_submit(device, uploadContext, [&](VkCommandBuffer cmd) {
//        VkImageSubresourceRange range;
//        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//        range.baseMipLevel = 0;
//        range.levelCount = 1;
//        range.baseArrayLayer = 0;
//        range.layerCount = 6;
//
//        VkImageMemoryBarrier imageBarrier = {};
//        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
//        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//        imageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
//        imageBarrier.image = outTexture._image;
//        imageBarrier.subresourceRange = range;
//        imageBarrier.srcAccessMask = 0;
//        imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
//        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
//
//        outTexture._imageLayout = VK_IMAGE_LAYOUT_GENERAL;
//
//        outTexture.updateDescriptor();
//    });

    // === Prepare render pass ===

    this->_offscreen_pass = RenderPass(device);
    RenderPass::Attachment depth = this->_offscreen_pass.depth(format);
    depth.ref.attachment = 0;
    depth.description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    std::vector<VkSubpassDependency> dependencies;
    dependencies.reserve(2);
    dependencies.push_back({VK_SUBPASS_EXTERNAL, 0,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT});
    dependencies.push_back({0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT});
    std::vector<VkAttachmentReference> references = {};
    VkSubpassDescription subpass = this->_offscreen_pass.subpass_description(references, &depth.ref);
    std::vector<VkAttachmentDescription> attachments = {depth.description};

    this->_offscreen_pass.init(attachments, dependencies, subpass);

    // Prepare framebuffer
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext = nullptr;
    framebufferInfo.renderPass = this->_offscreen_pass._renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &outTexture._imageView;
    framebufferInfo.width = SHADOW_WIDTH;
    framebufferInfo.height = SHADOW_HEIGHT;
    framebufferInfo.layers = 1;

    VkFramebuffer framebuffer {};
    VK_CHECK(vkCreateFramebuffer(device._logicalDevice, &framebufferInfo, nullptr, &_offscreen_framebuffer));

    // this->_offscreen_pass = renderPass;
    this->_offscreen_shadow = outTexture;
    // this->_offscreen_framebuffer = framebuffer;

    // return { outTexture, framebuffer, renderPass};
}

void ShadowMapping::allocate_buffers(Device& device) {
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        g_frames[i].offscreenBuffer = Buffer::create_buffer(device, sizeof(GPUDepthData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU); // memoryUsage type deprecated
    }
}

void ShadowMapping::setup_descriptors(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout) { // ShadowMapping::ImageMap shadowMap,
//    DescriptorLayoutCache layoutCache = DescriptorLayoutCache(device); // Maybe we don't need standalone layout manager?
//    DescriptorAllocator allocator = DescriptorAllocator(device); // Re-use the main descriptor allocator (for now), or handle it somewhere else

    std::vector<VkDescriptorPoolSize> offscreenPoolSizes = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
    };

//    Texture inTexture = shadowMap.texture;
//    std::vector<VkDescriptorPoolSize> envPoolSizes = {
//            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
//            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
//            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
//            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
//            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 }
//    };

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        // 1. Offscreen descriptor set

        VkDescriptorBufferInfo offscreenBInfo{};
        offscreenBInfo.buffer = g_frames[i].offscreenBuffer._buffer;
        offscreenBInfo.offset = 0;
        offscreenBInfo.range = sizeof(GPUDepthData);

        DescriptorBuilder::begin(layoutCache, allocator)
                .bind_buffer(offscreenBInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0)
                .layout(setLayout)
                .build(g_frames[i].offscreenDescriptor, setLayout, offscreenPoolSizes);

        // 2. Scene descriptor set
//        DescriptorBuilder::begin(layoutCache, allocator)
//                .bind_buffer(offscreenBInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0)
//                .bind_image(inTexture._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
//                .layout(setLayout)
//                .build(g_frames[i].environmentDescriptor, setLayout, offscreenPoolSizes);

//        DescriptorBuilder::begin(layoutCache, allocator)
//                .bind_buffer(camBInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
//                .bind_buffer(lightingBInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1)
//                .bind_image(_skybox->_environment._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2)
//                .bind_image(_skybox->_prefilter._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3)
//                .bind_image(_skybox->_brdf._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4)
//                .bind_image()
//                .layout(_descriptorSetLayouts.environment)
//                .build(g_frames[i].environmentDescriptor, _descriptorSetLayouts.environment, poolSizes);
    }

}

void ShadowMapping::render_pass(RenderPass& renderPass, VkFramebuffer& framebuffer, CommandBuffer& cmd, VkDescriptorSet& descriptor, std::shared_ptr<Material> offscreenPass) {

    VkClearValue clearValue;
    clearValue.depthStencil = {1.0f, 0};

    VkExtent2D extent;
    extent.width = static_cast<uint32_t>(SHADOW_WIDTH);
    extent.height = static_cast<uint32_t>(SHADOW_HEIGHT);

    VkRenderPassBeginInfo renderPassInfo = vkinit::renderpass_begin_info(renderPass._renderPass, extent, framebuffer);
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;

    VkViewport viewport = vkinit::get_viewport((float) SHADOW_WIDTH, (float) SHADOW_HEIGHT);
    vkCmdSetViewport(cmd._commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = vkinit::get_scissor((float) SHADOW_WIDTH, (float) SHADOW_HEIGHT);
    vkCmdSetScissor(cmd._commandBuffer, 0, 1, &scissor);

    vkCmdBeginRenderPass(cmd._commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    {
        vkCmdSetDepthBias(cmd._commandBuffer, 1.25f, 0.0f, 1.75f);

        vkCmdBindPipeline(cmd._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, offscreenPass->pipeline);
        vkCmdBindDescriptorSets(cmd._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, offscreenPass->pipelineLayout, 0,1, &descriptor, 0, nullptr);
        // todo draw scene

        // cube->draw(cmd._commandBuffer, converterPass->pipelineLayout, 0, true);

    }
    vkCmdEndRenderPass(cmd._commandBuffer);
}

std::shared_ptr<Material> ShadowMapping::setup_offscreen_pipeline(Device& device, MaterialManager& materialManager, std::vector<VkDescriptorSetLayout> setLayouts, RenderPass& renderPass) {
    GraphicPipeline offscreenPipeline = GraphicPipeline(device, this->_offscreen_pass);
    offscreenPipeline._colorBlending.attachmentCount = 0;
    offscreenPipeline._colorBlending.pAttachments = nullptr;
    offscreenPipeline._rasterizer.depthBiasEnable = VK_TRUE;
    offscreenPipeline._dynamicStateEnables.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);

    std::vector<std::pair<ShaderType, const char*>> offscreen_module {
            {ShaderType::VERTEX, "../src/shaders/shadow_map/offscreen.vert.spv"},
    };

    std::vector<PushConstant> constants {
            {2 * sizeof(glm::mat4), ShaderType::VERTEX},
    };

    materialManager._pipelineBuilder = &offscreenPipeline;
    this->_offscreen_effect = materialManager.create_material("offscreen", setLayouts, constants, offscreen_module);

    return this->_offscreen_effect;
}

std::shared_ptr<Material> ShadowMapping::setup_scene_pipeline(Device& device, MaterialManager& materialManager, std::vector<VkDescriptorSetLayout> setLayouts, RenderPass& renderPass) {
    GraphicPipeline scenePipeline = GraphicPipeline(device, renderPass);

    std::vector<std::pair<ShaderType, const char*>> shadow_modules {
            {ShaderType::VERTEX, "../src/shaders/shadow_map/scene.vert.spv"},
            {ShaderType::FRAGMENT, "../src/shaders/shadow_map/scene.frag.spv"},
    };

    std::vector<PushConstant> constants {
            {2 * sizeof(glm::mat4), ShaderType::VERTEX},
    };

    materialManager._pipelineBuilder = &scenePipeline;
    std::shared_ptr<Material> shadowEffect = materialManager.create_material("shadow", setLayouts, constants, shadow_modules);

    return shadowEffect;
}
