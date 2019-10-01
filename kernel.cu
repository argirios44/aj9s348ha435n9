#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <cuda_runtime.h>
#include <cusparse.h>
//using namespace std;

#define cols 2
const int blocksize = 128;

//Get 2 vectors/rows and multiply element-wise
__global__ void Hadamard_kernel(int* A,int* C,int* Output,int N){
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	int j = blockIdx.y * blockDim.y + threadIdx.y;
	int index = i + j * N;
	if (i < 128 && j < N)
		Output[index] = A[index] + C[index];
}


int main(int argc, char** argv) {
	cudaError_t cudaStat1, cudaStat2, cudaStat3;
	cusparseHandle_t handle = 0;
	cusparseStatus_t status;
	cusparseMatDescr_t descr = 0;
	FILE* fp;
	int rows = 0, curr_row = 0, curr_col;
	char *token, filename[50], c, str[50], line[50]= "";
	size_t len = 20;
	char* dilimeter = ",";
	printf("Enter data file name: ");
	scanf("%s", filename);
	printf("\n");
	//READ FILE//
	//Count file lines
	fp = fopen(filename, "r");
	if (fp == NULL) {
		printf("Error on reading file.");
		return 0;
	}
	for (c = getc(fp); c != EOF; c = getc(fp))
		if (c == '\n')	rows = rows + 1;
	rewind(fp);
	printf("The file %s has %d lines\n ", filename, rows);
	//Create Array
	int nnz = rows,count,n,*z=0;
	int* cooRowIndexHostPtr = 0;
	int* cooColIndexHostPtr = 0;
	int* cooValHostPtr = 0;
	int* cooRowIndex = 0;
	int* cooColIndex = 0;
	int* cooVal = 0;
	int* csrRowPtr = 0;
	cooRowIndexHostPtr = (int*)malloc(nnz * sizeof(cooRowIndexHostPtr[0]));
	cooColIndexHostPtr = (int*)malloc(nnz * sizeof(cooColIndexHostPtr[0]));
	cooValHostPtr = (int*)malloc(nnz * sizeof(cooValHostPtr[0]));
		//
		n = 448695;//Array dimension
	//Read data line by line (data is in COO format exported by Matlab)
	while (fgets(line, sizeof(line), fp)) {
		//printf("%s",line);
		token = strtok(line, dilimeter);
		count = 0;
		while (token != NULL) {
			if (count == 0)
				cooRowIndexHostPtr[curr_row] = (int)atof(token);//string->float->int
			else
				cooColIndexHostPtr[curr_row] = (int)atof(token);
			token = strtok(NULL, dilimeter);
			count++;
		}
		cooValHostPtr[curr_row] = 1;
		curr_row++;
	}
	/*
	for (int i = 1;i < 20;i++) {
		printf("%d %d %d\n", cooRowIndexHostPtr[i], cooColIndexHostPtr[i], cooValHostPtr[i]);
	}
		printf("%d %d %d\n", cooRowIndexHostPtr[nnz - 1], cooColIndexHostPtr[nnz - 1], cooValHostPtr[nnz - 1]);
	*/
	//Allocate GPU Memory and copy data
	cudaStat1 = cudaMalloc((void**)&cooRowIndex, nnz * sizeof(cooRowIndex[0]));
	cudaStat2 = cudaMalloc((void**)&cooColIndex, nnz * sizeof(cooColIndex[0]));
	cudaStat3 = cudaMalloc((void**)&cooVal, nnz * sizeof(cooVal[0]));
	if ((cudaStat1 != cudaSuccess || cudaStat2 != cudaSuccess || cudaStat3 != cudaSuccess)) {
		printf("Device Malloc failed");
		return 1;
	}
	cudaStat1 = cudaMemcpy(cooRowIndex, cooRowIndexHostPtr, (size_t)(nnz*sizeof(cooRowIndex[0])), cudaMemcpyHostToDevice);
	cudaStat2 = cudaMemcpy(cooColIndex, cooColIndexHostPtr, (size_t)(nnz*sizeof(cooColIndex[0])), cudaMemcpyHostToDevice);
	cudaStat3 = cudaMemcpy(cooVal, cooValHostPtr, (size_t)(nnz*sizeof(cooVal[0])), cudaMemcpyHostToDevice);
	if ((cudaStat1 != cudaSuccess || cudaStat2 != cudaSuccess || cudaStat3 != cudaSuccess)) {
		printf("Device Malloc failed");
		return 1;
	}
	//Init cusparse
	status = cusparseCreate(&handle);
	if (status != CUSPARSE_STATUS_SUCCESS) {
		printf("Error: CUSPARSE library initialization failed.");
		return 1;
	}
	status = cusparseCreateMatDescr(&descr);
	if (status != CUSPARSE_STATUS_SUCCESS) {
		printf("Error: CUSPARSE matrix descriptor initialization failed.");
		return 1;
	}
	cusparseSetMatType(descr, CUSPARSE_MATRIX_TYPE_GENERAL);
	cusparseSetMatIndexBase(descr, CUSPARSE_INDEX_BASE_ZERO);
	//Convert COO to CSR format
	cudaStat1 = cudaMalloc((void**)&csrRowPtr, (n + 1) * sizeof(csrRowPtr[0]));
	status = cusparseXcoo2csr(handle, cooRowIndex, nnz, n, csrRowPtr, CUSPARSE_INDEX_BASE_ZERO);
	if (status != CUSPARSE_STATUS_SUCCESS) {
		printf("Error: Conversion from COO to CSR format failed.");
		return 1;
	}
	//
	int devId;
	cudaDeviceProp prop;
	cudaError_t cudaStat;
	cudaStat = cudaGetDevice(&devId);
	if (cudaSuccess != cudaStat) {
		printf("cudaGetDevice failed");
		printf("Error: cudaStat %d, %s\n", cudaStat, cudaGetErrorString(cudaStat));
		return 1;
	}
	cudaStat = cudaGetDeviceProperties(&prop, devId);
	if (cudaSuccess != cudaStat) {
		printf("cudaGetDeviceProperties failed");
		printf("Error: cudaStat %d, %s\n", cudaStat, cudaGetErrorString(cudaStat));
		return 1;
	}
	//FIND C=A*A
	int baseC, nnzC;
	int* csrRowPtrC=0;
	int* csrValC=0;
	int* csrColIndC=0;
	int *nnzTotalDevHostPtr = &nnzC;
	cusparseSetPointerMode(handle, CUSPARSE_POINTER_MODE_HOST);
	cudaMalloc((void**)&csrRowPtrC, sizeof(csrRowPtrC[0]) * (n + 1));
	cusparseXcsrgemmNnz(handle, CUSPARSE_OPERATION_NON_TRANSPOSE, CUSPARSE_OPERATION_NON_TRANSPOSE, n, n, n, descr, nnz,
		csrRowPtr, cooColIndex, descr, nnz, csrRowPtr, cooColIndex, descr, csrRowPtrC, nnzTotalDevHostPtr);
	//To nnzTotalDevHostPtr (αριθμός μη-μηδενικών του γινομένου) βγαίνει λάθος για κάποιο λόγο, ίσως κάποιο overflow ή segmentation fault που δε μπορώ να βρώ
	if (NULL != nnzTotalDevHostPtr) {
		nnzC = *nnzTotalDevHostPtr;
		printf("1\n");
	}
	else {
		cudaMemcpy(&nnzC, csrRowPtrC + n, sizeof(int), cudaMemcpyDeviceToHost);
		cudaMemcpy(&baseC, csrRowPtrC, sizeof(int), cudaMemcpyDeviceToHost);
		nnzC -= baseC;
		printf("2\n");
	}
	nnzC = 33050793;//Το βρήκα από Matlab, έβγαινε λάθος εδώ αλλά ο κώδικας υπάρχει παραπάνω
	printf("%d\n", nnzC);
	cudaMalloc((void**)&csrColIndC, sizeof(int) * nnzC);
	cudaMalloc((void**)&csrValC, sizeof(int) * nnzC);
	status = cusparseDcsrgemm(handle,CUSPARSE_OPERATION_NON_TRANSPOSE, CUSPARSE_OPERATION_NON_TRANSPOSE,n,n,n,descr,nnz,
		(double*)cooVal,csrRowPtr,cooColIndex,descr,nnz,(double*)cooVal,csrRowPtr,cooColIndex,
		descr,(double*)csrValC,csrRowPtrC,csrColIndC);
	printf("First product complete.\n");
	//WORKS UNTIL HERE, THE PART BELOW IS NOT COMPLETE
	/*
	int* CSR_Host_RowIndexHostPtr=0;
	int* CSR_Host_ColIndexHostPtr=0;
	int* CSR_Host_ValHostPtr=0;
	CSR_Host_RowIndexHostPtr = (int*)malloc((n+1) * sizeof(CSR_Host_RowIndexHostPtr[0]));
	CSR_Host_ColIndexHostPtr = (int*)malloc(nnzC * sizeof(CSR_Host_ColIndexHostPtr[0]));
	CSR_Host_ValHostPtr = (int*)malloc(nnzC * sizeof(CSR_Host_ValHostPtr[0]));
	cudaMemcpy(&CSR_Host_RowIndexHostPtr, csrRowPtrC, (size_t)(n + 1) * CSR_Host_RowIndexHostPtr[0], cudaMemcpyDeviceToHost);
	cudaMemcpy(&CSR_Host_ColIndexHostPtr, csrColIndC, (size_t)(nnzC) * CSR_Host_ColIndexHostPtr[0], cudaMemcpyDeviceToHost);
	cudaMemcpy(&CSR_Host_ValHostPtr, csrValC, (size_t)(nnzC) * CSR_Host_ValHostPtr[0], cudaMemcpyDeviceToHost);
	*/
	int* tempRowC = 0;
	int* tempValC = 0;
	int* tempColC = 0;
	int* tempRowA = 0;
	int* tempValA = 0;
	int* tempColA = 0;
	cudaMalloc((void**)&csrColIndC, sizeof(int) * nnzC);
	//init kernel
	dim3 dimBlock(blocksize, blocksize);
	dim3 dimGrid(n / dimBlock.x, n / dimBlock.y);
	int* A = 0;
	int* C = 0;
	int* Out = 0;
	cudaMalloc((void**)&C, sizeof(int) * (128*n));//temp array C
	cudaMalloc((void**)&A, sizeof(int)* (128 * n));//temp array A
	cudaMalloc((void**)&Out, sizeof(int)* (128 * n));//temp array A
	//Scatter blocks of the two arrays and do Hadamard multiplication 128 rows at a time
	for (int k = 0;k < n/128;k++) {
		cusparseDcsr2dense(handle, 128, n, descr,(double*)csrValC,csrRowPtrC+k,csrColIndC,(double*)C,128 );
		cusparseDcsr2dense(handle, 128, n, descr,(double*)cooVal,csrRowPtr+k,cooColIndex,(double*)A,128 );
		Hadamard_kernel <<<dimGrid, dimBlock >>> (A,C,Out,n);
	}
	//results
	printf("\nTime taken: %d");
}