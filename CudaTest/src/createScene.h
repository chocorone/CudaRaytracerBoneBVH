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

__global__ void init_data(HitableList** world, TransformList** transformPointer) 
{
    *world = new HitableList();
    *transformPointer = new TransformList();
}

//BVHの作成
__global__ void create_BVH(HitableList** world, BVHNode** bvh,curandState* state) {
    *bvh = new BVHNode((*world)->list, (*world)->list_size, 0, 1, state);
    (*bvh)->transform->rotation = vec3(0, 45, 0);
}

// オブジェクトの生成
__global__ void add_object(HitableList** world,  TransformList** transformPointer)
{
    if (threadIdx.x == 0 && blockIdx.x == 0)
    {

        Texture* checker = new CheckerTexture(new ConstantTexture(vec3(0.2, 0.3, 0.1)),
            new ConstantTexture(vec3(0.9, 0.9, 0.9)));
        (*world)->append(new Rectangle(new Lambertian(new ConstantTexture(vec3(0.8, 0.1, 0.1))), false, new Transform(vec3(-2.5, -2.5, 0), vec3(0), vec3(5, 2, 1))));
        (*world)->append(new Rectangle(new Lambertian(new ConstantTexture(vec3(0.1, 0.8, 0.1))), false, new Transform(vec3(0, 0, 0), vec3(0), vec3(2, 10, 1))));
        (*world)->append(new Rectangle(new Lambertian(new ConstantTexture(vec3(0.1, 0.1, 0.8))),false, new Transform(vec3(3, 2, 0), vec3(0), vec3(4, 4, 1))));
        //(*world)->append(new Sphere(new Transform(vec3(-10,-10, 0), vec3(0), vec3(1)), 1, new Lambertian(new ConstantTexture(vec3(0.8, 0.1, 0.1)))));
        //(*world)->append(new Sphere(new Transform(), 1, new Lambertian(new ConstantTexture(vec3(0.1, 0.8, 0.1)))));
        //(*world)->append(new Sphere(new Transform(vec3(5, 5, 0), vec3(0), vec3(1)), 1, new Lambertian(new ConstantTexture(vec3(0.1, 0.1, 0.8)))));
       

        /*Transform* transform1 = new Transform(vec3(0, 1, 0), vec3(0), vec3(1));
        Transform* transform2 = new Transform(vec3(-4, 1, 0), vec3(0), vec3(1));
        Transform* transform3 = new Transform(vec3(4, 1, 0), vec3(0), vec3(1));

        (*world)->append(new Sphere(transform1, 1.0, new Lambertian(checker)));
        (*world)->append(new Sphere(transform2, 1.0, new Dielectric(1.5)));
        (*world)->append(new Sphere(transform3, 1.0, new Metal(vec3(0.7, 0.6, 0.5), 0.0)));

        (*transformPointer)->append(transform1);
        (*transformPointer)->append(transform2);
        (*transformPointer)->append(transform3);*/
    }
}

__global__ void add_mesh_withNormal(HitableList** world, MeshData* data, TransformList** transformPointer)
{
    if (threadIdx.x == 0 && blockIdx.x == 0)
    {
        Material* mat = new Lambertian(new ConstantTexture(vec3(0.65, 0.05, 0.05)));

        int l = 0;
        for (int i = 0; i < data->nTriangles; i++) {
            vec3 idx = data->idxVertex[i];
            vec3 v[3] = { data->points[int(idx[2])], data->points[int(idx[1])], data->points[int(idx[0])] };
            Transform* transform = new Transform(vec3(0), vec3(0,0,0), vec3(1));
            //(*transformPointer)->append(transform);//とりあえずなしで
            (*world)->append(new Triangle(v, data->normals[i], mat, false, transform, true));
        }
    }
}


__global__ void cornell_box_scene(Hitable** world, Hitable** list, curandState* state)
{
    if (threadIdx.x == 0 && blockIdx.x == 0)
    {
        Material* red = new   Lambertian(new ConstantTexture(vec3(0.65, 0.05, 0.05)));
        Material* white = new   Lambertian(new ConstantTexture(vec3(0.73, 0.73, 0.73)));
        Material* green = new   Lambertian(new ConstantTexture(vec3(0.12, 0.45, 0.15)));
        Material* light = new DiffuseLight(new ConstantTexture(vec3(15, 15, 15)));

        Material* diele = new Dielectric(0.5);
        Material* metal = new Metal(vec3(0.7, 0.6, 0.5), 0.3);

        int listIndex = 0;
        //list[listIndex++] = new FlipNormals(new RectangleYZ(0, 555, 0, 555, 555, green));
        //list[listIndex++] = (new RectangleYZ(0, 555, 0, 555, 0, red));
        //list[listIndex++] = (new RectangleXZ(213, 343, 227, 332, 554, light));
        //list[listIndex++] = new FlipNormals(new RectangleXZ(0, 555, 0, 555, 555, white));
        //list[listIndex++] = (new RectangleXZ(0, 555, 0, 555, 0, white));
        //list[listIndex++] = new FlipNormals(new RectangleXY(0, 555, 0, 555, 555, white));
        list[listIndex++] = new Rectangle(red,true,new Transform(vec3(0,0,10),vec3(0,0,90),vec3(6,3,1)));
        //list[listIndex++] = new Sphere(new Transform(vec3(0),vec3(0),vec3(10,10,1)), 1, red);



        *world = new HitableList(list, listIndex);
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

