class LogisticRegression {

public:
    int N;  // num of inputs
    int n_in;
    int n_out;
    double **W;
    double *b;
    double **W_temp;
    double *b_temp;
    LogisticRegression(int, int, int);
    ~LogisticRegression();
    void train(int*, int*, double);
    void softmax(double*);
    void predict(int*, double*);
};
