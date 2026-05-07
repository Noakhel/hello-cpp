#include "graph.h"
#include <stdio.h>
#include <stdlib.h>


// הדפסת מסלול בפורמט: 0 -> 2 -> 5
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

    fscanf(file, "%d %d", start, end);
    fclose(file);
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