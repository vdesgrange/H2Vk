/*
*  H2Vk - Cascaded shadow mapping
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_cascaded_shadow_map.h"
#include "core/vk_framebuffers.h"
#include "core/vk_pipeline.h"
#include "core/manager/vk_material_manager.h"
#include "core/utilities/vk_initializers.h"
#include "core/utilities/vk_global.h"
#include "components/camera/vk_camera.h"
#include "components/model/vk_model.h"

CascadedShadowMapping::CascadedShadowMapping(Device &device) : _device(device), _offscreen_pass(RenderPass(device)) {
    this->prepare_offscreen_pass(device);
//    this->_offscreen_shadow = Texture();
}

CascadedShadowMapping::~CascadedShadowMapping() {
    this->_offscreen_shadow.destroy(_device);
    this->_offscreen_effect.reset();
    this->_debug_effect.reset();

//    for (auto& imageview: this->_cascades_imageview) {
//        vkDestroyImageView(_device._logicalDevice, imageview, nullptr);
//    }

    for (auto& c: this->_directional_shadows._cascades) {
        vkDestroyImageView(_device._logicalDevice, c._imageView, nullptr);
    }
}

void CascadedShadowMapping::allocate_buffers(Device& device) {
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        const size_t depthBufferSize =  helper::pad_uniform_buffer_size(device, sizeof(GPUCascadedShadowData));
        g_frames[i].cascadedOffscreenBuffer = Buffer::create_buffer(device, depthBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU); // memoryUsage type deprecated
    }
}

void CascadedShadowMapping::prepare_offscreen_pass(Device& device) {
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

void CascadedShadowMapping::prepare_depth_map(Device& device, UploadContext& uploadContext, LightingManager& lighting) {
    uint num_layers = CASCADE_COUNT;
//    this->_cascades_framebuffer.reserve(num_layers);
//    this->_cascades_imageview.reserve(num_layers);
//
//    for (int i=0; i < num_layers; i++) {
//        this->_cascades_imageview.push_back({});
//    }

    // Prepare image
    VkExtent3D imageExtent { SHADOW_WIDTH, SHADOW_HEIGHT, 1 };
    VkImageCreateInfo imgInfo = vkinit::image_create_info(DEPTH_FORMAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, imageExtent);
    imgInfo.arrayLayers = num_layers;

    VmaAllocationCreateInfo imgAllocinfo = {};
    imgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImage(device._allocator, &imgInfo, &imgAllocinfo, &this->_offscreen_shadow._image, &this->_offscreen_shadow._allocation, nullptr);

    // All layers
    VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(DEPTH_FORMAT, this->_offscreen_shadow._image, VK_IMAGE_ASPECT_DEPTH_BIT);
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    imageViewInfo.subresourceRange.layerCount = num_layers;
    vkCreateImageView(device._logicalDevice, &imageViewInfo, nullptr, &this->_offscreen_shadow._imageView);

    CommandBuffer::immediate_submit(device, uploadContext, [&](VkCommandBuffer cmd) {
        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = num_layers;

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

    // Sampler for cascade depth read
    VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    samplerInfo.maxLod = 1.0f;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    vkCreateSampler(device._logicalDevice, &samplerInfo, nullptr, &this->_offscreen_shadow._sampler);

    this->_offscreen_shadow._imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    this->_offscreen_shadow.updateDescriptor();

    // Per layer
    for (int l = 0; l < num_layers; l++) {
        // Create image view
        VkImageViewCreateInfo info = vkinit::imageview_create_info(DEPTH_FORMAT, this->_offscreen_shadow._image, VK_IMAGE_ASPECT_DEPTH_BIT);
        info.subresourceRange.baseArrayLayer = l;
        // vkCreateImageView(device._logicalDevice, &info, nullptr, &this->_cascades_imageview[l]);
        vkCreateImageView(device._logicalDevice, &info, nullptr, &this->_directional_shadows._cascades[l]._imageView);

        // std::vector<VkImageView> attachments = {_cascades_imageview[l]};
        // this->_cascades_framebuffer.emplace_back(FrameBuffer(_offscreen_pass, attachments, SHADOW_WIDTH, SHADOW_HEIGHT, 1));
        std::vector<VkImageView> attachments = {this->_directional_shadows._cascades[l]._imageView};
        this->_directional_shadows._cascades[l]._frameBuffer = std::make_unique<FrameBuffer>(_offscreen_pass, attachments, SHADOW_WIDTH, SHADOW_HEIGHT, 1);
    }
    
}

void CascadedShadowMapping::setup_descriptors(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout) {
    std::vector<VkDescriptorPoolSize> offscreenPoolSizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
    };

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VkDescriptorBufferInfo offscreenBInfo{};
        offscreenBInfo.buffer = g_frames[i].cascadedOffscreenBuffer._buffer;
        offscreenBInfo.offset = 0;
        offscreenBInfo.range = sizeof(GPUCascadedShadowData);

        this->_offscreen_shadow._descriptor.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        this->_offscreen_shadow.updateDescriptor();

        // 1. Offscreen descriptor set
        DescriptorBuilder::begin(layoutCache, allocator)
                .bind_buffer(offscreenBInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0)
                .bind_none(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  VK_SHADER_STAGE_FRAGMENT_BIT, 1) // useless
                .layout(setLayout)
                .build(g_frames[i].cascadedOffscreenDescriptor, setLayout, offscreenPoolSizes);

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

void CascadedShadowMapping::setup_pipelines(Device& device, MaterialManager& materialManager, std::vector<VkDescriptorSetLayout> setLayouts, RenderPass& renderPass) {
    // Compute depth map
    GraphicPipeline offscreenPipeline = GraphicPipeline(device, this->_offscreen_pass);
    offscreenPipeline._dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS };
    offscreenPipeline._colorBlending.attachmentCount = 0;
    offscreenPipeline._colorBlending.pAttachments = nullptr;
    offscreenPipeline._rasterizer.depthBiasEnable = VK_TRUE;

    std::vector<std::pair<ShaderType, const char*>> offscreen_module {
            {ShaderType::VERTEX, "../src/shaders/shadow_map/csm_offscreen.vert.spv"},
            {ShaderType::FRAGMENT, "../src/shaders/shadow_map/csm_offscreen.frag.spv"},
    };

    std::vector<PushConstant> constants {
            {sizeof(glm::mat4) + sizeof(uint32_t), ShaderType::VERTEX},
    };

    materialManager._pipelineBuilder = &offscreenPipeline;
    this->_offscreen_effect = materialManager.create_material("csm_offscreen", setLayouts, constants, offscreen_module);

    // Shadow map debug
    GraphicPipeline debugPipeline = GraphicPipeline(device, renderPass);
    debugPipeline._vertexInputInfo =  vkinit::vertex_input_state_create_info();
    debugPipeline._dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    std::vector<std::pair<ShaderType, const char*>> debug_module {
            {ShaderType::VERTEX, "../src/shaders/shadow_map/csm_debug_quad.vert.spv"},
            {ShaderType::FRAGMENT, "../src/shaders/shadow_map/csm_debug_quad.frag.spv"},
    };

    materialManager._pipelineBuilder = &debugPipeline;
    this->_debug_effect = materialManager.create_material("csm_debug", setLayouts, constants, debug_module);

}

void CascadedShadowMapping::run_offscreen_pass(FrameData& frame, Renderables& entities, LightingManager& lighting) {
    VkCommandBuffer& cmd = frame._commandBuffer->_commandBuffer;
    
    std::array<VkClearValue, 1> clearValues{};
    clearValues[0].depthStencil = {1.0f, 0};

    VkExtent2D extent;
    extent.width = static_cast<uint32_t>(CascadedShadowMapping::SHADOW_WIDTH);
    extent.height = static_cast<uint32_t>(CascadedShadowMapping::SHADOW_HEIGHT);

    VkViewport viewport = vkinit::get_viewport((float) CascadedShadowMapping::SHADOW_WIDTH, (float) CascadedShadowMapping::SHADOW_HEIGHT);
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = vkinit::get_scissor((float) CascadedShadowMapping::SHADOW_WIDTH, (float) CascadedShadowMapping::SHADOW_HEIGHT);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdSetDepthBias(cmd, 1.25f, 0.0f, 1.75f);

    // Per layer
    for (int l = 0; l < CASCADE_COUNT; l++) {
        //VkRenderPassBeginInfo offscreenPassInfo = vkinit::renderpass_begin_info(this->_offscreen_pass._renderPass, extent, _cascades_framebuffer[l]._frameBuffer);
        VkRenderPassBeginInfo offscreenPassInfo = vkinit::renderpass_begin_info(this->_offscreen_pass._renderPass, extent, this->_directional_shadows._cascades[l]._frameBuffer->_frameBuffer);
        offscreenPassInfo.clearValueCount = clearValues.size();
        offscreenPassInfo.pClearValues = clearValues.data();

        uint32_t pc = l;

        vkCmdBeginRenderPass(cmd, &offscreenPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_offscreen_effect->pipeline);

            uint32_t i = 0;
            std::shared_ptr<Model> lastModel = nullptr;
            vkCmdPushConstants(cmd, this->_offscreen_effect->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), sizeof(uint32_t), &pc);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_offscreen_effect->pipelineLayout, 0, 1, &frame.cascadedOffscreenDescriptor, 0, nullptr);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_offscreen_effect->pipelineLayout, 1, 1, &frame.objectDescriptor, 0, nullptr);

            for (auto const &object: entities) {
                this->draw(*object.model, cmd, object.material->pipelineLayout, i, object.model.get() != lastModel.get());
                lastModel = object.model;
                i++;
            }
        }
        vkCmdEndRenderPass(cmd);
    }

}

void CascadedShadowMapping::draw(Model& model, VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint32_t instance, bool bind) {
    if (bind) {
        VkDeviceSize offsets[1] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model._vertexBuffer._buffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, model._indexBuffer.allocation._buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    for (auto& node : model._nodes) {
        draw_node(node, commandBuffer, pipelineLayout, instance);
    }
}

void CascadedShadowMapping::draw_node(Node* node, VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint32_t instance) {
    if (!node->mesh.primitives.empty()) {
        glm::mat4 nodeMatrix = node->matrix;
        Node* parent = node->parent;
        while (parent) {
            nodeMatrix = parent->matrix * nodeMatrix;
            parent = parent->parent;
        }

        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &nodeMatrix);

        for (Primitive& primitive : node->mesh.primitives) { // to do : consider material for transparency
            if (primitive.indexCount > 0) {
                vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, instance);
            }
        }
    }

    for (auto& child : node->children) {
        draw_node(child, commandBuffer, pipelineLayout, instance);
    }
}

/**
 * @brief Compute shadow map cascades
 * [GPUGems3 - Chapter 10 Parallel-Split Shadow Maps on Programmable GPUs]
 * @param camera 
 * @param lighting 
 */
void CascadedShadowMapping::compute_cascades(Camera& camera, LightingManager& lighting) {
    float cascadeSplits[CASCADE_COUNT];
    float lambda = 0.95f;
    float zNear = camera.get_z_near();
    float zFar = camera.get_z_far();
    float clipRange = zFar - zNear;
    float clipRatio = zFar / zNear;


    for (uint32_t i = 0; i < CASCADE_COUNT; i++) {
        float p = (i + 1) / static_cast<float>(CASCADE_COUNT);
        float cLog = zNear * pow(clipRatio, p);
        float cUni = zNear + clipRange * p;
        float c = lambda * (cLog - cUni) + cUni;
        cascadeSplits[i] = (c - zNear) / clipRange;
    }

    glm::mat4 invViewProj = glm::inverse(camera.get_projection_matrix() * camera.get_view_matrix());

	glm::vec3 frustumCorners[8] = { // NDC Space
		glm::vec3(-1.0f,  1.0f, 0.0f), // left-bottom-front
		glm::vec3( 1.0f,  1.0f, 0.0f), // right-bottom-front
		glm::vec3( 1.0f, -1.0f, 0.0f), // right-top-front
		glm::vec3(-1.0f, -1.0f, 0.0f), // left-top-front
		glm::vec3(-1.0f,  1.0f,  1.0f), // left-bottom-back
		glm::vec3( 1.0f,  1.0f,  1.0f), // right-bottom-back
		glm::vec3( 1.0f, -1.0f,  1.0f), // right-top-back
		glm::vec3(-1.0f, -1.0f,  1.0f), // left-top-back
	};

    for (uint8_t i; i < 8; i++) {
        glm::vec4 invCorner = invViewProj * glm::vec4(frustumCorners[i], 1.0f);
        frustumCorners[i] = invCorner / invCorner.w;
    }


	float lastSplitDist = 0.0;
    for (uint32_t i = 0; i < CASCADE_COUNT; i++) {
        float splitDist = cascadeSplits[i];
        // Compute new frustum corners
	    for (uint32_t j = 0; j < 4; j++) {
		    glm::vec3 dist = frustumCorners[j + 4] - frustumCorners[j]; // back - front
		    frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist); // re-adjust back corner
		    frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist); // re-adjust front corner
        }

        // Compute new frustum center
        glm::vec3 frustumCenter = glm::vec3(0.0f); // center assumed at 0.
        for (uint32_t j = 0; j < 8; j++) {
			frustumCenter += frustumCorners[j];
		}
		frustumCenter /= 8.0f;

        float radius = 0.0f;
		for (uint32_t i = 0; i < 8; i++) {
			float distance = glm::length(frustumCorners[i] - frustumCenter);
			radius = glm::max(radius, distance);
		}
		radius = std::ceil(radius * 16.0f) / 16.0f;

        glm::vec3 maxExtents = glm::vec3(radius);
		glm::vec3 minExtents = -maxExtents;

        // Process directional lights
        uint32_t lightIndex = 0;
        for (auto const& l : lighting._entities) {
            std::shared_ptr<Light> light = std::static_pointer_cast<Light>(l.second);
            if (light->get_type() == Light::DIRECTIONAL) {

                glm::vec3 dir = normalize(light->get_rotation());
                glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
                glm::mat4 view = glm::lookAt(frustumCenter - dir * radius, frustumCenter, up);
                glm::mat4 proj = glm::ortho(-radius, radius, -radius, radius, 0.0f, 2.0f * radius);

                // Store split distance and matrix in cascade
			    _directional_shadows._cascades[i].splitDepth = - (camera.get_z_near() + splitDist * clipRange); // [lightIndex]
			    _directional_shadows._cascades[i].viewProj = proj * view; // [lightIndex]
            }
            lightIndex++;
	    }

        lastSplitDist = splitDist;
    }

}

GPUCascadedShadowData CascadedShadowMapping::gpu_format() {
    GPUCascadedShadowData offscreenData{};

    //for (auto& l : _directional_shadows) {
        for (uint8_t j = 0; j < _directional_shadows._cascades.size(); j++) {
            offscreenData.cascadeVP[j] = _directional_shadows._cascades.at(j).viewProj;
        }
    //}

    return offscreenData;
}

void CascadedShadowMapping::run_debug(FrameData& frame) {
    uint32_t idx = 0;
    vkCmdPushConstants(frame._commandBuffer->_commandBuffer, this->_offscreen_effect->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), sizeof(uint32_t), &idx);
    vkCmdBindDescriptorSets(frame._commandBuffer->_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_debug_effect->pipelineLayout, 0, 1, &frame.debugDescriptor, 0, nullptr);
    vkCmdBindPipeline(frame._commandBuffer->_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_debug_effect->pipeline);
    vkCmdDraw(frame._commandBuffer->_commandBuffer, 3, 1, 0, 0);
}
