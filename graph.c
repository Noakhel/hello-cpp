#include "graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Helper function to store the calculated path into the Traveler's structure */
void storePath(int parent[], int j, Traveler* t) {
    if (parent[j] == -1) {
        t->path[t->pathLength++] = j;
        return;
    }
    storePath(parent, parent[j], t);
    if (t->pathLength < MAX_NODES) {
        t->path[t->pathLength++] = j;
    }
}

/* Original function left intact for backward compatibility */
void printPath(int parent[], int j) {
    if (parent[j] == -1) {
        printf("%d", j);
        return;
    }
    printPath(parent, parent[j]);
    printf(" -> %d", j);
}

void readGraph(const char* filename, Graph* g, int* start, int* end) {
    int num_edges, u, v, w;
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open file\n");
        exit(1);
    }

    if (fscanf(file, "%d %d", &(g->numNodes), &num_edges) != 2) exit(1);

    for (int i = 0; i < g->numNodes; i++) {
        for (int j = 0; j < g->numNodes; j++) {
            g->weight[i][j] = (i == j) ? 0 : INF;
        }
    }

    for (int i = 0; i < num_edges; i++) {
        if (fscanf(file, "%d %d %d", &u, &v, &w) == 3) {
            if (w < 0) {
                printf("Error: Negative weight is invalid\n");
                fclose(file);
                exit(1);
            }
            g->weight[u][v] = w;
        }
    }

    /* Safely try to read old format single start/end if present */
    if (fscanf(file, "%d %d", start, end) != 2) {
        *start = 0;
        *end = 0;
    }
    fclose(file);
}

/* New function for Milestone 4 to read the extended travelers configuration */
int readTravelers(const char* filename, Traveler travelers[], int* travelerCount) {
    FILE* file = fopen(filename, "r");
    if (!file) return 0;

    char line[256];
    int foundTravelersTag = 0;
    *travelerCount = 0;

    /* Scan the file line by line to find the #travelers section */
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "#travelers") != NULL) {
            foundTravelersTag = 1;
            break;
        }
    }

    if (!foundTravelersTag) {
        fclose(file);
        return 0;
    }

    /* Read the total number of travelers */
    if (fscanf(file, "%d", travelerCount) != 1) {
        fclose(file);
        return 0;
    }

    /* Read each traveler's source and destination nodes */
    for (int i = 0; i < *travelerCount; i++) {
        if (fscanf(file, "%d %d", &travelers[i].startNode, &travelers[i].endNode) == 2) {
            travelers[i].pid = 0;
            travelers[i].pathLength = 0;
            travelers[i].isFinished = 0;
            travelers[i].currentX = 0;
            travelers[i].currentY = 0;
        }
    }

    fclose(file);
    return 1;
}

void dijkstra(Graph* g, int start, int end) {
    if (start == end) {
        printf("%d\n0\n", start);
        return;
    }

    int dist[MAX_NODES], visited[MAX_NODES], parent[MAX_NODES];
    for (int i = 0; i < g->numNodes; i++) {
        dist[i] = INF;
        visited[i] = 0;
        parent[i] = -1;
    }
    dist[start] = 0;

    for (int count = 0; count < g->numNodes - 1; count++) {
        int u = -1, min = INF;
        for (int i = 0; i < g->numNodes; i++)
            if (!visited[i] && dist[i] < min) { min = dist[i]; u = i; }

        if (u == -1) break;
        visited[u] = 1;

        for (int v = 0; v < g->numNodes; v++)
            if (!visited[v] && g->weight[u][v] != INF && dist[u] + g->weight[u][v] < dist[v]) {
                parent[v] = u;
                dist[v] = dist[u] + g->weight[u][v];
            }
    }

    if (dist[end] == INF) {
        printf("No path found\n");
    } else {
        printPath(parent, end);
        printf("\n%d\n", dist[end]);
    }
}

/* Extended Dijkstra function for Milestone 4 to pre-calculate and store paths for travelers */
void calculateTravelerPath(Graph* g, Traveler* t) {
    if (t->startNode == t->endNode) {
        t->path[0] = t->startNode;
        t->pathLength = 1;
        return;
    }

    int dist[MAX_NODES], visited[MAX_NODES], parent[MAX_NODES];
    for (int i = 0; i < g->numNodes; i++) {
        dist[i] = INF;
        visited[i] = 0;
        parent[i] = -1;
    }
    dist[t->startNode] = 0;

    for (int count = 0; count < g->numNodes - 1; count++) {
        int u = -1, min = INF;
        for (int i = 0; i < g->numNodes; i++)
            if (!visited[i] && dist[i] < min) { min = dist[i]; u = i; }

        if (u == -1) break;
        visited[u] = 1;

        for (int v = 0; v < g->numNodes; v++)
            if (!visited[v] && g->weight[u][v] != INF && dist[u] + g->weight[u][v] < dist[v]) {
                parent[v] = u;
                dist[v] = dist[u] + g->weight[u][v];
            }
    }

    if (dist[t->endNode] != INF) {
        storePath(parent, t->endNode, t);
    }
}