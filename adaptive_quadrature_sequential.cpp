#include <iostream>
#include <stack>
#include <chrono>
#include <math.h>
#include <iomanip>

using namespace std;

double function(double x) {
	return cos(x)*sin(exp(x)) + 1;
}


double trapezoidalArea(double x, double y1, double y2) {
	return x * fmin(y1, y2) + (fmax(y1, y2) - fmin(y1, y2)) * x / 2;
}

double adaptiveQuadrature(double* range, double tolerance) {
	stack<double*> s;
	s.push(range);
	double area = 0.0;
	double* r;	//range
	double m;	//midpoint
	while (!s.empty()) {
		r = s.top();
		s.pop();
		double y1 = function(r[0]);
		double y2 = function(r[1]);
		double x = fabs(r[1] - r[0]);
		m = r[0] + x / 2;
		double ym = function(m);
		double x1 = fabs(m - r[0]);
		double x2 = fabs(r[1] - m);
		double area1 = trapezoidalArea(x, y1, y2);
		double area2 = trapezoidalArea(x1, y1, ym) + trapezoidalArea(x2, ym, y2);
		if (fabs(area1 - area2) < 3 * x * tolerance) {
			delete r;
			area += area2;
		}
		else {
			double* r1 = new double[2];
			r1[0] = r[0];
			r1[1] = m;
			double* r2 = new double[2];
			r2[0] = m;
			r2[1] = r[1];
			s.push(r1);
			s.push(r2);
			delete r;
		}
	}
	return area;
}

int main(int argc, char** argv) {
	double x1, x2, t;
	cout << "Enter an x1: ";
	cin >> x1;
	cout << "Enter an x2: ";
	cin >> x2;
	cout << "Enter a threshold: ";
	cin >> t;
	double* range = new double[2];
	range[0] = x1;
	range[1] = x2;
	auto t1 = chrono::high_resolution_clock::now();
	double area = adaptiveQuadrature(range, t);
	auto t2 = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::milliseconds>(t2 - t1).count();
	cout << fixed << setprecision(26);

	cout << "The area is " << area << endl;
	cout<<"It ran in "<<duration<<" milliseconds.\n";
	return 0;
}