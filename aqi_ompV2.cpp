#include <iostream>
#include <iomanip>
#include <cmath>
#include <time.h>
#include <omp.h>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <cfloat>
#include <string>

using namespace std;

double integral_par(
        double (*f)(double x), /* function to integrate */
        double a, /* left interval boundary */
        double b, /* right interval boundary */
        double tolerance) /* error tolerance */
{
    double h, mid, one_trapezoid_area, two_trapezoid_area, integral_result;
    double  left_area=0.0;
    double right_area =0.0;
    h = b - a;
    mid = (a+b)/2;
    one_trapezoid_area = h * (f(a) + f(b)) / 2.0;
    two_trapezoid_area = h/2 * (f(a) + f(mid)) / 2.0 +
                         h/2 * (f(mid) + f(b)) / 2.0;
    if (fabs(one_trapezoid_area - two_trapezoid_area)
        < 3.0 * tolerance){

        /* error acceptable */
        integral_result = two_trapezoid_area;
    }else{
#pragma omp taskq
        {
#pragma omp task
            {
                left_area = integral_par(f, a, mid, tolerance / 2);
            } /* end task */
#pragma omp task
            {
                right_area = integral_par(f, mid, b, tolerance / 2);
            } /* end task */
            integral_result = left_area + right_area;
        } /* end taskq */
        integral_result = integral_result + left_area + right_area;
    }
    cout << "integral result is " <<integral_result << endl;
    return integral_result;
}

int main(int argc, char **argv)
{
    double answer = 0;
    int numThreads=0;
    int count = 0;
    numThreads= atoi(argv[2]);
    omp_set_num_threads(numThreads);
    cout << fixed << setprecision(8);
    cout << "about to enter parallel region 1" << endl;
#pragma omp parallel
    {
#pragma omp taskq lastprivate(answer)
        {
#pragma omp task
            answer = integral_par(sin, 0, M_PI, 0.7);
            count = count + 1;
            cout<<"count is " << count << endl;
            cout << "answer is " << answer<< endl;
        } /* end taskq */
    } /* end parallel */
    cout << "integration from 0 to pi is " << answer << endl;
    cout << "M_PI" << M_PI<< endl;
    return 0;
}

