#pragma once

#include "tiny_obj_loader.h"
#include "vk_model.h"

class Model;

class ModelOBJ final: public Model {
public:
    using Model::Model;

    bool load_model(VulkanEngine& engine, const char *filename) override;

private:
    void load_node(tinyobj::attrib_t attrib, std::vector<tinyobj::shape_t>& shapes);
};
