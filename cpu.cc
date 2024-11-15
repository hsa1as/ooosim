#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <list>
#include <memory>
#include <queue>
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
  int op;
  int dest;
  int src1;
  Instruction *src1_tag;
  int src2;
  Instruction *src2_tag;
  pair<ll,ll> fetch_cycles;
  pair<ll,ll> dispatch_cycles;
  pair<ll,ll> issue_cycles;
  pair<ll,ll> execute_cycles;
  pair<ll,ll> writeback_cycles;
  vector<Instruction *> whatamidoing;
  ll life;
  bool src1_ready;
  bool src2_ready;
  bool ready;

  void print_info(){
    cout<<tag<<" "<<"fu{"<<op<<"} src{"<<src1<<","<<src2<<"} dst{"<<dest<<"} ";
    cout<<"IF{"<<fetch_cycles.first<<","<<fetch_cycles.second<<"} ";
    cout<<"ID{"<<dispatch_cycles.first<<","<<dispatch_cycles.second<<"} ";
    cout<<"IS{"<<issue_cycles.first<<","<<issue_cycles.second<<"} ";
    cout<<"EX{"<<execute_cycles.first<<","<<execute_cycles.second<<"} ";
    cout<<"WB{"<<writeback_cycles.first<<","<<writeback_cycles.second<<"} ";
    cout<<endl;
  }

  Instruction(ifstream &file){
    string pc, optype, d, s1, s2;
    if(file >> pc >> optype >> d >> s1 >> s2){
      char *p;
      op                 = strtol(optype.c_str(), NULL, 10);
      addr               = strtol(pc.c_str(), NULL, 16);
      dest               = strtol( d.c_str(), NULL, 10);
      src1               = strtol(s1.c_str(), NULL, 10);
      src2               = strtol(s2.c_str(), NULL, 10);
      life               = 0;
      state              = Fetch;
      fetch_cycles.first = curtick;
      tag                = curtag; // for practical purposes pointers are my tag
      src1_tag           = NULL;
      src2_tag           = NULL;
      ready              = true;
      src1_ready         = true;
      src2_ready         = true;
      curtag++;

    }else{
      op = 255;
    }
  }

  bool tick(){
    life++;
    if(op == 0 && life == 1) return true;
    if(op == 1 && life == 2) return true;
    if(op == 2 && life == 10) return true;

    return false;
  }
};

class CPU{
public:
  ll bandwidth;
  ll queuesz;
  // Am I Stupid?
  list<Instruction*> dispatchq;
  list<Instruction*> scheduleq;
  list<Instruction*> executeq;
  queue<unique_ptr<Instruction>> oplist;
  ifstream infile;
  vector<pair<bool, Instruction*>> registers;
    
  CPU(ll bandwidth, ll queuesz, string filename) : bandwidth(bandwidth), queuesz(queuesz), infile(filename),
                                                   registers(vector<pair<bool,Instruction*>>(128, pair<bool,Instruction*>(true, 0))){}

  bool fetch(){
    ll numfetched = 0;
    while(numfetched < bandwidth && dispatchq.size() < 2*bandwidth){
      //TODO: manage lifetimes and memory better? 
      // shared_ptr for all queues ?
      unique_ptr<Instruction> instr(new Instruction(infile));
      if(instr->op == 255){
        return false;
      }
      Instruction *raw = instr.get();
      // :(
      oplist.push(std::move(instr));
      dispatchq.push_back(raw);
      numfetched++;
      //cout<<"Done with "<<curtag<<" lines"<<endl;
    }
    if(dispatchq.size() >= 2*bandwidth){
//      cout<<"Dispatch queue full"<<endl;
    }
    return true;
  }

  bool dispatch(){
    vector<pair<Instruction *, list<Instruction*>::iterator>> temp;
    for(auto it = dispatchq.begin(); it != dispatchq.end(); it++){
      if((*it)->state == Dispatch){
        temp.push_back(make_pair(*it, it));
      }else{
        // If the instruction is in fetch, i think we can make it dispatch now
        (*it)->state = Dispatch;
        // Update timings for fetch tick and Dispatch tick
        (*it)->fetch_cycles.second = curtick - (*it)->fetch_cycles.first;
        (*it)->dispatch_cycles.first = curtick;
      }
    }
    sort(temp.begin(), temp.end(), 
         [](pair<Instruction *, list<Instruction*>::iterator> a,
            pair<Instruction *, list<Instruction*>::iterator> b){
              return a.first->tag < b.first->tag;
         });
    ll numdispatched = 0;
    for(auto it = temp.begin(); it != temp.end(); it++){
      if(numdispatched >= bandwidth) return false;
      if(scheduleq.size() >= queuesz){
//        cout<<"Schedule q full\n";
        return false;
      }
      // this instruction can now go into issue
      // Change instruction state
      numdispatched++;
      auto curinstr = (*it).first;
      curinstr->state = Issue;
      // Update timing ticks
      curinstr->dispatch_cycles.second = curtick - curinstr->dispatch_cycles.first;
      curinstr->issue_cycles.first = curtick;

      // Do register renaming or whatever idk
       if(curinstr->src1 != -1){
        // put tag of src1 if the reg is not ready
        if(registers[curinstr->src1].first == false){
          curinstr->src1_tag = registers[curinstr->src1].second;
          curinstr->src1_ready = false;
          curinstr->ready = false;
          // We are now dependent on the producer of src1
          // add to the producer's dependent list
          curinstr->src1_tag->whatamidoing.push_back(curinstr);
        }
      }
      if(curinstr->src2 != -1){
        // put tag of src2 if the reg is not ready
        if(registers[curinstr->src2].first == false){
          curinstr->src2_tag = registers[curinstr->src2].second;
          curinstr->src2_ready = false;
          curinstr->ready = false;
          // We are now dependent on the producer of src2
          // add to the producer's dependent list
          curinstr->src2_tag->whatamidoing.push_back(curinstr);
        }
      }
      // Dest reg is now no longer ready, put tag there
      if(curinstr->dest != -1){
        registers[curinstr->dest].first  = false;
        registers[curinstr->dest].second = curinstr;
      }

      // Put the unholy pointer into the scheduleq 
      scheduleq.push_back(curinstr); 
      // Remove the unholy thing from the dispatch queue using the stupid iterator
      // thank god for stl list
      dispatchq.erase((*it).second);
    }
    return true;
  }

  bool issue(){
    vector<pair<Instruction *, list<Instruction*>::iterator>> temp;
    for(auto it = scheduleq.begin(); it != scheduleq.end(); it++){
      if((*it)->ready){
        // this instruction can be issued
        temp.push_back(make_pair(*it, it));
      }
    }
    // sort the temp array according to tag;
    sort(temp.begin(), temp.end(), 
         [](pair<Instruction *, list<Instruction*>::iterator> a,
            pair<Instruction *, list<Instruction*>::iterator> b){
              return a.first->tag < b.first->tag;
         });
    ll numissued = 0;
    for(auto it = temp.begin(); it != temp.end(); it++){
      // Issue up to N 
      if(numissued >= bandwidth){
        //cout<<"Issued more than bandwidth"<<endl;
        return false;
      }
      numissued++;
      // TODO: No size limit on executeq?
      // this instruction can now go into execute 
      // Change instruction state
      auto curinstr = (*it).first;
      curinstr->state = Execute;
      // Update timing ticks
      curinstr->issue_cycles.second = curtick - curinstr->issue_cycles.first;
      curinstr->execute_cycles.first = curtick;
      // Put the unholy pointer into the executeq 
      executeq.push_back(curinstr); 
      // Remove the unholy thing from the schedule queue using the stupid iterator
      // thank god for stl list
      scheduleq.erase((*it).second);
    }   
    return true;
  }

  bool execute(){
    // tick tick boom
    for(auto it = executeq.begin(); it != executeq.end();){
      // tick for it
      bool finishing = (*it)->tick();
      if(finishing){
        auto curinstr = *it;
        executeq.erase(it++);
        // update bookkeeping and state
        curinstr->state = Writeback;
        curinstr->execute_cycles.second = curtick - curinstr->execute_cycles.first;
        curinstr->writeback_cycles.first = curtick;
        curinstr->writeback_cycles.second = 1;
        // dest of curinstr is now ready
        if(curinstr->dest != -1){
          if(registers[curinstr->dest].second == curinstr){
            // register dependent on us still, release it
            registers[curinstr->dest].first = true;
          }
          // update the dependent Instructions
          for(auto &x: curinstr->whatamidoing){
            if(x->src1_tag == curinstr){
              // we hate dangling pointers here
              x->src1_tag = NULL;
              x->src1_ready = true;
              x->ready = x->src1_ready and x->src2_ready;
            }
            if(x->src2_tag == curinstr){
              // no dangle
              x->src2_tag = NULL;
              x->src2_ready = true;
              x->ready = x->src1_ready and x->src2_ready;
            }
          }
        }
      }else{
          it++;
      }

    }
  return true;
  }
  bool retire() {
    while(!oplist.empty() && oplist.front()->state == Writeback){
      oplist.front()->print_info();
      oplist.pop();
    }
    return true;
  }
};


void start_cpu(ll bandwidth, ll queuesz, string filename){
  CPU cpu(bandwidth, queuesz, filename);
  bool c = true;
  while(c | !cpu.oplist.empty()){
    cpu.retire();
    cpu.execute();
    cpu.issue();
    cpu.dispatch();
    c = cpu.fetch();
    curtick++;
  }
  cout<<"CONFIGURATION"<<endl;
  cout<<"superscalar bandwidth (N) = "<<cpu.bandwidth<<endl;
  cout<<"dispatch queue size (2*N) = "<<2*cpu.bandwidth<<endl;
  cout<<"schedule queue size (S) = "<<cpu.queuesz<<endl;
  cout<<"RESULTS"<<endl;
  cout<<"number of instructions = "<<curtag<<endl;
  cout<<"number of cycles = "<<curtick-1<<endl;
  double ipc = curtag/((double)curtick-1);
  cout<<"IPC = "<<setprecision(3)<<ipc<<endl;
}
