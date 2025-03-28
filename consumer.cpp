// Zyaire Bush 
// semaphore Project 
//consumer.cpp 
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



// global variables
SharedTable* sharedTable;
sem_t* mutex;          // For mutual exclusion when accessing the table
sem_t* empty;          // Counts empty spaces in the table
sem_t* full;           // Counts filled spaces in the table
int shouldContinue = 1;
int itemsConsumed = 0;


// Signal handler to end
void signalHandler(int signum) {
    std::cout << "Consumer: Received signal " << signum << ", terminating..." << std::endl;
    shouldContinue = 0;
}



// Function to consume an item 
void consumeItem(int item) {
    itemsConsumed++;
    std::cout << "Consumer: Consumed item " << item << " (Total consumed: " << itemsConsumed 
              << "/" << sharedTable->maxItems << ")" << std::endl;
}




// Function to remove an item from the table
int removeItemFromTable() {
    if (sharedTable->count > 0) {
        // Get the first item 
        int item = sharedTable->items[0];
        
        // Shift remaining items
        if (sharedTable->count > 1) {
            sharedTable->items[0] = sharedTable->items[1];
        }
        
        sharedTable->count--;
        std::cout << "Consumer: Removed item from the table. Table count: " << sharedTable->count << std::endl;
        return item;
    } else {
        std::cerr << "Consumer: Table is empty!" << std::endl;
        return -1;
    }
}


// Consumer thread function
void* consumer(void* arg) {
    while (shouldContinue && (itemsConsumed < sharedTable->maxItems || !sharedTable->producerDone)) {
        // Check if we should continue waiting
        if (sharedTable->count == 0 && sharedTable->producerDone && itemsConsumed == sharedTable->totalProduced) {
            std::cout << "Consumer: Producer is done and all items have been consumed." << std::endl;
            break;
        }

        // Wait for an item 
        // Use a timed wait so its not infinitely checking extremely fast
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 1; // Wait for at most 1 second
        


        if (sem_timedwait(full, &ts) == -1) {
            if (errno == ETIMEDOUT) {
                // Check if we should exit

                sem_wait(mutex);
                bool shouldExit = sharedTable->producerDone && sharedTable->count == 0;
                sem_post(mutex);
                
                if (shouldExit) {
                    std::cout << "Consumer: Producer is done and no more items to consume." << std::endl;
                    break;
                }

                continue;
            } else {
                std::cerr << "Consumer: Error on sem_timedwait: " << strerror(errno) << std::endl;
                break;
            }
        }
        
        // Get that critical access to the table
        sem_wait(mutex);
        
        // Crit section Remove item from the table
        int item = removeItemFromTable();
        
        // Release that mutex
        sem_post(mutex);
        
        // let em know that a new empty slot is available
        sem_post(empty);
        
        // Consume that item
        if (item != -1) {
            consumeItem(item);
        }
        
        // Sleep random time for simulation purposes
        usleep(rand() % 1500000);
    }
    


    std::cout << "Consumer thread exiting... Total items consumed: " << itemsConsumed << std::endl;
    return NULL;
}

int main() {

    // Seed that
    srand(time(NULL) + 1);  // Add 1 to get a different seed than the producer
    
    // Set up signal handler
    signal(SIGINT, signalHandler);
    
    // 0pen up the  shared memory for the table
    int shmfd = shm_open("/producer_consumer_table", O_RDWR, 0666);
    if (shmfd == -1) {
        std::cerr << "Consumer: Failed to open shared memory: " << strerror(errno) << std::endl;
        return 1;
    }
    


    // draw the shared memory object
    sharedTable = (SharedTable*)mmap(NULL, sizeof(SharedTable), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (sharedTable == MAP_FAILED) {
        std::cerr << "Consumer: Failed to map shared memory: " << strerror(errno) << std::endl;
        return 1;
    }
    

    // Open the semaphores
    mutex = sem_open("/producer_consumer_mutex", 0);

    empty = sem_open("/producer_consumer_empty", 0);

    full = sem_open("/producer_consumer_full", 0);
    

    if (mutex == SEM_FAILED || empty == SEM_FAILED || full == SEM_FAILED) {
        std::cerr << "Consumer: Failed to open semaphores: " << strerror(errno) << std::endl;
        return 1;
    }
    
    
    // Create a consumer thread
    pthread_t consumerThread;
    if (pthread_create(&consumerThread, NULL, consumer, NULL) != 0) {
        std::cerr << "Consumer: Failed to create thread: " << strerror(errno) << std::endl;
        return 1;
    }
    
    std::cout << "Consumer: Started. Will consume up to " << sharedTable->maxItems << " items." << std::endl;
    
    // Wait for the consumer thread to finish
    pthread_join(consumerThread, NULL);
    
    // Clean up
    sem_close(mutex);
    sem_close(empty);
    sem_close(full);
    

    // Unlink the semaphore and get rid of the shared memory 
    sem_unlink("/producer_consumer_mutex");

    sem_unlink("/producer_consumer_empty");

    sem_unlink("/producer_consumer_full");

    shm_unlink("/producer_consumer_table");
    
    std::cout << "Consumer: Exiting. Consumed " << itemsConsumed << " items." << std::endl;
    return 0;
}
