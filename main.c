#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX_RAM 512
#define CPU2_RAM 2048
#define CPU_CAPACITY 100
#define QUANTUM_TIME 8

typedef struct {
    char id[10];
    int arrival_time;
    int priority;
    int burst_time;
    int ram_required;
    int cpu_required;
    bool completed;
    int remaining_time; // Add remaining_time for Round Robin
} Process;

typedef struct Node {
    Process* process;
    struct Node* next;
} Node;

typedef struct {
    Node* front;
    Node* rear;
} Queue;

void initQueue(Queue* q) {
    q->front = q->rear = NULL;
}

bool isQueueEmpty(Queue* q) {
    return q->front == NULL;
}

void enqueue(Queue* q, Process* process) {
    Node* temp = (Node*)malloc(sizeof(Node));
    temp->process = process;
    temp->next = NULL;
    if (q->rear == NULL) {
        q->front = q->rear = temp;
        return;
    }
    q->rear->next = temp;
    q->rear = temp;
}

Process* dequeue(Queue* q) {
    if (isQueueEmpty(q)) {
        return NULL;
    }
    Node* temp = q->front;
    q->front = q->front->next;
    if (q->front == NULL) {
        q->rear = NULL;
    }
    Process* process = temp->process;
    free(temp);
    return process;
}

Process* readProcesses(const char* filename, int* process_count) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Unable to open file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    *process_count = 0;
    while (fgets(line, sizeof(line), file)) {
        (*process_count)++;
    }

    fseek(file, 0, SEEK_SET);
    Process* processes = (Process*)malloc(sizeof(Process) * (*process_count));
    int i = 0;
    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%[^,],%d,%d,%d,%d,%d", processes[i].id, &processes[i].arrival_time, 
               &processes[i].priority, &processes[i].burst_time, 
               &processes[i].ram_required, &processes[i].cpu_required);
        processes[i].completed = false;
        processes[i].remaining_time = processes[i].burst_time; // Initialize remaining_time
        i++;
    }

    fclose(file);
    return processes;
}

bool checkResources(Process* process, int available_ram, int available_cpu) {
    return process->ram_required <= available_ram && process->cpu_required <= available_cpu;
}

void fcfsScheduling(Queue* queue, FILE* output, int* available_ram, int* current_time, char* processed_order) {
    while (!isQueueEmpty(queue)) {
        Process* process = dequeue(queue);
        if (*current_time < process->arrival_time) {
            *current_time = process->arrival_time;
        }
        fprintf(output, "Process %s is assigned to CPU-1 at time %d.\n", process->id, *current_time);
        *current_time += process->burst_time;
        process->completed = true;
        fprintf(output, "Process %s is completed and terminated at time %d.\n", process->id, *current_time);
        *available_ram += process->ram_required;

        // Add the process id to the processed order list
        strcat(processed_order, process->id);
        strcat(processed_order, "-");
    }
    
}


void sjfScheduling(Queue* queue, FILE* output, int* available_ram, int* current_time, char* processed_order, Process* processes, int process_count) {
    while (!isQueueEmpty(queue)) {
        Node* shortest_prev = NULL;
        Node* shortest = NULL;
        Node* prev = NULL;
        Node* current = queue->front;

        // Find the shortest job available at the current time
        while (current != NULL) {
            if (current->process->arrival_time <= *current_time) {
                if (shortest == NULL || current->process->burst_time < shortest->process->burst_time) {
                    shortest = current;
                    shortest_prev = prev;
                }
            }
            prev = current;
            current = current->next;
        }

        // If we found a shortest job that is available at the current time
        if (shortest != NULL) {
            Process* process = shortest->process;
            if (shortest_prev == NULL) {
                queue->front = shortest->next;
            } else {
                shortest_prev->next = shortest->next;
            }
            if (shortest == queue->rear) {
                queue->rear = shortest_prev;
            }
            free(shortest);

            if (*current_time < process->arrival_time) {
                *current_time = process->arrival_time;
            }
            fprintf(output, "Process %s is assigned to CPU-2 at time %d.\n", process->id, *current_time);
            *current_time += process->burst_time;
            process->completed = true;
            fprintf(output, "Process %s is completed and terminated at time %d.\n", process->id, *current_time);
            *available_ram += process->ram_required;

            // Add the process id to the processed order list
            strcat(processed_order, process->id);
            strcat(processed_order, "-");

            // Check for new processes that can be queued
            for (int i = 0; i < process_count; ++i) {
                if (!processes[i].completed && processes[i].priority == 1 && processes[i].arrival_time <= *current_time) {
                    if (checkResources(&processes[i], *available_ram, CPU_CAPACITY)) {
                        enqueue(queue, &processes[i]);
                        *available_ram -= processes[i].ram_required;
                        fprintf(output, "Process %s is queued to be assigned to CPU-2 at time %d.\n", processes[i].id, *current_time);
                        processes[i].completed = true; // Mark as queued to avoid re-queuing
                    }
                }
            }
        } else {
            // If no jobs are available at the current time, increment the time
            *current_time += 1;
        }
    }
}
void printQueueStatus(Queue* queue, const char* queue_name, const char* description) {
    printf("%s (%s) -> ", queue_name, description);
    Node* temp = queue->front;
    while (temp != NULL) {
        printf("%s", temp->process->id);
        if (temp->next != NULL) {
            printf("-");
        }
        temp = temp->next;
    }
    printf("\n");
}




int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s input.txt\n", argv[0]);
        return 1;
    }

    int process_count;
    Process* processes = readProcesses(argv[1], &process_count);

    Queue cpu1_queue;
    initQueue(&cpu1_queue);

    Queue cpu2_queue;
    initQueue(&cpu2_queue);

    Queue queue3;
    initQueue(&queue3);

    Queue queue4;
    initQueue(&queue4);

    int available_ram_cpu1 = MAX_RAM;
    int available_ram_cpu2 = CPU2_RAM;
    int current_time_cpu1 = 0;
    int current_time_cpu2 = 0;

    FILE* output = fopen("output.txt", "w");
    if (!output) {
        perror("Unable to open output file");
        return 1;
    }

    char processed_order_cpu1[1024] = "";  // Buffer to store the order of processed processes for CPU-1
    char processed_order_cpu2[1024] = "";  // Buffer to store the order of processed processes for CPU-2
    char processed_order_queue3[1024] = "";  // Buffer to store the order of processed processes for queue3
    char processed_order_queue4[1024] = "";  // Buffer to store the order of processed processes for queue4

    for (int i = 0; i < process_count; i++) {
        if (!processes[i].completed) {
            if (processes[i].priority == 0) {
                if (checkResources(&processes[i], available_ram_cpu1, CPU_CAPACITY)) {
                    enqueue(&cpu1_queue, &processes[i]);
                    available_ram_cpu1 -= processes[i].ram_required;
                    fprintf(output, "Process %s is queued to be assigned to CPU-1 at time %d.\n", processes[i].id, processes[i].arrival_time);
                    processes[i].completed = true; // Mark as queued to avoid re-queuing
                }
            } else if (processes[i].priority == 1) {
                if (checkResources(&processes[i], available_ram_cpu2, CPU_CAPACITY)) {
                    enqueue(&cpu2_queue, &processes[i]);
                    available_ram_cpu2 -= processes[i].ram_required;
                    fprintf(output, "Process %s is queued to be assigned to CPU-2 at time %d.\n", processes[i].id, processes[i].arrival_time);
                    processes[i].completed = true; // Mark as queued to avoid re-queuing
                }
            } else if (processes[i].priority == 2) {
                if (checkResources(&processes[i], available_ram_cpu2, CPU_CAPACITY)) {
                    enqueue(&queue3, &processes[i]);
                    available_ram_cpu2 -= processes[i].ram_required;
                    fprintf(output, "Process %s is queued to be assigned to queue3 at time %d.\n", processes[i].id, processes[i].arrival_time);
                    processes[i].completed = true; // Mark as queued to avoid re-queuing
                }
            } else if (processes[i].priority == 3) {
                if (checkResources(&processes[i], available_ram_cpu2, CPU_CAPACITY)) {
                    enqueue(&queue4, &processes[i]);
                    available_ram_cpu2 -= processes[i].ram_required;
                    fprintf(output, "Process %s is queued to be assigned to queue4 at time %d.\n", processes[i].id, processes[i].arrival_time);
                    processes[i].completed = true; // Mark as queued to avoid re-queuing
                }
            }
        }
    }

    // Set the initial current time to 0
    current_time_cpu1 = 0;
    current_time_cpu2 = 0;

    while (!isQueueEmpty(&cpu1_queue) || !isQueueEmpty(&cpu2_queue) || !isQueueEmpty(&queue3) || !isQueueEmpty(&queue4)) {
        // Process CPU-1 queue using FCFS
        if (!isQueueEmpty(&cpu1_queue)) {
            fcfsScheduling(&cpu1_queue, output, &available_ram_cpu1, &current_time_cpu1, processed_order_cpu1);
        }

        // Process CPU-2 queue using SJF
        if (!isQueueEmpty(&cpu2_queue)) {
            sjfScheduling(&cpu2_queue, output, &available_ram_cpu2, &current_time_cpu2, processed_order_cpu2, processes, process_count);
        }

        // Process queue3 using Round Robin
        if (!isQueueEmpty(&queue3)) {
            roundRobinScheduling(&queue3, output, &available_ram_cpu2, &current_time_cpu2, processed_order_queue3, QUANTUM_TIME);
        }

        // Process queue4 using Round Robin
        if (!isQueueEmpty(&queue4)) {
            roundRobinScheduling(&queue4, output, &available_ram_cpu2, &current_time_cpu2, processed_order_queue4, QUANTUM_TIME);
        }

        // Check for new processes that can be queued
        for (int i = 0; i < process_count; ++i) {
            if (!processes[i].completed && processes[i].priority == 0 && processes[i].arrival_time <= current_time_cpu1) {
                if (checkResources(&processes[i], available_ram_cpu1, CPU_CAPACITY)) {
                    enqueue(&cpu1_queue, &processes[i]);
                    available_ram_cpu1 -= processes[i].ram_required;
                    fprintf(output, "Process %s is queued to be assigned to CPU-1 at time %d.\n", processes[i].id, current_time_cpu1);
                    processes[i].completed = true; // Mark as queued to avoid re-queuing
                }
            }
            if (!processes[i].completed && processes[i].priority == 1 && processes[i].arrival_time <= current_time_cpu2) {
                if (checkResources(&processes[i], available_ram_cpu2, CPU_CAPACITY)) {
                    enqueue(&cpu2_queue, &processes[i]);
                    available_ram_cpu2 -= processes[i].ram_required;
                    fprintf(output, "Process %s is queued to be assigned to CPU-2 at time %d.\n", processes[i].id, current_time_cpu2);
                    processes[i].completed = true; // Mark as queued to avoid re-queuing
                }
            }
            if (!processes[i].completed && processes[i].priority == 2 && processes[i].arrival_time <= current_time_cpu2) {
                if (checkResources(&processes[i], available_ram_cpu2, CPU_CAPACITY)) {
                    enqueue(&queue3, &processes[i]);
                    available_ram_cpu2 -= processes[i].ram_required;
                    fprintf(output, "Process %s is queued to be assigned to queue3 at time %d.\n", processes[i].id, current_time_cpu2);
                    processes[i].completed = true; // Mark as queued to avoid re-queuing
                }
            }
            if (!processes[i].completed && processes[i].priority == 3 && processes[i].arrival_time <= current_time_cpu2) {
                if (checkResources(&processes[i], available_ram_cpu2, CPU_CAPACITY)) {
                    enqueue(&queue4, &processes[i]);
                    available_ram_cpu2 -= processes[i].ram_required;
                    fprintf(output, "Process %s is queued to be assigned to queue4 at time %d.\n", processes[i].id, current_time_cpu2);
                    processes[i].completed = true; // Mark as queued to avoid re-queuing
                }
            }
        }
    }

    // Remove the trailing dash and print the processed order
    if (strlen(processed_order_cpu1) > 0) {
        processed_order_cpu1[strlen(processed_order_cpu1) - 1] = '\0';
    }
    printf("CPU-1 queue (priority-0) (FCFS) -> %s\n", processed_order_cpu1);

    if (strlen(processed_order_cpu2) > 0) {
        processed_order_cpu2[strlen(processed_order_cpu2) - 1] = '\0';
    }
    printf("CPU-2 queue (priority-1) (SJF) -> %s\n", processed_order_cpu2);

    if (strlen(processed_order_queue3) > 0) {
        processed_order_queue3[strlen(processed_order_queue3) - 1] = '\0';
    }
    printf("queue3 (priority-2) (Round Robin) -> %s\n", processed_order_queue3);

    if (strlen(processed_order_queue4) > 0) {
        processed_order_queue4[strlen(processed_order_queue4) - 1] = '\0';
    }
    printf("queue4 (priority-3) (Round Robin) -> %s\n", processed_order_queue4);

    fclose(output);
    free(processes);
    return 0;
}
