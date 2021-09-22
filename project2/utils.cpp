#include <iostream>
#include "utils.h"

using namespace std;

void print_list(list<int> collection) {
    cout << "The ports that are in the list are as follows: \n";
    for (list<int>::iterator i=collection.begin(); i!=collection.end(); i++)
       cout << *i << endl;
}