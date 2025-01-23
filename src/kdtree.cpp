#include "kdtree.h"
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

GDKDTree::GDKDTree(int32_t buffer_capacity)
{
    buffer = std::vector<KDNode>(buffer_capacity);
    root = nullptr;
}

GDKDTree::GDKDTree()
{
    buffer = std::vector<KDNode>(64);
    root = nullptr;
}

GDKDTree::~GDKDTree()
{
}

KDNode::KDNode()
    : position(Vector3())
    , left(nullptr)
    , right(nullptr)
{
}

KDNode::KDNode(Vector3 position)
    : position(position)
    , left(nullptr)
    , right(nullptr)
{
}

void GDKDTree::reset_buffer(int32_t buffer_size)
{
    root = nullptr;
    buffer.clear();
    if (buffer_size > 0) {
        buffer.resize(buffer_size);
    }
}

KDNode* GDKDTree::alloc_node(Vector3 position)
{
    buffer.emplace_back(position);
    return &buffer.back();
}

bool GDKDTree::contains_point(Vector3 position, float margin)
{
    if (buffer.capacity() == buffer.size()) {
        return false;
    }

    if (root == nullptr) {
        root = alloc_node(position);
        return false;
    }

    int axis = 0;
    KDNode* node = root;
    while (node != nullptr) {
        if (node->position.distance_squared_to(position) < margin * margin) {
            return true;
        }
        switch (axis) {
        case 0:
            if (position.x < node->position.x) {
                if (node->left == nullptr) {
                    node->left = alloc_node(position);
                    node = nullptr;
                } else {
                    node = node->left;
                }
            } else {
                if (node->right == nullptr) {
                    node->right = alloc_node(position);
                    node = nullptr;
                } else {
                    node = node->right;
                }
            }
            break;
        case 1:
            if (position.y < node->position.y) {
                if (node->left == nullptr) {
                    node->left = alloc_node(position);
                    node = nullptr;
                } else {
                    node = node->left;
                }
            } else {
                if (node->right == nullptr) {
                    node->right = alloc_node(position);
                    node = nullptr;
                } else {
                    node = node->right;
                }
            }
            break;
        case 2:
            if (position.z < node->position.z) {
                if (node->left == nullptr) {
                    node->left = alloc_node(position);
                    node = nullptr;
                } else {
                    node = node->left;
                }
            } else {
                if (node->right == nullptr) {
                    node->right = alloc_node(position);
                    node = nullptr;
                } else {
                    node = node->right;
                }
            }
            break;
        }
        axis += 1;
        if (axis > 2) {
            axis = 0;
        }
    }

    return false;
}

void GDKDTree::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("reset_buffer", "buffer_size"), &GDKDTree::reset_buffer);
    ClassDB::bind_method(D_METHOD("contains_point", "position", "margin"), &GDKDTree::contains_point);
}