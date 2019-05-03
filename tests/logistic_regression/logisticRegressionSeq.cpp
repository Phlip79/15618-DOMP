#include <iostream>
#include <string>
#include <math.h>
#include "logisticRegression.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
using namespace std;


LogisticRegression::~LogisticRegression() {}

LogisticRegressionSeq::LogisticRegressionSeq(int size, int in, int out) : LogisticRegression() {

    N = size;
    n_in = in;
    n_out = out;

    // initialize W, b
    W = new double *[n_out];
    for (int i = 0; i < n_out; i++) W[i] = new double[n_in];
    b = new double[n_out];

    for (int i = 0; i < n_out; i++) {
        for (int j = 0; j < n_in; j++) {
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

LogisticRegressionSeq::~LogisticRegressionSeq()  {
    for(int i=0; i<n_out; i++) delete[] W_temp[i];
    delete[] W_temp;
    delete[] b_temp;

    for(int i=0; i<n_out; i++) delete[] W[i];
    delete[] W;
    delete[] b;
}


void LogisticRegressionSeq::train(int *x, int *y, double lr) {
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

void LogisticRegressionSeq::softmax(double *x) {
    double max = 0.0;
    double sum = 0.0;

    for(int i=0; i<n_out; i++) if(max < x[i]) max = x[i];
    for(int i=0; i<n_out; i++) {
        x[i] = exp(x[i] - max);
        sum += x[i];
    }

    for(int i=0; i<n_out; i++) x[i] /= sum;
}

void LogisticRegressionSeq::predict(int *x, double *y) {
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
            //cout << line << endl;
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
            //cout << line << endl;
        }
        labelFile.close();
    }
    else {
        cout << "Error in opening label file" << endl;
    }

    // training data
//    int train_X[6][6] = {
//            {1, 1, 1, 0, 0, 0},
//            {1, 0, 1, 0, 0, 0},
//            {1, 1, 1, 0, 0, 0},
//            {0, 0, 1, 1, 1, 0},
//            {0, 0, 1, 1, 0, 0},
//            {0, 0, 1, 1, 1, 0}
//    };
//
//    int train_Y[6][2] = {
//            {1, 0},
//            {1, 0},
//            {1, 0},
//            {0, 1},
//            {0, 1},
//            {0, 1}
//    };


    // construct LogisticRegressionSeq
    LogisticRegressionSeq classifier(train_N, n_in, n_out);


    // train online
    for(int epoch=0; epoch<n_epochs; epoch++) {

        for (int i = 0; i < train_N; i++) {
            classifier.train(&train_X[6*i], &train_Y[2*i], learning_rate);
        }

        for (int i = 0; i < n_out; i++) {
            for (int j = 0; j < n_in; j++) {
                classifier.W[i][j] = classifier.W_temp[i][j];
            }
            //cout << "HERE: " << classifier.W[i][0] << endl;
            classifier.b[i] = classifier.b_temp[i];
            // learning_rate *= 0.95;
        }
    }


    // test data
    int test_X[2][6] = {
            {1, 0, 1, 0, 0, 0},
            {0, 0, 1, 1, 1, 0}
    };

    double test_Y[2][2];


    // test
    for(int i=0; i<test_N; i++) {
        classifier.predict(test_X[i], test_Y[i]);
        for(int j=0; j<n_out; j++) {
            cout << test_Y[i][j] << " ";
        }
        cout << endl;
    }

}


int main() {
    test_lr();
    return 0;
}
