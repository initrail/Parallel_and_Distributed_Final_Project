#include <iostream>
#include <mpi.h>
#include <unistd.h>
using namespace std;

int idle = 0;
float area = 0;
MPI_Win idlewin;
MPI_Win areawin;
int P;
static const int ARRAY_LEN = 1000;
struct task{
	float a;
	float b;
};

void rmaOperation(void* var1, MPI_Op op, void* var2, MPI_Datatype type,  MPI_Win& window/*, int rank*/){	
	MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, window);
	MPI_Fetch_and_op(var2, var1, type, 0, 0, op, window);
	//MPI_Get(var1, 1, type, 0, 0, 1, type, window);
	MPI_Win_unlock(0, window);
	//MPI_Get and the print statement are for debugging purposes
	/*if(type == MPI_INT)
		printf("p %d idle = %d\n", rank, *((int*)var1));
	else if(type == MPI_FLOAT)
		printf("p %d area = %f\n", rank, *((float*)var1));*/
}

void* rmaGet(void* var, MPI_Datatype type, MPI_Win& window){
	MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, window);
	MPI_Get(var, 1, type, 0, 0, 1, type, window);
	MPI_Win_unlock(0, window);
	return var;
}

void adaptiveQuadrature(float* range, float tolerance, int rank){
	int decr = -1;
	int incr = 1;
	float area1 = 0.0;
	/*while(true){
		idle = *((int*)rmaGet(&idle, MPI_INT, idlewin));
		if(workready() || (idle != P))
			//idle = idle - 1
			rmaOperation(&idle, MPI_SUM, &decr, MPI_INT, idlewin);
		else
			break;
		while(getwork()){

		}
		//idle = idle + 1
		rmaOperation(&idle, MPI_SUM, &incr, MPI_INT, idlewin);
	}*/
}

int main(int argc, char** argv){
	int rank;
	float* range;
	float tolerance;
	task tasks[ARRAY_LEN];
	MPI_Datatype taskType;
	int blocklen[2] = {1, 1};
	MPI_Aint indices[2];
	indices[0] = (MPI_Aint)&tasks[0].a - (MPI_Aint)tasks;
	indices[1] = (MPI_Aint)&tasks[0].b - (MPI_Aint)tasks;
	int types[3] = {MPI_FLOAT, MPI_FLOAT};
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &P);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Type_create_struct(2, blocklen, indices, types, &taskType);
	MPI_Type_commit(&taskType);
	idle = P;
	if(rank == 0){
		MPI_Win_create(&idle, sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &idlewin);
		MPI_Win_create(&area, sizeof(float), sizeof(float), MPI_INFO_NULL, MPI_COMM_WORLD, &areawin);
	}
	else {
		MPI_Win_create(NULL, 0, sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &idlewin);
		MPI_Win_create(NULL, 0, sizeof(float), MPI_INFO_NULL, MPI_COMM_WORLD, &areawin);
	}
	adaptiveQuadrature(range, tolerance, rank);
	MPI_Win_free(&idlewin);
	MPI_Win_free(&areawin);
	MPI_Finalize();
	return 0;
}