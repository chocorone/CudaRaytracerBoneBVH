#pragma once

#include "../hitable/hitable.h"
#include "texture.h" 

__device__ vec3 reflect(const vec3& v, const vec3& n) {
    return v - 2.0f * dot(v, n) * n;
}


__device__ vec3 random_in_unit_sphere(curandState* state) {
    vec3 p;
    do {
        p = 2.0 * vec3(curand_uniform(state),
            curand_uniform(state),
            curand_uniform(state)) - vec3(1, 1, 1);
    } while (p.squared_length() >= 1.0f);
    return p;
}


__device__ float schlick(float cosine, float ref_idx) {
    float r0 = (1.0f - ref_idx) / (1.0f + ref_idx);
    r0 = r0 * r0;
    return r0 + (1.0f - r0) * pow((1.0f - cosine), 5.0f);
}


__device__ bool refract(const vec3& v, const vec3& n, float ni_over_nt, vec3& refracted) {
    vec3 uv = unit_vector(v);
    float dt = dot(uv, n);
    float discriminant = 1.0f - ni_over_nt * ni_over_nt * (1 - dt * dt);
    if (discriminant > 0) {
        refracted = ni_over_nt * (uv - n * dt) - n * sqrt(discriminant);
        return true;
    }
    else
        return false;
}


class Material {
public:
    __device__ virtual bool scatter(const Ray& r_in,
        const HitRecord& rec,
        vec3& attenuation,
        Ray& scattered,
        curandState* state) const = 0;
    __device__ virtual vec3 emitted(float u,
        float v,
        const vec3& p) const {
        return vec3(0, 0, 0);
    }
};


class Lambertian : public Material {
public:
    __device__ Lambertian(Texture* a) : albedo(a) {}

    __device__ virtual bool scatter(const Ray& r_in,
        const HitRecord& rec,
        vec3& attenuation,
        Ray& scattered,
        curandState* state) const {
        vec3 target = rec.p + rec.normal + random_in_unit_sphere(state);
        //vec3 target = rec.p + rec.normal;
        scattered = Ray(rec.p, target - rec.p, r_in.time());
        attenuation = albedo->value(0, 0, rec.p);
        return true;
    }

    Texture* albedo;
};


class Metal : public Material {
public:
    __device__ Metal(const vec3& a, float f) : albedo(a) {
        fuzz = f < 1 ? f : 1;
    }

    __device__ virtual bool scatter(const Ray& r_in,
        const HitRecord& rec,
        vec3& attenuation,
        Ray& scattered,
        curandState* state) const {
        vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
        scattered = Ray(rec.p, reflected + fuzz * random_in_unit_sphere(state));
        attenuation = albedo;
        return (dot(scattered.direction(), rec.normal) > 0.0f);
    }

    vec3 albedo;
    float fuzz;
};


class Dielectric : public Material {
public:
    __device__ Dielectric(float ri) : ref_idx(ri) {}

    __device__ virtual bool scatter(const Ray& r_in,
        const HitRecord& rec,
        vec3& attenuation,
        Ray& scattered,
        curandState* state) const {
        vec3 outward_normal;
        vec3 reflected = reflect(r_in.direction(), rec.normal);
        float ni_over_nt;
        attenuation = vec3(1.0, 1.0, 1.0);
        vec3 refracted;
        float reflect_prob;
        float cosine;
        if (dot(r_in.direction(), rec.normal) > 0.0f) {
            outward_normal = -rec.normal;
            ni_over_nt = ref_idx;
            cosine = dot(r_in.direction(), rec.normal) / r_in.direction().length();
            cosine = sqrt(1 - ref_idx * ref_idx * (1 - cosine * cosine));
        }
        else {
            outward_normal = rec.normal;
            ni_over_nt = 1.0f / ref_idx;
            cosine = -dot(r_in.direction(), rec.normal) / r_in.direction().length();
        }

        if (refract(r_in.direction(), outward_normal, ni_over_nt, refracted)) {
            reflect_prob = schlick(cosine, ref_idx);
        }
        else {
            reflect_prob = 1.0f;
        }

        if (curand_uniform(state) < reflect_prob) {
            scattered = Ray(rec.p, reflected);
        }
        else {
            scattered = Ray(rec.p, refracted);
        }
        return true;
    }
    float ref_idx;
};


class DiffuseLight : public Material {
public:
    __device__ DiffuseLight(Texture* texture) : emit(texture) {}
    __device__ virtual bool scatter(const Ray& r_in,
        const HitRecord& rec,
        vec3& attenuation,
        Ray& scattered,
        curandState* state) const {
        return false;
    }
    __device__ virtual vec3 emitted(float u, float v, const vec3& p) const {
        return emit->value(u, v, p);
    }

    Texture* emit;
};

