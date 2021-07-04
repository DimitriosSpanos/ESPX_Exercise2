#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include "covidTrace.h"
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>


#define ALL_DEVICES 7000  // Number of all the possible devices that would arrive. (fixed number)
#define NUM_OF_DAYS 30    // Number of days the program will run. (random, but fixed number)
#define SPEEDUP 0.01      // The program will run 100 times faster than normal with a SPEEDUP of 0.01.
#define in_milli 1000     // Helper variable so that the msleep function works correctly. (Its' argument is in milliseconds)


    /* --------------------------------

        Declaration of helper functions

       -------------------------------- */


int msleep(long msec);
double time_spent(struct timespec start,struct timespec end);

    /* --------------------------

            Global Variables

       -------------------------- */


pthread_mutex_t lock;
FILE *search_times_ptr;
FILE *contacts_ptr;

struct timespec main_start;
struct timespec program_starting_time;
double main_thread_time = 0;
double addreses_thread_time = 0;
double contacts_thread_time = 0;
double search_thread_time = 0;
double test_thread_time = 0;

double four_mins = (double)4*(double)60*SPEEDUP;
double twenty_mins = (double)20*(double)60*SPEEDUP;
double fourteen_days = (double)14 * (double)86400*SPEEDUP;

    /* --------------------------

             Defined structs

       -------------------------- */


typedef struct uint48 {
	uint64_t v : 48 ;
} uint48;

typedef struct macaddress {
    uint48 value;
    struct timespec timestamp;
} macaddress;

typedef struct all_addresses {

    macaddress addresses[ALL_DEVICES];
    macaddress close_contacts[ALL_DEVICES];

}all_addresses;



    /* --------------------------

            Thread functions

       -------------------------- */



bool testCOVID(){
    if (rand() % 100 < 4) // 4% we have positive COVID
        return true;
    else
        return false;
}


macaddress BTnearMe(){

    macaddress nearMe;
    int address_number;
    nearMe.value.v = (uint64_t)(rand() % ALL_DEVICES); // 48-bit

    struct timespec timestamp;
    clock_gettime( CLOCK_MONOTONIC, &timestamp);
    nearMe.timestamp = timestamp;

    return nearMe;
}



/// Search for nearby devices
void *searchBT(void *input){

    search_times_ptr = fopen("timestamps.bin","wb");
    double buffer[1];
    double search_offset = 0;
    while(1){
        struct timespec start;  /// Start
        clock_gettime( CLOCK_MONOTONIC, &start);


        macaddress nearMe = BTnearMe();
        buffer[0] = time_spent(program_starting_time, nearMe.timestamp) - search_offset;
        fwrite(buffer,sizeof(buffer),1,search_times_ptr);
        search_offset += (double)10 *SPEEDUP;
        double time_diff = time_spent(((all_addresses*)input)->addresses[nearMe.value.v].timestamp, nearMe.timestamp);
        // 4min <= time_diff <= 20min
        if (four_mins <= time_diff && time_diff <= twenty_mins){
            pthread_mutex_lock(&lock);
            ((all_addresses*)input)->close_contacts[nearMe.value.v] = nearMe;
            pthread_mutex_unlock(&lock);
        }
        pthread_mutex_lock(&lock);
        ((all_addresses*)input)->addresses[nearMe.value.v] = nearMe;
        pthread_mutex_unlock(&lock);


        struct timespec end; /// End
        clock_gettime( CLOCK_MONOTONIC, &end);
        search_thread_time += time_spent(start,end); /// CPU Idleness calculation
        msleep(10*SPEEDUP*in_milli); // sleep for 10 seconds
    }
}



/// Memory of close contacts
void *contactsMemory(void *close_contacts){

    sleep(13 * 86400*SPEEDUP); // sleep for 13 days
    while(1){
        struct timespec start;  /// Start
        clock_gettime( CLOCK_MONOTONIC, &start);


        struct timespec current_time;
        clock_gettime( CLOCK_MONOTONIC, &current_time);
        pthread_mutex_lock(&lock);
        for(int i=0; i< ALL_DEVICES; i++){
            if (time_spent(((macaddress *)close_contacts)[i].timestamp,current_time) > fourteen_days)
                ((macaddress *)close_contacts)[i].timestamp.tv_sec = 0;
        }
        pthread_mutex_unlock(&lock);

        struct timespec end; /// End
        clock_gettime( CLOCK_MONOTONIC, &end);
        contacts_thread_time += time_spent(start,end); /// CPU Idleness calculation
        sleep(86400*SPEEDUP); // each day take a look
    }
}


/// Memory of addresses
void *addressesMemory(void *input){

    msleep(19 * 60*SPEEDUP * in_milli); // sleep for 19 minutes
    struct timespec current_time;
    while(1){
        struct timespec start;   /// Start
        clock_gettime( CLOCK_MONOTONIC, &start);


        clock_gettime( CLOCK_MONOTONIC, &current_time);
        pthread_mutex_lock(&lock);
        for(int i=0; i< ALL_DEVICES; i++){
            // IF not a close contact AND timestamp of address more than 20mins ago
            if (((all_addresses*)input)->close_contacts[i].timestamp.tv_sec == 0 && time_spent(((all_addresses*)input)->addresses[i].timestamp, current_time) > twenty_mins)
                ((all_addresses*)input)->addresses[i].timestamp.tv_sec = 0; // THEN forget it
        }
        pthread_mutex_unlock(&lock);

        struct timespec end;   /// End
        clock_gettime( CLOCK_MONOTONIC, &end);
        addreses_thread_time += time_spent(start,end); /// CPU Idleness calculation
        msleep(60*SPEEDUP*in_milli); // each minute take a look
    }
}


void uploadContacts(macaddress *close_contacts){

    uint64_t buffer[1];
    bool close_contacts_existed = false;
    for(int i=0; i<ALL_DEVICES; i++){ //upload all the close contacts to the server
        if(close_contacts[i].timestamp.tv_sec != 0){ // close contact is not forgotten
            buffer[0] = (close_contacts[i].value.v + (uint64_t)1);
            fwrite(buffer,sizeof(buffer),1,contacts_ptr);
            close_contacts_existed = true;
        }
    }
    // 0 denotes that the i-th covidTrace stops there.
    if(close_contacts_existed){
        buffer[0] = 0;
        fwrite(buffer,sizeof(buffer),1,contacts_ptr);
    }
}


/// Test for COVID
void *testForCOVID(void *close_contacts){

    contacts_ptr = fopen("covidTrace.bin","wb");
    while(1){
        struct timespec start;     /// Start
        clock_gettime( CLOCK_MONOTONIC, &start);


        if(testCOVID()){
            pthread_mutex_lock(&lock);
            uploadContacts(((macaddress *)close_contacts));
            pthread_mutex_unlock(&lock);
        }


        struct timespec end;      /// End
        clock_gettime( CLOCK_MONOTONIC, &end);
        test_thread_time += time_spent(start,end); /// CPU Idleness calculation
        msleep(4 *3600*SPEEDUP*in_milli); // each 4 hours take a look
    }
}


int main()
{
    clock_gettime( CLOCK_MONOTONIC, &main_start);
    clock_gettime( CLOCK_MONOTONIC, &program_starting_time);
    srand(time(NULL));
    all_addresses *input = (all_addresses *)malloc(sizeof(all_addresses));;

    // if i-th address is not assigned, then timestamp.secs = 0
    for(int i=0; i<ALL_DEVICES; i++){
        input->addresses[i].timestamp.tv_sec = 0;
        input->close_contacts[i].timestamp.tv_sec = 0;
    }

    /* --------------------------

          START OF THE PROGRAM

       -------------------------- */

    int status;

    /// Search for nearby devices
    pthread_t search;
    status = pthread_create(&search, NULL, &searchBT, (void *)input);
    if (status){
        printf("ERROR; return code from searchBT is %d\n", status);
        exit(-1);
    }

    /// Memory of close contacts
    pthread_t contacts_memory;
    status = pthread_create(&contacts_memory, NULL, &contactsMemory, (void *)input->close_contacts);
    if (status){
        printf("ERROR; return code from contacts_memory is %d\n", status);
        exit(-1);
    }

    /// Memory of addresses
    pthread_t addresses_memory;
    status = pthread_create(&addresses_memory, NULL, &addressesMemory, (void *)input);
    if (status){
        printf("ERROR; return code from addresses_memory is %d\n", status);
        exit(-1);
    }

    /// Test for COVID
    pthread_t test_COVID;
    status = pthread_create(&test_COVID, NULL, &testForCOVID, (void *)input->close_contacts);
    if (status){
        printf("ERROR; return code from test_COVID is %d\n", status);
        exit(-1);
    }

    struct timespec main_end;
    clock_gettime( CLOCK_MONOTONIC, &main_end);
    main_thread_time += time_spent(main_start, main_end);

    /// 30 days of simulation handler
    struct timespec BT_current;
    while(1){
        struct timespec start;    /// Start
        clock_gettime( CLOCK_MONOTONIC, &start);


        clock_gettime( CLOCK_MONOTONIC, &BT_current);
        printf("Progress: %0.2f%%  \n", time_spent(program_starting_time, BT_current) / ((double)NUM_OF_DAYS *(double)86400*SPEEDUP) * (double)100);
        if(time_spent(program_starting_time, BT_current) >= NUM_OF_DAYS *(double)86400*SPEEDUP){ // 30 days
            pthread_cancel(contacts_memory);
            pthread_join(contacts_memory,NULL);
            pthread_cancel(search);
            pthread_join(search, NULL);
            pthread_cancel(addresses_memory);
            pthread_join(addresses_memory,NULL);
            pthread_cancel(test_COVID);
            pthread_join(test_COVID, NULL);
            fclose(search_times_ptr);
            fclose(contacts_ptr);
            break;
        }

        struct timespec end;    /// End
        clock_gettime( CLOCK_MONOTONIC, &end);
        main_thread_time += time_spent(start, end); /// CPU Idleness calculation
        msleep(5 *3600*SPEEDUP*in_milli); // 5 hour
    }
    struct timespec program_ending_time;
    clock_gettime( CLOCK_MONOTONIC, &program_ending_time);


    double program_running_time = time_spent(program_starting_time, program_ending_time);
    printf("****************************************\n");
    printf("Thread \"Main\"     : %f%% usage.\n",(main_thread_time*(double)100/program_running_time));
    printf("Thread \"Search\"   : %f%% usage.\n",(search_thread_time*(double)100/program_running_time));
    printf("Thread \"Addresses\": %f%% usage.\n",(addreses_thread_time*(double)100/program_running_time));
    printf("Thread \"Contacts\" : %f%% usage.\n",(contacts_thread_time*(double)100/program_running_time));
    printf("Thread \"Test\"     : %f%% usage.\n",(test_thread_time*(double)100/program_running_time));
    printf("****************************************\n");

    pthread_exit(NULL);
    return 0;
}


double time_spent(struct timespec start,struct timespec end){
        struct timespec temp;
        if ((end.tv_nsec - start.tv_nsec) < 0){
            temp.tv_sec = end.tv_sec - start.tv_sec - 1;
            temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
        }
        else{
            temp.tv_sec = end.tv_sec - start.tv_sec;
            temp.tv_nsec = end.tv_nsec - start.tv_nsec;
        }
        return (double)temp.tv_sec +(double)((double)temp.tv_nsec/(double)1000000000);
}


/* msleep(): Sleep for the requested number of milliseconds. */
int msleep(long msec){
    struct timespec ts;
    int res;
    if (msec < 0){
        errno = EINVAL;
        return -1;
    }
    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;
    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);
    return res;
}
