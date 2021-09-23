#include <iostream>
#include "utils.h"

using namespace std;

void print_list(vector<int> vec) {
    cout << "The ports that are in the list are as follows: \n";
    for (int i = 0; i <= vec.size()-1; i++) {
        cout << vec[i] << endl;
    }
}