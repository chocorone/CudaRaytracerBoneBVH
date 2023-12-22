#pragma once

#include <curand.h>
#include <curand_kernel.h>

#include <float.h>


#include "hitable/hitable.h"
#include "hitable/bvh.h"
#include "hitable/hitable_list.h"
#include "core/camera.h"
#include "shapes/sphere.h"
#include "shapes/triangle.h"
#include "shapes/box.h"
#include "material/material.h"
#include "hitable/animationData.h"
#include "shapes/MeshObject.h"

__device__ float rand(curandState* state) {
    return float(curand_uniform(state));
}

__global__ void init_data(HitableList** list, TransformList** transformPointer)
{
    *list = new HitableList();
    *transformPointer = new TransformList();
}

//BVHの作成
__global__ void create_BVH(HitableList** list, BVHNode** bvh,curandState* state) {
    *bvh = new BVHNode((*list)->list, (*list)->list_size, 0, 1, state);
    //(*bvh)->transform->rotation = vec3(0, 45, 0);
}

// オブジェクトの生成
__global__ void add_object(HitableList** list,  TransformList** transformPointer)
{
    if (threadIdx.x == 0 && blockIdx.x == 0)
    {

        Texture* checker = new CheckerTexture(new ConstantTexture(vec3(0.2, 0.3, 0.1)),
            new ConstantTexture(vec3(0.9, 0.9, 0.9)));
        //(*list)->append(new Sphere(new Transform(vec3(-10,-10, 0), vec3(0), vec3(1)), 1, new Lambertian(new ConstantTexture(vec3(0.8, 0.1, 0.1)))));
        //(*list)->append(new Sphere(new Transform(), 1, new Lambertian(new ConstantTexture(vec3(0.1, 0.8, 0.1)))));
        //(*list)->append(new Sphere(new Transform(vec3(5, 5, 0), vec3(0), vec3(1)), 1, new Lambertian(new ConstantTexture(vec3(0.1, 0.1, 0.8)))));
       

        /*Transform* transform1 = new Transform(vec3(0, 1, 0), vec3(0), vec3(1));
        Transform* transform2 = new Transform(vec3(-4, 1, 0), vec3(0), vec3(1));
        Transform* transform3 = new Transform(vec3(4, 1, 0), vec3(0), vec3(1));

        (*list)->append(new Sphere(transform1, 1.0, new Lambertian(checker)));
        (*list)->append(new Sphere(transform2, 1.0, new Dielectric(1.5)));
        (*list)->append(new Sphere(transform3, 1.0, new Metal(vec3(0.7, 0.6, 0.5), 0.0)));

        (*transformPointer)->append(transform1);
        (*transformPointer)->append(transform2);
        (*transformPointer)->append(transform3);*/
    }
}

__global__ void add_mesh_withNormal(HitableList** list, MeshData* data, TransformList** transformPointer)
{
    if (threadIdx.x == 0 && blockIdx.x == 0)
    {
        Material* mat = new Lambertian(new ConstantTexture(vec3(0.65, 0.05, 0.05)));
        for (int i = 0; i < data->nTriangles; i++) {
            vec3 idx = data->idxVertex[i];
            vec3 v[3] = { data->points[int(idx[2])], data->points[int(idx[1])], data->points[int(idx[0])] };
            Transform* transform = new Transform(vec3(0), vec3(0,0,0), vec3(1));
            //(*transformPointer)->append(transform);//とりあえずなしで
            (*list)->append(new Triangle(v, data->normals[i], mat, false, transform, true));
        }
    }
}




__global__ void add_mesh_withNormal(HitableList** list, FBXObject* data, TransformList** transformPointer)
{
    if (threadIdx.x == 0 && blockIdx.x == 0)
    {
        Material* mat = new Lambertian(new ConstantTexture(vec3(0.65, 0.05, 0.05)));
        for (int i = 0; i < data->mesh->nTriangles; i++) {
            vec3 idx = data->mesh->idxVertex[i];
            vec3 v[3] = { data->mesh->points[int(idx[2])], data->mesh->points[int(idx[1])], data->mesh->points[int(idx[0])] };
            Transform* transform = new Transform(vec3(0), vec3(0, 0, 0), vec3(1));
            //(*transformPointer)->append(transform);//とりあえずなしで
            (*list)->append(new Triangle(v, data->mesh->normals[i], mat, false, transform, true));
        }
    }
}

__global__ void add_mesh_fromPoseData(HitableList** list, FBXObject* data,BonePoseData* pose) {
    if (threadIdx.x == 0 && blockIdx.x == 0)
    {
        data->triangleData = (Triangle**)malloc(data->mesh->nTriangles*sizeof(Triangle*));
        Material* mat = new Lambertian(new ConstantTexture(vec3(0.65, 0.05, 0.05)));
        for (int i = 0; i < data->mesh->nTriangles; i++) {
            vec3 idx = data->mesh->idxVertex[i];
            vec3 v[3] = { data->mesh->points[int(idx[2])], data->mesh->points[int(idx[1])], data->mesh->points[int(idx[0])] };
            Triangle* triagnle = new Triangle(v, data->mesh->normals[i], mat, false, new Transform(), true);
            data->triangleData[i] = triagnle;
            (*list)->append(triagnle);
        }
    }
}

__global__ void update_mesh_fromPoseData(FBXObject* data, BonePoseData* pose) {
    if (threadIdx.x == 0 && blockIdx.x == 0)
    {
        //vec3* newPos = (vec3*)malloc(sizeof(vec3) * data->mesh->nPoints);
        //CalcFBXVertexPos(data,pose,newPos);

        for (int i = 0; i < data->mesh->nTriangles; i++) {
            //vec3 idx = data->mesh->idxVertex[i];
            /*vec3 v[3] = {data->mesh->points[int(idx[2])], data->mesh->points[int(idx[1])], data->mesh->points[int(idx[0])]};
            for (int vi = 0; vi < 3; vi++) {
                data->triangleData[i]->vertices[vi] = v[vi];
            }*/
            Lambertian* mat = (Lambertian*)data->triangleData[i]->material;
            ConstantTexture* tex = (ConstantTexture*)mat->albedo;
            tex->color = vec3(tex->color.r(), tex->color.g() + 0.1f>1?1: tex->color.g() + 0.1f, tex->color.b() + 0.1f > 1 ? 1 : tex->color.b() + 0.1f);
        }
    }
}



__global__ void create_camera(Camera** camera, int nx, int ny,
    vec3 lookfrom, vec3 lookat, float dist_to_focus, float aperture, float vfov)
{
    if (threadIdx.x == 0 && blockIdx.x == 0) {
        *camera = new Camera(lookfrom,
            lookat,
            vec3(0, 1, 0),
            vfov,
            float(nx) / float(ny),
            aperture,
            dist_to_focus);
    }
}

