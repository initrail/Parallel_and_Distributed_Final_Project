#include <omp.h>
#include <iostream>
#include <stack>
#include <chrono>
#include <math.h>
#include <iomanip>


using namespace std;

float function(float x) {
	return x;
}

float trapezoidalArea(float x, float y1, float y2) {
	return x * fmin(y1, y2) + (fmax(y1, y2) - fmin(y1, y2)) * x / 2;
}

bool getWork(stack<float*> stack, float * result) {
	if (!stack.empty()) {
		result = stack.top();
		stack.pop();
		return true;
	}
	else {
		return false;
	}
	return false;
}

bool putWork(stack<float*> stack, float * newRegion) {
	try
	{
		stack.push(newRegion);
		return true;
	}
	catch (const std::exception&)
	{
		return false;
	}
}

float adaptiveQuadrature(float* range, float tolerance, int numThreads) {
	stack<float*> s;
	s.push(range);
	float area = 0;	//answer
	float* r;	//range

	int idle = numThreads;
	bool gotwork = false;
	bool more;
	//shared: #of processes, idle, area, r

	omp_lock_t lock;
	omp_lock_t lock2;
	omp_init_lock(&lock);
	omp_init_lock(&lock2);

#pragma omp parallel shared(s, area, idle, numThreads, tolerance) firstprivate(gotwork) private(r, more)
		while (true)
		{
			gotwork = false;
			//critical section work
			omp_set_lock(&lock);
			more = !s.empty() || idle != numThreads;
			if (more)
			{
				idle = idle - 1;
			}
			omp_unset_lock(&lock);

			//end of critical section
			if (!more)
				break;
			while (!s.empty())
			{
				cout << "I got here " << endl;
				omp_set_lock(&lock2);
				if (s.empty())
					break;
				r = s.top();
				s.pop();
				omp_unset_lock(&lock2);

				float y1 = function(r[0]);
				float y2 = function(r[1]);
				float x = fabs(r[1] - r[0]);
				float m = r[0] + x / 2;	//midpoint
				float ym = function(m);
				float x1 = fabs(m - r[0]);
				float x2 = fabs(r[1] - m);
				float area1 = trapezoidalArea(x, y1, y2);
				float area2 = trapezoidalArea(x1, y1, ym) + trapezoidalArea(x2, ym, y2);
				if (fabs(area1 - area2) < 3 * x * tolerance) {
					delete r;
					//critical section
					omp_set_lock(&lock2);
					area += area2;
					omp_unset_lock(&lock2);
					//end of critical section
				}
				else {
					float* r1 = new float[2];
					r1[0] = r[0];
					r1[1] = m;
					float* r2 = new float[2];
					r2[0] = m;
					r2[1] = r[1];

					//critical section work
					omp_set_lock(&lock);
					s.push(r1);
					s.push(r2);
					omp_unset_lock(&lock);

					//end of critical section work

					delete r;
				}
			}//end of if

			//critical section work
			omp_set_lock(&lock);
			idle = idle + 1;
			omp_unset_lock(&lock);

			//end of critical section

		}// end of outer true loop
	omp_destroy_lock(&lock);
	omp_destroy_lock(&lock2);


	//delete processes here
	return area;
}

int main(int argc, char** argv) {
	float x1, x2, t;
	int numThread = 1;
	cout << "Enter an x1: ";
	cin >> x1;
	cout << "Enter an x2: ";
	cin >> x2;
	cout << "Enter a threshold: ";
	cin >> t;
	cout << "Enter number of threads: ";
	cin >> numThread;
	float* range = new float[2];
	range[0] = x1;
	range[1] = x2;
	
	omp_set_num_threads(numThread);

	double runtime = omp_get_wtime();
	
	float area = adaptiveQuadrature(range, t, numThread);
	
	runtime = omp_get_wtime() - runtime;

	
	cout << "The area is " << area << endl;
	cout << "It ran in " << setiosflags(ios::fixed)
		<< setprecision(8) << runtime << " seconds.\n";
	return 0;
}