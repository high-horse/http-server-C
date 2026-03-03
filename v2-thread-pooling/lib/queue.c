
#include <stdio.h>
#include <stdlib.h>

typedef struct{
    int data;
    struct Node *next;
} Node;

typedef struct {
    Node *front;
    Node *rear;
} Queue;

void init_queue(Queue *q) {
    q->front = NULL;
    q->rear = NULL;
}

int is_empty(Queue *q) {
    return q->front == NULL;
}

void enque(Queue *q, int data) {
    Node *new_node = (Node *) calloc(1, sizeof(Node));
    if(new_node == NULL) {
        perror("MEMORY ALLOCATION FAILED:");
        return;
    }
    
    new_node->data = data;
    new_node->next = NULL;
    
    if(is_empty(q)) {
        q->front = new_node;
        q->rear = new_node;
    } else {
        q->rear->next = new_node;
        q->rear = new_node;
    }
}


int dequeue(Queue *q) {
    if(is_empty(q)){
        printf("QUEUE EMPTY\n");
        return EXIT_FAILURE;
    }
    
    Node *temp = q->front;
    int value = temp->data;
    
    q->front = temp->next;
    if(q->front == NULL) {
        q->rear = NULL;
    }
    free(temp);
    return  value;
}


void print_queue(Queue *q) {
    Node *temp = q->front;
    while(temp != NULL) {
        printf("%d -> ", temp->data);
        temp = temp->next;
    }
    printf("NULL\n");
}   

int main() {
    Queue q;
    init_queue(&q);
    enque(&q, 1);
    enque(&q, 2);
    
    enque(&q, 3);

    enque(&q, 4);
    
    print_queue(&q);
    printf("dequed %d\n", dequeue(&q));
    print_queue(&q);
    printf("dequed %d\n", dequeue(&q));
    
}