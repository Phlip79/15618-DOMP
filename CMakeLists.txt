cmake_minimum_required(VERSION 3.10)
project(DOMP)

set(CMAKE_CXX_STANDARD 11)

add_executable(DOMP
        lib/Makefile
        README.md lib/util/DoublyLinkedList.h tests/arraySum.cpp lib/domp.h lib/domp.cpp lib/DataManager.cpp lib/DataManager.h lib/util/SplitList.cpp lib/util/SplitList.h lib/CommandManager.cpp lib/CommandManager.h tests/testDataTransfer.cpp tests/logistic_regression/logisticRegression.cpp tests/logistic_regression/logisticRegression.h tests/logistic_regression/logisticRegressionSeq.cpp lib/util/CycleTimer.cpp lib/util/CycleTimer.h)
