// Zyaire Bush 
// Nematode Peoject 
//Procuder.cpp 
#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <cstring>
#include <csignal>


// Struct for shared memory table
struct SharedTable {
    int items[2];      // Table can hold two items
    int count;         // Number of items currently on the table
    int totalProduced; // Counter for total items produced
    int maxItems;      // Maximum number of items to produce
    bool producerDone; // Flag to indicate producer is done
};



// Global variables
SharedTable* sharedTable;
sem_t* mutex;          // For mutual exclusion when accessing the table
sem_t* empty;          // Counts empty spaces in the table
sem_t* full;           // Counts filled spaces in the table
int shouldContinue = 1;

// Signal handler toend
void signalHandler(int signum) {
    std::cout << "Producer: Received signal " << signum << ", terminating..." << std::endl;
    shouldContinue = 0;
}


// Function to create a random item
int produceItem() {
    return rand() % 100 + 1;  // Random number between 1 and 100
}



// funct to add item to the table
void addItemToTable(int item) {
    if (sharedTable->count < 2) {
        sharedTable->items[sharedTable->count] = item;
        sharedTable->count++;
        sharedTable->totalProduced++;
        std::cout << "Producer: Added item " << item << " to the table. Table count: " << sharedTable->count 
                  << " (Total produced: " << sharedTable->totalProduced << "/" << sharedTable->maxItems << ")" << std::endl;
    } else {
        std::cerr << "Producer: Table is full!!!" << std::endl;
    }
}


// Producer thread funct
void* producer(void* arg) {
    while (shouldContinue && sharedTable->totalProduced < sharedTable->maxItems) {
        // Produce item
        int item = produceItem();
        
        // Wait for empty slot
        sem_wait(empty);
        
        // Get crit access to the table
        sem_wait(mutex);
        
        // Check again if should still produce
        if (shouldContinue && sharedTable->totalProduced < sharedTable->maxItems) {
            // Critical sect Add item to table
            addItemToTable(item);
        }
        
        // let go the mutex
        sem_post(mutex);

        
        // Signal that new item is available
        sem_post(full);
        
        // Sleep for a random time
        usleep(rand() % 1000000);
    }
    

    // set indication to indicate producer is done
    sem_wait(mutex);
    sharedTable->producerDone = true;
    sem_post(mutex);
    
    std::cout << "Producer thread exiting... Total items produced: " << sharedTable->totalProduced << std::endl;
    return NULL;
}



int main(int argc, char* argv[]) {
    //  number of items to produce
    int maxItems = 10;
    
   
    if (argc > 1) {
        maxItems = atoi(argv[1]);
        if (maxItems <= 0) {
            std::cerr << "Invalid number of items. Using default: 10" << std::endl;
            maxItems = 10;
        }
    }
    

    // Seed if you want
    srand(time(NULL));
    

    // Set up signal handler
    signal(SIGINT, signalHandler);
    

    // make shared memory for the table
    int shmfd = shm_open("/producer_consumer_table", O_CREAT | O_RDWR, 0666);
    if (shmfd == -1) {
        std::cerr << "Producer: Failed to create shared memory: " << strerror(errno) << std::endl;
        return 1;
    }
    


    //  size of the shared memory object
    if (ftruncate(shmfd, sizeof(SharedTable)) == -1) {
        std::cerr << "Producer: Failed to set size of shared memory: " << strerror(errno) << std::endl;
        return 1;
    }
    

    //  shared memory object
    sharedTable = (SharedTable*)mmap(NULL, sizeof(SharedTable), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (sharedTable == MAP_FAILED) {
        std::cerr << "Producer: Failed to map shared memory: " << strerror(errno) << std::endl;
        return 1;
    }
    
    // start table
    sharedTable->count = 0;

    sharedTable->totalProduced = 0;

    sharedTable->maxItems = maxItems;

    sharedTable->producerDone = false;

    
    // make  semaphores
    mutex = sem_open("/producer_consumer_mutex", O_CREAT, 0666, 1);
    empty = sem_open("/producer_consumer_empty", O_CREAT, 0666, 2);  // Table has 2 empty spots initially
    full = sem_open("/producer_consumer_full", O_CREAT, 0666, 0);    // Table has 0 full spots initially
    
    if (mutex == SEM_FAILED || empty == SEM_FAILED || full == SEM_FAILED) {
        std::cerr << "Producer: Failed to create semaphores: " << strerror(errno) << std::endl;
        return 1;
    }
    

    // make producer thread
    pthread_t producerThread;
    if (pthread_create(&producerThread, NULL, producer, NULL) != 0) {
        std::cerr << "Producer: Failed to create thread: " << strerror(errno) << std::endl;
        return 1;
    }
    
    std::cout << "Producer: Started. Will produce " << maxItems << " items." << std::endl;
    


    // Wait for the producer thread to finish
    pthread_join(producerThread, NULL);
    


    // Clean up
    sem_close(mutex);
    sem_close(empty);
    sem_close(full);
    
    // pleas edont mess with sem or links or program will explode and break
    
    std::cout << "Producer: Exiting. Produced " << sharedTable->totalProduced << " items." << std::endl;
    return 0;
}