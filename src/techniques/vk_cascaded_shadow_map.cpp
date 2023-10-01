/*
*  H2Vk - Cascaded shadow map
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_cascaded_shadow_map.h"
#include "core/utilities/vk_global.h"
#include "core/vk_renderpass.h"
#include "core/vk_shaders.h"
#include "core/vk_descriptor_builder.h"
#include "core/vk_pipeline.h"
#include "core/manager/vk_material_manager.h"
#include "components/lighting/vk_light.h"
#include "components/camera/vk_camera.h"
#include "core/utilities/vk_initializers.h"

CascadedShadow::CascadedShadow(Device& device) : _device(device), _depthPass(RenderPass(device)) {
    prepare_resources(device);
}

CascadedShadow::~CascadedShadow() {
    _depth.destroy(_device);
    _depthEffect.reset();
    _debugEffect.reset();

    for (auto& c : _cascades) {
        c.destroy(_device._logicalDevice);
    }
}

void CascadedShadow::prepare_resources(Device& device) {
    // === Prepare depth render pass ===
    _depthPass = RenderPass(device);

    RenderPass::Attachment depth = _depthPass.depth(DEPTH_FORMAT);
    depth.description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    depth.ref.attachment = 0;
    depth.ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::vector<VkAttachmentDescription> attachments{depth.description};
    std::vector<VkAttachmentReference> colorRef{};

    std::vector<VkSubpassDependency> dependencies{};
    dependencies.reserve(2);
    dependencies.push_back({VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT});
    dependencies.push_back({0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT});

    VkSubpassDescription subpass{}; // = _depthPass.subpass_description(colorRef, &depth.ref);
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;
    subpass.pDepthStencilAttachment = &depth.ref; // link depth attachment to sub-pass

    _depthPass.init(attachments, dependencies, subpass);

    // === Prepare depth map ===
    VkExtent3D extent { CascadedShadow::SHADOW_WIDTH, CascadedShadow::SHADOW_HEIGHT, 1};
    VkImageCreateInfo info = vkinit::image_create_info(CascadedShadow::DEPTH_FORMAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, extent);
    info.arrayLayers = CascadedShadow::COUNT;

    VmaAllocationCreateInfo alloc{};
    alloc.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImage(device._allocator, &info, &alloc, &_depth._image, &_depth._allocation, nullptr);

    VkImageViewCreateInfo viewInfo = vkinit::imageview_create_info(CascadedShadow::DEPTH_FORMAT, _depth._image, VK_IMAGE_ASPECT_DEPTH_BIT);
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    viewInfo.subresourceRange.layerCount = CascadedShadow::COUNT;
    vkCreateImageView(device._logicalDevice, &viewInfo, nullptr, &_depth._imageView);

    VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.maxLod = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    vkCreateSampler(device._logicalDevice, &samplerInfo, nullptr, &_depth._sampler);

    _depth._imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    _depth.updateDescriptor();

    for (uint8_t i = 0; i < CascadedShadow::COUNT; i++) {
        VkImageViewCreateInfo view = vkinit::imageview_create_info(CascadedShadow::DEPTH_FORMAT, _depth._image, VK_IMAGE_ASPECT_DEPTH_BIT);
        view.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        view.subresourceRange.baseArrayLayer = i;
        vkCreateImageView(device._logicalDevice, &view, nullptr, &_cascades[i]._view);

        std::vector<VkImageView> attachments = {_cascades[i]._view};
        _cascades[i]._framebuffer = std::make_unique<FrameBuffer>(_depthPass, attachments, CascadedShadow::SHADOW_WIDTH, CascadedShadow::SHADOW_HEIGHT, 1);
    }
}

void CascadedShadow::allocate_buffers(Device &device) {
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        const size_t size = helper::pad_uniform_buffer_size(device, sizeof(GPUCascadedShadowData));
        g_frames[i].cascadedOffscreenBuffer = Buffer::create_buffer(device, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    }
}

void CascadedShadow::setup_descriptors(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout) {
    std::vector<VkDescriptorPoolSize> offscreenSizes = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
    };

    for (uint32_t i = 0; i < CascadedShadow::COUNT; i++) {
        VkDescriptorBufferInfo info{};
        info.buffer = g_frames[0].cascadedOffscreenBuffer._buffer;
        info.offset = 0;
        info.range = sizeof(GPUCascadedShadowData);

        DescriptorBuilder::begin(layoutCache, allocator)
                .bind_buffer(info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0)
                .bind_image(_depth._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
                .layout(setLayout)
                .build(_cascades[i]._descriptor, setLayout, offscreenSizes);
    }

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        g_frames[i].offscreenDescriptor = VkDescriptorSet();

        VkDescriptorBufferInfo info{};
        info.buffer = g_frames[i].cascadedOffscreenBuffer._buffer;
        info.offset = 0;
        info.range = sizeof(GPUCascadedShadowData);

//        DescriptorBuilder::begin(layoutCache, allocator)
//            .bind_buffer(info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0)
//            .bind_none(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
//            .layout(setLayout)
            // .build(g_frames[i].cascadedOffscreenDescriptor, setLayout, offscreenSizes);

        DescriptorBuilder::begin(layoutCache, allocator)
                .bind_buffer(info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0)
                .bind_image(_depth._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
                .layout(setLayout)
                .build(g_frames[i].debugDescriptor, setLayout, offscreenSizes);
    }
}

void CascadedShadow::setup_pipelines(Device& device, MaterialManager &materialManager, std::vector<VkDescriptorSetLayout> setLayouts, RenderPass& renderPass) {
    for (const auto& l : setLayouts) {
        if (l == VK_NULL_HANDLE) {
            return;
        }
    }

    GraphicPipeline pipelineBuilder = GraphicPipeline(device, _depthPass);
    pipelineBuilder._dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    pipelineBuilder._rasterizer.cullMode = VK_CULL_MODE_NONE;
    pipelineBuilder._rasterizer.depthClampEnable = VK_TRUE;
    pipelineBuilder._rasterizer.depthBiasEnable = VK_FALSE;
    pipelineBuilder._colorBlending.attachmentCount = 0;
    pipelineBuilder._colorBlending.pAttachments = nullptr;
    pipelineBuilder._depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    std::vector<std::pair<ShaderType, const char*>> modules {
            {ShaderType::VERTEX, "../src/shaders/shadow_map/csm_offscreen.vert.spv"},
    };

    std::vector<PushConstant> constants {
        {sizeof(glm::mat4) + sizeof(int), ShaderType::VERTEX},
        {sizeof(Materials::Factors), ShaderType::FRAGMENT},
    };

    materialManager._pipelineBuilder = &pipelineBuilder;
    _depthEffect = materialManager.create_material("cascades", setLayouts, constants, modules);

    GraphicPipeline debugPipeline = GraphicPipeline(device, renderPass);
    debugPipeline._vertexInputInfo = vkinit::vertex_input_state_create_info();
    debugPipeline._rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;

    std::vector<std::pair<ShaderType, const char*>> debugMod {
            {ShaderType::VERTEX, "../src/shaders/shadow_map/csm_debug_quad.vert.spv"},
            {ShaderType::FRAGMENT, "../src/shaders/shadow_map/csm_debug_quad.frag.spv"},
    };

    std::vector<PushConstant> debugConst {
            {sizeof(glm::mat4) + sizeof(int), ShaderType::VERTEX},
    };

    materialManager._pipelineBuilder = &debugPipeline;
    _debugEffect = materialManager.create_material("debugCascades", setLayouts, debugConst, debugMod);

}

void CascadedShadow::compute_resources(FrameData& frame, Renderables& renderables) {
    if (_depthEffect.get() == nullptr) {
        return;
    }

    std::array<VkClearValue, 1> clearValues{};
    clearValues[0].depthStencil = {1.0f, 0};

    VkCommandBuffer& cmd = frame._commandBuffer->_commandBuffer;

    VkExtent2D extent{CascadedShadow::SHADOW_WIDTH, CascadedShadow::SHADOW_HEIGHT};

    VkViewport viewport = vkinit::get_viewport((float) CascadedShadow::SHADOW_WIDTH, (float) CascadedShadow::SHADOW_HEIGHT);
    vkCmdSetViewport(frame._commandBuffer->_commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = vkinit::get_scissor((float) CascadedShadow::SHADOW_WIDTH, (float) CascadedShadow::SHADOW_HEIGHT);
    vkCmdSetScissor(frame._commandBuffer->_commandBuffer, 0, 1, &scissor);

    for (uint8_t l = 0; l < CascadedShadow::COUNT; l++) {
        VkRenderPassBeginInfo renderPassInfo = vkinit::renderpass_begin_info(_depthPass._renderPass, extent, _cascades[l]._framebuffer->_frameBuffer);
        renderPassInfo.clearValueCount = clearValues.size();
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        {
            std::shared_ptr<Model> lastModel = nullptr;
            int pc = l;
            int i = 0; // a retirer? semble casser
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _depthEffect->pipeline);
            vkCmdPushConstants(cmd, _depthEffect->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), sizeof(int), &pc);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _depthEffect->pipelineLayout, 0, 1, &_cascades[l]._descriptor, 0, nullptr);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _depthEffect->pipelineLayout, 1, 1, &frame.objectDescriptor, 0, nullptr);
            for (auto const &object: renderables) {
                object.model->draw(cmd, _depthEffect->pipelineLayout, sizeof(glm::mat4) + sizeof(int), i, object.model.get() != lastModel.get());
                lastModel = object.model;
                i++;
            }
        }
        vkCmdEndRenderPass(cmd);
    }


}

void CascadedShadow::compute_cascades(Camera& camera, LightingManager& lightManager) {
    float splits[COUNT];
    float zNear = camera.get_z_near();
    float zFar = camera.get_z_far();

    float range = zFar - zNear;
    float ratio = zFar / zNear;

    for (uint32_t i = 0; i < CascadedShadow::COUNT; i++) {
        float p = (i + 1) / static_cast<float>(CascadedShadow::COUNT);
        float log = zNear * std::pow(ratio, p);
        float uniform = zNear + range * p;
        float d = _lb * (log - uniform) + uniform;
        splits[i] = (d - zNear) / range;
    }

    float lastSplitDist = 0.0;
    for (uint32_t i = 0; i < CascadedShadow::COUNT; i++) {
        float splitDist = splits[i];

        glm::vec3 frustumCorners[8] = {
                glm::vec3(-1.0f,  1.0f, 0.0f),
                glm::vec3( 1.0f,  1.0f, 0.0f),
                glm::vec3( 1.0f, -1.0f, 0.0f),
                glm::vec3(-1.0f, -1.0f, 0.0f),
                glm::vec3(-1.0f,  1.0f,  1.0f),
                glm::vec3( 1.0f,  1.0f,  1.0f),
                glm::vec3( 1.0f, -1.0f,  1.0f),
                glm::vec3(-1.0f, -1.0f,  1.0f),
        };

        glm::mat4 invCam = glm::inverse(camera.get_projection_matrix() * camera.get_view_matrix());
        for (uint32_t i = 0; i < 8; i++) {
            glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
            frustumCorners[i] = invCorner / invCorner.w;
        }

        for (uint32_t i = 0; i < 4; i++) {
            glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
            frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
            frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
        }

        // Get frustum center
        glm::vec3 frustumCenter = glm::vec3(0.0f);
        for (uint32_t i = 0; i < 8; i++) {
            frustumCenter += frustumCorners[i];
        }
        frustumCenter /= 8.0f;

        float radius = 0.0f;g
        for (uint32_t i = 0; i < 8; i++) {
            float distance = glm::length(frustumCorners[i] - frustumCenter);
            radius = glm::max(radius, distance);
        }
        radius = std::ceil(radius * 16.0f) / 16.0f;

        for (const auto& e : lightManager._entities) {
            std::shared_ptr<Light> light = std::static_pointer_cast<Light>(e.second);

            if (light->get_type() == Light::Type::DIRECTIONAL) {
                glm::vec3 lightDir = normalize(-light->get_rotation());
                glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter + lightDir * radius, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
                glm::mat4 lightOrthoMatrix = glm::ortho(-radius, radius, -radius, radius, 0.0f, 2.0f * radius);

                // Store split distance and matrix in cascade
                _cascades[i].splitDepth = (camera.get_z_near() + splitDist * range) * -1.0f;
                _cascades[i].viewProjMatrix = lightOrthoMatrix * lightViewMatrix;
            }
        }

        lastSplitDist = splits[i];
    }
}

void CascadedShadow::debug_depth(FrameData& frame) {
    if (_debugEffect.get() != nullptr) {
        VkCommandBuffer cmd = frame._commandBuffer->_commandBuffer;
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _debugEffect->pipeline);
        vkCmdPushConstants(cmd, _debugEffect->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), sizeof(int), &_cascadeIdx);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _debugEffect->pipelineLayout, 0, 1, &frame.debugDescriptor, 0, nullptr);
        vkCmdDraw(cmd, 3, 1, 0, 0);
    }
}

CascadedShadow::GPUCascadedShadowData CascadedShadow::gpu_format() {
    GPUCascadedShadowData data{};

    for (uint8_t i = 0; i < COUNT; i++) {
        data.VPMat[i] = _cascades[i].viewProjMatrix;
        data.split[i] = _cascades[i].splitDepth;
    }
    data.colorCascades = _colorCascades;
    
    return data;
}