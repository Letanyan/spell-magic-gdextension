#ifndef GDKDTREE_H
#define GDKDTREE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <vector>

namespace godot {

struct KDNode {
public:
    Vector3 position;
    KDNode *left, *right;
    KDNode();
    KDNode(Vector3 position);
};

class GDKDTree : public RefCounted {
    GDCLASS(GDKDTree, RefCounted)

    std::vector<KDNode> buffer;
    KDNode* root;

protected:
    static void _bind_methods();

public:
    GDKDTree(int32_t buffer_capacity);
    GDKDTree();
    ~GDKDTree();

    KDNode* alloc_node(Vector3 position);

    void reset_buffer(int32_t buffer_size);
    bool contains_point(Vector3 position, float margin);
};

}

#endif