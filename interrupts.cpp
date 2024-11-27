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
        static_cast<unsigned int>(PCBinfo[1]),  // Arrival Time
        static_cast<unsigned int>(PCBinfo[2]),  // Total CPU Time
        static_cast<unsigned int>(PCBinfo[2]),  // Remaining CPU Time (initialize to Total CPU Time)
        static_cast<unsigned int>(PCBinfo[3]),  // I/O Frequency
        static_cast<unsigned int>(PCBinfo[4]),  // I/O Duration
        static_cast<unsigned int>(PCBinfo[4]),  // Remaining I/O Duration
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


// Helper functions for algorithms
int findBestPartition(const int& memorySize){

    for (int i = MAX_PARTITIONS - 1; i < 0; i--){
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

    // Write header for table once
        if (!headerWritten){
            memoryStatusFile << "+------------------------------------------------------------------------------------+\n";
            memoryStatusFile << "| Time of Event | Memory Used | Partition State | Total Free Memory | Usable Free Mem|    ory    |\n";
            memoryStatusFile << "+------------------------------------------------------------------------------------+\n";
    }
    
    // Data Row
    memoryStatusFile << "| " << setw(16) << currentTime;
                    
    for (int i = 0; i < MAX_PARTITIONS; i++) {
        memoryStatusFile << setw(2) << memoryPartitions[i].OccupiedBy;
        if (i < MAX_PARTITIONS - 1) memoryStatusFile << ", ";
    }
    memoryStatusFile << " | " << setw(14) << memoryStatus->totalFreeMemory
                     << " | " << setw(14) << memoryStatus->usableFreeMemory
                     << " |\n"
                     << "+----------------------------------------------------------------------------+\n";
}

void transitionState(PCB* process, ProcessState newState, const int currentTime, ofstream& executionFile) {
    static bool headerWritten = false;

    // Write the table header once
    if (!headerWritten) {
        executionFile << "+------------------+--------+----------------+----------------+\n";
        executionFile << "| Time (ms)       | PID    | Old State      | New State      |\n";
        executionFile << "+------------------+--------+----------------+----------------+\n";
        headerWritten = true;
    }

    // Write the transition data in table format
    executionFile << "| " << setw(16) << currentTime
                  << " | " << setw(6) << process->pid
                  << " | " << setw(14) << process->currentState
                  << " | " << setw(14) << newState
                  << " |\n";

    // Update the PCB's state
    process->prevState = process->currentState;
    process->currentState = newState;
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

// Algorithms
void runFCFSScheduler(HeadTailPCB& newQueue, ofstream& executionFile, ofstream& memoryStatusFile) {

    HeadTailPCB waitingQueue, readyQueue;   // WAITING & READY 

    initializeHeadTail(waitingQueue);
    initializeHeadTail(readyQueue);

    long currentTime = 0;                    // current time


    // exists a process in all queues that is yet to be executed
    while (newQueue.head && waitingQueue.head && readyQueue.head) {     

        // NEW -> READY
        PCB* newTemp = newQueue.head;
        while (newTemp && newTemp->arrivalTime == currentTime){
            int partitionIndex = findBestPartition(newTemp->memorySize);
            if (partitionIndex != -1){
                
                updateMemoryStatus(partitionIndex, newTemp, ALLOCATE);
                displayMemoryStatus(currentTime, memoryStatusFile);
                newTemp->partition = partitionIndex++; // Assign PID its partition 
                transitionState(newTemp, READY, currentTime, executionFile);
                appendPCB(newTemp, newQueue, readyQueue);
            } else{
                break; // wasn't able to allocate memory
            }
            newTemp = newTemp->next;
        }

        // WAITING -> READY
        PCB* waitTemp = waitingQueue.head;
        if (waitTemp) {
            waitTemp->ioDuration--;  // Decrement remaining I/O duration
            if (waitTemp->ioDuration == 0) {
                // Reset I/O duration for future bursts
                waitTemp->ioDuration = waitTemp->initialIODuration;

                // Transition to READY
                transitionState(waitTemp, READY, currentTime, executionFile);
                appendPCB(waitTemp, waitingQueue, readyQueue);
            }
        }

        // Increment waitTime for all processes in ready Queue
        PCB* readyTemp = readyQueue.head;
        bool isFirst = true;

        while (readyTemp){
            if (isFirst) readyTemp = readyTemp->next;
            
            isFirst = false;
            readyTemp->waitTime++;           
        }
        
        // READY -> RUNNING (execution)
        PCB* runningProcess = readyQueue.head;
        if (runningProcess) {
            // Transition to RUNNING
            transitionState(runningProcess, RUNNING, currentTime, executionFile);

            // Update response time if this is the first time the process runs
            if (runningProcess->responseTime == 0) {
                runningProcess->responseTime = currentTime - runningProcess->arrivalTime;
            }
            
            // Decrement remaining CPU time
            runningProcess->remainingCPUTime--;

            // Check for process completion: RUNNING -> TERMINATED
            if (runningProcess->remainingCPUTime == 0 && runningProcess->ioDuration == 0){
                transitionState(runningProcess, TERMINATED, currentTime,executionFile);
                updateMemoryStatus(runningProcess->partition -1, runningProcess, FREE);
                displayMemoryStatus(currentTime, memoryStatusFile);
                runningProcess->turnaroundTime = currentTime - runningProcess->arrivalTime;
                popPCB(runningProcess, readyQueue); // removes PCB from the linked list and free's it
            }
            // IO Burst: RUNNING -> WAITING
            else if((runningProcess->totalCPUTime - runningProcess->remainingCPUTime) % 2 == 0){
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
    if (argc < 2){
        cerr << "Error loading input file" << endl;
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

    // Run FCFS
    runFCFSScheduler(list, output_execution_file, output_memory_status);

    // Close files
    output_execution_file.close();
    output_memory_status.close();
    inputFile.close();

    // Delete allocated memory
    // freePCB(list);
    delete memoryStatus;

    return 0;
}



