#ifndef PARALLEL_RAY_TRACER_CAMERA_T_H
#define PARALLEL_RAY_TRACER_CAMERA_T_H

#include <cmath>
#include <cfloat>
#include "ray_t.h"

class camera_t {
public:
    camera_t(
        vec3_t lookfrom,
        vec3_t lookat,
        vec3_t vup,
        float vfov_deg,
        float aspect_ratio
    ) {
        float vfov_rad = degree_to_radian(vfov_deg);
        float viewpoint_height = 2.0f * std::tan(vfov_rad / 2.0f);
        float viewpoint_width = viewpoint_height * aspect_ratio;

        vec3_t w = (lookfrom - lookat).unit_vector();
        vec3_t v = (vup - dot(vup, w) * w).unit_vector();
        vec3_t u = cross(v, w);

        origin = lookfrom;
        horizontal = viewpoint_width * u;
        vertical = viewpoint_height * v;
        lower_left_corner = origin - horizontal / 2 - vertical / 2 - w;
    }

    __device__ ray_t get_ray(float s, float t) const {
        return {
            origin,
            lower_left_corner + s * horizontal + t * vertical - origin,
            0.0f,
            FLT_MAX
        };
    }

private:
    vec3_t origin;
    vec3_t horizontal;
    vec3_t vertical;
    vec3_t lower_left_corner;
};

#endif //PARALLEL_RAY_TRACER_CAMERA_T_H
