#ifndef PROJECT_MILESTONE_1_GRAPH_H
#define PROJECT_MILESTONE_1_GRAPH_H

#define MAX_NODES 100
#define INF 999999

typedef struct {
    int weight[MAX_NODES][MAX_NODES];
    int numNodes;
    int x[MAX_NODES];
    int y[MAX_NODES];
} Graph;
void dijkstra(Graph *g, int start, int end);
void readGraph(const char* filename, Graph* g, int* start, int* end);
int findShortestPath(Graph* g, int start, int end);

#endif //PROJECT_MILESTONE_1_GRAPH_H