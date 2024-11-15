#include <iostream>
#include <fstream>
#include <iomanip>
#include "cpu.h"

using namespace std;
using ll = long long;
int main(int argc, char **argv){

  if(argc != 4){
    cerr<<"Usage: "<<argv[0]<<"<N> <S> <trace_file>"<<endl;
    return -1;
  }

  ll bandwidth, queuesz;
  bandwidth       = stoi(argv[1]);
  queuesz         = stoi(argv[2]);
  string filename = argv[3]; 
  start_cpu(bandwidth, queuesz, filename);
 
  return 1;
}
