#include "vk_skybox.h"
#include "core/vk_device.h"
#include "core/vk_buffer.h"
#include "core/vk_mesh_manager.h"
#include "core/vk_pipeline.h"
#include "core/utilities/vk_initializers.h"
#include "core/vk_command_buffer.h"
#include "core/model/vk_poly.h"

#include <stb_image.h>

Skybox::Skybox(Device& device, PipelineBuilder& pipelineBuilder, TextureManager& textureManager, MeshManager& meshManager, UploadContext& uploadContext) :
    _device(device), _pipelineBuilder(pipelineBuilder), _textureManager(textureManager), _meshManager(meshManager), _uploadContext(uploadContext) {

}

Skybox::~Skybox() {
    this->destroy();
}

void Skybox::destroy() {
    // must be done before device allocator destruction otherwise buffer memory not all freed
    this->_model.reset(); // smart pointer. Carefull, might break because destroyed automatically.
    this->_texture.destroy(this->_device); // Must be called before vmaDestroyAllocator
}

void Skybox::load() {

    if (_type == Type::box) {
        _model = ModelPOLY::create_cube(&_device, {-100.0f, -100.0f, -100.0f},  {100.f, 100.f, 100.0f});
        load_cube_texture();
    } else {
        _model = ModelPOLY::create_uv_sphere(&_device, {0.0f, 0.0f, 0.0f}, 100.0f, 32, 32);
        load_sphere_texture();
    }

//    _pipelineBuilder->skybox({_descriptorSetLayouts.skybox});
//    _material = _pipelineBuilder.get_material("skyboxMaterial");

    _meshManager.upload_mesh(*_model);
    // _meshManager._models.emplace("skybox", _model);
}

void Skybox::load_cube_texture() {
    std::vector<const char*> files = {
            "../assets/skybox/right.jpg",
            "../assets/skybox/left.jpg",
            "../assets/skybox/top.jpg",
            "../assets/skybox/bottom.jpg",
            "../assets/skybox/front.jpg",
            "../assets/skybox/back.jpg",
    };

    _texture = Texture();
    int count = files.size();
    int texWidth, texHeight, texChannels; // same for all images (otherwise use an array)
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    stbi_uc *pixels[6];

    for (int i=0; i < count; i++) {
        pixels[i] = stbi_load(files[i], &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (!pixels[i]) {
            std::cout << "Failed to load texture file " << files[i] << std::endl;
        }
    }

    VkDeviceSize layerSize = texWidth * texHeight * 4;
    VkDeviceSize imageSize = layerSize * count;
    AllocatedBuffer buffer = Buffer::create_buffer(_device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void* data;
    vmaMapMemory(_device._allocator, buffer._allocation, &data);
    stbi_uc* dataSSBO = (stbi_uc*)data;
    for (int i=0; i < count; i++) {
        void* pixel_ptr = pixels[i];
        memcpy(dataSSBO + i * layerSize, pixel_ptr, static_cast<size_t>(layerSize));
        stbi_image_free(pixels[i]);
    }
    vmaUnmapMemory(_device._allocator, buffer._allocation);

    VkExtent3D imageExtent; // Same texWidth, texHeight for all faces
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
    vmaCreateImage(_device._allocator, &imgInfo, &imgAllocinfo, &_texture._image, &_texture._allocation, nullptr);

    CommandBuffer::immediate_submit(_device, _uploadContext, [&](VkCommandBuffer cmd) {
        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 6;

        VkImageMemoryBarrier imageBarrier_toTransfer = {};
        imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toTransfer.image = this->_texture._image;
        imageBarrier_toTransfer.subresourceRange = range;
        imageBarrier_toTransfer.srcAccessMask = 0;
        imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        //barrier the image into the transfer-receive layout
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);

        uint64_t offset = 0;
        std::vector<VkBufferImageCopy> copyRegions;
        for (int i=0; i < count; i++) {
            VkBufferImageCopy copyRegion = {};
            copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.imageSubresource.mipLevel = 0;
            copyRegion.imageSubresource.baseArrayLayer = i;
            copyRegion.imageSubresource.layerCount = 1;
            copyRegion.imageExtent = imageExtent;
            copyRegion.bufferOffset = offset + i * layerSize;
            copyRegion.bufferRowLength = 0;
            copyRegion.bufferImageHeight = 0;

            copyRegions.push_back(copyRegion);
        }

        //copy the buffer into the image
        vkCmdCopyBufferToImage(cmd, buffer._buffer, this->_texture._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copyRegions.size()), copyRegions.data());

        VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
        imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);

        // Change texture image layout to shader read after all mip levels have been copied
        _texture._imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);
        vkCreateSampler(_device._logicalDevice, &samplerInfo, nullptr, &_texture._sampler);

        VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(format, _texture._image, VK_IMAGE_ASPECT_COLOR_BIT);
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        imageViewInfo.subresourceRange.layerCount = 6;
        vkCreateImageView(_device._logicalDevice, &imageViewInfo, nullptr, &_texture._imageView);

        _texture.updateDescriptor();
    });

    vmaDestroyBuffer(_device._allocator, buffer._buffer, buffer._allocation);
    std::cout << "Skybox texture loaded successfully" << std::endl;
}

void Skybox::load_sphere_texture() {
    const char* file = "../assets/skybox/grand_canyon_yuma_point_8k.jpg"; // "../assets/skybox/tokyo_bigsight_8k.jpg"; // "../assets/debug/uv_checker_4k.png"; //

    int texWidth, texHeight, texChannels;

    stbi_uc* pixels = stbi_load(file, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        std::cout << "Failed to load texture file " << file << std::endl;
    }

    void* pixel_ptr = pixels;
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
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

    VkImageCreateInfo imgInfo = vkinit::image_create_info(format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imgAllocinfo = {};
    imgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImage(_device._allocator, &imgInfo, &imgAllocinfo, &_texture._image, &_texture._allocation, nullptr);

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
        imageBarrier_toTransfer.image = _texture._image;
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
        vkCmdCopyBufferToImage(cmd, buffer._buffer, _texture._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
        imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);

        // Change texture image layout to shader read after all mip levels have been copied
        _texture._imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);
        vkCreateSampler(_device._logicalDevice, &samplerInfo, nullptr, &_texture._sampler);

        VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(format, _texture._image, VK_IMAGE_ASPECT_COLOR_BIT);
        vkCreateImageView(_device._logicalDevice, &imageViewInfo, nullptr, &_texture._imageView);

        _texture.updateDescriptor();
    });

    vmaDestroyBuffer(_device._allocator, buffer._buffer, buffer._allocation);
    std::cout << "Sphere map loaded successfully " << file << std::endl;
}

void  Skybox::setup_descriptor() {
    // todo
}

void Skybox::setup_pipeline(PipelineBuilder& pipelineBuilder, std::vector<VkDescriptorSetLayout> setLayouts) {
    std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> modules = {
            {VK_SHADER_STAGE_VERTEX_BIT, "../src/shaders/skybox/skysphere.vert.spv"},
            {VK_SHADER_STAGE_FRAGMENT_BIT, "../src/shaders/skybox/skysphere.frag.spv"},
    };

//    if (_type == Type::box) {
//         modules = {
//                {VK_SHADER_STAGE_VERTEX_BIT, "../src/shaders/skybox/skybox.vert.spv"},
//                {VK_SHADER_STAGE_FRAGMENT_BIT, "../src/shaders/skybox/skybox.frag.spv"},
//        };
//    } else {
//        modules = {
//                {VK_SHADER_STAGE_VERTEX_BIT, "../src/shaders/skybox/skysphere.vert.spv"},
//                {VK_SHADER_STAGE_FRAGMENT_BIT, "../src/shaders/skybox/skysphere.frag.spv"},
//        };
//    }

    VkPushConstantRange push_constant;
    push_constant.offset = 0;
    push_constant.size = sizeof(glm::mat4);
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::shared_ptr<ShaderEffect> effect = pipelineBuilder.build_effect(setLayouts, {push_constant}, modules);
    std::shared_ptr<ShaderPass> pass = pipelineBuilder.build_pass(effect);
    pipelineBuilder.create_material("skyboxMaterial", pass);
    this->_material = pipelineBuilder.get_material("skyboxMaterial");

    for (auto& shader : effect->shaderStages) {
        vkDestroyShaderModule(_device._logicalDevice, shader.shaderModule, nullptr);
    }
}

void Skybox::draw(VkCommandBuffer& commandBuffer) {
    _model->draw(commandBuffer, _material->pipelineLayout, 0, true);
}