#include "vk_shadow_map.h"
#include "core/vk_renderpass.h"
#include "core/vk_descriptor_allocator.h"
#include "core/vk_descriptor_cache.h"
#include "core/vk_descriptor_builder.h"
#include "core/vk_pipeline.h"
#include "core/manager/vk_material_manager.h"
#include "core/camera/vk_camera.h"
#include "core/lighting/vk_light.h"
#include "core/model/vk_model.h"
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
    this->_debug_effect.reset();

    vkDestroyFramebuffer(_device._logicalDevice, _offscreen_framebuffer, nullptr); // todo use FrameBuffer class
}

void ShadowMapping::allocate_buffers(Device& device) {
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        // const size_t depthBufferSize = FRAME_OVERLAP * helper::pad_uniform_buffer_size(device, sizeof(GPUDepthData));
        const size_t depthBufferSize =  helper::pad_uniform_buffer_size(device, sizeof(GPUDepthData)); // It doesn't work while we have 2 g_frames. Why?
        g_frames[i].offscreenBuffer = Buffer::create_buffer(device, depthBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU); // memoryUsage type deprecated
    }
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
}

void ShadowMapping::prepare_depth_map(Device& device, UploadContext& uploadContext, RenderPass& renderPass) {
    this->_offscreen_shadow._descriptor = {};
    this->_offscreen_shadow._width = SHADOW_WIDTH;
    this->_offscreen_shadow._height = SHADOW_HEIGHT;

    // Prepare image
    VkExtent3D imageExtent { SHADOW_WIDTH, SHADOW_HEIGHT, 1 };
    VkImageCreateInfo imgInfo = vkinit::image_create_info(DEPTH_FORMAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, imageExtent);

    VmaAllocationCreateInfo imgAllocinfo = {};
    imgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImage(device._allocator, &imgInfo, &imgAllocinfo, &this->_offscreen_shadow._image, &this->_offscreen_shadow._allocation, nullptr);

    VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(DEPTH_FORMAT, this->_offscreen_shadow._image, VK_IMAGE_ASPECT_DEPTH_BIT);
    vkCreateImageView(device._logicalDevice, &imageViewInfo, nullptr, &this->_offscreen_shadow._imageView);

    VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    samplerInfo.maxLod = 1.0f;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    vkCreateSampler(device._logicalDevice, &samplerInfo, nullptr, &this->_offscreen_shadow._sampler);

    this->_offscreen_shadow._imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    this->_offscreen_shadow.updateDescriptor();

    // Prepare offscreen render pass
    this->prepare_offscreen_pass(device);

    // Prepare framebuffer
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext = nullptr;
    framebufferInfo.renderPass = this->_offscreen_pass._renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &this->_offscreen_shadow._imageView;
    framebufferInfo.width = SHADOW_WIDTH;
    framebufferInfo.height = SHADOW_HEIGHT;
    framebufferInfo.layers = 1;

    VK_CHECK(vkCreateFramebuffer(device._logicalDevice, &framebufferInfo, nullptr, &_offscreen_framebuffer));
}

void ShadowMapping::setup_descriptors(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout) {
    std::vector<VkDescriptorPoolSize> offscreenPoolSizes = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
    };

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VkDescriptorBufferInfo offscreenBInfo{};
        offscreenBInfo.buffer = g_frames[i].offscreenBuffer._buffer;
        offscreenBInfo.offset = 0;
        offscreenBInfo.range = sizeof(GPUDepthData);

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
    GraphicPipeline offscreenPipeline = GraphicPipeline(device,this->_offscreen_pass);
    offscreenPipeline._dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS };
    offscreenPipeline._colorBlending.attachmentCount = 0;
    offscreenPipeline._colorBlending.pAttachments = nullptr;
    offscreenPipeline._rasterizer.depthBiasEnable = VK_TRUE;

    std::vector<std::pair<ShaderType, const char*>> offscreen_module {
            {ShaderType::VERTEX, "../src/shaders/shadow_map/offscreen.vert.spv"},
    };

    std::vector<PushConstant> constants {
            {sizeof(glm::mat4), ShaderType::VERTEX},
    };

    materialManager._pipelineBuilder = &offscreenPipeline;
    this->_offscreen_effect = materialManager.create_material("offscreen", setLayouts, constants, offscreen_module);

    GraphicPipeline debugPipeline = GraphicPipeline(device, renderPass);
    debugPipeline._vertexInputInfo =  vkinit::vertex_input_state_create_info();
    debugPipeline._dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    std::vector<std::pair<ShaderType, const char*>> debug_module {
            {ShaderType::VERTEX, "../src/shaders/shadow_map/quad.vert.spv"},
            {ShaderType::FRAGMENT, "../src/shaders/shadow_map/quad.frag.spv"},
    };

    materialManager._pipelineBuilder = &debugPipeline;
    this->_debug_effect = materialManager.create_material("debug", setLayouts, constants, debug_module);
}

void ShadowMapping::run_offscreen_pass(FrameData& frame, Renderables& entities) {
    VkCommandBuffer& cmd = frame._commandBuffer->_commandBuffer;

    std::array<VkClearValue, 1> clearValues{};
    clearValues[0].depthStencil = {1.0f, 0};

    VkExtent2D extent;
    extent.width = static_cast<uint32_t>(ShadowMapping::SHADOW_WIDTH);
    extent.height = static_cast<uint32_t>(ShadowMapping::SHADOW_HEIGHT);

    VkRenderPassBeginInfo offscreenPassInfo = vkinit::renderpass_begin_info(this->_offscreen_pass._renderPass, extent, this->_offscreen_framebuffer);
    offscreenPassInfo.clearValueCount = clearValues.size();
    offscreenPassInfo.pClearValues = clearValues.data();

    VkViewport viewport = vkinit::get_viewport((float) ShadowMapping::SHADOW_WIDTH, (float) ShadowMapping::SHADOW_HEIGHT);
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = vkinit::get_scissor((float) ShadowMapping::SHADOW_WIDTH, (float) ShadowMapping::SHADOW_HEIGHT);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdSetDepthBias(cmd, 1.25f, 0.0f, 1.75f);

    vkCmdBeginRenderPass(cmd, &offscreenPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_offscreen_effect->pipeline);
//            vkCmdBindDescriptorSets(frame._commandBuffer->_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,_shadow->_offscreen_effect->pipelineLayout, 0, 1,&get_current_frame().offscreenDescriptor, 0, nullptr);
//            vkCmdDraw(frame._commandBuffer->_commandBuffer, 3, 1, 0, 0);

        uint32_t i = 0;
        std::shared_ptr<Model> lastModel = nullptr;
        for (auto const& object : entities) {
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_offscreen_effect->pipelineLayout, 0, 1, &frame.offscreenDescriptor, 0, nullptr);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_offscreen_effect->pipelineLayout, 1, 1, &frame.objectDescriptor, 0,nullptr);
            this->draw( *object.model, cmd, object.material->pipelineLayout, i, object.model.get() != lastModel.get());
            lastModel = object.model;
            i++;
        }
    }
    vkCmdEndRenderPass(cmd);
}

void ShadowMapping::run_debug(FrameData& frame) {
    if (debug) {
        vkCmdBindDescriptorSets(frame._commandBuffer->_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_debug_effect->pipelineLayout, 0, 1, &frame.debugDescriptor, 0, nullptr);
        vkCmdBindPipeline(frame._commandBuffer->_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_debug_effect->pipeline);
        vkCmdDraw(frame._commandBuffer->_commandBuffer, 3, 1, 0, 0);
    }
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