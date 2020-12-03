#include <iostream>
#include <mpi.h>
#include <unistd.h>
#include <cmath>
#include <cstdlib>
using namespace std;

int vars[2];
float area = 0;
int P;
bool work;
MPI_Win varswin;
MPI_Win areawin;
MPI_Win taskwin;
MPI_Win workwin;
MPI_Datatype taskType;
static const int TASK_LEN = 10;
struct task{
	float a;
	float b;
};
int rank;
task tasks[TASK_LEN];
void* rmaOperation(void* var1, MPI_Op op, void* var2, int disp, MPI_Datatype type,  MPI_Win& window){	
	MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, window);
	MPI_Fetch_and_op(var2, var1, type, 0, disp, op, window);
	MPI_Get(var1, 1, type, 0, disp, 1, type, window);
	MPI_Win_unlock(0, window);
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

bool getwork(task& t){
	//Assume that work needs to be done
	bool avail = true;
	int decr = -1;
	int incr = 1;
	//Decrement the disposition of tasks to assume there is work
	vars[1] = *((int*)rmaOperation(&vars[1], MPI_SUM, &decr, 1, MPI_INT, varswin));
	//if(rank == 0)
	//printf("Rank %d has a disposition of %d.\n", rank, vars[1]);
	//If the disposition is less than 0 that means there are no more tasks available
	if(vars[1] < 0){
		//if(rank == 0)
		//printf("Rank %d entered the if.\n", rank);
		avail = false;
		rmaPut(&avail, 0, MPI_C_BOOL, workwin);
		//If there is no work put the disposition back to what it was before assuming there was work
		vars[1] = *((int*)rmaOperation(&vars[1], MPI_SUM, &incr, 1, MPI_INT, varswin));
		//if(rank == 0)
		//printf("The second print rank %d has disposition of %d.\n", rank, vars[1]);
	}
	else {
		//printf("The rank %d has disposition of %d.\n", rank, vars[1]);
		t = *((task*)rmaGet(&t, vars[1], MPI_FLOAT, taskwin));
	}
	return avail;
}

bool workready(){
	work = *((bool*)rmaGet(&work, 0, MPI_C_BOOL, workwin));
	return work;
}

float trapezoidalArea(float x, float y1, float y2) {
	return x * fmin(y1, y2) + (fmax(y1, y2) - fmin(y1, y2)) * x / 2;
}

float function(float x) {
	return x*x;
}

void adaptiveQuadrature(float* range, float tolerance, int rank){
	int decr = -1;
	int incr = 1;
	int two = 2;
	float area1 = 0.0, area2 = 0.0, m = 0.0;
	task task0, task1, task2;
	//while(true){
		//Get idle value
		vars[0] = *((int*)rmaGet(vars, 0, MPI_INT, varswin));
		//See if there is a task 
		if(workready() || (vars[0] != P))
			//idle = idle - 1
			rmaOperation(vars, MPI_SUM, &decr, 0, MPI_INT, varswin);
		else;
			
		while(getwork(task0)){
			printf("rank %d has entered the computational area.\n", rank);
			float y1 = function(task0.a);
			float y2 = function(task0.b);
			float x = fabs(task0.b - task0.a);
			m = task0.a + x / 2;
			float ym = function(m);
			float x1 = fabs(m - task0.a);
			float x2 = fabs(task0.b - m);
			area1 = trapezoidalArea(x, y1, y2);
			area2 = trapezoidalArea(x1, y1, ym) + trapezoidalArea(x2, ym, y2);
			if (fabs(area1 - area2) < 3 * (task0.b - task0.a) * tolerance)
				rmaOperation(&area, MPI_SUM, &area2, 0, MPI_FLOAT, areawin);
			else {
				m = (task0.b - task0.a)/2;
				task1.a = task0.a;
				task1.b = m;
				task2.a = m;
				task2.b = task0.b;
				// get disposition to avoid race conditions
				do {
					//vars[1] = *((int*)rmaOperation(&vars[1], MPI_SUM, &two, 1, MPI_INT, varswin));
					vars[1] = *((int*)rmaGet(&vars[1], 1, MPI_INT, varswin));
				}
				while(vars[1] < 0 || vars[1] > TASK_LEN);
				bool w = true;
				printf("rank %d is putting in a task at disp %d.\n", rank, vars[1] + 0);
				int res1 = rmaPut(&task1, vars[1], taskType, taskwin);
				printf("rank %d is putting in a task at disp %d.\n", rank, vars[1] + 1);
				int res2 = rmaPut(&task2, vars[1]+1, taskType, taskwin);
				rmaOperation(&vars[1], MPI_SUM, &two, 1, MPI_INT, varswin);
				rmaPut(&w, 0, MPI_C_BOOL, workwin);
			}
		}
		//idle = idle + 1
		rmaOperation(vars, MPI_SUM, &incr, 0, MPI_INT, varswin);
	//}
}

void getargs(int& argc, char** argv, float& tol, task* t){
	if(argc != 4)
		exit(1);
	tol = atof(argv[3]);
	t->a = atoi(argv[1]);
	t->b = atoi(argv[2]);
	work = true;
}

int main(int argc, char** argv){
	float* range;
	float tolerance;
	getargs(argc, argv, tolerance, tasks);
	int blocklen[2] = {1, 1};
	MPI_Aint indices[2];
	indices[0] = (MPI_Aint)&tasks[0].a - (MPI_Aint)tasks;
	indices[1] = (MPI_Aint)&tasks[0].b - (MPI_Aint)tasks;
	int types[3] = {MPI_FLOAT, MPI_FLOAT};
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &P);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	if(rank == 0){
		cout<<"initial task.a = "<<tasks[0].a<<" and initial task.b = "<<tasks[0].b<<endl;
		cout<<"tolerance = "<<tolerance<<endl;
	}
	MPI_Type_create_struct(2, blocklen, indices, types, &taskType);
	MPI_Type_commit(&taskType);
	vars[0] = P;
	//There must be atleast one task, vars[1] is the index of tasks
	vars[1] = 1;
	if(rank == 0){
		//Expose the memory of process 0 to the other processes
		MPI_Win_create(&work, sizeof(bool), sizeof(bool), MPI_INFO_NULL, MPI_COMM_WORLD, &workwin);
		MPI_Win_create(tasks, sizeof(task)*TASK_LEN, sizeof(task), MPI_INFO_NULL, MPI_COMM_WORLD, &taskwin);
		MPI_Win_create(vars, sizeof(int)*2, sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &varswin);
		MPI_Win_create(&area, sizeof(float), sizeof(float), MPI_INFO_NULL, MPI_COMM_WORLD, &areawin);
	}
	else {
		//The other processes don't need to expose any of their memory
		MPI_Win_create(NULL, 0, sizeof(bool), MPI_INFO_NULL, MPI_COMM_WORLD, &workwin);
		MPI_Win_create(NULL, 0, sizeof(task), MPI_INFO_NULL, MPI_COMM_WORLD, &taskwin);
		MPI_Win_create(NULL, 0, sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &varswin);
		MPI_Win_create(NULL, 0, sizeof(float), MPI_INFO_NULL, MPI_COMM_WORLD, &areawin);
	}
	adaptiveQuadrature(range, tolerance, rank);
	if(rank == 0)
		cout << "The area is " << area << endl;
	MPI_Win_free(&varswin);
	MPI_Win_free(&taskwin);
	MPI_Win_free(&areawin);
	MPI_Win_free(&workwin);
	MPI_Finalize();
	return 0;
}