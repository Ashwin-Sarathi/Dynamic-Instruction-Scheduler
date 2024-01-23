#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "sim_proc.h"

void measurements(int ROB_size, int IQ_size, int width, string trace_file_name, int clock_cycles) {
    float temp;
    temp = static_cast<float>(10000) / static_cast<float>(clock_cycles);
    cout << "# === Simulator Command =========" << endl;
    cout << "# ./sim " << ROB_size << " " << IQ_size << " " << width << " " << trace_file_name << endl;
    cout << "# === Processor Configuration ===" << endl;
    cout << "# ROB_SIZE = " << ROB_size << endl;
    cout << "# IQ_SIZE = " << IQ_size << endl;
    cout << "# WIDTH = " << width << endl;
    cout << "# === Simulation Results ========" << endl;
    cout << left << setw(30) << "# Dynamic Instruction Count" << "= 10000" << endl;
    cout << left << setw(30) << "# Cycles" << "= " << clock_cycles << endl;
    cout << left << setw(30) << "# Instructions Per Cycle (IPC)" << "= " << fixed << setprecision(2) << temp << endl;
}

/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim 256 32 4 gcc_trace.txt
    argc = 5
    argv[0] = "sim"
    argv[1] = "256"
    argv[2] = "32"
    ... and so on
*/
int main (int argc, char* argv[])
{
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    proc_params params;       // look at sim_bp.h header file for the the definition of struct proc_params
    //int op_type, dest, src1, src2;  // Variables are read from trace file
    //uint64_t pc; // Variable holds the pc read from input file
    
    if (argc != 5)
    {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }
    
    params.rob_size     = strtoul(argv[1], NULL, 10);
    params.iq_size      = strtoul(argv[2], NULL, 10);
    params.width        = strtoul(argv[3], NULL, 10);
    trace_file          = argv[4];

    Dynamic_Scheduler my_ooo(params.rob_size, params.iq_size, params.width);

    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }
    
    fstream new_file;
    new_file.open(trace_file, ios::in);
    if (new_file.is_open()) {
        string trace;
        int op_count = 0;

        while (getline(new_file, trace)) {
            trace = to_string(op_count) + " " + trace;
            my_ooo.trace_parse(trace);
            my_ooo.add_to_ins();
            op_count ++;
        }
        my_ooo.execute_pipeline();
        my_ooo.print_output();
        measurements(params.rob_size, params.iq_size, params.width, trace_file, my_ooo.final_clock);        
    }
    return 0;
}
