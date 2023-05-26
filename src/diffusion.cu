#include <iostream>

namespace DiffusionSimulation {
    /**
     * TODO:
     *  -Check cudaMallocPitch vs. cudaMallocManaged (for speed)
     *  -Check breaking if-else possibilities into separate kernels
     *  -See if I can split the threads that handle the boundaries into their own warps.
     *  -Error checking - validate that the diffusion kernel runs correctly, with initially uniform
     *      concentration, and a "drop" in the middle
     *  -Get diffusion simulation processing more than one time step, by swapping the arrays each time step
     *      *Simple - synch with device, perform swap on CPU
     *      *Maybe harder - make a kernel that performs the swap, so it all happens on the GPU.
     *  -Set up diffusion simulation to output to screen
     *  -Something else?
    */
    __global__
    void Diffusion(float** current, float** next, float xN, float yN, float dx, float dy, float dt, float D) {
        const float xFactor = D*dt*dy/dx;
        const float yFactor = D*dt*dx/dy;
        const int xIndex = blockIdx.x*blockDim.x + threadIdx.x;
        const int yIndex = blockIdx.y*blockDim.y + threadIdx.y;
        // Assuming row-major order, 0,0 is at the bottom-left corner
        const float oldVal = current[yIndex][xIndex];
        float yDiff = 0.0f;
        if (yIndex < (yN-1) && yIndex > 0) {
            yDiff = (current[yIndex+1][xIndex]-2*current[yIndex][xIndex]+current[yIndex-1][xIndex])/(dy*dy);
        } else {
            if (yIndex == 0) {
                yDiff = (-current[yIndex+2][xIndex]+8*current[yIndex+2][xIndex]-7*current[yIndex][xIndex])/(2*dy*dy);
            } else {
                yDiff = (-current[yIndex-2][xIndex]+8*current[yIndex-1][xIndex]-7*current[yIndex][xIndex])/(2*dy*dy);
            }
        }
        float xDiff = 0.0f;
        if (xIndex < (xN-1) && xIndex > 0) {
            // (d^2/dx^2)(f(x, y, t)) = (-f(x+2dx)+8*f(x-dx)-7*f(x))/(2dx^2) + O(h^2)
            xDiff = (current[yIndex][xIndex+1]-2*current[yIndex][xIndex]+current[yIndex][xIndex-1])/(dx*dx);
        } else {
            if (xIndex == 0) {
                xDiff = (-current[yIndex][xIndex+2]+8*current[yIndex][xIndex+1]-7*current[yIndex][xIndex])/(2*dx*dx);
            } else {
                xDiff = (-current[yIndex][xIndex-2]+8*current[yIndex][xIndex-1]-7*current[yIndex][xIndex])/(2*dx*dx);
            }
        }
        float phiDiff = -D*(xDiff+yDiff)*dt;
        next[yIndex][xIndex] = phiDiff;
    }

    void RunDiffusion() {
        float xLength = 1000;
        float yLength = 1000;
        
        float D = 1.0f;

        float dx = 0.1f;
        float dy = 0.1;
        float dt = 0.1f;

        int xSize = (int) ceil(xLength/dx);
        int ySize = (int) ceil(yLength/dy);

        float **current;
        float **next;

        cudaMallocManaged(&current, xLength*yLength*sizeof(float));
        cudaMallocManaged(&next, xLength*yLength*sizeof(float));
        for (size_t y = 0; y < ySize; y++) {
            for (size_t x = 0; x < xSize; x++) {
                current[y][x] = 0.0f;
                next[y][x] = 0.0f;
            }
        }
        
        float initialTotal = 0.0f;
        for (size_t y = 0; y < ySize; y++) {
            for (size_t x = 0; x < xSize; x++) {
                initialTotal += current[y][x];
            }
        }

        Diffusion<<<1, 1>>>(current, next, xSize, ySize, dx, dy, dt, D);
        cudaDeviceSynchronize();
        
        float finalTotal = 0.0f;
        for (size_t y = 0; y < ySize; y++) {
            for (size_t x = 0; x < xSize; x++) {
                finalTotal += current[y][x];
            }
        }

        printf("Difference: %f", finalTotal-initialTotal);

        cudaFree(current);
        cudaFree(next);
    }
}