#include <stdio.h>
#include <stdlib.h>
#include "graph.h"
#include "raylib.h"
#include <math.h>

void AssignNodePositions(Graph *g) {
    int centerX = 400, centerY = 300, radius = 200;
    for (int i = 0; i < g->numNodes; i++) {
        double angle = i * (2 * PI / g->numNodes);
        g->x[i] = centerX + radius * cos(angle);
        g->y[i] = centerY + radius * sin(angle);
    }
}

void DrawArrow(Vector2 start, Vector2 end, Color color) {
    float lineThickness = 2.0f;
    DrawLineEx(start, end, lineThickness, color);
    float angle = atan2f(end.y - start.y, end.x - start.x);
    float arrowSize = 15.0f;
    DrawLineEx(end, (Vector2){ end.x - arrowSize * cosf(angle - PI/6), end.y - arrowSize * sinf(angle - PI/6) }, lineThickness, color);
    DrawLineEx(end, (Vector2){ end.x - arrowSize * cosf(angle + PI/6), end.y - arrowSize * sinf(angle + PI/6) }, lineThickness, color);
}

int main() {
    Graph myGraph;
    int startNode, endNode;

    readGraph("input.txt", &myGraph, &startNode, &endNode);
    AssignNodePositions(&myGraph);

    InitWindow(800, 600, "Graph Project - Milestone 3");
    SetTargetFPS(60);

    // קריאה לפונקציה המקורית שלך
    dijkstra(&myGraph, startNode, endNode);

    // הגדרת המסלול לאנימציה (מותאם לקובץ הקלט שלך)
    int path[] = {0, 2, 5}; 
    int pathLength = 3;
    int pathIndex = 0;

    float playerX = myGraph.x[path[0]];
    float playerY = myGraph.y[path[0]];
    
    int playing = 0; 
    int isWaiting = 0;
    float timer = 0.0f;

    Rectangle button = {20, 20, 100, 40};

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (CheckCollisionPointRec(GetMousePosition(), button)) {
                playing = !playing;
                if (pathIndex >= pathLength - 1) {
                    pathIndex = 0;
                    timer = 0;
                    isWaiting = 0;
                    playerX = myGraph.x[path[0]];
                    playerY = myGraph.y[path[0]];
                }
            }
        }

        if (playing && pathIndex < pathLength - 1) {
            int from = path[pathIndex];
            int to = path[pathIndex + 1];
            int weight = myGraph.weight[from][to];

            timer += deltaTime;

            if (isWaiting) {
                if (timer >= 1.0f) { // המתנה של שניה בצומת
                    isWaiting = 0;
                    timer = 0;
                }
            } else {
                float totalTimeForEdge = weight * 0.3f; // 300ms לכל יחידת משקל
                float t = timer / totalTimeForEdge;

                if (t >= 1.0f) {
                    playerX = myGraph.x[to];
                    playerY = myGraph.y[to];
                    pathIndex++;
                    timer = 0;
                    if (pathIndex < pathLength - 1) isWaiting = 1;
                } else {
                    playerX = myGraph.x[from] + (myGraph.x[to] - myGraph.x[from]) * t;
                    playerY = myGraph.y[from] + (myGraph.y[to] - myGraph.y[from]) * t;
                }
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        for (int i = 0; i < myGraph.numNodes; i++) {
            for (int j = 0; j < myGraph.numNodes; j++) {
                if (myGraph.weight[i][j] > 0 && myGraph.weight[i][j] < 1000) {
                    Vector2 start = {(float)myGraph.x[i], (float)myGraph.y[i]};
                    Vector2 end = {(float)myGraph.x[j], (float)myGraph.y[j]};
                    float angle = atan2f(end.y - start.y, end.x - start.x);
                    Vector2 edgePoint = {end.x - 25 * cosf(angle), end.y - 25 * sinf(angle)};
                    DrawArrow(start, edgePoint, GRAY);
                    DrawText(TextFormat("%d", myGraph.weight[i][j]), (start.x + end.x) / 2, (start.y + end.y) / 2, 15, DARKGRAY);
                }
            }
        }

        for (int i = 0; i < myGraph.numNodes; i++) {
            Color nodeColor = (i == startNode) ? GREEN : (i == endNode) ? RED : BLUE;
            DrawCircle(myGraph.x[i], myGraph.y[i], 25, nodeColor);
            DrawText(TextFormat("%d", i), myGraph.x[i] - 5, myGraph.y[i] - 8, 20, WHITE);
        }

        DrawCircle(playerX, playerY, 15, BLACK);
        DrawRectangleRec(button, LIGHTGRAY);
        DrawText(playing ? "STOP" : "START", button.x + 20, button.y + 10, 20, BLACK);

        if (pathIndex >= pathLength - 1) {
            DrawText("DESTINATION REACHED!", 250, 50, 30, DARKGREEN);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
