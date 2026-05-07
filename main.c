#include <stdio.h>
#include <stdlib.h>
#include "graph.h"
#include "raylib.h"
#include <math.h>

void AssignNodePositions(Graph *g) {
    int centerX = 400, centerY = 300, radius = 200;
    for (int i = 0; i < g->numNodes; i++) {
        // חישוב מיקום על מעגל
        double angle = i * (2 * PI / g->numNodes);
        g->x[i] = centerX + radius * cos(angle);
        g->y[i] = centerY + radius * sin(angle);
    }
}

void DrawArrow(Vector2 start, Vector2 end, Color color) {
    float lineThickness = 2.0f;
    DrawLineEx(start, end, lineThickness, color);

    // Calculate the angle of the line
    float angle = atan2f(end.y - start.y, end.x - start.x);
    float arrowSize = 15.0f;

    // Draw two lines to create the arrow head at the end point
    DrawLineEx(end, (Vector2){ end.x - arrowSize * cosf(angle - PI/6),
                               end.y - arrowSize * sinf(angle - PI/6) }, lineThickness, color);
    DrawLineEx(end, (Vector2){ end.x - arrowSize * cosf(angle + PI/6),
                               end.y - arrowSize * sinf(angle + PI/6) }, lineThickness, color);
}

int main() {
    Graph myGraph;
    int startNode, endNode;


    readGraph("input.txt", &myGraph, &startNode, &endNode);


    AssignNodePositions(&myGraph);


    InitWindow(800, 600, "Graph Project - Milestone 2");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);


        for (int i = 0; i < myGraph.numNodes; i++) {
            for (int j = 0; j < myGraph.numNodes; j++) {
                // Inside the drawing loop:
                if (myGraph.weight[i][j] > 0 && myGraph.weight[i][j] < 1000) {
                    Vector2 start = { (float)myGraph.x[i], (float)myGraph.y[i] };
                    Vector2 end = { (float)myGraph.x[j], (float)myGraph.y[j] };

                    // Calculate the distance to "pull back" the arrow head so it sits on the circle's edge
                    float angle = atan2f(end.y - start.y, end.x - start.x);
                    int radius = 25; // The radius of your nodes
                    Vector2 edgePoint = { end.x - radius * cosf(angle), end.y - radius * sinf(angle) };

                    DrawArrow(start, edgePoint, GRAY);

                    // Draw the weight text
                    int midX = (start.x + end.x) / 2;
                    int midY = (start.y + end.y) / 2;
                    DrawText(TextFormat("%d", myGraph.weight[i][j]), midX, midY, 15, DARKGRAY);
                }
            }
        }


        for (int i = 0; i < myGraph.numNodes; i++) {
            Color nodeColor = BLUE;
            if (i == startNode) nodeColor = GREEN; // צומת התחלה בירוק
            if (i == endNode) nodeColor = RED;     // צומת סיום באדום

            DrawCircle(myGraph.x[i], myGraph.y[i], 25, nodeColor);
            DrawText(TextFormat("%d", i), myGraph.x[i] - 5, myGraph.y[i] - 8, 20, WHITE);
        }

        EndDrawing();
    }


    CloseWindow();
    dijkstra(&myGraph, startNode, endNode);

    return 0;
}
