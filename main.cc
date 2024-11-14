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
  
  ifstream infile(filename);
  string pc, optype, d, s1, s2;


  while(infile >> pc >> optype >> d >> s1 >> s2){

    char *p;
    uint32_t addr   = strtol(pc.c_str(), &p, 16);
    uint8_t uop     = strtol(optype.c_str(), &p, 10);
    int8_t dest_reg = strtol( d.c_str(), &p, 10) & 0xFF;
    int8_t src_reg1 = strtol(s1.c_str(), &p, 10) & 0xFF;
    int8_t src_reg2 = strtol(s1.c_str(), &p, 10) & 0xFF;

    do_op(addr, uop, dest_reg, src_reg1, src_reg2);

  }

  print_results();
  
  return 1;
}
