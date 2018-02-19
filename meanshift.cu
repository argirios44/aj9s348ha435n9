#include <stdio.h>
#include <stdlib.h>
#include <math.h>

__global__ void mykernel(double* X_dev,double* Y_dev,double* temp,double* temp_vect,double* m_vect,int size,int dim) {
	int i=blockIdx.x*blockDim.x+threadIdx.x;
	int j=blockIdx.y*blockDim.y+threadIdx.y;
	double m_norm=e+1,s1,s2;
		while (m_norm>e){
			m_norm=0;
			s1=0;
			s2=0;
			for (j=0;j<dim;j++){
				temp_vect[j]=0;
			}
			for (j=0;j<N;j++){
				dist=distance(X_dev,Y_dev,i,j);
				if (dist<=pow(s,2)){
					s1_temp=exp(-1*pow(dist,2)/(2*pow(s,2)));
					for (z=0;z<dim;z++){
						temp[j][z]=X[j][z];
						temp[j][z]*=s1_temp;
						temp_vect[z]+=temp[j][z];
					}
					s2+=exp(-1*pow(dist,2)/(2*pow(s,2)));
				}
			}
			for(z=0;z<dim;z++){
				temp_vect[z]=temp_vect[z]/s2;
			}
			for (j=0;j<dim;j++){
				m_vect[j]=temp_vect[j]-Y[i][j];
				Y_dev[i][j]=temp_vect[j];
				m_norm+=pow(m_vect[j],2)
			}
			m_norm=sqrt(m_norm);
		}
}

#define K 1
#define dimension 2

int main(int argc,char **argv) {
	FILE *file;
	long size;
	double *buffer,*temp_vect,*m_vect,s1,s2,sq_temp,dist,m_norm,*X_dev,*Y_dev;
	int i,j,z,dim=dimension;
	file=fopen("data.bin","rb");
	if (!file){
		printf("Unable to open file.");
		return 1;
	}
	fseek(file,0,SEEK_END);
	size=ftell(file);
	rewind(file);
	printf("%ld",size);
	buffer=(double*)malloc(sizeof(double)*(size/8));
	fread(buffer,sizeof(double),size,file);
	fclose(file);
	size=size/(8*dim);
	double **X=Create2DarrayDouble(size,dim);
	double **Y=Create2DarrayDouble(size,dim);
	double **temp=Create2DarrayDouble(size,dim);
	cudaMalloc(&temp_vect,dim*sizeof(double));
	cudaMalloc(&m_vect,dim*sizeof(double));
	for (i=0;i<size;i++){
		for (j=0;j<dim;j++){
			X[i,j]=buffer[i*dim+j];
			Y[i,j]=buffer[i*dim+j];
		}
	}
	size_t pitch;
	cudaMallocPitch((void**)&X_dev,&pitch,dim,size);
	cudaMallocPitch((void**)&Y_dev,&pitch,dim,size);
	cudaMallocPitch((void**)&temp,&pitch,dim,size);
	cudaMemcpy2D(X_dev,pitch,X,dim*sizeof(double),dim*sizeof(double),size,cudaMemcpyHostToDevice);
	cudaMemcpy2D(Y_dev,pitch,Y,dim*sizeof(double),dim*sizeof(double),size,cudaMemcpyHostToDevice);
	int blocks=N/100;
	mykernel<<<blocks,100>>>(X_dev,Y_dev,temp,temp_vect,m_vect,size,dim);
	cudaMemcpy2D(Y,pitch,Y_dev,dim*sizeof(double),dim*sizeof(double),size,cudaMemcpyDeviceToHost);
}

__device__ double distance(double** X,double** Y,int i,int j){
	int a;
	double dist=0;
	for (a=0;a<dim;a++){
		dist+=pow(Y[i][a]-X[j][a],2)
	}
	dist=sqrt(dist);
	return dist;
}

double **Create2DarrayDouble(int rows, int clmn){
	int i;
	double *data=(double*)malloc(rows*clmn*sizeof(double));
	double **array=(double**)malloc(rows*sizeof(double*));
	 for (int i=0; i<rows; i++)
        array[i] = &(data[clmn*i]);
    return array;
}
