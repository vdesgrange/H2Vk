/*
*  H2Vk - Shadow mapping
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_shadow_map.h"
#include "core/vk_renderpass.h"
#include "core/vk_descriptor_allocator.h"
#include "core/vk_descriptor_cache.h"
#include "core/vk_descriptor_builder.h"
#include "core/vk_pipeline.h"
#include "core/vk_framebuffers.h"
#include "core/manager/vk_material_manager.h"
#include "components/camera/vk_camera.h"
#include "components/lighting/vk_light.h"
#include "components/model/vk_model.h"
#include "core/utilities/vk_initializers.h"
#include "core/utilities/vk_global.h"


#include <array>

ShadowMapping::ShadowMapping(Device &device) : _device(device), _offscreen_pass(RenderPass(device)) {
    this->prepare_offscreen_pass(device);
    this->_offscreen_shadow = Texture();
}

ShadowMapping::~ShadowMapping() {
    this->_offscreen_shadow.destroy(_device);
    this->_offscreen_effect.reset();
    this->_debug_effect.reset();

    for (auto& imageview: this->_offscreen_imageview) {
        vkDestroyImageView(_device._logicalDevice, imageview, nullptr);
    }
}

void ShadowMapping::allocate_buffers(Device& device) {
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        const size_t depthBufferSize =  helper::pad_uniform_buffer_size(device, sizeof(GPUShadowData));
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

void ShadowMapping::prepare_depth_map(Device& device, UploadContext& uploadContext, LightingManager& lighting) {
    uint num_lights = 2 * MAX_LIGHT; // lighting._entities.size();
    this->_offscreen_shadow._descriptor = {};
    this->_offscreen_shadow._width = SHADOW_WIDTH;
    this->_offscreen_shadow._height = SHADOW_HEIGHT;
    this->_offscreen_framebuffer.reserve(num_lights);
    this->_offscreen_imageview.reserve(num_lights);

    for (int i=0; i < num_lights; i++) {
        this->_offscreen_imageview.push_back({});
    }

    // Prepare image
    VkExtent3D imageExtent { SHADOW_WIDTH, SHADOW_HEIGHT, 1 };
    VkImageCreateInfo imgInfo = vkinit::image_create_info(DEPTH_FORMAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, imageExtent);
    imgInfo.arrayLayers = num_lights;

    VmaAllocationCreateInfo imgAllocinfo = {};
    imgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImage(device._allocator, &imgInfo, &imgAllocinfo, &this->_offscreen_shadow._image, &this->_offscreen_shadow._allocation, nullptr);

    VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(DEPTH_FORMAT, this->_offscreen_shadow._image, VK_IMAGE_ASPECT_DEPTH_BIT);
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    imageViewInfo.subresourceRange.layerCount = num_lights;
    vkCreateImageView(device._logicalDevice, &imageViewInfo, nullptr, &this->_offscreen_shadow._imageView);

    CommandBuffer::immediate_submit(device, uploadContext, [&](VkCommandBuffer cmd) {
        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = num_lights;

        VkImageMemoryBarrier imageBarrier = {};
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        imageBarrier.image = this->_offscreen_shadow._image;
        imageBarrier.subresourceRange = range;
        imageBarrier.srcAccessMask = 0;
        imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
    });

    VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    samplerInfo.maxLod = 1.0f;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    vkCreateSampler(device._logicalDevice, &samplerInfo, nullptr, &this->_offscreen_shadow._sampler);

    this->_offscreen_shadow._imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    this->_offscreen_shadow.updateDescriptor();

    for (int l = 0; l < num_lights; l++) {
        // Create image view
        VkImageViewCreateInfo info = vkinit::imageview_create_info(DEPTH_FORMAT, this->_offscreen_shadow._image, VK_IMAGE_ASPECT_DEPTH_BIT);
        info.subresourceRange.baseArrayLayer = l; // sampler 2D array layer
        info.subresourceRange.layerCount = 1;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        vkCreateImageView(device._logicalDevice, &info, nullptr, &_offscreen_imageview[l]);

        std::vector<VkImageView> attachments = {_offscreen_imageview[l]};
        this->_offscreen_framebuffer.emplace_back(FrameBuffer(_offscreen_pass, attachments, SHADOW_WIDTH, SHADOW_HEIGHT, 1));
    }
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
        offscreenBInfo.range = sizeof(GPUShadowData);

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
            {sizeof(glm::mat4) + 2 * sizeof(uint32_t), ShaderType::VERTEX},
    };

    materialManager._pipelineBuilder = &offscreenPipeline;
    this->_offscreen_effect = materialManager.create_material("offscreen", setLayouts, constants, offscreen_module);

    GraphicPipeline debugPipeline = GraphicPipeline(device, renderPass);
    debugPipeline._vertexInputInfo =  vkinit::vertex_input_state_create_info();
    debugPipeline._dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    std::vector<std::pair<ShaderType, const char*>> debug_module {
            {ShaderType::VERTEX, "../src/shaders/shadow_map/depth_debug_quad.vert.spv"},
            {ShaderType::FRAGMENT, "../src/shaders/shadow_map/depth_debug_quad.frag.spv"},
    };

    materialManager._pipelineBuilder = &debugPipeline;
    this->_debug_effect = materialManager.create_material("debug", setLayouts, constants, debug_module);
}

void ShadowMapping::run_offscreen_pass(FrameData& frame, Renderables& entities, LightingManager& lighting) {
    VkCommandBuffer& cmd = frame._commandBuffer->_commandBuffer;

    std::array<VkClearValue, 1> clearValues{};
    clearValues[0].depthStencil = {1.0f, 0};

    VkExtent2D extent;
    extent.width = static_cast<uint32_t>(ShadowMapping::SHADOW_WIDTH);
    extent.height = static_cast<uint32_t>(ShadowMapping::SHADOW_HEIGHT);

    VkViewport viewport = vkinit::get_viewport((float) ShadowMapping::SHADOW_WIDTH, (float) ShadowMapping::SHADOW_HEIGHT);
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = vkinit::get_scissor((float) ShadowMapping::SHADOW_WIDTH, (float) ShadowMapping::SHADOW_HEIGHT);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdSetDepthBias(cmd, 1.25f, 0.0f, 1.75f);

    uint32_t lightIndex = 0;
    for (auto const& l : lighting._entities) {
        std::shared_ptr<Light> light = std::static_pointer_cast<Light>(l.second);

        uint32_t lightType = light->get_type();

        VkRenderPassBeginInfo offscreenPassInfo = vkinit::renderpass_begin_info(this->_offscreen_pass._renderPass, extent, this->_offscreen_framebuffer.at(lightIndex)._frameBuffer);
        offscreenPassInfo.clearValueCount = clearValues.size();
        offscreenPassInfo.pClearValues = clearValues.data();

        uint32_t shadowPushConstant[2];
        shadowPushConstant[0] = lightIndex;
        shadowPushConstant[1] = lightType;

        vkCmdBeginRenderPass(cmd, &offscreenPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_offscreen_effect->pipeline);

            uint32_t i = 0;
            std::shared_ptr<Model> lastModel = nullptr;
            vkCmdPushConstants(cmd, this->_offscreen_effect->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,sizeof(glm::mat4), 2 * sizeof(uint32_t), &shadowPushConstant);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_offscreen_effect->pipelineLayout,0, 1, &frame.offscreenDescriptor, 0, nullptr);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_offscreen_effect->pipelineLayout, 1, 1, &frame.objectDescriptor, 0, nullptr);

            for (auto const &object: entities) {
                this->draw(*object.model, cmd, object.material->pipelineLayout, i,object.model.get() != lastModel.get());
                lastModel = object.model;
                i++;
            }

        }
        vkCmdEndRenderPass(cmd);

        lightIndex++;
    }
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

GPUShadowData ShadowMapping::gpu_format(const LightingManager* lightingManager, Camera* camera) {
    GPUShadowData offscreenData{};
    uint32_t  dirLightCount = 0;
    uint32_t  spotLightCount = 0;
    for (auto& l : lightingManager->_entities) {
        std::shared_ptr<Light> light = std::static_pointer_cast<Light>(l.second);

        if (light->get_type() == Light::Type::DIRECTIONAL) {
            glm::vec3 eye = -light->get_rotation();
            glm::vec3 up = glm::dot(glm::vec3(0.0f, 1.0f, 0.0f), eye) == (glm::length(glm::vec3(0.0f, 1.0f, 0.0f)) * glm::length(eye)) ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0);
            glm::mat4 depthProjectionMatrix = glm::ortho<float>(-10,10,-10,10,-10.0f,50.0f); // entire scene must be visible
            glm::mat4 depthViewMatrix = glm::lookAt(eye + camera->get_position_vector() + camera->get_rotation_vector() * 10.0f, camera->get_position_vector() + camera->get_rotation_vector() * 10.0f, up);
            glm::mat4 depthModelMatrix = glm::mat4(1.0f);
            offscreenData.directionalMVP[dirLightCount] = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;
            dirLightCount++;
        }

        if (light->get_type() == Light::Type::SPOT) {
            glm::mat4 depthProjectionMatrix = glm::perspective(glm::radians(45.0f), (float)ShadowMapping::SHADOW_WIDTH / (float)ShadowMapping::SHADOW_HEIGHT, 0.01f,100.0f); // change zNear/zFar
            glm::mat4 depthViewMatrix = glm::lookAt(glm::vec3(light->get_position()), glm::vec3(light->get_position() + light->get_rotation()), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 depthModelMatrix = glm::mat4(1.0f);
            offscreenData.spotMVP[spotLightCount] = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;
            spotLightCount++;
        }

    }
    offscreenData.num_lights = glm::vec2(spotLightCount, dirLightCount);

    return offscreenData;
}