#include <iostream>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <random>
#include <vector>
#include <chrono>

#define TILE_DIM 32
#define M_DIM 50

__global__ void multiplyNaive(const int *mat_1, const int *mat_2, int *mat_prod, const int n, const int p) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (y < n && x < p) {
        int sum = 0;
    
        for (int i = 0; i < M_DIM; i++) {
            sum += mat_1[y*M_DIM+i] * mat_2[i*p+x];
        }
    
        mat_prod[y*p+x] = sum;
    }
}

__global__ void multiplySharedMem(const int *mat_1, const int *mat_2, int *mat_prod, const int n, const int p) {
    __shared__ float aTile[TILE_DIM][M_DIM], bTile[M_DIM][TILE_DIM];

    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    
    int h = (M_DIM + blockDim.x - 1)/blockDim.x;
    
    int start_a = h*threadIdx.x;
    int end_a = M_DIM < h*(threadIdx.x+1)?M_DIM:h*(threadIdx.x+1);
    
    for (int i = start_a; i < end_a; i++) {
        aTile[threadIdx.y][i] = mat_1[y*M_DIM+i];
    }
    
    h = (M_DIM + blockDim.y - 1)/blockDim.y;
    
    int start_b = h*threadIdx.y;
    int end_b = M_DIM < h*(threadIdx.y+1)?M_DIM:h*(threadIdx.y+1);
    
    for (int i = start_b; i < end_b; i++) {
        bTile[i][threadIdx.x] = mat_2[i*p+x];
    }
    
    __syncthreads();
    
    if (y < n && x < p) {
        int sum = 0;
    
        for (int i = 0; i < M_DIM; i++) {
            sum += aTile[threadIdx.y][i] * bTile[i][threadIdx.x];
        }
    
        mat_prod[y*p+x] = sum;
    }
}

std::vector<std::vector<int>> random_matrix(const int num_rows, const int num_cols, const int min_val=0.0, const int max_val=1000.0) {
    std::vector<std::vector<int>> my_arr;
    static std::random_device rd;
    static std::mt19937 mte(rd());
    std::uniform_int_distribution<int> dist(min_val, max_val);
    
    for (int i = 0; i < num_rows; i++) {
        std::vector<int> my_arr_col;
        for (int j = 0; j < num_cols; j++) {
            my_arr_col.push_back(dist(mte));
        }
        my_arr.push_back(my_arr_col);
    }
    
    return my_arr;
}

bool check_correctness(const int *mat_1, const int *mat_2, int *mat_prod, const int n, const int p) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < p; j++) {
            int sum = 0;
            for (int k = 0; k < M_DIM; k++) {
                sum += mat_1[i*M_DIM+k] * mat_2[k*p+j];
            }
            if (sum != mat_prod[i*p+j]) {
                return false;
            }
        }
    }
    return true;
}

int main(void) {
    int n = 5000;
    int p = 8000;

    dim3 dimGrid((p + TILE_DIM - 1)/TILE_DIM, (n + TILE_DIM - 1)/TILE_DIM, 1);
    dim3 dimBlock(TILE_DIM, TILE_DIM, 1);
    
    int *mat_1, *mat_2, *mat_prod; 

    cudaMallocManaged(&mat_1, n*M_DIM*sizeof(int));
    cudaMallocManaged(&mat_2, M_DIM*p*sizeof(int));
    cudaMallocManaged(&mat_prod, n*p*sizeof(int));
    
    std::vector<std::vector<int>> my_arr_1 = random_matrix(n, M_DIM, 0, 10);
    std::vector<std::vector<int>> my_arr_2 = random_matrix(M_DIM, p, 0, 10);
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < M_DIM; j++) {
            mat_1[M_DIM*i + j] = my_arr_1[i][j];
        }
    }
    
    for (int i = 0; i < M_DIM; i++) {
        for (int j = 0; j < p; j++) {
            mat_2[p*i + j] = my_arr_2[i][j];
        }
    }
    
    auto t1 = std::chrono::high_resolution_clock::now();
    multiplySharedMem<<<dimGrid, dimBlock>>>(mat_1, mat_2, mat_prod, n, p);
    cudaDeviceSynchronize();
    auto t2 = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( t2 - t1 ).count();

    std::cout << duration << std::endl;
    std::cout << check_correctness(mat_1, mat_2, mat_prod, n, p) << std::endl;
    
    return 0;
}

