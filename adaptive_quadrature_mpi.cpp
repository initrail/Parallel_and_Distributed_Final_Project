#include <iostream>
#include <mpi.h>
#include <unistd.h>
using namespace std;

void adaptiveQuadrature(float* range, float tolerance, int numProcesses, int pId, MPI_Win &window){
	int idle = numProcesses;
	

	//while(true){
		idle-=1;
		cout<<"Process "<<pId<<" accessing lock\n";
		MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, window);
		sleep(5);
		MPI_Put(&idle, 1, MPI_INT, 0, 0, 1, MPI_INT, window);
		MPI_Win_unlock(0, window);
		cout<<"Process "<<pId<<"left lock\n";
		
		MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, window);
		MPI_Get(&idle, 1, MPI_INT, 0, 0, 1, MPI_INT, window);
		MPI_Win_unlock(0, window);
		MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, window);
		idle+=1;
		MPI_Put(&idle, 1, MPI_INT, 0, 0, 1, MPI_INT, window);
		MPI_Win_unlock(0, window);
	//}
}

int main(int argc, char** argv){
	int numProcesses, pId;
	MPI_Win window;
	float* range;
	float tolerance;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numProcesses);
	MPI_Comm_rank(MPI_COMM_WORLD, &pId);
	int loc[2];
	if(pId == 0){
		MPI_Win_create(loc, 2*sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &window);
	} else {
		MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &window);
	}
	adaptiveQuadrature(range, tolerance, numProcesses, pId, window);
	MPI_Finalize();
	return 0;
}