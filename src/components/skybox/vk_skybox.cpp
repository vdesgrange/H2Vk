/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_skybox.h"
#include "core/vk_window.h"
#include "core/vk_device.h"
#include "core/vk_buffer.h"
#include "core/vk_command_buffer.h"
#include "core/manager/vk_mesh_manager.h"
#include "core/manager/vk_material_manager.h"
#include "components/model/vk_poly.h"
#include "components/camera/vk_camera.h"
#include "core/utilities/vk_initializers.h"
#include "core/utilities/vk_global.h"
#include "techniques/vk_env_map.h"

#include "stb_image.h"

Skybox::Skybox(Device& device, MeshManager& meshManager, UploadContext& uploadContext) :
    _device(device), _meshManager(meshManager), _uploadContext(uploadContext) {}

Skybox::~Skybox() {
    this->destroy();
}

void Skybox::destroy() {
    // must be done before device allocator destruction otherwise buffer memory not all freed
    this->_model.reset(); // smart pointer. Carefull, might break because destroyed automatically.
    this->_background.destroy(this->_device); // Must be called before vmaDestroyAllocator
    this->_environment.destroy(this->_device);
    this->_prefilter.destroy(this->_device);
    this->_brdf.destroy(this->_device);
}

void Skybox::load() {

    EnvMap envMap{};

    if (_type == Type::box) {
        _model = ModelPOLY::create_cube(&_device, _uploadContext, {-100.0f, -100.0f, -100.0f},  {100.f, 100.f, 100.0f});
        Texture original {};
        Texture hdr {};

        load_sphere_texture("../assets/skybox/grand_canyon_yuma_point_8k.jpg", original, VK_FORMAT_R8G8B8A8_SRGB);
        load_sphere_texture("../assets/skybox/GCanyon_C_YumaPoint_3k.hdr", hdr, VK_FORMAT_R8G8B8A8_SRGB);
        // _background = envMap.cube_map_converter(_device, _uploadContext, _meshManager, original);

        Texture tmp = envMap.cube_map_converter(_device, _uploadContext, _meshManager, hdr);

        _background = envMap.irradiance_cube_mapping(_device, _uploadContext, _meshManager, tmp);
        _environment = envMap.irradiance_cube_mapping(_device, _uploadContext, _meshManager, tmp);
        _prefilter = envMap.prefilter_cube_mapping(_device, _uploadContext, _meshManager, tmp);
        _brdf = envMap.brdf_convolution(_device, _uploadContext);

        original.destroy(_device);
        hdr.destroy(_device);
        tmp.destroy(_device);
    } else {
        Texture original {};
        Texture hdr {};

        _model = ModelPOLY::create_uv_sphere(&_device, _uploadContext, {0.0f, 0.0f, 0.0f}, 100.0f, 32, 32);

        load_sphere_texture("../assets/skybox/grand_canyon_yuma_point_8k.jpg", _background);
        load_sphere_texture("../assets/skybox/GCanyon_C_YumaPoint_3k.hdr", hdr, VK_FORMAT_R8G8B8A8_SRGB);

        Texture tmp = envMap.cube_map_converter(_device, _uploadContext, _meshManager, hdr);
        _environment = envMap.irradiance_cube_mapping(_device, _uploadContext, _meshManager, tmp);
        _prefilter = envMap.prefilter_cube_mapping(_device, _uploadContext, _meshManager, tmp);
        _brdf = envMap.brdf_convolution(_device, _uploadContext);

        hdr.destroy(_device);
        original.destroy(_device);
        tmp.destroy(_device);
    }

    _meshManager.upload_mesh(*_model);
}

void Skybox::load_sphere_texture(const char* file, Texture& texture, VkFormat format) {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(file, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        std::cout << "Failed to load texture file " << file << std::endl;
    }

    texture._width = texWidth;
    texture._height = texHeight;

    VkFormatProperties formatProperties;

    // Get device properties for the requested texture format
    vkGetPhysicalDeviceFormatProperties(_device._physicalDevice, format, &formatProperties);
    // Check if requested image format supports image storage operations
    // assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT);


    void* pixel_ptr = pixels;
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    // VkFormat format = VK_FORMAT_R8G8B8A8_UNORM; // VK_FORMAT_R8G8B8A8_UNORM
    AllocatedBuffer buffer = Buffer::create_buffer(_device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void* data;
    vmaMapMemory(_device._allocator, buffer._allocation, &data);
    memcpy(data, pixel_ptr, static_cast<size_t>(imageSize));
    vmaUnmapMemory(_device._allocator, buffer._allocation);
    stbi_image_free(pixels);

    VkExtent3D imageExtent;
    imageExtent.width = static_cast<uint32_t>(texWidth);
    imageExtent.height = static_cast<uint32_t>(texHeight);
    imageExtent.depth = 1;

    VkImageCreateInfo imgInfo = vkinit::image_create_info(format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent); //  | VK_IMAGE_USAGE_STORAGE_BIT
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imgAllocinfo = {};
    imgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImage(_device._allocator, &imgInfo, &imgAllocinfo, &texture._image, &texture._allocation, nullptr);

    VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(format, texture._image, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCreateImageView(_device._logicalDevice, &imageViewInfo, nullptr, &texture._imageView);

    VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    vkCreateSampler(_device._logicalDevice, &samplerInfo, nullptr, &texture._sampler);

    CommandBuffer::immediate_submit(_device, _uploadContext, [&](VkCommandBuffer cmd) {
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
        imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_GENERAL; // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        // VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);

        // Change texture image layout to shader read after all mip levels have been copied
        texture._imageLayout = VK_IMAGE_LAYOUT_GENERAL; // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    });

    texture.updateDescriptor();

    vmaDestroyBuffer(_device._allocator, buffer._buffer, buffer._allocation);
    std::cout << "Sphere map loaded successfully " << file << std::endl;
}

void Skybox::setup_descriptors(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout) {
    std::vector<VkDescriptorPoolSize> skyboxPoolSizes = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3}
    };

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        g_frames[i].skyboxDescriptor = VkDescriptorSet();

        // todo : cameraBuffer must be allocated once only. Need to handle allocation dependencies ordering.
        // g_frames[i].cameraBuffer = Buffer::create_buffer(_device, sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        VkDescriptorBufferInfo camBInfo{};
        camBInfo.buffer = g_frames[i].cameraBuffer._buffer;
        camBInfo.offset = 0;
        camBInfo.range = sizeof(GPUCameraData);

        DescriptorBuilder::begin(layoutCache, allocator)
                .bind_buffer(camBInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
                .bind_image(this->_background._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
                .layout(setLayout)
                .build(g_frames[i].skyboxDescriptor, setLayout, skyboxPoolSizes);

    }
}

void Skybox::setup_pipeline(MaterialManager& materialManager, std::vector<VkDescriptorSetLayout> setLayouts) {
    std::unordered_map<Skybox::Type, std::string> shader = {
            {Type::box, "../src/shaders/skybox/skybox"},
            {Type::sphere, "../src/shaders/skybox/skysphere"},
    };

    std::string vert = shader.at(_type) + ".vert.spv";
    std::string frag = shader.at(_type) + ".frag.spv";

    std::vector<std::pair<ShaderType, const char*>> modules {
            {ShaderType::VERTEX, vert.c_str()},
            {ShaderType::FRAGMENT, frag.c_str()},
    };

    std::vector<PushConstant> constants {
            {sizeof(glm::mat4), ShaderType::VERTEX},
    };

    this->_material = materialManager.create_material("skyboxMaterial", setLayouts, constants, modules);
}

void Skybox::draw(VkCommandBuffer& commandBuffer) {
    _model->draw(commandBuffer, _material->pipelineLayout, sizeof(glm::mat4), 0, true);
}

void Skybox::build_command_buffer(VkCommandBuffer& commandBuffer, VkDescriptorSet* descriptor) {
     if (_display) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_material->pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_material->pipelineLayout, 0,1, descriptor, 0, nullptr);
        this->draw(commandBuffer);
     }
}