#include "vk_shadow_map.h"
#include "core/vk_renderpass.h"
#include "core/vk_descriptor_allocator.h"
#include "core/vk_descriptor_cache.h"
#include "core/vk_descriptor_builder.h"
#include "core/vk_pipeline.h"
#include "core/manager/vk_material_manager.h"
#include "core/utilities/vk_initializers.h"
#include "core/utilities/vk_global.h"
#include "core/camera/vk_camera.h"
#include "core/lighting/vk_light.h"
#include "core/model/vk_model.h"
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

void ShadowMapping::prepare_offscreen_pass(Device& device) {
    this->_offscreen_pass = RenderPass(device);
    RenderPass::Attachment depth = this->_offscreen_pass.depth(DEPTH_FORMAT);
    depth.description.format = DEPTH_FORMAT;
    depth.description.flags = 0;
    depth.description.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    depth.ref.attachment = 0;
    depth.ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::vector<VkSubpassDependency> dependencies;
    dependencies.reserve(2);
    dependencies.push_back({VK_SUBPASS_EXTERNAL, 0,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT});
    dependencies.push_back({0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT});

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;
    subpass.pColorAttachments = nullptr;
    subpass.pDepthStencilAttachment = &depth.ref;
    std::vector<VkAttachmentDescription> attachments = {depth.description};

    this->_offscreen_pass.init(attachments, dependencies, subpass);

//    this->_offscreen_pass = RenderPass(device);
//    RenderPass::Attachment color = this->_offscreen_pass.color(format);
//    RenderPass::Attachment depth = this->_offscreen_pass.depth(VK_FORMAT_D32_SFLOAT);
//    color.description.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
//    color.ref.layout = VK_IMAGE_LAYOUT_GENERAL;
//    std::vector<VkAttachmentReference> references = {color.ref};
//    std::vector<VkAttachmentDescription> attachments = {color.description, depth.description};
//    std::vector<VkSubpassDependency> dependencies = {color.dependency, depth.dependency};
//    VkSubpassDescription subpass = this->_offscreen_pass.subpass_description(references, &depth.ref);
//
//    this->_offscreen_pass.init(attachments, dependencies, subpass);

}

void ShadowMapping::prepare_depth_map(Device& device, UploadContext& uploadContext, RenderPass& renderPass) {
    this->_offscreen_shadow._descriptor = {};
    this->_offscreen_shadow._width = SHADOW_WIDTH;
    this->_offscreen_shadow._height = SHADOW_HEIGHT;

    VkExtent3D imageExtent;
    imageExtent.width = SHADOW_WIDTH;
    imageExtent.height = SHADOW_HEIGHT;
    imageExtent.depth = 1;

    // VkImageCreateInfo imgInfo = vkinit::image_create_info(format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
    VkImageCreateInfo imgInfo = vkinit::image_create_info(DEPTH_FORMAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, imageExtent);
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imgAllocinfo = {};
    imgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImage(device._allocator, &imgInfo, &imgAllocinfo, &this->_offscreen_shadow._image, &this->_offscreen_shadow._allocation, nullptr);

    VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(DEPTH_FORMAT, this->_offscreen_shadow._image, VK_IMAGE_ASPECT_DEPTH_BIT); // VK_IMAGE_ASPECT_COLOR_BIT
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vkCreateImageView(device._logicalDevice, &imageViewInfo, nullptr, &this->_offscreen_shadow._imageView);

    VkSamplerCreateInfo samplerInfo {}; // = vkinit::sampler_create_info(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.pNext = nullptr;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = samplerInfo.addressModeU;
    samplerInfo.addressModeW = samplerInfo.addressModeU;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    vkCreateSampler(device._logicalDevice, &samplerInfo, nullptr, &this->_offscreen_shadow._sampler);

//    CommandBuffer::immediate_submit(device, uploadContext, [&](VkCommandBuffer cmd) {
//        VkImageSubresourceRange range;
//        range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; // VK_IMAGE_ASPECT_COLOR_BIT; // VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
//        range.baseMipLevel = 0;
//        range.levelCount = 1;
//        range.baseArrayLayer = 0;
//        range.layerCount = 1;
//
//        VkImageMemoryBarrier imageBarrier = {};
//        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
//        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//        imageBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//        imageBarrier.image = this->_offscreen_shadow._image;
//        imageBarrier.subresourceRange = range;
//        imageBarrier.srcAccessMask = 0;
//        imageBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; // VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
//        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
//
//        this->_offscreen_shadow._imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // VK_IMAGE_LAYOUT_GENERAL; // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//        this->_offscreen_shadow.updateDescriptor();
//    });

    this->_offscreen_shadow._imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; // VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    this->_offscreen_shadow.updateDescriptor();

    // === Prepare render pass ===
    this->prepare_offscreen_pass(device);

//    VkImageCreateInfo imageInfo = vkinit::image_create_info(VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,  {SHADOW_WIDTH,SHADOW_HEIGHT,1});
//    VmaAllocationCreateInfo imageAllocInfo{};
//    imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
//    imageAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
//    vmaCreateImage(device._allocator, &imageInfo, &imageAllocInfo, &_depthImage, &_depthAllocation, nullptr);
//    VkImageViewCreateInfo viewInfo = vkinit::imageview_create_info(_depthFormat, _depthImage, VK_IMAGE_ASPECT_DEPTH_BIT);
//    VK_CHECK(vkCreateImageView(device._logicalDevice, &viewInfo, nullptr, &_depthImageView));

    // std::array<VkImageView, 2> attachments = {this->_offscreen_shadow._imageView, this->_depthImageView};
    std::array<VkImageView, 1> attachments = {this->_offscreen_shadow._imageView};

    // Prepare framebuffer
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext = nullptr;
    framebufferInfo.renderPass = this->_offscreen_pass._renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = SHADOW_WIDTH;
    framebufferInfo.height = SHADOW_HEIGHT;
    framebufferInfo.layers = 1;

    VK_CHECK(vkCreateFramebuffer(device._logicalDevice, &framebufferInfo, nullptr, &_offscreen_framebuffer));
}

void ShadowMapping::allocate_buffers(Device& device) {
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        g_frames[i].offscreenBuffer = Buffer::create_buffer(device, sizeof(GPUDepthData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU); // memoryUsage type deprecated
    }
}

void ShadowMapping::setup_descriptors(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout) { // ShadowMapping::ImageMap shadowMap,
    std::vector<VkDescriptorPoolSize> offscreenPoolSizes = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
    };

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VkDescriptorBufferInfo offscreenBInfo{};
        offscreenBInfo.buffer = g_frames[i].offscreenBuffer._buffer;
        offscreenBInfo.offset = 0;
        offscreenBInfo.range = sizeof(GPUDepthData);

//        Texture tmp = this->_offscreen_shadow;
//        tmp._imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//        tmp.updateDescriptor();

        this->_offscreen_shadow._descriptor.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        this->_offscreen_shadow.updateDescriptor();

        // 1. Offscreen descriptor set
        DescriptorBuilder::begin(layoutCache, allocator)
                .bind_buffer(offscreenBInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0)
                .bind_none(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  VK_SHADER_STAGE_FRAGMENT_BIT, 1) // useless
                .layout(setLayout)
                .build(g_frames[i].offscreenDescriptor, setLayout, offscreenPoolSizes);

        this->_offscreen_shadow._descriptor.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        this->_offscreen_shadow.updateDescriptor();

        // 2. Debug descriptor set
        DescriptorBuilder::begin(layoutCache, allocator)
                .bind_buffer(offscreenBInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0)
                .bind_image(_offscreen_shadow._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  VK_SHADER_STAGE_FRAGMENT_BIT, 1)
                .layout(setLayout)
                .build(g_frames[i].debugDescriptor, setLayout, offscreenPoolSizes);

    }

}

void ShadowMapping::setup_offscreen_pipeline(Device& device, MaterialManager& materialManager, std::vector<VkDescriptorSetLayout> setLayouts, RenderPass& renderPass) {
    GraphicPipeline offscreenPipeline = GraphicPipeline(device,this->_offscreen_pass); // this->_offscreen_pass);
//    offscreenPipeline._vertexInputInfo =  vkinit::vertex_input_state_create_info();
//    offscreenPipeline._rasterizer.cullMode = VK_CULL_MODE_NONE;
//    offscreenPipeline._dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    offscreenPipeline._colorBlending.attachmentCount = 0;
    offscreenPipeline._colorBlending.pAttachments = nullptr;
    offscreenPipeline._depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    offscreenPipeline._rasterizer.depthBiasEnable = VK_FALSE;
    offscreenPipeline._dynamicStateEnables.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);

    std::vector<std::pair<ShaderType, const char*>> offscreen_module {
            {ShaderType::VERTEX, "../src/shaders/shadow_map/offscreen.vert.spv"},
            // {ShaderType::FRAGMENT, "../src/shaders/shadow_map/offscreen.frag.spv"},
    };

    std::vector<PushConstant> constants {
            {sizeof(glm::mat4), ShaderType::VERTEX},
    };

    materialManager._pipelineBuilder = &offscreenPipeline;
    this->_offscreen_effect = materialManager.create_material("offscreen", setLayouts, constants, offscreen_module);

    GraphicPipeline debugPipeline = GraphicPipeline(device, renderPass);
    debugPipeline._vertexInputInfo =  vkinit::vertex_input_state_create_info();
    debugPipeline._rasterizer.cullMode = VK_CULL_MODE_NONE;
    debugPipeline._dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    std::vector<std::pair<ShaderType, const char*>> debug_module {
            {ShaderType::VERTEX, "../src/shaders/shadow_map/quad.vert.spv"},
            {ShaderType::FRAGMENT, "../src/shaders/shadow_map/quad.frag.spv"},
    };

    materialManager._pipelineBuilder = &debugPipeline;
    this->_debug_effect = materialManager.create_material("debug", setLayouts, constants, debug_module);
}

void ShadowMapping::setup_debug_pipeline(Device& device, MaterialManager& materialManager, std::vector<VkDescriptorSetLayout> setLayouts, RenderPass& renderPass) {

}

void ShadowMapping::draw(Model& model, VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint32_t instance, bool bind) {
    if (bind) {
        VkDeviceSize offsets[1] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model._vertexBuffer._buffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, model._indexBuffer.allocation._buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    for (auto& node : model._nodes) {
        draw_node(node, commandBuffer, pipelineLayout, instance);
    }
}

void ShadowMapping::draw_node(Node* node, VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint32_t instance) {
    if (!node->mesh.primitives.empty()) {
        glm::mat4 nodeMatrix = node->matrix;
        Node* parent = node->parent;
        while (parent) {
            nodeMatrix = parent->matrix * nodeMatrix;
            parent = parent->parent;
        }

        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &nodeMatrix);

        for (Primitive& primitive : node->mesh.primitives) {
            if (primitive.indexCount > 0) {
                vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, instance);
            }
        }
    }

    for (auto& child : node->children) {
        draw_node(child, commandBuffer, pipelineLayout, instance);
    }
}