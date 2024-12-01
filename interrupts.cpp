#include "interrupts.hpp"

// Helper functions to initialize

// Initializes the memory partitions, smallest partition at last index
void initMemoryPartitions(MemoryPartition (&partitions)[MAX_PARTITIONS]) {
    int sizes[MAX_PARTITIONS] = {40, 25, 15, 10, 8, 2};
    for (int i = 0; i < MAX_PARTITIONS; i++) {
        partitions[i].partitionNumber = i + 1;
        partitions[i].size = sizes[i];
        partitions[i].OccupiedBy = -1;  // initialize all partitions to free
    }
}

void initializeMemoryStatus() {
    if (!memoryStatus) {
        memoryStatus = new MemoryStatus;
        memoryStatus->totalFreeMemory = 0;
        memoryStatus->usableFreeMemory = 0;

        // Calculate initial free memory
        for (int i = 0; i < MAX_PARTITIONS; i++) {
            memoryStatus->totalFreeMemory += memoryPartitions[i].size;
            memoryStatus->usableFreeMemory += memoryPartitions[i].size;
        }
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
        static_cast<unsigned int>(PCBinfo[2]),  // Arrival Time
        static_cast<unsigned int>(PCBinfo[3]),  // Total CPU Time
        static_cast<unsigned int>(PCBinfo[3]),  // Remaining CPU Time (initialize to Total CPU Time)
        static_cast<unsigned int>(PCBinfo[4]),  // I/O Frequency
        static_cast<unsigned int>(PCBinfo[5]),  // I/O Duration
        static_cast<unsigned int>(PCBinfo[5]),  // Remaining I/O Duration
        -1,                                     // Place in memory partition (initialize to -1)
        NEW,                                    // Current State (initialize to NEW)
        NEW,                                    // Previous State (initialize to NEW)
        static_cast<int>(PCBinfo[1]),           // Memory Size
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

// Helper functions for algorithms
int findBestPartition(const int& memorySize){

    for (int i = MAX_PARTITIONS - 1; i >= 0; i--){
        MemoryPartition partition = memoryPartitions[i];
        if(partition.OccupiedBy == -1 && memorySize <= partition.size) return i + 1;
    }
    return -1;
}

void updateMemoryStatus(int partitionIndex, const PCB* pcb, MemoryAction action) {
    if (!memoryStatus) return;  // Ensure memoryStatus is initialized

    switch (action){
        case ALLOCATE:
            // Allocate the partition
            memoryStatus->totalFreeMemory -= pcb->memorySize;
            memoryStatus->usableFreeMemory -= memoryPartitions[partitionIndex].size;
            memoryPartitions[partitionIndex].OccupiedBy = pcb->pid;        
            break;

        case FREE:
            // Free the partition
            memoryStatus->totalFreeMemory += pcb->memorySize;
            memoryStatus->usableFreeMemory += memoryPartitions[partitionIndex].size;
            memoryPartitions[partitionIndex].OccupiedBy = -1;
            break;
    }
}

void displayMemoryStatus(int currentTime, ofstream& memoryStatusFile) {
    static bool headerWritten = false;

    // Write header for the table once
    if (!headerWritten) {
        memoryStatusFile << "+-------------------------------------------------------------------------------------------------------------+\n";
        memoryStatusFile << "| Time of Event |  Memory Used    |         Partition State          | Total Free Memory | Usable Free Memory |\n";
        memoryStatusFile << "+-------------------------------------------------------------------------------------------------------------+\n";
        headerWritten = true;
    }

    // Data Row
    memoryStatusFile << "| " << setw(13) << currentTime << " | ";
    memoryStatusFile << setw(13) << 100 - memoryStatus->totalFreeMemory  << "   | ";

    // Display memory usage for each partition
    for (int i = 0; i < MAX_PARTITIONS; i++) {
        memoryStatusFile << setw(3) << memoryPartitions[i].OccupiedBy;
        if (i < MAX_PARTITIONS - 1) memoryStatusFile << ", ";
    }

    // Add padding to align Partition State column
    memoryStatusFile << setw(32 - (4 * MAX_PARTITIONS)) << " | ";

    // Append total free memory and usable free memory
    memoryStatusFile << setw(10) << memoryStatus->totalFreeMemory << setw(10) << " | "
                     << setw(10) << memoryStatus->usableFreeMemory << setw(10) << "   |\n";
}

void transitionState(PCB* process, ProcessState newState, const int currentTime, ofstream& executionFile) {
    static bool headerWritten = false;

    // Write the table header once
    if (!headerWritten) {
        executionFile << "+-------------------------------------------------------------+\n";
        executionFile << "|    Time (ms)     | PID    |  Old State     |   New State    |\n";
        executionFile << "+-------------------------------------------------------------+\n";
        headerWritten = true;
    }

    // Write the transition data in table format
    executionFile << "| " << setw(16) << currentTime
                  << " | " << setw(6) << process->pid
                  << " | " << setw(14) << processStateToString(process->currentState)
                  << " | " << setw(14) << processStateToString(newState)
                  << " |\n";

    process->currentState = newState;
}

string processStateToString(const ProcessState state){
    switch (state)
    {
        case NEW: return "NEW";
        case READY: return "READY";
        case RUNNING: return "RUNNING";
        case WAITING: return "WAITING";
        case TERMINATED: return "TERMINATED";
        default: return "UNKOWN";
    }
}

// Appends a PCB to the tail of the queue, removing it from its current queue if necessary
void appendPCB(PCB* pcb, HeadTailPCB& fromQueue, HeadTailPCB& toQueue) {
    if (!pcb) return;  // If PCB is null, no operation is needed

    // --- Removing PCB from `fromQueue` (if applicable) ---

    // Case 1: PCB is the only element in the `fromQueue`
    if (fromQueue.head == pcb && fromQueue.tail == pcb) {
        fromQueue.head = nullptr;
        fromQueue.tail = nullptr;
    }
    // Case 2: PCB is the head of the `fromQueue` (but not the only element)
    else if (fromQueue.head == pcb) {
        fromQueue.head = pcb->next;
        if (fromQueue.head) {
            fromQueue.head->prev = nullptr;  // Update the new head's prev pointer
        }
    }
    // Case 3: PCB is the tail of the `fromQueue` (but not the only element)
    else if (fromQueue.tail == pcb) {
        fromQueue.tail = pcb->prev;
        if (fromQueue.tail) {
            fromQueue.tail->next = nullptr;  // Update the new tail's next pointer
        }
    }
    // Case 4: PCB is in the middle of the `fromQueue`
    else {
        if (pcb->prev) {
            pcb->prev->next = pcb->next;  // Link the previous node to the next node
        }
        if (pcb->next) {
            pcb->next->prev = pcb->prev;  // Link the next node to the previous node
        }
    }

    // Update the `fromQueue` process count
    fromQueue.processCount--;

    // Reset PCB pointers (disconnect it from the `fromQueue`)
    pcb->next = nullptr;
    pcb->prev = nullptr;

    // --- Adding PCB to `toQueue` ---

    // Case 1: `toQueue` is empty
    if (toQueue.head == nullptr && toQueue.tail == nullptr) {
        toQueue.head = pcb;
        toQueue.tail = pcb;
    }
    // Case 2: `toQueue` is not empty
    else {
        pcb->prev = toQueue.tail;  // Point PCB's prev to the current tail
        if (toQueue.tail) {
            toQueue.tail->next = pcb;  // Link current tail's next to PCB
        }
        toQueue.tail = pcb;  // Update the tail pointer to the new PCB
    }

    // Ensure PCB is properly terminated at the tail
    pcb->next = nullptr;

    // Update the `toQueue` process count
    toQueue.processCount++;
}

// Pops the PCB at the head of the queue, handling all edge cases for linked list management
void popPCB(PCB* pcb, HeadTailPCB& queue) {
    if (!pcb || !queue.head) return;  // If PCB or queue is null/empty, no operation is needed

    // --- Removing PCB from `queue` ---

    // Case 1: PCB is the only element in the queue
    if (queue.head == pcb && queue.tail == pcb) {
        queue.head = nullptr;
        queue.tail = nullptr;
    }
    // Case 2: PCB is the head of the queue (but not the only element)
    else if (queue.head == pcb) {
        queue.head = pcb->next;  // Update the head pointer to the next PCB
        if (queue.head) {
            queue.head->prev = nullptr;  // Update the new head's prev pointer
        }
    }
    // No need for other cases because FCFS ensures we're always popping the head

    // Update the `queue` process count
    queue.processCount--;

    // Free the memory of the PCB
    delete pcb;
}



void debugPrintPCBList(const HeadTailPCB& list) {
    PCB* current = list.head;  // Start from the head of the list
    cout << "+--------------------------------------------------------------------------+" << endl;
    cout << "| PID   | Arrival | Total CPU | Remaining CPU | I/O Freq | I/O Dur | MemSize |" << endl;
    cout << "+--------------------------------------------------------------------------+" << endl;

    while (current) {
        cout << "| " << setw(6) << current->pid
             << " | " << setw(7) << current->arrivalTime
             << " | " << setw(9) << current->totalCPUTime
             << " | " << setw(13) << current->remainingCPUTime
             << " | " << setw(8) << current->ioFrequency
             << " | " << setw(7) << current->ioDuration
             << " | " << setw(7) << current->memorySize
             << " |" << endl;

        current = current->next;  // Move to the next PCB in the list
    }

    cout << "+--------------------------------------------------------------------------+" << endl;
    cout << "Total processes: " << list.processCount << endl;
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



// Algorithms
void runFCFSScheduler(HeadTailPCB& newQueue, ofstream& executionFile, ofstream& memoryStatusFile) {

    long currentTime = 0;
    
    HeadTailPCB waitingQueue, readyQueue;   // WAITING & READY 
    initializeHeadTail(waitingQueue);
    initializeHeadTail(readyQueue);
    displayMemoryStatus(currentTime, memoryStatusFile);


    // exists a process in all queues that is yet to be executed (execute if any of them are true)
    while (newQueue.head || waitingQueue.head || readyQueue.head) {     

        // NEW -> READY
        while (newQueue.head && newQueue.head->arrivalTime == currentTime){
            PCB* nextTemp = newQueue.head->next; // Save the next element before modifyiing list moving pcb from NEW to READY

            int assignedPartition = findBestPartition(newQueue.head->memorySize);
            if (assignedPartition != -1){
                newQueue.head->partition = assignedPartition; // Assign PID its partition 
                updateMemoryStatus(assignedPartition - 1, newQueue.head, ALLOCATE);
                displayMemoryStatus(currentTime, memoryStatusFile);
                transitionState(newQueue.head, READY, currentTime, executionFile);
                appendPCB(newQueue.head, newQueue, readyQueue);
                
            } else{
                break; // wasn't able to allocate memory
            }
            newQueue.head = nextTemp;
        }

        // WAITING -> READY
        PCB* waitTemp = waitingQueue.head; // since the head of WAITING queue can chane each cycle
        while (waitTemp) {
            PCB* nextTemp = waitTemp->next; // Saving next pointer to pcb in waiting queue
            waitingQueue.head->ioDuration--;  // Decrement remaining I/O duration

            if (waitingQueue.head->ioDuration == 0) {
                // Reset I/O duration for future bursts
                waitingQueue.head->ioDuration = waitingQueue.head->initialIODuration;

                transitionState(waitingQueue.head, READY, currentTime, executionFile);
                appendPCB(waitingQueue.head, waitingQueue, readyQueue);  
            }
            waitTemp = nextTemp;
        }

        // Increment waitTime for all processes in readyQueue except the first (runningProcess)
        PCB* readyTemp = readyQueue.head;

        // Skip the first PCB in the readyQueue (runningProcess)
        if (readyTemp) { readyTemp = readyTemp->next; }

        // Increment waitTime fot all remaining PCBs in the readyQueue
        while (readyTemp){
                readyTemp->waitTime++;
                readyTemp = readyTemp->next;
            }        
    
        
        // READY -> RUNNING (execution)
        PCB* runningProcess = readyQueue.head;
        if (runningProcess) {
            PCB* nextRunning = runningProcess->next;

            // Transition to RUNNING
            transitionState(runningProcess, RUNNING, currentTime, executionFile);

            // Update response time if this is the first time the process runs
            if (runningProcess->responseTime == 0) {
                runningProcess->responseTime = currentTime - runningProcess->arrivalTime;
            }
            
            // Decrement remaining CPU time
            runningProcess->remainingCPUTime--;

            // Check for process completion: RUNNING -> TERMINATED
            if (runningProcess->remainingCPUTime == 0){
                transitionState(runningProcess, TERMINATED, currentTime, executionFile);
                updateMemoryStatus(runningProcess->partition -1, runningProcess, FREE);
                displayMemoryStatus(currentTime, memoryStatusFile);
                runningProcess->turnaroundTime = currentTime - runningProcess->arrivalTime;
                popPCB(runningProcess, readyQueue); // removes PCB from the linked list and free's it
            }
            // IO Burst: RUNNING -> WAITING
            else if((runningProcess->totalCPUTime - runningProcess->remainingCPUTime) % runningProcess->ioFrequency == 0){
                transitionState(runningProcess, WAITING, currentTime, executionFile);
                appendPCB(runningProcess, readyQueue, waitingQueue); // In FCFS I alwyas execut the beginning of the ready queue
            }
        }
        currentTime++;
    }
}

// Main function to load files and process the trace
int main(int argc, char* argv[]) {

    // File is passed
    if (argc < 3){
        cerr << "Usage: " << argv[0] << " <input_file> <algorithm>" << endl;
        return 0;
    }

    // Opening File
    ifstream inputFile(argv[1]);
    if (!inputFile){
        cerr << "Error openning file" << argv[1] << endl;
        return 0;
    }

    // Schedular to run
    string algorithm = argv[2];

    // Initialize Memory
    initMemoryPartitions(memoryPartitions);
    initializeMemoryStatus();

    // Load Data in Double Linked list
    string trace;
    HeadTailPCB list;
    initializeHeadTail(list);
    loadTrace(list, inputFile, trace);

    // Create files
    ofstream output_execution_file("execution.txt");
    if(!output_execution_file){
        cerr << "Error generating execution.txt file" << endl;
        return 1;
    }
    ofstream output_memory_status("memory_status.txt");
    if(!output_memory_status){
        cerr << "Error generating memory_status.txt file" << endl;
        return 1;
    }

    debugPrintPCBList(list);

    // Run the selected scheduler
    if (algorithm == "FCFS") {
        runFCFSScheduler(list, output_execution_file, output_memory_status);
    } else if (algorithm == "PR") {
        cerr << "Priority scheduling (PR) not implemented yet." << endl;
    } else if (algorithm == "RR") {
        cerr << "Round Robin scheduling (RR) not implemented yet." << endl;
    } else {
        cerr << "Unknown scheduling algorithm: " << algorithm << endl;
        return 1;
    }


    output_memory_status << "+-------------------------------------------------------------------------------------------------------------+";
    output_execution_file << "+--------------------------------------------------------------+";


    // Close files
    output_execution_file.close();
    output_memory_status.close();
    inputFile.close();

    // Delete allocated memory
    // freePCB(list);
    delete memoryStatus;

    return 1;
}



