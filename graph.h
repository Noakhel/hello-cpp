#ifndef PROJECT_MILESTONE_1_GRAPH_H
#define PROJECT_MILESTONE_1_GRAPH_H

#define MAX_NODES 100
#define INF 999999
#define MAX_TRAVELERS 20

typedef struct {
    int weight[MAX_NODES][MAX_NODES];
    int numNodes;
    int x[MAX_NODES];
    int y[MAX_NODES];
} Graph;

/* New structure to represent a single traveler for Milestone 4 */
typedef struct {
    int startNode;
    int endNode;
    int pid;             /* Stores the process ID of the child process as an integer */
    int currentX;        /* Used for GUI rendering position */
    int currentY;        /* Used for GUI rendering position */
    int path[MAX_NODES]; /* Stores the calculated Dijkstra path nodes */
    int pathLength;      /* Total number of nodes in the traveler's path */
    int isFinished;      /* Flag indicating if the traveler completed their journey */
} Traveler;

void dijkstra(Graph *g, int start, int end);
void readGraph(const char* filename, Graph* g, int* start, int* end);
int findShortestPath(Graph* g, int start, int end);

/* Declaring the two new functions added for Milestone 4 */
int readTravelers(const char* filename, Traveler travelers[], int* travelerCount);
void calculateTravelerPath(Graph* g, Traveler* t);

#endif //PROJECT_MILESTONE_1_GRAPH_H