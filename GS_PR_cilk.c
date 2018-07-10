#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <math.h>
#include <string.h>
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
//OPTIONS
	long double tol=0.00000000001;
	int maxiter=500,n;
	long double c=0.85;
//FUNCTIONS//
long double Mul_Vector_Vector(long double** Vector1, long double* Vector2,int k);
long double Taxicab_norm(long double *Vector);
//****MAIN****//
void main(int argc,char **argv){
	int i,j,m,N,*deg,*e,*d,**E,iter;
	long double *v,*x,*dx,*hist,*xnew,*diff,**P,**D,**F,**A,delta,dt,norm_x_1,*y,w,sum,temp1,temp2,timet[2];
	struct timeval tim;
	//CHECK INPUT//
	if (argc==2){
		printf("Starting with default maximum iterations, 500.\n");}
	else if(argc==3){
		maxiter=atoi(argv[2]);
		printf("Starting with maximum iterations: %d.\n",maxiter);
	}
	else{
		printf("Please enter in the form: '%s 'path to file containing querry' '(optional) maximum number of iterations' \n",argv[0]);
	}
	
	//READ FILE//
	//Read an adjastency list+node list and create adjastency matrix E
	/**** Program used: http://www.cs.toronto.edu/~tsap/experiments/download/list2matrix.c ****/
	FILE *fnodes;
	char nodes_file[1000];
	FILE *flist;
	char  list_file[1000];
	FILE *fmatrix;
	char  matrix_file[1000];
	char *path;
	path = strdup(argv[1]);
	/*** open the nodes file to obtain the number of nodes ***/
	sprintf(nodes_file,"%s/graph/nodes",path);
	fnodes = fopen(nodes_file,"r");
	if (fnodes == NULL){
		printf("ERROR: Cant open file %s\n",nodes_file);
		exit(1);
	}
	fscanf(fnodes,"%d",&N);
	fclose(fnodes);
	/**** Read List and Construct the adjacency matrix E ****/
	E = (int **)malloc(N*sizeof(int *));
	for (i = 0; i < N; i ++){
		E[i] = (int *)malloc(N*sizeof(int));
		for (j = 0; j < N; j ++){
				E[i][j] = 0;
		}
	}
	sprintf(list_file,"%s/graph/adj_list",path);
	flist = fopen(list_file,"r");
	for (i = 0; i < N; i ++){
		fscanf(flist,"%*d: %d",&j);
		while (j != -1){
				E[i][j] = 1;
				fscanf(flist,"%d",&j);
		}
	}
	fclose(flist);
	/*** print the adjacency matrix ***/
	sprintf(matrix_file,"%s/graph/adj_matrix",path);
	fmatrix = fopen(matrix_file,"w");
	for (i = 0; i < N; i ++){
			for (j = 0; j < N; j ++){
				fprintf(fmatrix,"%d ", E[i][j]);
			}
		fprintf(fmatrix,"\n");
	}
	fclose(fmatrix);
	//FILES ARE READ//
	//***********************************************************************************//
	//CALCULATE P//
	//0. Allocate memory
	n=N;
	deg=malloc(N*sizeof(int));
	d=malloc(n*sizeof(int));
	v=malloc(n*sizeof(long double));
	//e=malloc(n*sizeof(int));
	P=(long double **)malloc(N*sizeof(long double *));
	//D=(long double **)malloc(N*sizeof(long double *));
	//F=(long double **)malloc(N*sizeof(long double *));
	//A=(long double **)malloc(N*sizeof(long double *));
	for (i = 0; i < N; i ++){
		P[i] = (long double *)malloc(N*sizeof(long double));
		//D[i] = (long double *)malloc(N*sizeof(long double));
		//F[i] = (long double *)malloc(N*sizeof(long double));
		//A[i] = (long double *)malloc(N*sizeof(long double));
		for (j = 0; j < N; j ++){
				P[i][j] = 0;
		}
	}
	//Set max number of workers equal to number of nodes N
	char nworkers[20];
	snprintf (nworkers,sizeof(nworkers), "%d",N);
	//itoa(N,nworkers,10);
	__cilkrts_set_param("nworkers", nworkers);
	//start timing
	gettimeofday(&tim,NULL);
	timet[0]=tim.tv_sec*1000000+tim.tv_usec;
	//1. Find outdegrees
	for(i=0;i<N;i++){
		deg[i]=0;
	}
	for(i=0;i<N;i++){
		for(j=0;j<N;j++){
			if(E[i][j]==1){deg[i]++;}
		}
		if(deg[i]==0) {d[i]=1;}
		else {d[i]=0;}
	}
	//2. Find P,v
	for(i=0;i<N;i++){
		v[i]=(long double)1/n;
		for(j=0;j<N;j++){
			if(deg[i]!=0&&E[i][j]!=0) {
				P[i][j]=(long double)1/deg[i];
			}
		}
	}
	//3. Find P'
	/*for (i=0;i<n;i++) {
		v[i]=(long double)1/n;
		//e[i]=1;
	}
	for(i=0;i<N;i++){
		for(j=0;j<N;j++){
			D[i][j]=d[i]*v[j];
			P[i][j]+=D[i][j];
		}
	}	
	//4. Find P''
	for(i=0;i<N;i++){
		for(j=0;j<N;j++){
			F[i][j]=e[i]*v[j];
			P[i][j]=c*P[i][j]+(1-c)*F[i][j];//<--P''
		}
	}
	//5. Find A=transpose(P'')
	for(i=0;i<N;i++){
		for(j=0;j<N;j++){
			A[i][j]=P[i][j];
		}
	}
	//6. Normalize A columnwise
	/*for(i=0;i<N;i++){
		sum =0;
		for(j=0;j<N;j++){
			sum+=A[j][i];
		}
			A[i][j]=A[i][j]/sum;
	}*/
//***********************************************************************************//	
	x=malloc(n*sizeof(long double));
	dx=malloc(n*sizeof(long double));
	diff=malloc(n*sizeof(long double));
	hist=malloc(maxiter*sizeof(long double));
	xnew=malloc(n*sizeof(long double));
	//PageRank
	for (i=0;i<n;i++){x[i]=v[i];}
	delta=1;
	iter=0;
	while(delta>tol&&iter<maxiter){
		for (i=0;i<n;i++){
			xnew[i]=cilk_spawn Mul_Vector_Vector(P,x,i);
			//xnew[i]+=A[i][j]*x[j];
		}
		cilk_sync;
		for (i=0;i<n;i++){
			xnew[i]=xnew[i]*c;
		}	
		temp1=cilk_spawn Taxicab_norm(x);
		temp2=cilk_spawn Taxicab_norm(xnew);
		cilk_sync;
		w=temp1-temp2;
		for (i=0;i<n;i++){
			xnew[i]+=w*v[i];
		}
		for(i=0;i<n;i++){
			diff[i]=x[i]-xnew[i];
			x[i]=xnew[i];
			}
		temp1=Taxicab_norm(diff);
		delta=temp1;
		printf("delta=%.11Lf\n",delta);
		hist[iter]=delta;
		iter++;
	}
	//resize hist
	//renormalize vector
	temp1=Taxicab_norm(x);
	for(i=0;i<n;i++){
		x[i]=x[i]/temp1;
	}
	//end timing
	gettimeofday(&tim,NULL);
	timet[1]=tim.tv_sec*1000000+tim.tv_usec;
	FILE *xout=fopen("xout_cilk.txt","w");
	for(i=0;i<n;i++){
		fprintf(xout,"\n%.11Lf",x[i]);
	}
	fclose(xout);	
	FILE *deltaout=fopen("delta_cilk.txt","w");
	for(i=0;i<iter;i++){
		fprintf(deltaout,"\n%.11Lf",hist[i]);
	}
	fclose(deltaout);	
	if(delta>tol&&iter==maxiter) printf("\nUnsucessfull: PageRank did not converge after %d iterations.Try entering a bigger number of maximum iterrations.",maxiter);
	else printf("\nConverged after %d iterrations. Time taken: %Lf sec\n",iter,(timet[1]-timet[0])/1000000);
}

long double Taxicab_norm(long double *Vector){
	long double sum=0;
	int i;
	for (i=0;i<n;i++){
		sum+=fabs(Vector[i]);
	}
	return sum;
}

long double Mul_Vector_Vector(long double** Vector1, long double* Vector2,int k){
	int i,j;
	long double sum=0;
	for (i=0;i<n;i++){
		sum+=Vector1[i][k]*Vector2[i];
	}
	return sum;
}