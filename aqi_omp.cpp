#include <iostream>
#include <iomanip>
#include <cmath>
#include <omp.h>
#include <stdlib.h>

using namespace std;

double func(double x) {
    return cos(x)*sin(exp(x))+1;
}

double integral_par(
        double (*f)(double x), /* function to integrate */
        double a, /* left interval boundary */
        double b, /* right interval boundary */
        double tolerance) /* error tolerance */

{
    double h, mid, one_trapezoid_area, two_trapezoid_area, integral_result=0.0;
    double  left_area=0.0;
    double right_area =0.0;
    double ans = 0.0;

    h = b - a;
    mid = (a+b)/2;
    one_trapezoid_area = h * (f(a) + f(b)) / 2.0;
    two_trapezoid_area = h/2 * (f(a) + f(mid)) / 2.0 +
                         h/2 * (f(mid) + f(b)) / 2.0;
    if (fabs(one_trapezoid_area - two_trapezoid_area)
        < 3.0 * tolerance)
    {/* error acceptable */

        integral_result = two_trapezoid_area;
        return(integral_result);

    }

#pragma omp parallel
#pragma omp single
    {
#pragma omp task shared(left_area) //private(a,b,h, mid, one_trapezoid_area, two_trapezoid_area) shared(tolerance, integral_result)
        {

            left_area = integral_par(f, a, mid, tolerance / 2);

        } /* end task */
#pragma omp task shared(right_area) //private(a,b,h, mid, one_trapezoid_area, two_trapezoid_area) shared(tolerance, integral_result)
        {

            right_area = integral_par(f, mid, b, tolerance / 2);

        } /* end task */


    }
#pragma omp tastwait //shared(integral_result)
    {
        integral_result = left_area + right_area;

    }

    return integral_result;
}

int main(int argc, char **argv)
{
    double a, b, tolerance, answer, runtime;
    int numThreads=0;

    a = atof(argv[1]); // getting the lower bound
    b = atof(argv[2]); // // getting the upper bound
    tolerance = atof(argv[3]); // getting the tolerance value
    numThreads= atoi(argv[4]); // getting the number of threads
    answer = 0.0; // initiallizaing the answer variable

    omp_set_num_threads(numThreads);
    cout << fixed << setprecision(8);

    runtime = omp_get_wtime();
    answer = integral_par(func, a, b, tolerance); // compute the integral

    printf("Result: %f\n",answer);
    runtime = omp_get_wtime() - runtime;
    cout<< "Program runs in " << setiosflags(ios::fixed) << setprecision(8) << runtime << " seconds\n";

    return 0;
}