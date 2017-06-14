#include <iostream>
#include<memory.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<malloc.h>

void SGEMM_NN(int M, int N, int K, double ALPHA, double* A, int LDA, double* B, int LDB, double BETA, double* C, int LDC);

int main(int argc,char *argv[])
{
    double *matrix_A,*matrix_B, *matrix_C;
	//int loop=atoi(argv[1]);
	int width,height;
	width=height=atoi(argv[1]);
	int size=width*height;
	
    int size_M_height_A_C = height;
    int size_N_width_B_C = width;
    int size_K_width_A_height_B = width;
    double coeff_ALPHA = 1.0;
    int size_LDA = std::max(1, size_M_height_A_C);
    int size_LDB = std::max(1, size_K_width_A_height_B);
    double coeff_BETA = 0.0;
    int size_LDC = std::max(1, size_M_height_A_C);

    matrix_A = (double *)memalign(getpagesize(),sizeof(double)*size);
    matrix_B = (double *)memalign(getpagesize(),sizeof(double)*size);
    matrix_C = (double *)memalign(getpagesize(),sizeof(double)*size);
    
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < width; ++j) {
            matrix_A[i * width + j] = 1.0;
            matrix_B[i * width + j] = 1.0;
            matrix_C[i * width + j] = 0.0;
        }
    }

    SGEMM_NN(size_M_height_A_C, size_N_width_B_C, size_K_width_A_height_B, coeff_ALPHA, matrix_A, size_LDA, matrix_B, size_LDB, coeff_BETA, matrix_C, size_LDC);

    // MJL_Test output
    // for (int i = 0; i < width; ++i) {
    //     for (int j = 0; j < width; ++j) {
    //         std::cout << " " << matrix_C[i * width + j];
    //     }
    //     std::cout << std::endl;
    // }

    free(matrix_A);
    free(matrix_B);
    free(matrix_C);
}

void SGEMM_NN(int M, int N, int K, double ALPHA, double* A, int LDA, double* B, int LDB, double BETA, double* C, int LDC) {
    double* TEMP;
    int I, J, L, NROWA, NROWB;//, NCOLA;
    
    NROWA = M;
    // NCOLA = K;
    NROWB = K;

    if ((M < 0) ||
        (N < 0) ||
        (K < 0) ||
        (LDA < NROWA) ||
        (LDB < NROWB) ||
        (LDC < M)) {
        std::cout << "Error: Wrong input!" << std::endl;
        return;
    }
    
    TEMP = (double *)malloc(sizeof(double) * K);

    //Form  C := alpha*A*B + beta*C.
    for (I = 0; I < M; ++I) {
        if (BETA == 0) {
            for (J = 0; J < N; ++J) {
                C[I * N + J] = 0;
            }
        } else if (BETA != 1) {
            for (J = 0; J < N; ++J) {
                C[I * N + J] = BETA*C[I * N + J];
            }
        }
        for (J = 0; J < N; ++J) {
            for (L = 0; L < K; ++L) {
                TEMP[L] = A[I * N + L] * B[L * K + J];
            }
            for (L = 0; L < K; ++L) {
                C[I * N + J] = C[I * N + J] + ALPHA * TEMP[L];
            }
        }

        // MJL_Comment original implementation
        // for (L = 0; L < K; ++L) {
        //     TEMP = ALPHA * A[I * N + L];
        //     for (J = 0; J < N; ++J) {
        //         C[I * N + J] = C[I * N + J] + TEMP*B[L * K + J];
        //     }
        // }
    }

    free(TEMP);
    
    return;
}