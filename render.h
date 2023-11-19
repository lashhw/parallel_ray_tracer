#ifndef PARALLEL_RAY_TRACER_RENDER_H
#define PARALLEL_RAY_TRACER_RENDER_H

bool occluded(const scene_t &scene, void* excluded_obj, ray_t &ray) {
    record_t record{};
    for (const auto &sphere : scene.spheres)
        if (&sphere != excluded_obj && sphere.hit(ray, record))
            return true;
    for (const auto &trig : scene.trigs)
        if (&trig != excluded_obj && trig.hit(ray, record))
            return true;
    return false;
}

vec3_t get_color(const scene_t &scene, ray_t &ray) {
    vec3_t color = vec3_t::make_zeros();
    vec3_t multiplier = vec3_t::make_ones();

    for (int i = 1; i <= PATH_MAX; i++) {
        bool hit = false;
        void* hit_obj;
        record_t record{};
        for (const auto &sphere : scene.spheres) {
            if (sphere.hit(ray, record)) {
                hit = true;
                hit_obj = (void*)(&sphere);
            }
        }
        for (const auto &trig : scene.trigs) {
            if (trig.hit(ray, record)) {
                hit = true;
                hit_obj = (void*)(&trig);
            }
        }

        if (hit) {
            for (const auto &light : scene.point_lights) {
                vec3_t dir = light.position - record.hit_point;

                // not in same hemisphere
                if (dot(ray.direction, record.unit_n) * dot(dir, record.unit_n) >= 0.f)
                    break;

                ray_t shadow_ray(record.hit_point, dir, EPS, 1.0f);
                if (!occluded(scene, hit_obj, shadow_ray)) {
                    float t2 = shadow_ray.direction.length_squared();
                    float t = std::sqrt(t2);
                    color = color + multiplier * light.intensity / t2 * dot(shadow_ray.direction, record.unit_n) / t;
                }
            }

            ray = {record.hit_point,
                   record.unit_n + vec3_t::uniform_sample_sphere(),
                   EPS,
                   std::numeric_limits<float>::max()};
            multiplier = multiplier * vec3_t(0.7f, 0.7f, 0.7f);
        } else {
            break;
        }
    }

    return color;
}

#endif //PARALLEL_RAY_TRACER_RENDER_H
