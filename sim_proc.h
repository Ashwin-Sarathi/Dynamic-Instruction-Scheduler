#ifndef SIM_PROC_H
#define SIM_PROC_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <thread>
#include <chrono>

using namespace std;

typedef struct proc_params{
    unsigned long int rob_size;
    unsigned long int iq_size;
    unsigned long int width;
}proc_params;


struct Instruction
{
    int seq_no;
    int pc;
    int operation_type;
    int dest;
    int dest_rob;
    int src1;
    bool src1_rdy;
    bool src1_rob_valid;
    int src1_rob;
    int src2;
    bool src2_rdy;
    bool src2_rob_valid;
    int src2_rob;
    int FE_start;
    int DE_start;
    int RN_start;
    int RR_start;
    int DI_start;
    int IS_start;
    int EX_start;
    int WB_start;
    int RT_start;
    int out_of_pl;
};

struct Rename_map_table {
    int reg_no;
    bool valid;
    int rob_tag;
};

struct Issue_queue {
    bool valid;
    int seq_no;
    int dest_rob_tag;
    bool src1_rdy;
    int src1;
    bool is_src1_rob_tag;
    int src1_rob_tag;
    bool src2_rdy;
    int src2;
    bool is_src2_rob_tag;
    int src2_rob_tag;
};

struct Reorder_buffer {
    int rob_tag;
    int seq_no;
    int value;
    int dest;
    bool rdy;
    int pc;
};

struct Execution_list {
    bool valid;
    int seq_no;
    int cycles_left;
    int dest_rob_tag;
};

struct Write_back {
    bool valid;
    int seq_no;
};

class Dynamic_Scheduler {
    public:
        int global_clock = 1;

        int ROB_size;
        int IQ_size;
        int width;
        int no_of_reg = 67;

        int seq_no;
        int PC;
        int operation_type;
        int dest_reg;
        int src1_reg;
        int src2_reg;

        string trace;

        vector<Rename_map_table> RMT;
        vector<Issue_queue> IQ;
        vector<Reorder_buffer> ROB;
        vector<Instruction> ins;
        vector<Execution_list> ex_list;
        vector<Write_back> WB;

        int head_rob_tag = 0;
        int tail_rob_tag = 0;
        int rob_entry_counter = 0;
        int issue_queue_counter = 0;

        vector<int> DE;
        vector<int> RN;
        vector<int> RR;
        vector<int> DI;
        vector<int> RT;
        // vector<int> WB;

        int latency_0 = 1;
        int latency_1 = 2;
        int latency_2 = 5;

        int no_of_ins = 0;
        size_t to_be_fetched = 0;

        int final_clock = 0;

        Dynamic_Scheduler(int ROB_size, int IQ_size, int width) {
            this->ROB_size = ROB_size;
            this->IQ_size = IQ_size;
            this->width = width;
            this->seq_no = seq_no;
            arch_setup();
        }

        void arch_setup() {
            RMT.resize(no_of_reg);
            for (int i = 0; i <= 66; i++) {
                RMT[i].reg_no = i;
                RMT[i].valid = 0;
                RMT[i].rob_tag = 0;
            }

            IQ.resize(IQ_size);
            for (int i = 0; i < IQ_size; i++) {
                IQ[i].valid = 0;
                IQ[i].seq_no = 0;
                IQ[i].dest_rob_tag = -1;
                IQ[i].src1_rdy = 0;
                IQ[i].src1 = -1;
                IQ[i].src2_rdy = 0;
                IQ[i].src2 = -1;
                IQ[i].is_src1_rob_tag = 0;
                IQ[i].is_src2_rob_tag = 0;
                IQ[i].src1_rob_tag = -1;
                IQ[i].src2_rob_tag = -1;
            }

            ROB.resize(ROB_size);
            for (int i = 0; i < ROB_size; i++) {
                ROB[i].rob_tag = i;
                ROB[i].seq_no = -1;
                ROB[i].value = 0;
                ROB[i].dest = 0;
                ROB[i].rdy = 0;
                ROB[i].pc = 0;
            }

            DE.resize(width);
            for (int i = 0; i < width; i++) {
                DE[i] = -1;
            }

            RN.resize(width);
            for (int i = 0; i < width; i++) {
                RN[i] = -1;
            }

            RR.resize(width);
            for (int i = 0; i < width; i++) {
                RR[i] = -1;
            }

            DI.resize(width);
            for (int i = 0; i < width; i++) {
                DI[i] = -1;
            }

            ex_list.resize(width * 5);
            for (int i = 0; i < width * 5; i ++) {
                ex_list[i].valid = false;
                ex_list[i].seq_no = -1;
                ex_list[i].cycles_left = -1;
                ex_list[i].dest_rob_tag = -1;
            }

            WB.resize(width * 5);
            for (int i = 0; i < (width * 5); i++) {
                WB[i].valid = false;
                WB[i].seq_no = -1;
            }

            RT.resize(width);
            for (int i = 0; i < (width); i++) {
                RT[i] = -1;
            }
        }

        void clock_increment() {
            global_clock ++;
        }

        void trace_parse(string trace) {
            istringstream iss(trace);
            iss >> dec >> seq_no;
            iss >> hex >> PC;
            iss >> dec >> operation_type;
            iss >> dec >> dest_reg;
            iss >> dec >> src1_reg;
            iss >> dec >> src2_reg;
        }

        void add_to_ins() {
            ins.push_back({seq_no, PC, operation_type, dest_reg, -1, src1_reg, 1, 0, -1, src2_reg, 1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
            no_of_ins += 1;
        }

        void remove_top_ins() {            
            ins.erase(ins.begin());
        }

        void execute_pipeline() {
            do {
                retire();
                writeback();
                execute();
                issue();
                dispatch();
                register_read();
                rename();
                decode();
                fetch();
            } while (advance_cycle());
        }

        void retire() {
            for (int i = 0; i < width; i++) {
                if (ROB_head_ready_to_retire()) {
                    retire_from_ROB();
                }
                else {
                    break;
                }
            }
        }

        void writeback() {
            if (!check_WB_empty()) {
                for (int i = 0; i < (width * 5); i++) {
                    if (WB[i].valid) {
                        WB[i].valid = false;
                        mark_ready_in_rob(WB[i].seq_no);
                    }
                }
            }
            else {
                do_nothing();
            }
        }
        
        void execute() {
            if (!check_exec_list_empty()) {
                for (int i = 0; i < (width * 5); i++) {
                    if (ex_list[i].valid && !ex_list[i].cycles_left) {
                        ex_list[i].valid = false;
                        advance_to_writeback(ex_list[i].seq_no);
                        wakeup(ex_list[i].dest_rob_tag);
                    }
                }
            }
            else {
                do_nothing();
            }

        }

        void issue() {
            if (!check_IQ_empty()) {
                if (count_ready_to_ex() > width) {
                    for (int i = 0; i < width; i++) {
                        add_to_ex_list(find_oldest_ready(), i);
                    }
                }
                else {
                    for (int i = 0; i < IQ_size; i++) {
                        if (IQ[i].valid && IQ[i].src1_rdy && IQ[i].src2_rdy) {
                            add_to_ex_list(IQ[i].seq_no, i);
                        }
                    }
                }
            }
            else {
                do_nothing();
            }
        }

        void dispatch() {
            if (!check_DI_empty()) {
                if (num_of_free_IQ_slots() >= size_of_dispatch_bundle()) {
                    for (int i = 0; i < width; i++) {
                        if (DI[i] > -1) {
                            add_to_IQ(DI[i], i);
                        }
                    }
                }
                else {
                    do_nothing();
                }
            }
            else {
                do_nothing();
            }
        }

        void register_read() {
            if (!check_RR_empty()) {
                if (!check_DI_empty()) {
                    check_readiness();
                    do_nothing();
                }
                else {
                    check_readiness();
                    for (int i = 0; i < width; i++) {
                        if (RR[i] > -1) {
                            advance_to_dispatch(i);
                        }
                    }
                }
            }
            else {
                do_nothing();
            }
        }

        void rename() {
            if (!check_RN_empty()) {
                if (!check_RR_empty() || check_ROB_full()) {
                    if (check_ROB_full()) {
                    }
                    do_nothing();
                }
                else {
                    for (int i = 0; i < width; i++) {
                        if (RN[i] > -1) {
                            add_to_ROB(RN[i]);
                            advance_to_reg_read(i);
                        }
                    }                
                }
            }
            else {
                do_nothing();
            }
        }

        void decode() {
            if (!check_DE_empty()) {
                if (!check_RN_empty()) {
                    do_nothing();
                }
                else {
                    for (int i = 0; i < width; i++) {
                        advance_to_rename(i);
                    }
                }
            }
            else {
                do_nothing();
            }
        }

        void fetch() {
            if ((to_be_fetched > (ins.size() - 1)) || !check_DE_empty()) {
                do_nothing();
            }
            else {
                for (int i = 0; i < width; i ++) {
                    if (to_be_fetched < ins.size()) {
                        fetch_into_decode(to_be_fetched, i);
                        to_be_fetched ++;
                    }
                    else {
                        break;
                    }
                }
            }
        }

        int advance_cycle() {
            if (check_pipeline_empty()) {
                final_clock = global_clock;
                return 0;
            }
            clock_increment();
            advance_execution_cycles();
            return 1;
        }

        int check_pipeline_empty() {
            if ((check_DE_empty() && check_RN_empty() && check_RR_empty() && check_DI_empty() && check_IQ_empty() && check_exec_list_empty() && check_WB_empty() && check_ROB_empty())) {
                return 1;
            }
            else {
                return 0;
            }
        }

        void retire_from_ROB() {
            if (ROB[head_rob_tag].rdy) {
                ins[ROB[head_rob_tag].seq_no].out_of_pl = global_clock;

                if (ROB[head_rob_tag].dest > -1) {
                    invalidate_RMT(ROB[head_rob_tag].rob_tag, ROB[head_rob_tag].dest);
                }

                ROB[head_rob_tag].rdy = 0;

                if (head_rob_tag == (ROB_size - 1)) {
                    head_rob_tag = 0;
                }
                else {
                    head_rob_tag ++;
                }
                rob_entry_counter --;
            }
        }

        void invalidate_RMT(int ROB_tag, int reg_no) {
            if (RMT[reg_no].rob_tag == ROB_tag) {
                RMT[reg_no].valid = false;
            }
            else {
                return;
            }
        }

        int ROB_head_ready_to_retire() {            
            if (ROB[head_rob_tag].rdy) {
                return 1;
            }   
            else {
                return 0;
            }         
        }

        int check_ROB_empty() {
            if (head_rob_tag == tail_rob_tag) {
                return 1;
            }
            else {
                return 0;
            }
        }

        void mark_ready_in_rob(int seq_no) {
            for (int i = 0; i < ROB_size; i++) {
                if (ROB[i].seq_no == seq_no) {
                    ROB[i].rdy = true;
                    break;
                }
            }
            ins[seq_no].RT_start = global_clock;
        }

        int check_WB_empty() {
            for (size_t i = 0; i < WB.size(); i++) {
                if (WB[i].valid) {
                    return 0;
                }
            }
            return 1;
        }

        void wakeup(int wake_tag) {
            for (int i = 0; i < IQ_size; i++) {
                if (IQ[i].valid && (IQ[i].src1_rob_tag == wake_tag)) {
                    IQ[i].src1_rdy = true;
                }

                if (IQ[i].valid && (IQ[i].src2_rob_tag == wake_tag)) {
                    IQ[i].src2_rdy = true;
                }
            }

            for (size_t i = 0; i < ins.size(); i++) {
                if ((ins[i].src1_rob == wake_tag) && ins[i].src1_rob_valid) {
                    ins[i].src1_rdy = true;
                }

                if ((ins[i].src2_rob == wake_tag) && ins[i].src2_rob_valid) {
                    ins[i].src2_rdy = true;
                }
            }
        }

        void advance_to_writeback(int seq_no) {
            for (size_t i = 0; i < WB.size(); i++) {
                if  (!WB[i].valid) {
                    WB[i].valid = true;
                    WB[i].seq_no = seq_no;
                    ins[seq_no].WB_start = global_clock;
                    break;
                }
            }
        }

        int check_exec_list_empty() {
            for (int i = 0; i < (width * 5); i++) {
                if (ex_list[i].valid) {
                    return 0;
                }
            }
            return 1;
        }

        void advance_execution_cycles() {
            for (size_t i = 0; i < ex_list.size(); i++) {
                if (ex_list[i].valid) {
                    ex_list[i].cycles_left --;
                }
            }
        }

        void add_to_ex_list(int ins_no, int IQ_slot) {
            for (int i = 0; i < IQ_size; i++) {
                if (IQ[i].valid && (IQ[i].seq_no == ins_no)) {
                    IQ[i].valid = false;
                }
            }

            for (int i = 0; i < width * 5; i++) {
                if (!ex_list[i].valid) {
                    ex_list[i].valid = true;
                    ex_list[i].seq_no = ins[ins_no].seq_no;
                    ex_list[i].dest_rob_tag = ins[ins_no].dest_rob;

                    if (ins[ins_no].operation_type == 0) {
                        ex_list[i].cycles_left = latency_0;
                    }
                    else if (ins[ins_no].operation_type == 1) {
                        ex_list[i].cycles_left = latency_1;
                    }
                    else {
                        ex_list[i].cycles_left = latency_2;
                    }

                    ins[ins_no].EX_start = global_clock;

                    break;
                }
            }
        }

        int find_oldest_ready() {
            int oldest = 20000;
            for (int i = 0; i < IQ_size; i++) {
                if (IQ[i].valid && IQ[i].src1_rdy && IQ[i].src2_rdy) {
                    if (IQ[i].seq_no < oldest) {
                        oldest = IQ[i].seq_no;
                    }
                }
            }
            return oldest;
        }

        int count_ready_to_ex() {
            int ready_to_ex = 0;
            for (int i = 0; i < IQ_size; i++) {
                if (IQ[i].valid && IQ[i].src1_rdy && IQ[i].src2_rdy) {
                    ready_to_ex ++;
                }
            }
            return ready_to_ex;
        }

        void add_to_IQ(int seq_no, int DI_location) {
            for (int i = 0; i < IQ_size; i++) {
                if (!IQ[i].valid) {
                    ins[seq_no].IS_start = global_clock;

                    IQ[i].valid = true;
                    IQ[i].seq_no = ins[seq_no].seq_no;
                    IQ[i].dest_rob_tag = ins[seq_no].dest_rob;
                    IQ[i].src1_rdy = ins[seq_no].src1_rdy;
                    IQ[i].src2_rdy = ins[seq_no].src2_rdy;
                    IQ[i].src1_rob_tag = ins[seq_no].src1_rob;
                    IQ[i].src1 = ins[seq_no].src1; 
                    IQ[i].src2_rob_tag = ins[seq_no].src2_rob;
                    IQ[i].src2 = ins[seq_no].src2;
                    
                    if (ins[seq_no].src1_rob_valid) {
                        IQ[i].is_src1_rob_tag = 1;
                    }
                    else {
                        IQ[i].is_src1_rob_tag = 0;                     
                    }

                    if (ins[seq_no].src2_rob_valid) {
                        IQ[i].is_src2_rob_tag = 1;
                    }
                    else {
                        IQ[i].is_src2_rob_tag = 0;                    
                    }

                    DI[DI_location] = -1;
                    break;
                }
            }
        }

        int check_IQ_empty() {
            for (int i = 0; i < IQ_size; i++) {
                if (IQ[i].valid) {
                    return 0;
                }
            }
            return 1;
        }

        int size_of_dispatch_bundle() {
            int size = 0;
            for (int i = 0; i < width; i++) {
                if (DI[i] > -1) {
                    size ++;
                }
            }
            return size;
        }

        int num_of_free_IQ_slots() {
            int free_slots = 0;
            for (int i = 0; i < IQ_size; i++) {
                if (!IQ[i].valid) {
                    free_slots ++;
                }
            }
            return free_slots;
        }

        int check_DI_empty() {
            for (int i = 0; i < width; i++) {
                if (DI[i] > -1) {
                    return 0;
                }
            }
            return 1;
        }

        void advance_to_dispatch(int location) {
            if (RR[location] > -1) {
                DI[location] = RR[location];
                ins[RR[location]].DI_start = global_clock;
                RR[location] = -1;
            }
        }

        int check_RR_empty() {
            for (int i = 0; i < width; i++) {
                if (RR[i] > -1) {
                    return 0;
                }
            }
            return 1;
        }

        void check_readiness() {
            for (int i = 0; i < width; i++) {
                if (RR[i] > -1) {
                    for (int j = 0; j < ROB_size; j++) {
                        if (ins[RR[i]].src1_rob_valid && (ins[RR[i]].src1_rob == ROB[j].rob_tag)) {
                            if (ROB[j].rdy) {
                                ins[RR[i]].src1_rdy = ROB[j].rdy;
                            }
                        }

                        if (ins[RR[i]].src2_rob_valid && (ins[RR[i]].src2_rob == ROB[j].rob_tag)) {
                            if (ROB[j].rdy) {
                                ins[RR[i]].src2_rdy = ROB[j].rdy;
                            }
                        }
                    }
                }
            }
        }

        void advance_to_reg_read(int location){
            if (RN[location] > -1) {
                RR[location] = RN[location];
                ins[RN[location]].RR_start = global_clock;
                RN[location] = -1;
            }
        }

        void add_to_ROB(int ins_no) {            
            ROB[tail_rob_tag].dest = ins[ins_no].dest;
            ROB[tail_rob_tag].pc = ins[ins_no].pc;
            ROB[tail_rob_tag].seq_no = ins[ins_no].seq_no;
            ROB[tail_rob_tag].rdy = 0;
            update_src_reg(ins_no, tail_rob_tag);
            update_dest_reg(ins_no, tail_rob_tag);
            if (ROB[tail_rob_tag].dest > -1) {
                update_RMT(ROB[tail_rob_tag].dest, ROB[tail_rob_tag].rob_tag);
            }
            rob_entry_counter ++;
            if (tail_rob_tag == (ROB_size - 1)) {
                tail_rob_tag = 0;
            }
            else {
                tail_rob_tag ++;
            }
        }

        int check_rename_size() {
            int size = 0;
            for (int i = 0; i < width; i++) {
                if (RN[i] > -1) {
                    size ++;
                }
            }
            return size;
        }

        int check_ROB_full() {
            if (rob_entry_counter > (ROB_size - check_rename_size())) {
                return 1;
            }
            else {
                return 0;
            }
        }
        
        void update_RMT(int reg_value, int rob_tag) {
            for (int i = 0; i < no_of_reg; i++) {
                if (RMT[i].reg_no == reg_value) {
                    RMT[i].valid = true;
                    RMT[i].rob_tag = rob_tag;
                }
            }
        }
        void update_src_reg(int ins_no, int rob_tag) {
            for (int i = 0; i < no_of_reg; i++) {
                if ((ins[ins_no].src1 == RMT[i].reg_no) && RMT[i].valid) {
                    ins[ins_no].src1_rob_valid = 1;
                    ins[ins_no].src1_rob = RMT[i].rob_tag;
                    for (int j = 0; j < ROB_size; j++) {
                        if (RMT[i].rob_tag == ROB[j].rob_tag) {
                            ins[ins_no].src1_rdy = ROB[j].rdy;
                        }
                    }
                }
                else if ((ins[ins_no].src2 == RMT[i].reg_no) && RMT[i].valid) {
                    // ins[ins_no].src2_rdy = 0;
                    ins[ins_no].src2_rob_valid = 1;
                    ins[ins_no].src2_rob = RMT[i].rob_tag;
                    for (int j = 0; j < ROB_size; j++) {
                        if (RMT[i].rob_tag == ROB[j].rob_tag) {
                            ins[ins_no].src2_rdy = ROB[j].rdy;
                        }
                    }
                }
            }
        }

        void update_dest_reg(int ins_no, int rob_tag) {
            ins[ins_no].dest_rob = rob_tag;
        }

        int check_RN_empty() {
            for (int i = 0; i < width; i++) {
                if (RN[i] > -1) {
                    return 0;
                }
            }
            return 1;
        }

        void advance_to_rename(int location) {
            if (DE[location] > -1) {
                RN[location] = DE[location];
                ins[DE[location]].RN_start = global_clock;
                DE[location] = -1;
            }
        }

        void fetch_into_decode(int fetch_value, int DE_location) {
            DE[DE_location] = ins[fetch_value].seq_no;
            ins[fetch_value].FE_start = global_clock - 1;
            ins[fetch_value].DE_start = global_clock;
        }

        int check_DE_empty() {
            for (int i = 0; i < width; i++) {
                if (DE[i] > -1) {
                    return 0;
                }
            }
            return 1;
        }

        void do_nothing() {
        }

    void print_output() {
        for (size_t i = 0; i < ins.size(); i++) {
            cout << ins[i].seq_no << " ";
            cout << "fu{" << ins[i].operation_type << "} ";
            cout << "src{" << ins[i].src1 << "," << ins[i].src2 << "} ";
            cout << "dst{" << ins[i].dest << "} ";
            cout << "FE{" << ins[i].FE_start << "," << ins[i].DE_start - ins[i].FE_start << "} ";
            cout << "DE{" << ins[i].DE_start << "," << ins[i].RN_start - ins[i].DE_start << "} ";
            cout << "RN{" << ins[i].RN_start << "," << ins[i].RR_start - ins[i].RN_start << "} ";
            cout << "RR{" << ins[i].RR_start << "," << ins[i].DI_start - ins[i].RR_start << "} ";
            cout << "DI{" << ins[i].DI_start << "," << ins[i].IS_start - ins[i].DI_start << "} ";
            cout << "IS{" << ins[i].IS_start << "," << ins[i].EX_start - ins[i].IS_start << "} ";
            cout << "EX{" << ins[i].EX_start << "," << ins[i].WB_start - ins[i].EX_start << "} ";
            cout << "WB{" << ins[i].WB_start << "," << ins[i].RT_start - ins[i].WB_start << "} ";
            cout << "RT{" << ins[i].RT_start << "," << ins[i].out_of_pl - ins[i].RT_start << "}" << endl;
        }
    }
};

#endif
