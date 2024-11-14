#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>

using namespace std;
using ll = long long;

enum InstructionState{
  Fetch,
  Dispatch,
  Issue,
  Execute,
  Writeback,
  Retired
};

static ll curtick = 0;
static ll curtag = 0;

class Instruction{
  public:
  ll tag;
  uint32_t addr;
  InstructionState state;
  uint8_t op;
  int8_t dest;
  int8_t src1;
  int8_t src2;
  pair<ll,ll> fetch_cycles;
  pair<ll,ll> dispatch_cycles;
  pair<ll,ll> issue_cycles;
  pair<ll,ll> execute_cycles;
  pair<ll,ll> writeback_cycles;
  ll life;

  void print_info(){
    cout<<tag<<" "<<"fu{"<<op<<"} src{"<<src1<<","<<src2<<"} dst{"<<dest<<"} ";
    cout<<"IF{"<<fetch_cycles.first<<","<<fetch_cycles.second<<"} ";
    cout<<"ID{"<<dispatch_cycles.first<<","<<dispatch_cycles.second<<"} ";
    cout<<"IS{"<<issue_cycles.first<<","<<issue_cycles.second<<"} ";
    cout<<"EX{"<<execute_cycles.first<<","<<execute_cycles.second<<"} ";
    cout<<"WB{"<<writeback_cycles.first<<","<<writeback_cycles.second<<"} ";
  }

  Instruction(ifstream &file){
    string pc, optype, d, s1, s2;
    if(file >> pc >> optype >> d >> s1 >> s2){
      char *p;
      addr               = strtol(pc.c_str(), &p, 16);
      op                 = strtol(optype.c_str(), &p, 10) & 0xFF;
      dest               = strtol( d.c_str(), &p, 10) & 0xFF;
      src1               = strtol(s1.c_str(), &p, 10) & 0xFF;
      src2               = strtol(s1.c_str(), &p, 10) & 0xFF;
      life               = 0;
      state              = Fetch;
      fetch_cycles.first = curtick;
      tag                = curtag;
    }else{
      op = 255;
    }
  }

  void tick(){
    life++;
  }
};

class CPU{
public:
  ll bandwidth;
  ll queuesz;
  vector<Instruction> dispatchq;
  vector<Instruction> scheduleq;
  vector<Instruction> executeq;
  ifstream infile;

  CPU(ll bandwidth, ll queuesz, string filename) : bandwidth(bandwidth), queuesz(queuesz), infile(filename){}

  bool fetch(){
    ll numfetched = 0;
    while(numfetched < bandwidth && dispatchq.size() != 2*bandwidth){
      Instruction instr = Instruction(infile);
      if(instr.op == 255){
        return false;
      }
      dispatchq.push_back(instr);
      numfetched++;
    }
    return true;
  }

  bool dispatch(){
    for(auto it = dispatchq.begin(); it != dispatchq.end(); it++){
      if(it->state == Dispatch){
        if(scheduleq.size() >= queuesz){
          return false;
        }
        it->state = Issue;
        scheduleq.push_back((*it));
        dispatchq.erase(it);
      }else{
        it->state = Dispatch;
      }
    } 
    return true;
  }
  
  

};


void start_cpu(ll bandwidth, ll queuesz, string filename){


}
