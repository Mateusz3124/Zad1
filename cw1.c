#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

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

void spawn_n_procs(int master_pid) {
	pid_t pid;
	printf("not nested\n");
	for(int i=0; i<num_procs; ++i) {
		switch (pid = fork()){
			case -1:
				printf("Error in fork\n");
				return;
			case 0:
				printf(" ");
				int sum_result = sum(i);
				printf("Hello from process: %d;sum: %d; mean: %.2f\n",getpid(), sum_result, mean(sum_result, divisions[i]));
				return;
			default:
				break;
		}
	}
	for(int i=0;i<num_procs;++i){
		if(wait(0) == -1)
			printf("Error in wait\n");
	}
	return;
}

void spawn_n_nested_procs(int master_pid, int n) {
	if(master_pid == getpid()) {
		if(n == 0){
		printf("nested\n");
		}
		if(n == num_procs) {return;}
		int pid;
		switch (pid = fork()){
			case -1:
				printf("Error in fork\n");
				return;
			case 0:
				spawn_n_nested_procs(getpid(), n+1);
				return;
			default:
				printf("Hello from process %d\n", master_pid);
				int sum_result = sum(n);
				printf("sum: %d\n", sum_result);
				printf("mean: %.2f\n", mean(sum_result, divisions[n]));
				if(wait(0) == -1)
					printf("Error with wait nested\n");
		}
		return;
	} else {
		return;
	}
}
int main(int argc, char **argv)
{
	if(argc != 2) { 
		printf("Correct usage: ./program <num_procs>");
		return 0;
	}

	num_procs = atoi(argv[1]);
	if(num_procs <= 0) {
		printf("num_procs must be positive number");
		return 0;
	}

	pid_t pid;
	pid = getpid();
	FILE *file = fopen("vector", "rb");
	if(file == NULL) {
		printf("Could not read the file");
		return 0;
	}
	fseek(file, 0L, SEEK_END);
	int size = ftell(file);
	fseek(file, 0L, SEEK_SET);
	content = malloc(size);	
	fread(content, size, 1, file);
	printf("Read bytes: %d", size);
	fclose(file);
	printf("Read contents: %s", content);

	num_numbers = 0;
	for(int i=0; i<size; ++i) {
		if(*(content+i) == ',') ++num_numbers;
	}
	if(num_procs>num_numbers){
		printf("can't have more processes than numbers");
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
	printf("\n");
	spawn_n_procs(pid);
	spawn_n_nested_procs(pid, 0);

	return 0;
}
