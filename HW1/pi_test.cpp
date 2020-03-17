#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <ctime>
#include <pthread.h>
#include <unistd.h>
#include <queue>
using namespace std;

pthread_mutex_t mutex;
double pi_estimate;
unsigned long long  number_of_cpu, number_of_tosses, number_in_circle, each_tosses;

void *Generate_rand(void *rand_pool){
    queue<double> *pointer = (queue<double> *)rand_pool;
    unsigned int seed = time(NULL);
    for(unsigned long long i = 0; i < 2*number_of_tosses; i++){
        double temp;
        temp = (2 * (double)rand_r(&seed) / RAND_MAX) - 1;
        //cout << temp << endl;
        cout << i << endl;
        pointer->push(temp);
    }
    pthread_exit(NULL);
}

void *Thread_sum(void *rand_pool){
    queue<double> *pointer = (queue<double> *)rand_pool;
    unsigned long long toss, local_count;
    double distance_squared, x, y;
    
    local_count = 0;
    for (toss = 0; toss < each_tosses; toss++) {
        if(pointer->size() == 0) sleep(1);
        x = pointer->front();
        pointer->pop();
        y = pointer->front();
        pointer->pop();
        // x = random double between -1 and 1;
        // y = random double between -1 and 1;
        distance_squared = x*x + y*y;
        if (distance_squared <= 1){
            local_count++;
        }
    }
    pthread_mutex_lock(&mutex);
    number_in_circle += local_count;
    pthread_mutex_unlock(&mutex);
    //cout << "thread " << pid << " done" << endl;

    return NULL;
}

int main(int argc, char **argv)
{
    cin.tie(0);
    ios::sync_with_stdio(false);

    long thread;
    pthread_t *thread_handles;
    time_t startwtime, endwtime;
    queue<double> rand_pool;

    if ( argc < 2) {
        exit(-1);
    }
    number_of_cpu = atoi(argv[1]);
    number_of_tosses = atoi(argv[2]);
    if (( number_of_cpu < 1) || ( number_of_tosses < 0)) {
        exit(-1);
    }
    //startwtime = time (NULL);

    // thread init
    thread_handles = (pthread_t*) malloc ((number_of_cpu+1)*sizeof(pthread_t));
    pthread_mutex_init(&mutex, NULL);
    number_in_circle = 0;
    each_tosses = number_of_tosses / number_of_cpu;
    
    //create thread
    pthread_create(&thread_handles[0], NULL, Generate_rand, &rand_pool);

    for(thread = 1; thread <= number_of_cpu; thread++){
        pthread_create(&thread_handles[thread], NULL, Thread_sum, &rand_pool);
    }

    // join thread
    for(thread = 0; thread <= number_of_cpu; thread++){
        pthread_join(thread_handles[thread], NULL);
    }

    pi_estimate = 4*number_in_circle/((double) number_of_tosses);
    pthread_mutex_destroy(&mutex);

    //endwtime = time (NULL);
    printf("%f\n",pi_estimate);
    //cout << (endwtime - startwtime) << " S" << endl;
    return 0;
}