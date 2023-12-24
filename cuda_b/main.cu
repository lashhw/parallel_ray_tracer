#include <fstream>
#include "camera_t.h"
#include "scene_t.h"
#include "render.h"

__global__ void init_kernel(vec3_t* d_framebuffer) {
    int thread_id = (int)(blockIdx.x * blockDim.x + threadIdx.x);
    if (thread_id >= NUM_PIXELS)
        return;

    d_framebuffer[thread_id] = vec3_t::make_zeros();
}

__global__ void render_kernel(camera_t* d_camera, scene_t* d_scene, vec3_t* d_framebuffer) {
    int thread_id = (int)(blockIdx.x * blockDim.x + threadIdx.x);
    if (thread_id >= NUM_PIXELS * SAMPLES_PER_PIXEL)
        return;

    int pixel_idx = thread_id / SAMPLES_PER_PIXEL;
    int x = pixel_idx % IMAGE_WIDTH;
    int y = pixel_idx / IMAGE_WIDTH;

    curandState rand_state;
    curand_init(0, thread_id, 0, &rand_state);
    float s = float(x) / float(IMAGE_WIDTH - 1);
    float t = 1.0f - float(y) / float(IMAGE_HEIGHT - 1);
    ray_t camera_ray = d_camera->get_ray(s, t);
    vec3_t color = get_color(*d_scene, camera_ray, rand_state);
    d_framebuffer[pixel_idx].atomic_add(color);
}

int main() {
    // camera
    vec3_t lookfrom(0.5f, 0.5f, 1.0f);
    vec3_t lookat(0.5f, 0.5f, 0.0f);
    vec3_t vup(0.0f, 1.0f, 0.0f);
    float vfov = 55.0f;
    float aspect_ratio = 1.0f;
    camera_t camera(lookfrom, lookat, vup, vfov, aspect_ratio);
    camera_t* d_camera;
    CHECK_CUDA(cudaMalloc(&d_camera, sizeof(camera_t)));
    CHECK_CUDA(cudaMemcpy(d_camera, &camera, sizeof(camera_t), cudaMemcpyHostToDevice));

    // color
    vec3_t red(0.65f, 0.05f, 0.05f);
    vec3_t green(0.12f, 0.45f, 0.15f);
    vec3_t white(0.73f, 0.73f, 0.73f);
    vec3_t brown(0.62f, 0.57f, 0.54f);

    // scene
    std::vector<sphere_t> spheres;
    std::vector<trig_t> trigs;
    auto add_rectangle = [&](const vec3_t &p0, const vec3_t &p1, const vec3_t &p2,
                             const vec3_t &p3, const vec3_t &albedo) {
        trigs.emplace_back(p0, p1, p2, albedo);
        trigs.emplace_back(p2, p3, p0, albedo);
    };
    spheres.emplace_back(vec3_t(0.5f, 0.2f, -0.25f), 0.2f, brown);
    add_rectangle(vec3_t(0.0f, 0.0f, 0.0f),
                  vec3_t(0.0f, 1.0f, 0.0f),
                  vec3_t(0.0f, 1.0f, -1.0f),
                  vec3_t(0.0f, 0.0f, -1.0f),
                  red);
    add_rectangle(vec3_t(0.0f, 0.0f, -1.0f),
                  vec3_t(0.0f, 1.0f, -1.0f),
                  vec3_t(1.0f, 1.0f, -1.0f),
                  vec3_t(1.0f, 0.0f, -1.0f),
                  white);
    add_rectangle(vec3_t(1.0f, 0.0f, 0.0f),
                  vec3_t(1.0f, 1.0f, 0.0f),
                  vec3_t(1.0f, 1.0f, -1.0f),
                  vec3_t(1.0f, 0.0f, -1.0f),
                  green);
    add_rectangle(vec3_t(0.0f, 1.0f, 0.0f),
                  vec3_t(0.0f, 1.0f, -1.0f),
                  vec3_t(1.0f, 1.0f, -1.0f),
                  vec3_t(1.0f, 1.0f, 0.0f),
                  white);
    add_rectangle(vec3_t(0.0f, 0.0f, 0.0f),
                  vec3_t(0.0f, 0.0f, -1.0f),
                  vec3_t(1.0f, 0.0f, -1.0f),
                  vec3_t(1.0f, 0.0f, 0.0f),
                  white);
    scene_t scene{};
    scene.num_spheres = (int)spheres.size();
    CHECK_CUDA(cudaMalloc(&scene.spheres, scene.num_spheres * sizeof(sphere_t)));
    CHECK_CUDA(cudaMemcpy(scene.spheres, spheres.data(), scene.num_spheres * sizeof(sphere_t), cudaMemcpyHostToDevice));
    scene.num_trigs = (int)trigs.size();
    CHECK_CUDA(cudaMalloc(&scene.trigs, scene.num_trigs * sizeof(trig_t)));
    CHECK_CUDA(cudaMemcpy(scene.trigs, trigs.data(), scene.num_trigs * sizeof(trig_t), cudaMemcpyHostToDevice));
    scene.point_light = {vec3_t(0.95f, 0.95f, 0.3f), vec3_t(0.9f, 0.9f, 0.9f)};
    scene_t* d_scene;
    CHECK_CUDA(cudaMalloc(&d_scene, sizeof(scene_t)));
    CHECK_CUDA(cudaMemcpy(d_scene, &scene, sizeof(scene_t), cudaMemcpyHostToDevice));

    // render
    vec3_t* d_framebuffer;
    CHECK_CUDA(cudaMalloc(&d_framebuffer, NUM_PIXELS * sizeof(vec3_t)));
    init_kernel<<<(NUM_PIXELS + BLOCK_SIZE - 1) / BLOCK_SIZE, BLOCK_SIZE>>>(d_framebuffer);
    CHECK_CUDA(cudaGetLastError());
    render_kernel<<<(NUM_PIXELS * SAMPLES_PER_PIXEL + BLOCK_SIZE - 1), BLOCK_SIZE>>>(d_camera, d_scene, d_framebuffer);
    CHECK_CUDA(cudaGetLastError());

    // write framebuffer to file
    vec3_t framebuffer[NUM_PIXELS];
    CHECK_CUDA(cudaMemcpy(framebuffer, d_framebuffer, NUM_PIXELS * sizeof(vec3_t), cudaMemcpyDeviceToHost));
    std::ofstream image_fs("image.ppm");
    image_fs << "P3\n" << IMAGE_WIDTH << ' ' << IMAGE_HEIGHT << "\n255\n";
    for (int i = 0; i < IMAGE_HEIGHT; i++) {
        for (int j = 0; j < IMAGE_WIDTH; j++) {
            vec3_t color = framebuffer[i * IMAGE_WIDTH + j] / SAMPLES_PER_PIXEL;
            color.write_color(image_fs);
        }
    }

    return 0;
}
