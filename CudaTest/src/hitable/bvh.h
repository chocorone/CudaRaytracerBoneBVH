#pragma once

#include <thrust/sort.h>
#include <curand.h>
#include <curand_kernel.h>
#include "hitable.h"


struct BoxCompare {
    __device__ BoxCompare(int m) : mode(m) {}
    __device__ bool operator()(Hitable* a, Hitable* b) const {
        // return true;

        AABB box_left, box_right;
        Hitable* ah = a;
        Hitable* bh = b;

        if (!ah->GetBV(0, 0, box_left) || !bh->GetBV(0, 0, box_right)) {
            return false;
        }

        float val1, val2;
        if (mode == 1) {
            val1 = box_left.min().x();
            val2 = box_right.min().x();
        }
        else if (mode == 2) {
            val1 = box_left.min().y();
            val2 = box_right.min().y();
        }
        else if (mode == 3) {
            val1 = box_left.min().z();
            val2 = box_right.min().z();
        }

        if (val1 - val2 < 0.0) {
            return false;
        }
        else {
            return true;
        }
    }
    // mode: 1, x; 2, y; 3, z
    int mode;
};


class BVHNode : public Hitable {
public:
    __device__ BVHNode() {}
    __device__ BVHNode(Hitable** l,
        int n,
        float time0,
        float time1,
        curandState* state) ;

    __device__ virtual bool collision_detection(const Ray& r,
        float t_min,
        float t_max,
        HitRecord& rec, int frameIndex) const;

    __device__ virtual bool bounding_box(float t0,
        float t1,
        AABB& b) const;

    __device__ void UpdateBVH();

    Hitable* left;
    Hitable* right;
    AABB box;
    bool childIsNode;
};


__device__ BVHNode::BVHNode(Hitable** l,
    int n,
    float time0,
    float time1,
    curandState* state) {
    transform->ResetTransform();
    //printf("transform %f,%f,%f\n", transform->rotation.x(), transform->rotation.y(), transform->rotation.z());

    int axis = int(3 * curand_uniform(state));
    if (axis == 0) {
        thrust::sort(l, l + n, BoxCompare(1));
    }
    else if (axis == 1) {
        thrust::sort(l, l + n, BoxCompare(2));
    }
    else {
        thrust::sort(l, l + n, BoxCompare(3));
    }

    if (n == 1) {
        left = right = l[0];
        childIsNode = false;
    }
    else if (n == 2) {
        left = l[0];
        right = l[1];
        childIsNode = false;
    }
    else {
        left = new BVHNode(l, n / 2, time0, time1, state);
        right = new BVHNode(l + n / 2, n - n / 2, time0, time1, state);
        childIsNode = true;
    }

    AABB box_left, box_right;
    if (!left->GetBV(time0, time1, box_left) ||
        !right->GetBV(time0, time1, box_right)) {
        return;
        // std::cerr << "no bounding box in BVHNode constructor \n";
    }
   box = surrounding_box(box_left, box_right);
}


__device__ bool BVHNode::bounding_box(float t0,
    float t1,
    AABB& b) const {
    b = box;
    return true;
}

__device__ void BVHNode::UpdateBVH()
{
    if (childIsNode) {
        ((BVHNode*)left)->UpdateBVH();
        ((BVHNode*)right)->UpdateBVH();
    }

    AABB box_left, box_right;
    if (!left->GetBV(0, 1, box_left) ||
        !right->GetBV(0, 1, box_right)) {
        return;
        // std::cerr << "no bounding box in BVHNode constructor \n";
    }
    //�g��
    box = surrounding_box(box_left, box);
    box = surrounding_box(box_right, box);
}

__device__ bool BVHNode::collision_detection(const Ray& r,
    float t_min,
    float t_max,
    HitRecord& rec, int frameIndex) const {
    if (box.hit(r, t_min, t_max)) {
        HitRecord left_rec, right_rec;
        bool hit_left = left->hit(r, t_min, t_max, left_rec, frameIndex);
        bool hit_right = right->hit(r, t_min, t_max, right_rec, frameIndex);

        if (hit_left && hit_right) {
            if (left_rec.t < right_rec.t) {
                rec = left_rec;
            }
            else {
                rec = right_rec;
            }
            return true;
        }
        else if (hit_left) {
            rec = left_rec;
            return true;
        }
        else if (hit_right) {
            rec = right_rec;
            return true;
        }
        else {
            return false;
        }
    }
    return false;
}
