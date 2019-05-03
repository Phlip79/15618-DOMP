#include <iostream>
#include <string>
#include <math.h>
#include "logisticRegression.h"
#include "../../lib/domp.h"
#include <fstream>
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

    W_temp = new double *[n_out];
    for (int i = 0; i < n_out; i++) W_temp[i] = new double[n_in];
    b_temp = new double[n_out];

    for (int i = 0; i < n_out; i++) {
        for (int j = 0; j < n_in; j++) {
            W_temp[i][j] = 0;
        }
        b_temp[i] = 0;
    }

}

LogisticRegression::~LogisticRegression() {
    for(int i=0; i<n_out; i++) delete[] W_temp[i];
    delete[] W_temp;
    delete[] b_temp;

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
            W_temp[i][j] += lr * dy[i] * x[j] / N;
        }

        b_temp[i] += lr * dy[i] / N;
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

    int *train_X;
    int *train_Y;

    //ifstream inputFile;
    //inputFile.open("array_input.txt");

    ifstream inputFile ("tests/logistic_regression/array_input.txt");
    string line;
    if (inputFile.is_open()) {
        getline(inputFile, line);
        train_N = atoi(line.c_str());

        train_X = new int[train_N * 6];
        train_Y = new int[train_N * 2];

        int i = 0;
        string delimiter = ",";
        while(getline(inputFile, line)) {
            size_t pos = 0;
            string token;
            while ((pos = line.find(delimiter)) != string::npos) {
                token = line.substr(0, pos);
                train_X[i] = atoi(token.c_str());
                line.erase(0, pos + delimiter.length());
                i++;
            }
            token = line.substr(0, pos);
            train_X[i] = atoi(token.c_str());
            i++;
        }
        inputFile.close();
    }
    else {
        cout << "Error in opening input file" << endl;
    }

    ifstream labelFile ("tests/logistic_regression/array_label.txt");
    if (labelFile.is_open()) {
        getline(labelFile, line);

        int i = 0;
        string delimiter = ",";
        while(getline(labelFile, line)) {
            size_t pos = 0;
            string token;
            while ((pos = line.find(delimiter)) != string::npos) {
                token = line.substr(0, pos);
                train_Y[i] = atoi(token.c_str());
                line.erase(0, pos + delimiter.length());
                i++;
            }
            token = line.substr(0, pos);
            train_Y[i] = atoi(token.c_str());
            i++;
        }
        labelFile.close();
    }
    else {
        cout << "Error in opening label file" << endl;
    }


    // training data

    //Inputs
    // 6 x 6 matrix
//    int train_X[36] = {
//            1, 1, 1, 0, 0, 0,
//            1, 0, 1, 0, 0, 0,
//            1, 1, 1, 0, 0, 0,
//            0, 0, 1, 1, 1, 0,
//            0, 0, 1, 1, 0, 0,
//            0, 0, 1, 1, 1, 0
//    };
//
//    //Labels
//    // 6 x 2 matrix
//    int train_Y[12] = {
//            1, 0,
//            1, 0,
//            1, 0,
//            0, 1,
//            0, 1,
//            0, 1
//    };

    // construct LogisticRegression
    LogisticRegression classifier(train_N, n_in, n_out);

    int offset, size;


    DOMP_REGISTER(train_X, MPI_INT, train_N * n_in);
    DOMP_REGISTER(train_Y, MPI_INT, train_N * n_out);
    DOMP_PARALLELIZE(train_N, &offset, &size);

    DOMP_EXCLUSIVE(train_X, offset, size);
    DOMP_EXCLUSIVE(train_Y, offset, size);
    DOMP_SYNC;

    // train online
    for(int epoch=0; epoch<n_epochs; epoch++) {

        for(int i=offset; i<offset+size; i++) {
            classifier.train(&train_X[6*i], &train_Y[2*i], learning_rate);
        }

        // learning_rate *= 0.95;
        for (int i = 0; i < n_out; i++) {
            DOMP_ARRAY_REDUCE_ALL(classifier.W_temp[i], MPI_DOUBLE, MPI_SUM, 0, n_in);
            for (int j = 0; j < n_in; j++) {
                classifier.W[i][j] += classifier.W_temp[i][j];
                classifier.W_temp[i][j] = 0;
            }

            DOMP_ARRAY_REDUCE_ALL(classifier.b_temp, MPI_DOUBLE, MPI_SUM, 0, n_out);
            classifier.b[i] += classifier.b_temp[i];
            classifier.b_temp[i] = 0;

            if (DOMP_IS_MASTER) {
                //cout << "HERE DIFF: " << classifier.W_temp[i][0] << endl;
                //cout << "HERE: " << classifier.W[i][0] << endl;
            }
        }

    }

    if (DOMP_IS_MASTER) {


        // test data inputs
        int test_X[2][6] = {
                {1, 0, 1, 0, 0, 0},
                {0, 0, 1, 1, 1, 0}
        };

        //predicted labels
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
