#include "vk_poly.h"
#include "../vk_device.h"

#include <array>


std::shared_ptr<Model> ModelPOLY::create_cube(Device* device, const glm::vec3& p0, const glm::vec3& p1, std::optional<PBRProperties> props) {
    std::shared_ptr<Model> model = std::make_shared<ModelPOLY>(device);
    model->_name = "Cube";
    Node *node = new Node{};
    node->matrix = glm::mat4(1.f);
    node->parent = nullptr;

    {
        std::vector<Vertex> vertices = {
                {glm::vec3(p0.x, p0.y, p0.z), glm::vec3(-1, 0, 0), glm::vec2(0), glm::vec3{255.0f, 0.0f, 0.0f}},
                {glm::vec3(p0.x, p0.y, p1.z), glm::vec3(-1, 0, 0), glm::vec2(0), glm::vec3{255.0f, 0.0f, 0.0f}},
                {glm::vec3(p0.x, p1.y, p1.z), glm::vec3(-1, 0, 0), glm::vec2(0), glm::vec3{255.0f, 0.0f, 0.0f}},
                {glm::vec3(p0.x, p1.y, p0.z), glm::vec3(-1, 0, 0), glm::vec2(0), glm::vec3{255.0f, 0.0f, 0.0f}},

                {glm::vec3(p1.x, p0.y, p1.z), glm::vec3(1, 0, 0),  glm::vec2(0), glm::vec3{0.0f, 255.0f, 0.0f}},
                {glm::vec3(p1.x, p0.y, p0.z), glm::vec3(1, 0, 0),  glm::vec2(0), glm::vec3{0.0f, 255.0f, 0.0f}},
                {glm::vec3(p1.x, p1.y, p0.z), glm::vec3(1, 0, 0),  glm::vec2(0), glm::vec3{0.0f, 255.0f, 0.0f}},
                {glm::vec3(p1.x, p1.y, p1.z), glm::vec3(1, 0, 0),  glm::vec2(0), glm::vec3{0.0f, 255.0f, 0.0f}},

                {glm::vec3(p1.x, p0.y, p0.z), glm::vec3(0, 0, -1), glm::vec2(0), glm::vec3{0.0f, 0.0f, 255.0f}},
                {glm::vec3(p0.x, p0.y, p0.z), glm::vec3(0, 0, -1), glm::vec2(0), glm::vec3{0.0f, 0.0f, 255.0f}},
                {glm::vec3(p0.x, p1.y, p0.z), glm::vec3(0, 0, -1), glm::vec2(0), glm::vec3{0.0f, 0.0f, 255.0f}},
                {glm::vec3(p1.x, p1.y, p0.z), glm::vec3(0, 0, -1), glm::vec2(0), glm::vec3{0.0f, 0.0f, 255.0f}},

                {glm::vec3(p0.x, p0.y, p1.z), glm::vec3(0, 0, 1),  glm::vec2(0), glm::vec3{255.0f, 255.0f, 0.0f}},
                {glm::vec3(p1.x, p0.y, p1.z), glm::vec3(0, 0, 1),  glm::vec2(0), glm::vec3{255.0f, 255.0f, 0.0f}},
                {glm::vec3(p1.x, p1.y, p1.z), glm::vec3(0, 0, 1),  glm::vec2(0), glm::vec3{255.0f, 255.0f, 0.0f}},
                {glm::vec3(p0.x, p1.y, p1.z), glm::vec3(0, 0, 1),  glm::vec2(0), glm::vec3{255.0f, 255.0f, 0.0f}},

                {glm::vec3(p0.x, p0.y, p0.z), glm::vec3(0, -1, 0), glm::vec2(0), glm::vec3{255.0f, 0.0f, 255.0f}},
                {glm::vec3(p1.x, p0.y, p0.z), glm::vec3(0, -1, 0), glm::vec2(0), glm::vec3{255.0f, 0.0f, 255.0f}},
                {glm::vec3(p1.x, p0.y, p1.z), glm::vec3(0, -1, 0), glm::vec2(0), glm::vec3{255.0f, 0.0f, 255.0f}},
                {glm::vec3(p0.x, p0.y, p1.z), glm::vec3(0, -1, 0), glm::vec2(0), glm::vec3{255.0f, 0.0f, 255.0f}},

                {glm::vec3(p1.x, p1.y, p0.z), glm::vec3(0, 1, 0),  glm::vec2(0), glm::vec3{0.0f, 255.0f, 255.0f}},
                {glm::vec3(p0.x, p1.y, p0.z), glm::vec3(0, 1, 0),  glm::vec2(0), glm::vec3{0.0f, 255.0f, 255.0f}},
                {glm::vec3(p0.x, p1.y, p1.z), glm::vec3(0, 1, 0),  glm::vec2(0), glm::vec3{0.0f, 255.0f, 255.0f}},
                {glm::vec3(p1.x, p1.y, p1.z), glm::vec3(0, 1, 0),  glm::vec2(0), glm::vec3{0.0f, 255.0f, 255.0f}},

        };

        std::vector<uint32_t> indices = {
                0, 1, 2, 0, 2, 3,
                4, 5, 6, 4, 6, 7,
                8, 9, 10, 8, 10, 11,
                12, 13, 14, 12, 14, 15,
                16, 17, 18, 16, 18, 19,
                20, 21, 22, 20, 22, 23
        };


        for (auto vertex: vertices) {
            model->_verticesBuffer.push_back(vertex);
        }

        for (auto idx: indices) {
            model->_indexesBuffer.push_back(idx);
        }
    }

    Primitive primitive{};
    primitive.firstIndex = 0;
    primitive.indexCount = model->_indexesBuffer.size();
    primitive.materialIndex = -1;

    if (props) {
        primitive.materialIndex = 0;

        Materials material {};
        material.properties = props.value();
        material.pbr = true;
        model->_materials.push_back(material);
    }

    node->mesh.primitives.push_back(primitive);
    model->_nodes.push_back(node);

    return model;
}

std::shared_ptr<Model> ModelPOLY::create_uv_sphere(Device* device, const glm::vec3& center, float radius, uint32_t stacks, uint32_t sectors, glm::vec3 color, std::optional<PBRProperties> props) {
    std::shared_ptr<Model> model = std::make_shared<ModelPOLY>(device);
    model->_name = "UV_sphere";
    float x, y, z, xy = 0;
    Node* node = new Node{};
    node->matrix = glm::mat4(1.f);
    node->parent = nullptr;

    for (uint32_t i = 0; i <= stacks; i++) {
        float phi = - M_PI / 2 - M_PI * i / stacks;
        xy = cos(phi);
        z = sin(phi);

        for (uint32_t j = 0; j <= sectors; j++) {
            float theta = 2 * M_PI * j / sectors;
            x = xy * cos(theta);
            y = xy * sin(theta);

            Vertex vertex{};
            vertex.position = center + radius * glm::vec3({x, z, y});
            vertex.normal = glm::normalize(glm::vec3({x, z, y}));
            vertex.uv = glm::vec2({ static_cast<float>(j) / sectors, static_cast<float>(i) / stacks});
            vertex.color = color;

            model->_verticesBuffer.push_back(vertex);
        }
    }

    for (int j = 0; j < stacks; ++j)
    {
        for (int i = 0; i < sectors; ++i)
        {
            const auto j0 = (j + 0) * (sectors + 1);
            const auto j1 = (j + 1) * (sectors + 1);
            const auto i0 = i + 0;
            const auto i1 = i + 1;

            model->_indexesBuffer.push_back(j0 + i0);
            model->_indexesBuffer.push_back(j1 + i0);
            model->_indexesBuffer.push_back(j1 + i1);

            model->_indexesBuffer.push_back(j0 + i0);
            model->_indexesBuffer.push_back(j1 + i1);
            model->_indexesBuffer.push_back(j0 + i1);
        }
    }

    Primitive primitive{};
    primitive.firstIndex = 0;
    primitive.indexCount = model->_indexesBuffer.size();
    primitive.materialIndex = -1;

    if (props) {
        primitive.materialIndex = 0;

        Materials material {};
        material.properties = props.value();
        material.pbr = true;
        model->_materials.push_back(material);
    }

    node->mesh.primitives.push_back(primitive);
    model->_nodes.push_back(node);

    return model;
}

std::shared_ptr<Model> ModelPOLY::create_triangle(Device* device, glm::vec3 color, std::optional<PBRProperties> props) {
    std::shared_ptr<Model> model = std::make_shared<ModelPOLY>(device);
    model->_name = "Triangle";

    Node* node = new Node{};
    node->matrix = glm::mat4(1.f);
    node->parent = nullptr;

    model->_verticesBuffer.resize(3);
    model->_verticesBuffer[0].position = { 1.f, 1.f, 0.0f };
    model->_verticesBuffer[1].position = {-1.f, 1.f, 0.0f };
    model->_verticesBuffer[2].position = { 0.f,-1.f, 0.0f };

    model->_verticesBuffer[0].color = color;
    model->_verticesBuffer[1].color = color;
    model->_verticesBuffer[2].color = color;

    model->_indexesBuffer = {0, 1, 2};

    Primitive primitive{};
    primitive.firstIndex = 0;
    primitive.indexCount = model->_indexesBuffer.size();
    primitive.materialIndex = -1;

    if (props) {
        primitive.materialIndex = 0;

        Materials material {};
        material.properties = props.value();
        material.pbr = true;
        model->_materials.push_back(material);
    }

    node->mesh.primitives.push_back(primitive);
    model->_nodes.push_back(node);

    return model;
}

std::shared_ptr<Model> ModelPOLY::create_plane(Device* device, const glm::vec3& p0, const glm::vec3& p1, glm::vec3 color, std::optional<PBRProperties> props) {
    std::shared_ptr<Model> model = std::make_shared<ModelPOLY>(device);
    model->_name = "Plane";

    Node *node = new Node{};
    node->matrix = glm::mat4(1.f);
    node->parent = nullptr;

    glm::vec3 u = glm::vec3(0.0f, p0.y - p1.y, p0.z - p1.z); // (p0.z + p1.z) / 2
    glm::vec3 v = glm::vec3(p0.x - p1.x, 0.0f, p0.z - p1.z); //  (p0.z + p1.z) / 2
    glm::vec3 normal = - glm::cross(u, v);

    std::vector<Vertex> vertices = {
            {glm::vec3(p0.x, p0.y, p0.z), normal, glm::vec2(0, 0), color},
            {glm::vec3(p0.x, p1.y, p1.z), normal, glm::vec2(0, 1), color},
            {glm::vec3(p1.x, p0.y, p0.z), normal, glm::vec2(1, 0), color},
            {glm::vec3(p1.x, p1.y, p1.z), normal, glm::vec2(1, 1), color},
    };

    std::vector<uint32_t> indices = {
            0, 1, 2, 1, 2, 3,
    };

    for (auto vertex: vertices) {
        model->_verticesBuffer.push_back(vertex);
    }

    for (auto idx: indices) {
        model->_indexesBuffer.push_back(idx);
    }

    Primitive primitive{};
    primitive.firstIndex = 0;
    primitive.indexCount = model->_indexesBuffer.size();
    primitive.materialIndex = -1;

    if (props) {
        primitive.materialIndex = 0;

        Materials material {};
        material.properties = props.value();
        material.pbr = true;
        model->_materials.push_back(material);
    }

    node->mesh.primitives.push_back(primitive);
    model->_nodes.push_back(node);

    return model;
}