#include <iostream>
#include <mpi.h>
#include <unistd.h>
#include <cmath>
#include <cstdlib>
using namespace std;

int vars[2];
float area = 0;
MPI_Win varswin;
MPI_Win areawin;
MPI_Win taskwin;
int P;
MPI_Datatype taskType;
static const int TASK_LEN = 1000;
struct task{
	float a;
	float b;
};

void* rmaOperation(void* var1, MPI_Op op, void* var2, MPI_Datatype type,  MPI_Win& window/*, int rank*/){	
	MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, window);
	MPI_Fetch_and_op(var2, var1, type, 0, 0, op, window);
	MPI_Get(var1, 1, type, 0, 0, 1, type, window);
	MPI_Win_unlock(0, window);
	//MPI_Get and the print statement are for debugging purposes
	/*if(type == MPI_INT)
		printf("p %d idle = %d\n", rank, *((int*)var1));
	else if(type == MPI_FLOAT)
		printf("p %d area = %f\n", rank, *((float*)var1));*/
	return var1;
}

int rmaPut(void* var, int disp, MPI_Datatype type, MPI_Win& window){
	int res;
	MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, window);
	res = MPI_Put(var, 1, type, 0, disp, 1, type, window);
	MPI_Win_unlock(0, window);
	return res;
}

void* rmaGet(void* var, int disp, MPI_Datatype type, MPI_Win& window){
	MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, window);
	MPI_Get(var, 1, type, 0, disp, 1, type, window);
	MPI_Win_unlock(0, window);
	return var;
}

void adaptiveQuadrature(float* range, float tolerance, int rank){
	int decr = -1;
	int incr = 1;
	int two = 2;
	float area1 = 0.0, area2 = 0.0;
	task task0, task1, task2;
	while(true){
		//Get idle value
		vars[0] = *((int*)rmaGet(vars, 0, MPI_INT, varswin));
		//See if there is a task 
		if(workready() || (vars[0] != P))
			//idle = idle - 1
			rmaOperation(vars, MPI_SUM, &decr, MPI_INT, varswin);
		else
			break;
		while(getwork()){
			if (fabs(area1 - area2) < 3 * (task0.b - task0.a) * tolerance)
				rmaOperation(&area, MPI_SUM, &area2, MPI_FLOAT, areawin);
			else {
				float m = (task0.b - task0.a)/2;
				task1.a = task0.a;
				task1.b = m;
				task2.a = m;
				task2.b = task0.b;
				//Atomically update AND THEN get disposition to avoid race conditions
				int disp = *((int*)rmaOperation(&vars[1], MPI_SUM, &two, MPI_FLOAT, varswin));
				disp-=2;
				int res1 = rmaPut(&task1, disp+1, taskType, taskwin);
				int res2 = rmaPut(&task2, disp+2, taskType, taskwin);
				if(res1 != MPI_SUCCESS || res2 != MPI_SUCCESS)
					exit(1);
			}
		}
		//idle = idle + 1
		rmaOperation(vars, MPI_SUM, &incr, MPI_INT, varswin);
	}
}

int main(int argc, char** argv){
	int rank;
	float* range;
	float tolerance;
	task tasks[TASK_LEN];
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
	vars[0] = P;
	if(rank == 0){
		//Expose the memory of process 0 to the other processes
		MPI_Win_create(tasks, sizeof(task)*TASK_LEN, sizeof(task), MPI_INFO_NULL, MPI_COMM_WORLD, &taskwin);
		MPI_Win_create(vars, sizeof(int)*2, sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &varswin);
		MPI_Win_create(&area, sizeof(float), sizeof(float), MPI_INFO_NULL, MPI_COMM_WORLD, &areawin);
	}
	else {
		//The other processes don't need to expose any of their memory
		MPI_Win_create(NULL, 0, sizeof(task), MPI_INFO_NULL, MPI_COMM_WORLD, &taskwin);
		MPI_Win_create(NULL, 0, sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &varswin);
		MPI_Win_create(NULL, 0, sizeof(float), MPI_INFO_NULL, MPI_COMM_WORLD, &areawin);
	}
	adaptiveQuadrature(range, tolerance, rank);
	MPI_Win_free(&varswin);
	MPI_Win_free(&taskwin);
	MPI_Win_free(&areawin);
	MPI_Finalize();
	return 0;
}