#include <iostream>
#include <string>
#include <math.h>
#include "logisticRegression.h"
#include "../../lib/domp.h"
using namespace std;
using namespace domp;


LogisticRegression::LogisticRegression(int size, int in, int out) {
    N = size;
    n_in = in;
    n_out = out;

    // initialize W, b
    W = new double*[n_out];
    for(int i=0; i<n_out; i++) W[i] = new double[n_in];
    b = new double[n_out];

    for(int i=0; i<n_out; i++) {
        for(int j=0; j<n_in; j++) {
            W[i][j] = 0;
        }
        b[i] = 0;
    }
}

LogisticRegression::~LogisticRegression() {
    for(int i=0; i<n_out; i++) delete[] W[i];
    delete[] W;
    delete[] b;
}


void LogisticRegression::train(int *x, int *y, double lr) {
    double *p_y_given_x = new double[n_out];
    double *dy = new double[n_out];

    for(int i=0; i<n_out; i++) {
        p_y_given_x[i] = 0;
        for(int j=0; j<n_in; j++) {
            p_y_given_x[i] += W[i][j] * x[j];
        }
        p_y_given_x[i] += b[i];
    }
    softmax(p_y_given_x);

    for(int i=0; i<n_out; i++) {
        dy[i] = y[i] - p_y_given_x[i];

        for(int j=0; j<n_in; j++) {
            W[i][j] += lr * dy[i] * x[j] / N;
        }

        b[i] += lr * dy[i] / N;
    }
    delete[] p_y_given_x;
    delete[] dy;
}

void LogisticRegression::softmax(double *x) {
    double max = 0.0;
    double sum = 0.0;

    for(int i=0; i<n_out; i++) if(max < x[i]) max = x[i];
    for(int i=0; i<n_out; i++) {
        x[i] = exp(x[i] - max);
        sum += x[i];
    }

    for(int i=0; i<n_out; i++) x[i] /= sum;
}

void LogisticRegression::predict(int *x, double *y) {
    for(int i=0; i<n_out; i++) {
        y[i] = 0;
        for(int j=0; j<n_in; j++) {
            y[i] += W[i][j] * x[j];
        }
        y[i] += b[i];
    }

    softmax(y);
}


void test_lr() {
    srand(0);

    double learning_rate = 0.1;
    int n_epochs = 500;

    int train_N = 6;
    int test_N = 2;
    int n_in = 6;
    int n_out = 2;


    // training data
    // 6 x 6 matrix
    int train_X[36] = {
            1, 1, 1, 0, 0, 0,
            1, 0, 1, 0, 0, 0,
            1, 1, 1, 0, 0, 0,
            0, 0, 1, 1, 1, 0,
            0, 0, 1, 1, 0, 0,
            0, 0, 1, 1, 1, 0
    };

    // 6 x 2 matrix
    int train_Y[12] = {
            1, 0,
            1, 0,
            1, 0,
            0, 1,
            0, 1,
            0, 1
    };

    // construct LogisticRegression
    LogisticRegression classifier(train_N, n_in, n_out);

    int offset, size;

    DOMP_REGISTER(train_X, MPI_INT, 36);
    DOMP_REGISTER(train_Y, MPI_INT, 12);
    DOMP_PARALLELIZE(train_N, &offset, &size);

    // train online
    for(int epoch=0; epoch<n_epochs; epoch++) {

        DOMP_EXCLUSIVE(train_X, offset, size);
        DOMP_EXCLUSIVE(train_Y, offset, size);
        DOMP_SYNC;

        for(int i=0; i<train_N; i++) {
            classifier.train(&train_X[6*i], &train_Y[6*i], learning_rate);
        }
        // learning_rate *= 0.95;
        for (int i=0; i<n_out; i++) {
            DOMP_ARRAY_REDUCE_ALL(classifier.W[i], MPI_DOUBLE, MPI_SUM, 0, n_in);
            if (DOMP_IS_MASTER) {
                cout << "HERE: " << classifier.W[i][0] << endl;
            }
        }
        DOMP_ARRAY_REDUCE_ALL(classifier.b, MPI_DOUBLE, MPI_SUM, 0, n_out);

    }

    if (DOMP_IS_MASTER) {


        // test data
        int test_X[2][6] = {
                {1, 0, 1, 0, 0, 0},
                {0, 0, 1, 1, 1, 0}
        };

        double test_Y[2][2];


        // test
        for (int i = 0; i < test_N; i++) {
            classifier.predict(test_X[i], test_Y[i]);
            for (int j = 0; j < n_out; j++) {
                cout << test_Y[i][j] << " ";
            }
            cout << endl;
        }
    }

}


int main(int argc, char **argv) {
    DOMP_INIT(&argc, &argv);
    test_lr();
    DOMP_FINALIZE();
    return 0;
}
