//GKLARAS ARGYRHS 7358//
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define K 25
#define columns 30
#define label_numb 10
#define BIG 100

typedef struct{
	double dist;
	int label,index;
}distances_struct;
int rows,rank,totnp;

int cmpfunc(const void * a, const void * b);
void kNN_search(distances_struct** distances,distances_struct** knn_array,double** traindata,double** testdata,int* labelset,int rows,int distances_clmn,int clmn,int p);
void find_classes(distances_struct** knn_array,int* label_fin);
distances_struct **Create2DarrayStruct(int rows, int clmn);
double **Create2DarrayDouble(int rows, int clmn);


/**----------------------------------MAIN----------------------------------**/
/**------------------------------------------------------------------------**/


int main( int argc, char** argv ) {
	int i,j,k,p,a;
	int ierr,sendto,rcvfrom,err,correct=0;
	int filesize_data,filesize_labels,size,*label_fin,distances_clmn;
	FILE *file;
	double acc,begin_time,end_time;	
	MPI_File in,out;
	MPI_Status status;
	MPI_Datatype fileview;
	div_t res;	
//-----------------------------start processes------------------------------// 
	err=MPI_Init(&argc,&argv);
	if (err){
		printf("Error '%i' in MPI_Init\n",err);
	}
	MPI_Comm_size(MPI_COMM_WORLD, &totnp);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);		
//-----------------read block of data file and labels file------------------// 
	file=fopen("data.bin","rb");
	if (!file){
		printf("Unable to open file.");
		return 1;
	}
	fseek(file,0,SEEK_END);
	filesize_data=ftell(file);
	rewind(file);
	fclose(file);
	int ndoubles_loc=filesize_data/(sizeof(double)*totnp);
	double *locdata=malloc(ndoubles_loc*sizeof(double));
    int starts = ndoubles_loc*rank;
	MPI_Type_create_subarray(1,&filesize_data,&ndoubles_loc,&starts,MPI_ORDER_C,MPI_DOUBLE,&fileview);
	MPI_Type_commit(&fileview);
	MPI_File_open(MPI_COMM_WORLD,"data.bin",MPI_MODE_RDONLY,MPI_INFO_NULL,&in);
	MPI_File_set_view(in,(MPI_Offset)0,MPI_DOUBLE,fileview,"native",MPI_INFO_NULL);
	MPI_File_read_all(in,locdata,ndoubles_loc,MPI_DOUBLE,&status);
	MPI_File_close(&in);
	filesize_labels=filesize_data/columns;
	ndoubles_loc=filesize_labels/(sizeof(double)*totnp);
	double *loclabels=malloc(ndoubles_loc*sizeof(double));
    starts = ndoubles_loc*rank;
	MPI_Type_create_subarray(1,&filesize_labels,&ndoubles_loc,&starts,MPI_ORDER_C,MPI_DOUBLE,&fileview);
	MPI_Type_commit(&fileview);
	MPI_File_open(MPI_COMM_WORLD,"labels.bin",MPI_MODE_RDONLY,MPI_INFO_NULL,&in);
	MPI_File_set_view(in,(MPI_Offset)0,MPI_DOUBLE,fileview,"native",MPI_INFO_NULL);
	MPI_File_read_all(in,loclabels,ndoubles_loc,MPI_DOUBLE,&status);
	MPI_File_close(&in);	
//---------------------------save data to arrays----------------------------// 
	size=ndoubles_loc*columns;
	rows=ndoubles_loc;	
	//rows=200;//test
	distances_clmn=rows/10;
	double **testdata=Create2DarrayDouble(rows,columns);
	double **traindata=Create2DarrayDouble(rows,columns);
	int *labelsset=malloc(sizeof(int)*ndoubles_loc);
	int *labelsset_orig=malloc(sizeof(int)*ndoubles_loc);
	label_fin=(int*)malloc(sizeof(int)*(ndoubles_loc));
	distances_struct **distances=Create2DarrayStruct(rows,distances_clmn);
	distances_struct **knn_array=Create2DarrayStruct(rows,3*K);
	for (i=0;i<rows*columns;i++){
		res=div(i,columns);
		traindata[res.quot][res.rem]=locdata[i];
		testdata[res.quot][res.rem]=locdata[i];
		labelsset[res.quot]=loclabels[res.quot];
		labelsset_orig[res.quot]=loclabels[res.quot];
	}	
	MPI_Barrier(MPI_COMM_WORLD);
	begin_time=MPI_Wtime();
//-------------------------------KNN search---------------------------------//
	for (p=0;p<totnp;p++){		
		kNN_search(distances,knn_array,traindata,testdata,labelsset,rows,distances_clmn,columns,p);			
		if (p==0){
			for (i=0;i<rows;i++){
				for (j=0;j<K;j++){
					knn_array[i][j].dist=knn_array[i][j+K].dist;
					knn_array[i][j].label=knn_array[i][j+K].label;
					knn_array[i][j].index=knn_array[i][j+K].index;
				}
			}
		}
		else{
			for (i=0;i<rows;i++){
				qsort(knn_array[i],2*K,sizeof knn_array[0][0],cmpfunc);
			}	
		}
//---------------------------send and receive data--------------------------//
		if (rank==0){
			rcvfrom=totnp-1;
			sendto=rank+1;
		}
		else if(rank+1==totnp){
			sendto=0;
			rcvfrom=rank-1;
		}
		else{
			sendto=rank+1;
			rcvfrom=rank-1;
		}
		MPI_Sendrecv(&traindata[0][0],rows*columns,MPI_DOUBLE,sendto,0,&traindata[0][0],rows*columns,MPI_DOUBLE,rcvfrom,0,MPI_COMM_WORLD,&status);
		MPI_Sendrecv(&labelsset[0],rows,MPI_DOUBLE,sendto,0,&labelsset[0],rows,MPI_DOUBLE,rcvfrom,0,MPI_COMM_WORLD,&status);
		MPI_Barrier(MPI_COMM_WORLD);
		//MPI_Isend(&traindata[0][0],rows*columns,MPI_DOUBLE,sendto,0,MPI_COMM_WORLD,&send_req);	
		//MPI_Irecv(&traindata_new[0][0],rows*columns,MPI_DOUBLE,rcvfrom,MPI_ANY_TAG,MPI_COMM_WORLD,&rcv_req);
	}
	find_classes(knn_array,label_fin);	
	for (i=0;i<rows;i++){
		if (labelsset_orig[i]==label_fin[i]) correct++;
	}
	acc=((double)correct/(double)rows)*100;
	printf("\n%f\n",acc);
	MPI_Barrier(MPI_COMM_WORLD);
	end_time=MPI_Wtime();
	/*int size_out=totnp*sizeof(knn_array[0][0])*rows*K;
	int ndoubles_out=rows*K;
	starts=ndoubles_out*rank;
	MPI_Type_create_subarray(1,&size_out,&ndoubles_out,&starts, MPI_ORDER_C, MPI_DOUBLE, &fileview);
    MPI_Type_commit(&fileview);
	MPI_File_open(MPI_COMM_WORLD,"results.bin",MPI_MODE_WRONLY|MPI_MODE_CREATE,MPI_INFO_NULL,&out);
	MPI_File_set_view(out,(MPI_Offset)0,MPI_DOUBLE,fileview,"native",MPI_INFO_NULL);
	MPI_File_write_all(out,&knn_array[0][0],ndoubles_out,MPI_DOUBLE,&status);
	MPI_File_close(&out);*/
	MPI_Finalize();
	if (rank==0) printf("\nTime elapsed=%f sec\n",end_time-begin_time);
}


/**-------------------------------FUNCTIONS--------------------------------**/
/**------------------------------------------------------------------------**/


void kNN_search(distances_struct** distances,distances_struct** knn_array,double** traindata,double** testdata,int* labelsset,int rows,int distances_clmn,int clmn,int p){
	int i,j,k,t;
	for (t=0;t<10;t++){
		for (i=0;i<rows;i++){
			for (j=t*distances_clmn;j<(t+1)*distances_clmn;j++){
				distances[i][j-(t*distances_clmn)].dist=0;
				for (k=0;k<clmn;k++){
					distances[i][j-(t*distances_clmn)].dist+=pow(testdata[i][k]-traindata[j][k],2);
				}
				distances[i][j-(t*distances_clmn)].dist=sqrt(distances[i][j-(t*distances_clmn)].dist);
				distances[i][j-(t*distances_clmn)].label=labelsset[j];
				if (rank-p<0){
					distances[i][j-(t*distances_clmn)].index=j+rows*(rank-p+totnp);
				}
				else{
					distances[i][j-(t*distances_clmn)].index=j+rows*(rank-p);
				}
				if (i==j&&p==0) distances[i][j-(t*distances_clmn)].dist=BIG;
			}
			qsort(distances[i],distances_clmn,sizeof distances[0][0],cmpfunc);
		}
		if (t==0){
			for (i=0;i<rows;i++){
				for (j=K;j<2*K;j++){
					knn_array[i][j].dist=distances[i][j-K].dist;
					knn_array[i][j].label=distances[i][j-K].label;
					knn_array[i][j].index=distances[i][j-K].index;
				}
			}
		}
		else{
			for (i=0;i<rows;i++){
				for (j=2*K;j<3*K;j++){
					knn_array[i][j].dist=distances[i][j-2*K].dist;
					knn_array[i][j].label=distances[i][j-2*K].label;
					knn_array[i][j].index=distances[i][j-2*K].index;
				}
				qsort(&knn_array[i][K],2*K,sizeof knn_array[0][0],cmpfunc);
			}	
		}
	}
}

void find_classes(distances_struct** knn_array,int* label_fin){
	int most_freq,i,j;
	for (i=0;i<rows;i++){
			int counter[label_numb]={0};
			for (j=0;j<K;j++){
				if (i!=j) counter[knn_array[i][j].label]++;
			}
			most_freq=-1;
			for (j=0;j<label_numb;j++){
				if (counter[j]>most_freq){
					most_freq=j;
				}
			}
			label_fin[i]=most_freq;
		}
}

distances_struct **Create2DarrayStruct(int rows, int clmn){
	int i;
	distances_struct *data=(distances_struct*)malloc(rows*clmn*sizeof(distances_struct));
	distances_struct **array=(distances_struct**)malloc(rows*sizeof(distances_struct*));
	 for (int i=0; i<rows; i++)
        array[i] = &(data[clmn*i]);
    return array;
}

double **Create2DarrayDouble(int rows, int clmn){
	int i;
	double *data=(double*)malloc(rows*clmn*sizeof(double));
	double **array=(double**)malloc(rows*sizeof(double*));
	 for (int i=0; i<rows; i++)
        array[i] = &(data[clmn*i]);
    return array;
}

int cmpfunc(const void * a, const void * b){
	distances_struct *sa = (distances_struct *)a;
	distances_struct *sb = (distances_struct *)b;
	if (sa->dist<sb->dist) return -1;
	else if (sa->dist>sb->dist) return 1;
	else return 0;
}