#include "interrupts.hpp"

// Initializes the memory partitions, smallest partition at last index
void initMemoryPartitions(MemoryPartition (&partitions)[MAX_PARTITIONS]) {
    int sizes[MAX_PARTITIONS] = {40, 25, 15, 10, 8, 2};
    for (int i = 0; i < MAX_PARTITIONS; i++) {
        partitions[i].partitionNumber = i + 1;
        partitions[i].size = sizes[i];
        partitions[i].OccupiedBy = -1;  // initialize all partitions to free
    }
}

void initializeHeadTail(HeadTailPCB& list) {
    list.head = nullptr;
    list.tail = nullptr;
    list.processCount = 0;
}

vector<int> splitToNumbers (const string& str, char delim){

    vector<int> result; // store the result in here
    string token;                       // Temporarily stores token (part of string) as split by delimiter
    std::istringstream stream(str);         // Create a stream of input data to read from one piece at a time

    while (getline(stream, token, delim)){ // reads token from stream until reaching delim
        result.push_back(std::stoi(token));
    }     
    return result;
}

void loadTrace(HeadTailPCB& list, ifstream& inputFile, string& line){

    while(getline(inputFile, line)){
        vector<int> pcbRow = splitToNumbers(line, ',');        // Parse data assign to vector
        addPCBNode(list, pcbRow);                       // Create newPCB with data
    }
}

void addPCBNode(HeadTailPCB& list, const vector<int>& PCBinfo){

    // create PCB
    PCB* newPCB = new PCB{
        static_cast<unsigned int>(PCBinfo[0]),  // PID
        static_cast<unsigned int>(PCBinfo[1]),  // Arrival Time
        static_cast<unsigned int>(PCBinfo[2]),  // Total CPU Time
        static_cast<unsigned int>(PCBinfo[2]),  // Remaining CPU Time (initialize to Total CPU Time)
        static_cast<unsigned int>(PCBinfo[3]),  // I/O Frequency
        static_cast<unsigned int>(PCBinfo[4]),  // I/O Duration
        -1,                                     // Place in memory partition (initialize to -1)
        NEW,                                    // Current State (initialize to NEW)
        NEW,                                    // Previous State (initialize to NEW)
        static_cast<int>(PCBinfo[5]),           // Memory Size
        0,                                      // Wait Time (initialize to 0)
        0,                                      // Turnaround Time (initialize to 0)
        0,                                      // Response Time (initialize to 0)
        nullptr,                                // Next pointer
        nullptr                                 // Prev pointer
    };

    // Case 1: List is empty
    if (list.head == nullptr){
        list.head = newPCB;
        list.tail = newPCB;
    }
    // Case 2: List is not empty
    else{
        newPCB->prev = list.tail;  // Link the new node's prev to the current tail
        list.tail->next = newPCB;  // Link the current tail's next to the new node
        list.tail = newPCB;        // Update the tail pointer to the new node
    }
    list.processCount++;
}

void freePCB(HeadTailPCB& list){

    PCB* current = list.head;
    while (current) {
        PCB* temp = current;  // Store the current node
        current = current->next;  // Move to the next node
        delete(temp);  // Free the memory of the current node
    }

    // Reset the head, tail, and process count
    list.head = nullptr;
    list.tail = nullptr;
    list.processCount = 0;
}

void printPCB(const HeadTailPCB& list, ofstream& file){

    for (PCB* temp = list.head; temp != nullptr; temp = temp->next){
        
        file << "PID: " << temp->pid
             << ", Arrival Time: " << temp->arrivalTime
             << ", Total CPU Time: " << temp->totalCPUTime
             << ", Remaining CPU Time: " << temp->remainingCPUTime
             << ", I/O Frequency: " << temp->ioFrequency
             << ", I/O Duration: " << temp->ioDuration
             << ", Memory Size: " << temp->memorySize
             << ", Current State: " << temp->currentState
             << ", Previous State: " << temp->prevState
             << "\n";    }
}

// Main function to load files and process the trace
int main(int argc, char* argv[]) {

    // File is passed
    if (argc < 1){
        cerr << "Error loading input file" << endl;
        return 0;
    }

    // Opening File
    ifstream inputFile(argv[1]);
    if (!inputFile){
        cerr << "Error openning file" << argv[1] << endl;
        return 0;
    }

    // Initialize Memory
    initMemoryPartitions(memoryPartitions);

    // Load Data in Double Linked list
    string trace;
    int current_time;
    HeadTailPCB list;
    initializeHeadTail(list);
    loadTrace(list, inputFile, trace);

    // Create files
    ofstream output_execution_file("execution.txt");
    if(!output_execution_file){
        cerr << "Error generating execution.txt file" << endl;
        return 1;
    }
    // ofstream output_memory_status("memory_status.txt");

    printPCB(list, output_execution_file);

    // Close files
    output_execution_file.close();
    inputFile.close();

    // Delete allocated memory
    freePCB(list);

    return 0;
}



