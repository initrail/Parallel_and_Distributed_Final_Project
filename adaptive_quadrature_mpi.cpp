#include <iostream>
#include <mpi.h>
#include <unistd.h>
using namespace std;

void update(void* var1, MPI_Op op, void* var2, MPI_Datatype type,  MPI_Win& window){	
	MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, window);
	MPI_Fetch_and_op(var2, var1, type, 0, 0, op, window);
	//MPI_Get(var1, 1, type, 0, 0, 1, type, window);
	MPI_Win_unlock(0, window);
	//MPI_Get and the print statement are for debugging purposes
	/*if(type == MPI_INT)
		printf("idle = %d\n", *((int*)var1));
	else if(type == MPI_FLOAT)
		printf("area = %f\n", *((float*)var1));*/
}

void* get(void* var, MPI_Datatype type, MPI_Win& window){
	MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, window);
	MPI_Get(var, 1, type, 0, 0, 1, type, window);
	MPI_Win_unlock(0, window);
	return var;
}

void adaptiveQuadrature(float* range,
			float tolerance,
			int& idle,
			float& area,
			int numProcesses,
			int rank,
			MPI_Win &idlewin,
			MPI_Win& areawin){
	int decr = -1;
	int incr = 1;
	float area1 = .45;
	float a;
	MPI_Request req;
	int i;
	update(&idle, MPI_SUM, &decr, MPI_INT, idlewin);
	i = *((int*) get(&i, MPI_INT, idlewin));
	printf("process %d idle = %d\n", rank, i);
	update(&area, MPI_SUM, &area1, MPI_FLOAT, areawin);
	a = *((float*) get(&a, MPI_FLOAT, areawin));
	printf("process %d area = %f\n", rank, a);
}

int main(int argc, char** argv){
	int numProcesses, rank;
	float* range;
	float tolerance;
	MPI_Win idlewin;
	MPI_Win areawin;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numProcesses);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int idle = numProcesses;
	float area = 0;
	cout<<idle<<endl;
	if(rank == 0){
		MPI_Win_create(&idle, sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &idlewin);
		MPI_Win_create(&area, sizeof(float), sizeof(float), MPI_INFO_NULL, MPI_COMM_WORLD, &areawin);
	}
	else {
		MPI_Win_create(NULL, 0, sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &idlewin);
		MPI_Win_create(NULL, 0, sizeof(float), MPI_INFO_NULL, MPI_COMM_WORLD, &areawin);
	}
	adaptiveQuadrature(range, tolerance, idle, area, numProcesses, rank, idlewin, areawin);
	MPI_Win_free(&idlewin);
	MPI_Win_free(&areawin);
	MPI_Finalize();
	return 0;
}