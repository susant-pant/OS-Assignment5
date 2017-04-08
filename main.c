#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_QUEUE_SIZE 20
typedef struct PCQueue {
	int array[MAX_QUEUE_SIZE];
	int head;
	int tail;
	int numElements;
	
	pthread_mutex_t mutex;
	pthread_cond_t consumerCond, producerCond;
} PCQueue;

typedef struct ThreadData {
	PCQueue* queue;
	int threadID;
} ThreadData;

void initPCQueue(PCQueue *queue) {
	queue->numElements = sizeof(queue->array)/sizeof(queue->array[0]);
	queue->head = 0;
	queue->tail = queue->head - 1;
	for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
		queue->array[i] = -1;
	}

	pthread_mutex_init(&(queue->mutex), NULL);	
	pthread_cond_init(&(queue->consumerCond), NULL);
	pthread_cond_init(&(queue->producerCond), NULL);
}

void PCQueueAdd(PCQueue *queue, int value) {
	// already checked that tail < numElements - 1
	queue->tail++;
	queue->array[queue->tail] = value;
}

int PCQueueRemove(PCQueue *queue) {
	// already checked that tail >= head
	int retVal = queue->array[queue->head];
	for (int i = queue->head; i < queue->tail; i++) {
		queue->array[i] = queue->array[i + 1];
	}
	queue->array[queue->tail] = -1;
	queue->tail--;

	return retVal;
}

void* Producer(void* threadData) {
	ThreadData* td = threadData;
	PCQueue* queue = td->queue;
	int id = td->threadID;

	for (int i = 10; i >= 1; i--) {
		pthread_mutex_lock(&(queue->mutex));
		while (queue->tail >= queue->numElements - 1) {
			pthread_cond_wait(&(queue->producerCond), &(queue->mutex));
		}

		printf("Producer %d has made %d things.\n", id, 11 - i);
		PCQueueAdd(queue, id);

		pthread_cond_signal(&(queue->producerCond));
		pthread_mutex_unlock(&(queue->mutex));

		if (i % 2 == 1) sleep(1);
	}
	pthread_exit(0);
}

void* Consumer(void* threadData) {
	ThreadData* td = threadData;
	PCQueue* queue = td->queue;

	for (int i = 1; i <= 100; i++) {
		//printf("%d\n", i);

		pthread_mutex_lock(&(queue->mutex));
		while (queue->tail < queue->head) {
			pthread_cond_wait(&(queue->consumerCond), &(queue->mutex));
		}

		int id = PCQueueRemove(queue);
		printf("Consumer just ate from Producer %d.\n", id);

		pthread_cond_signal(&(queue->producerCond));
		pthread_mutex_unlock(&(queue->mutex));

		if (i % 2 == 1) sleep(1);
	}
	pthread_exit(0);
}

int main() {
	pthread_t producerThread, consumerThread;

	PCQueue myQueue;
	initPCQueue(&myQueue);

	ThreadData td[10];

	for(int i = 1; i <= 10; i++) {
		td[i - 1].queue = &myQueue;
		td[i - 1].threadID = i;
  		pthread_create(&producerThread, NULL, Producer, (void*) &(td[i-1]));
	}

	ThreadData newTD;
	newTD.queue = &myQueue;
	newTD.threadID = 0;
	pthread_create(&consumerThread, NULL, Consumer, (void*) &newTD);

    pthread_join(consumerThread, NULL);
    pthread_join(producerThread, NULL);

	pthread_mutex_destroy(&(myQueue.mutex));
	pthread_cond_destroy(&(myQueue.consumerCond));
	pthread_cond_destroy(&(myQueue.producerCond));
}