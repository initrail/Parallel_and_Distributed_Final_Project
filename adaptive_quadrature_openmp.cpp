//#include <omp.h>
#include <iostream>
#include <stack>
#include <chrono>
#include <math.h>

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

float adaptiveQuadrature(float* range, float tolerance) {
	stack<float*> s;
	s.push(range);
	float area = 0;	//answer
	float* r;	//range


	//create processes here
	
	int numProcess = 8;	//replace with user argument for number of processes
	int idle = numProcess;
	//shared: #of processes, idle, area
	while (true)
	{
		//critical section work
		bool more = !s.empty() || idle != numProcess;
		if (more)
			idle = idle - 1;
		//end of critical section
		if (!more)
			break;
		while (getWork(s, r))
		{
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
				area += area2;
				//end of critical section
			}
			else {
				float* r1 = new float[2];
				r1[0] = r[0];
				r1[1] = m;
				float* r2 = new float[2];
				r2[0] = m;
				r2[1] = r[1];
				if (!putWork(s, r1) || !putWork(s, r2))
					cout << "No room in stack" << endl;
				delete r;
			}
		}//end of while
		//critical sectio work
		idle = idle + 1;
		//end of critical section

	}// end of outer true loop


	//delete processes here
	return area;
}

int main(int argc, char** argv) {
	float x1, x2, t;
	cout << "Enter an x1: ";
	cin >> x1;
	cout << "Enter an x2: ";
	cin >> x2;
	cout << "Enter a threshold: ";
	cin >> t;
	float* range = new float[2];
	range[0] = x1;
	range[1] = x2;
	auto t1 = chrono::high_resolution_clock::now();
	float area = adaptiveQuadrature(range, t);
	auto t2 = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::microseconds>(t2 - t1).count();
	cout << "The area is " << area << endl;
	cout << "It ran in " << duration << "micro seconds.\n";
	return 0;
}