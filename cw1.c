#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

char *content;
int *num_vec;
int *divisions;
int num_procs;
int num_numbers;

int sum(int itteration) {
	int s = 0;
	int offset = 0;
	for(int i= 0; i<itteration;i++){
		offset += divisions[i];
	}
	for(int i=offset; i<offset+divisions[itteration]; ++i) {
		s += num_vec[i];
	}
	return s;
}
float mean(int sum, int num) {
	return (float)sum/num;
}

void spawn_n_procs(int master_pid, int** shared_array) {
	pid_t pid;
	printf("not nested\n");
	for(int i=0; i<num_procs; ++i) {
		switch (pid = fork()){
			case -1:
				printf("Error in fork\n");
				return;
			case 0:	
				int sum_result = sum(i);
				printf(" Hello from process: %d;sum: %d; mean: %.2f\n",getpid(), sum_result, mean(sum_result, divisions[i]));
				*shared_array[i] = sum_result;
				return;
			default:
				break;
		}
	}
	for(int i=0;i<num_procs;++i){
		if(wait(0) == -1)
			printf("Error in wait\n");
	}
	int sum = 0;
	for(int i =0;i<num_procs;++i){
		sum += *shared_array[i];
	}
	printf("sum of everything: %d\n",sum);
	printf("mean of everything: %.2f\n", mean(sum, num_numbers));
	return;
}

void spawn_n_nested_procs(int master_pid, int n, int** shared_array_nested) {
	if(master_pid == getpid()) {
		if(n == -1){
		printf("nested\n");
		}
		if(n == num_procs-1) {
			int sum_result = sum(n);
			printf(" Hello from process: %d;sum: %d; mean: %.2f\n",getpid(), sum_result, mean(sum_result, divisions[n]));
			*shared_array_nested[n] = sum_result;
			return;
		}
		int pid;
		switch (pid = fork()){
			case -1:
				printf("Error in fork\n");
				return;
			case 0:
				spawn_n_nested_procs(getpid(), n+1, shared_array_nested);
				return;
			default:
				if(n != -1){					
					int sum_result = sum(n);
					printf(" Hello from process: %d;sum: %d; mean: %.2f\n",getpid(), sum_result, mean(sum_result, divisions[n]));
					*shared_array_nested[n] = sum_result;
				}
				if(wait(0) == -1)
					printf("Error with wait nested\n");
				if(n == -1){
						int sum = 0;
						for(int i =0; i< num_procs; ++i){
							sum += *shared_array_nested[i];
						}
						printf("sum of everything nested %d\n",sum);
						printf("mean of everything nested %.2f\n",mean(sum, num_numbers));
				}
		}
		return;
	} else {
		return;
	}
}

int main(int argc, char **argv)
{
	if(argc != 3) { 
		printf("Correct usage: ./program <num_procs> <name_file>");
		return 0;
	}

	num_procs = atoi(argv[1]);
	if(num_procs <= 0) {
		printf("num_procs must be positive number");
		return 0;
	}
	char *file_name = argv[2];
	pid_t pid;
	pid = getpid();
	FILE *file = fopen(file_name, "rb");
	if(file == NULL) {
		printf("Could not read the file");
		return 0;
	}
	fseek(file, 0L, SEEK_END);
	int size = ftell(file);
	fseek(file, 0L, SEEK_SET);
	content = malloc(size+1);	
	fread(content, size, 1, file);
	content[size] = '\0';
	printf("Read bytes: %d", size);
	fclose(file);
	printf("Read contents: %s\n", content);

	num_numbers = 0;
	for(int i=0; i<size; ++i) {
		if(*(content+i) == ',') ++num_numbers;
	}
	if(num_procs>num_numbers){
		printf("can't have more processes than numbers in file");
		return 0;
	}
	num_vec = (int*)malloc(num_numbers * sizeof(int));
	int num_start = 0;
	int num_idx = 0;
	char buffer[256];
	for(int i=0; i<size; ++i) {
		if(*(content+i) == ',') {
			memset(buffer, 0, 256);
			memcpy(buffer, content+num_start, i - num_start);
			num_vec[num_idx] = atoi(buffer);
			num_idx++;
			num_start = i+1;
		}
	}
	divisions = (int*)malloc(num_procs * sizeof(int));
	int size_of_division = num_numbers/num_procs;
	
	for(int i =0; i< num_procs; i++){
		divisions[i] = size_of_division;
	}
	int rest = num_numbers - num_procs * size_of_division;
	int counter = 0;
	for(int i = rest; i > 0; i--){
		divisions[counter] += 1;
		counter++;
		if(counter == num_procs){
			counter = 0;
		}
	}
	int* shared_array[num_procs]; 
	for(int i=0; i<num_procs;i++){
		shared_array[i] = mmap(NULL,sizeof(int),PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,-1,0);
	}
	spawn_n_procs(pid, shared_array);
	int* shared_array_nested[num_procs]; 
	for(int i=0; i<num_procs;i++){
		shared_array_nested[i] = mmap(NULL,sizeof(int),PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,-1,0);
	}
	spawn_n_nested_procs(pid, -1, shared_array_nested);
	for (int i = 0; i < num_procs; i++) {
        munmap(shared_array[i], sizeof(int));
    }
	for (int i = 0; i < num_procs; i++) {
        munmap(shared_array_nested[i], sizeof(int));
    }
	free(num_vec);
	free(divisions);
	free(content);
	return 0;
}
