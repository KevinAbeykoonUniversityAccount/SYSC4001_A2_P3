/**
 *
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 *
 */

#include "interrupts_101301971_101264747.hpp"

std::tuple<std::string, std::string, int> simulate_trace(
    std::vector<std::string> trace_file,
    int time,
    std::vector<std::string> vectors,
    std::vector<int> delays,
    std::vector<external_file> external_files,
    PCB current,
    std::vector<PCB> &wait_queue  // pass by reference
) {

    std::string trace;      //!< string to store single line of trace file
    std::string execution = "";  //!< string to accumulate the execution output
    std::string system_status = "";  //!< string to accumulate the system status output
    int current_time = time;

    //parse each line of the input trace file. 'for' loop to keep track of indices.
    for(size_t i = 0; i < trace_file.size(); i++) {
        auto trace = trace_file[i];

        auto [activity, duration_intr, program_name] = parse_trace(trace);

        if(activity == "CPU") { //As per Assignment 1
            execution += std::to_string(current_time) + ", " + std::to_string(duration_intr) + ", CPU Burst\n";
            current_time += duration_intr;
        } else if(activity == "SYSCALL") { //As per Assignment 1
            auto [intr, time] = intr_boilerplate(current_time, duration_intr, 10, vectors);
            execution += intr;
            current_time = time;

            execution += std::to_string(current_time) + ", " + std::to_string(delays[duration_intr]) + ", SYSCALL ISR\n";
            current_time += delays[duration_intr];

            execution +=  std::to_string(current_time) + ", 1, IRET\n";
            current_time += 1;
        } else if(activity == "END_IO") {
            auto [intr, time] = intr_boilerplate(current_time, duration_intr, 10, vectors);
            current_time = time;
            execution += intr;

            execution += std::to_string(current_time) + ", " + std::to_string(delays[duration_intr]) + ", ENDIO ISR\n";
            current_time += delays[duration_intr];

            execution +=  std::to_string(current_time) + ", 1, IRET\n";
            current_time += 1;

        } else if(activity == "FORK") {
            auto [intr, time] = intr_boilerplate(current_time, 2, 10, vectors);
            execution += intr;
            current_time = time;

            ///////////////////////////////////////////////////////////////////////////////////////////
            //Add your FORK output here

            // Clone the parent's PCB to create a child PCB
            static unsigned int next_pid = 1;
            PCB child = current;
            child.PID = next_pid++;
            child.PPID = current.PID;

            // Assign partition to child
            int child_partition = find_partition(child.size); 
            child.partition_number = child_partition;
            allocate_memory(&child);
            PCB parent = current;

            // add parent to wait queue
            wait_queue.push_back(parent);
            current = child;

            system_status += "time: " + std::to_string(current_time) + "; current trace: FORK, "+ std::to_string(duration_intr) + "\n";
            system_status += print_PCB(current, wait_queue);

            execution += std::to_string(current_time) + ", " + std::to_string(duration_intr) + ", cloning the PCB\n";
            current_time += duration_intr; 

            execution += std::to_string(current_time) + ", 0, scheduler called\n";
            execution += std::to_string(current_time) + ", 1, IRET\n";
            current_time += 1;

            ///////////////////////////////////////////////////////////////////////////////////////////
            
            //The following loop helps you do 2 things:
            // * Collect the trace of the chile (and only the child, skip parent)
            // * Get the index of where the parent is supposed to start executing from
            std::vector<std::string> child_trace;
            bool skip = true;
            bool exec_flag = false;
            int parent_index = 0;

            for(size_t j = i; j < trace_file.size(); j++) {
                auto [_activity, _duration, _pn] = parse_trace(trace_file[j]);
                if(skip && _activity == "IF_CHILD") {
                    skip = false;
                    continue;
                } else if(_activity == "IF_PARENT"){
                    skip = true;
                    parent_index = j;
                    if(exec_flag) {
                        break;
                    }
                } else if(skip && _activity == "ENDIF") {
                    skip = false;
                    continue;
                } else if(!skip && _activity == "EXEC") {
                    skip = true;
                    child_trace.push_back(trace_file[j]);
                    exec_flag = true;
                }

                if(!skip) {
                    child_trace.push_back(trace_file[j]);
                }
            }
            i = parent_index;

            ///////////////////////////////////////////////////////////////////////////////////////////
            //With the child's trace, run the child (HINT: think recursion)

            auto [child_execution, child_status, child_time] = simulate_trace(
                child_trace, 
                current_time, 
                vectors, 
                delays,
                external_files, 
                child, 
                wait_queue
            );

            execution += child_execution;
            system_status += child_status;
            current_time = child_time;
            current = parent;

            ///////////////////////////////////////////////////////////////////////////////////////////

        } else if(activity == "EXEC") {
            auto [intr, time] = intr_boilerplate(current_time, 3, 10, vectors);
            current_time = time;
            execution += intr;

            ///////////////////////////////////////////////////////////////////////////////////////////
            //Add your EXEC output here
            int program_size = 0;
            for (auto &file : external_files) {
                if (file.program_name == program_name) {
                    program_size = file.size;
                    break;
                }
            }

            //find empty partition to fit program
            int partition_number = find_partition(program_size);
            current.partition_number = partition_number;

            int loader_time = program_size * 15; // 15 ms * 1 MB
            execution += std::to_string(current_time) + ", " + std::to_string(duration_intr) + ", Program is " + std::to_string(program_size) + " Mb large\n";
            current_time += duration_intr;

            execution += std::to_string(current_time) + ", " + std::to_string(loader_time) + ", loading program into memory\n";
            current_time += loader_time;

            execution += std::to_string(current_time) + ", 3, marking partition as occupied\n";
            current_time += 3;

            current.program_name = program_name;
            current.size = program_size;
            allocate_memory(&current); 
            execution += std::to_string(current_time) + ", 6, updating PCB\n";
            current_time += 6;

            execution += std::to_string(current_time) + ", 0, scheduler called\n";
            execution += std::to_string(current_time) + ", 1, IRET\n";
            current_time += 1;

            // Log system status snapshot
            system_status += "time: " + std::to_string(current_time) + "; current trace: EXEC " + current.program_name + "\n";
            system_status += print_PCB(current, wait_queue);
            ///////////////////////////////////////////////////////////////////////////////////////////

            std::ifstream exec_trace_file(program_name + ".txt");

            std::vector<std::string> exec_traces;
            std::string exec_trace;
            while(std::getline(exec_trace_file, exec_trace)) {
                exec_traces.push_back(exec_trace);
            }

            ///////////////////////////////////////////////////////////////////////////////////////////
            //With the exec's trace (i.e. trace of external program), run the exec (HINT: think recursion)

            auto [exec_execution, exec_status, exec_time] = simulate_trace(
                exec_traces, 
                current_time, 
                vectors, 
                delays, 
                external_files, 
                current,   // the current PCB is running this exec wait_queue 
                wait_queue
            );

            execution += exec_execution; 
            system_status += exec_status; 
            current_time = exec_time;

            ///////////////////////////////////////////////////////////////////////////////////////////

            break; //Why is this important? (answer in report)
        }
    }

    return {execution, system_status, current_time};
}

// Finds the smallest free partition that can fit a program of given size
// Returns the partition index in the memory array, or -1 if no suitable partition exists
int find_partition(unsigned int program_size) {
    int best_index = -1;
    unsigned int best_size = UINT_MAX; // start with "infinite" size

    for (int i = 0; i < 6; i++) {
        if (memory[i].code == "empty" && memory[i].size >= program_size) {
            if (memory[i].size < best_size) {
                best_size = memory[i].size;
                best_index = i;
            }
        }
    }

    return best_index;
}

int main(int argc, char** argv) {

    //vectors is a C++ std::vector of strings that contain the address of the ISR
    //delays  is a C++ std::vector of ints that contain the delays of each device
    //the index of these elemens is the device number, starting from 0
    //external_files is a C++ std::vector of the struct 'external_file'. Check the struct in 
    //interrupt.hpp to know more.
    auto [vectors, delays, external_files] = parse_args(argc, argv);
    std::ifstream input_file(argv[1]);

    //Just a sanity check to know what files you have
    print_external_files(external_files);

    //Make initial PCB (notice how partition is not assigned yet)
    PCB current(0, -1, "init", 1, -1);
    //Update memory (partition is assigned here, you must implement this function)
    if(!allocate_memory(&current)) {
        std::cerr << "ERROR! Memory allocation failed!" << std::endl;
    }

    std::vector<PCB> wait_queue;

    /******************ADD YOUR VARIABLES HERE*************************/


    /******************************************************************/

    //Converting the trace file into a vector of strings.
    std::vector<std::string> trace_file;
    std::string trace;
    while(std::getline(input_file, trace)) {
        trace_file.push_back(trace);
    }

    auto [execution, system_status, _] = simulate_trace(   trace_file, 
                                            0, 
                                            vectors, 
                                            delays,
                                            external_files, 
                                            current, 
                                            wait_queue);

    input_file.close();

    write_output(execution, "execution.txt");
    write_output(system_status, "system_status.txt");

    return 0;
}
