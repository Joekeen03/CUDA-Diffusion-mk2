#include "imageRenderer.h"
#include "diffusion.cuh"

int main(int argc, char** argv) {
    constexpr int width = 1000;
    constexpr int height = 800;
    // if (!Rendering::Setup(argc, argv, width, height)) {
    //     return -1;
    // }
    // return Rendering::Run();
    DiffusionSimulation::RunDiffusion();
}