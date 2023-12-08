#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define ONE_GB (1024*1024*1024)
#define MAX_BLOCK_SIZE (100 * 1024 * 1024)
#define MAX_BLOCK_NUM 25600
#define BLOCK_SIZE 4096


int sequential_write(int block_num, char* device, char* log_directory, int stride, int stride_val, int BOUNDED, int LOWER_BOUND, int UPPER_BOUND, int READ) {
    // Initializing block sizes
    int block_size = block_num * BLOCK_SIZE;
    printf("Writing using granularity %d\n", block_size);
    if(READ){printf("Read Mode On\n");}else{printf("Write Mode On\n");}
    if(stride){printf("Stride mode on. Stride offset is %d\n", stride_val);}
    if(BOUNDED){printf("Random mode on. Random boundary logical block %d to logical block %d\n", LOWER_BOUND, UPPER_BOUND);}


    char *buffer;
    int file, random_cursor;
    clock_t start_cpu, end_cpu;
    struct timeval start, end;
    double cpu_time_used, throughput, total_time_used;

    // buffer with two different patterns.
    buffer = (char *)malloc(block_size*2);
    memset(buffer, '1', block_size);
    memset(buffer + block_size, '2', block_size);

    file = open(device, O_CREAT | O_DIRECT | O_RDWR);
    FILE* log = fopen(log_directory, "wb");

    int pattern_offset;
    gettimeofday(&start, NULL);
    start_cpu = clock();
    for (long long total_written = 0; total_written + block_size < ONE_GB; total_written += block_size) {
       if(BOUNDED){
            random_cursor = random();
            random_cursor = (random_cursor % (UPPER_BOUND - LOWER_BOUND - block_num)) * BLOCK_SIZE;
            lseek(file, random_cursor, SEEK_SET);
        }
       pattern_offset = (total_written/block_size)%2;
       if(READ){
          read(file, buffer + block_size * pattern_offset, block_size);
       }
       else{
          write(file, buffer + block_size * pattern_offset, block_size);
       }
       
       if(stride){lseek(file, total_written + stride_val, SEEK_SET);}
    }
    end_cpu = clock();
    gettimeofday(&end, NULL);

    cpu_time_used = ((double) (end_cpu - start_cpu)) / CLOCKS_PER_SEC;
    total_time_used =  ((end.tv_sec - start.tv_sec) * 1000000.0 + (end.tv_usec - start.tv_usec)) / (1000000 *1.0);
    throughput = (ONE_GB) / total_time_used /(1024*1024*1.0); // Throughput in MB/s

    printf("CPU_TIME_USED = %f\n", cpu_time_used);
    printf("TOTAL_TIME_USED = %f\n", total_time_used);
    printf("Block Size: %d bytes, Throughput: %f MB/s\n", block_size, throughput);
    fprintf(log, "%d\n%f\n%f\n%f\n", block_num, cpu_time_used, total_time_used, throughput);

    close(file);
    fclose(log);
    free(buffer);


    return 0;
}

void read_file(char* directory, int offset, int read_size){
    char* buffer = malloc(read_size*sizeof(char));
    int file = open(directory, O_RDONLY);
    lseek(file, offset, SEEK_SET);
    read(file, buffer, read_size);
    printf("%s\n",buffer);
    free(buffer);
    return;
}


int main(int argc, char ** argv){
    printf("we are inside main function\n");
    int stride, stride_val, bounded, upper_bound, lower_bound, c, READ, length;
    char* device;
    char* log_directory;
    stride = 0;
    bounded = 0;
    READ = 0;
    while ((c = getopt (argc, argv, "m:n:s:v:b:u:l:r")) != -1) {
        switch (c) {
        case 'm':
            length = strlen(optarg);
            device = malloc(length + 1);
            strcat(device, optarg);
            break;
        case 'n':
            length = strlen(optarg);
            log_directory = malloc(length + 1);
            strcat(log_directory, optarg);
            break;
        case 's':
            stride = atoi(optarg);
            break;
        case 'v':
            stride_val = atoi(optarg);
            break;
        case 'b':
            bounded = atoi(optarg);
            break;
        case 'u':
            upper_bound = atoi(optarg);
            break;
        case 'l':
            lower_bound = atoi(optarg);
            break;
        case 'r':
            READ = atoi(optarg);
            break;
        default:
            printf("wtf");
            return 1;
        }
    }
    for(int i = 1; i <= MAX_BLOCK_NUM; i+=20000){
        sequential_write(i, device, log_directory, stride, stride_val, bounded, upper_bound, lower_bound, READ);
    }

    // read_file("./test_file", 4096, 1000);
    return 0;
}
