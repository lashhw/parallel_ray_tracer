#include <fstream>
#include "camera_t.h"
#include "scene_t.h"
#include "render.h"
#include <immintrin.h>

#define __AVX2_AVAILABLE__

int main() {

    // camera
    vec3_t lookfrom(0.5f, 0.5f, 1.0f);
    vec3_t lookat(0.5f, 0.5f, 0.0f);
    vec3_t vup(0.0f, 1.0f, 0.0f);
    float vfov = 55.0f;
    float aspect_ratio = 1.0f;
    camera_t camera(lookfrom, lookat, vup, vfov, aspect_ratio);

    // color
    vec3_t red(0.65f, 0.05f, 0.05f);
    vec3_t green(0.12f, 0.45f, 0.15f);
    vec3_t white(0.73f, 0.73f, 0.73f);
    vec3_t brown(0.62f, 0.57f, 0.54f);

    // scene
    scene_t scene;
    scene.spheres.emplace_back(vec3_t(0.5f, 0.2f, -0.25f), 0.2f, brown);
    scene.add_rectangle(vec3_t(0.0f, 0.0f, 0.0f),
        vec3_t(0.0f, 1.0f, 0.0f),
        vec3_t(0.0f, 1.0f, -1.0f),
        vec3_t(0.0f, 0.0f, -1.0f),
        red);
    scene.add_rectangle(vec3_t(0.0f, 0.0f, -1.0f),
        vec3_t(0.0f, 1.0f, -1.0f),
        vec3_t(1.0f, 1.0f, -1.0f),
        vec3_t(1.0f, 0.0f, -1.0f),
        white);
    scene.add_rectangle(vec3_t(1.0f, 0.0f, 0.0f),
        vec3_t(1.0f, 1.0f, 0.0f),
        vec3_t(1.0f, 1.0f, -1.0f),
        vec3_t(1.0f, 0.0f, -1.0f),
        green);
    scene.add_rectangle(vec3_t(0.0f, 1.0f, 0.0f),
        vec3_t(0.0f, 1.0f, -1.0f),
        vec3_t(1.0f, 1.0f, -1.0f),
        vec3_t(1.0f, 1.0f, 0.0f),
        white);
    scene.add_rectangle(vec3_t(0.0f, 0.0f, 0.0f),
        vec3_t(0.0f, 0.0f, -1.0f),
        vec3_t(1.0f, 0.0f, -1.0f),
        vec3_t(1.0f, 0.0f, 0.0f),
        white);
    scene.point_light = { vec3_t(0.95f, 0.95f, 0.3f), vec3_t(0.9f, 0.9f, 0.9f) };

    // render
    int image_width = 600;
    int image_height = 600;
    vec3_t framebuffer[image_height][image_width];
    for (int i = 0; i < image_height; i++) {
        for (int j = 0; j < image_width; j++) {
            float s = float(j) / float(image_width - 1);
            float t = 1.0f - float(i) / float(image_height - 1);
            ray_t camera_ray = camera.get_ray(s, t);
            framebuffer[i][j] = vec3_t::make_zeros();
            for (int k = 1; k <= SAMPLES_PER_PIXEL; k++)
                framebuffer[i][j] = framebuffer[i][j] + get_color(scene, camera_ray);
            framebuffer[i][j] = framebuffer[i][j] / SAMPLES_PER_PIXEL;
        }
    }

    // write framebuffer to file
    std::ofstream image_fs("image.ppm");
    image_fs << "P3\n" << image_width << ' ' << image_height << "\n255\n";
    for (int i = 0; i < image_height; i++)
        for (int j = 0; j < image_width; j++)
            framebuffer[i][j].write_color(image_fs);

    return 0;
}
