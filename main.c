#include <stdio.h>
#include <stdlib.h>
#include "graph.h"
#include "raylib.h"
#include <math.h>

// ספריות עבור ניהול תהליכים, צינורות ותקשורת לא חוסמת
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

#define NUM_TRAVELERS 3

// הגדרת מבנה ההודעה שהבנים ישלחו לאב
typedef struct {
    pid_t pid;
    int current_node;
    int next_node;
    int is_finished;
    int is_waiting;
} TravelerMessage;

// מבנה פנימי של האבא כדי לשמור את מצב הנוסעים לצורך הציור ב-GUI
typedef struct {
    pid_t pid;
    float posX, posY;
    int current_node;
    int next_node;
    int active;
    int finished;
    int waiting;
} GuiTraveler;

// פונקציית עזר פנימית שמחלצת את המסלול האמיתי עבור הבן מתוך הגרף שלכן
int getTravelerPath(Graph* g, int start, int end, int path[]) {
    if (start == end) {
        path[0] = start;
        return 1;
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

    if (dist[end] == INF) return 0;

    int tempPath[MAX_NODES];
    int count = 0;
    int curr = end;
    while (curr != -1) {
        tempPath[count++] = curr;
        curr = parent[curr];
    }
    for (int i = 0; i < count; i++) {
        path[i] = tempPath[count - 1 - i];
    }
    return count;
}

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
    int dummyStart, dummyEnd;

    // קריאת הגרף מקובץ הקלט
    readGraph("input.txt", &myGraph, &dummyStart, &dummyEnd);
    AssignNodePositions(&myGraph);
    sem_t *node_sems = mmap(NULL, sizeof(sem_t) * myGraph.numNodes,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (node_sems == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    for (int i = 0; i < myGraph.numNodes; i++) {
        sem_init(&node_sems[i], 1, 1);
    }

    // נקודות מוצא ויעד אמיתיות מתוך קובץ הדוגמה
    int sources[NUM_TRAVELERS] = {0, 1, 2};
    int dests[NUM_TRAVELERS] = {4, 4, 4};

    // יצירת צינורות (Pipes)
    int pipes[NUM_TRAVELERS][2];
    for (int i = 0; i < NUM_TRAVELERS; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("Failed to create pipe");
            return 1;
        }
    }

    pid_t child_pids[NUM_TRAVELERS];

    for (int i = 0; i < NUM_TRAVELERS; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            return 1;
        }

        if (pid == 0) { // --------- קוד הבן ---------
            close(pipes[i][0]);
            for (int j = 0; j < NUM_TRAVELERS; j++) {
                if (j != i) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
            }

            // קריאה לפונקציית דייקסטרה המקורית
            dijkstra(&myGraph, sources[i], dests[i]);

            int myPath[MAX_NODES];
            int myPathLength = getTravelerPath(&myGraph, sources[i], dests[i], myPath);

            // סימולציית הנסיעה הדינמית של הבן
            for (int pathIdx = 0; pathIdx < myPathLength; pathIdx++) {
                TravelerMessage msg;
                msg.pid = getpid();
                msg.current_node = myPath[pathIdx];

                if (pathIdx < myPathLength - 1) {
                    msg.next_node = myPath[pathIdx + 1];
                    msg.is_finished = 0;
                } else {
                    msg.next_node = -1;
                    msg.is_finished = 1;
                }
                msg.is_waiting = 1;
                write(pipes[i][1], &msg, sizeof(TravelerMessage));

                sem_wait(&node_sems[msg.current_node]);

                msg.is_waiting = 0;
                write(pipes[i][1], &msg, sizeof(TravelerMessage));
                
                // --- התיקון המרכזי כאן ---
                if (msg.is_finished) {
                    sem_post(&node_sems[msg.current_node]); // שחרור המנעול בצומת היעד לפני סיום!
                    break;
                }

                int weight = myGraph.weight[msg.current_node][msg.next_node];
                usleep(weight * 300000); // 300ms לכל יחידת משקל
                usleep(1000000); // המתנה של שניה בצומת
                sem_post(&node_sems[msg.current_node]);
            }

            close(pipes[i][1]);
            exit(0);
        } else {
            child_pids[i] = pid;
            close(pipes[i][1]);

            // הפיכת הצינור ללא-חוסם (Non-blocking)
            int flags = fcntl(pipes[i][0], F_GETFL, 0);
            fcntl(pipes[i][0], F_SETFL, flags | O_NONBLOCK);
        }
    }

    // --------- קוד האב (ניהול ה-GUI) ---------
    InitWindow(800, 600, "Graph Project - Milestone 6");
    SetTargetFPS(60);

    GuiTraveler gui_travelers[NUM_TRAVELERS];
    for (int i = 0; i < NUM_TRAVELERS; i++) {
        gui_travelers[i].pid = child_pids[i];
        gui_travelers[i].current_node = sources[i];
        gui_travelers[i].next_node = sources[i];
        gui_travelers[i].posX = myGraph.x[sources[i]];
        gui_travelers[i].posY = myGraph.y[sources[i]];
        gui_travelers[i].active = 1;
        gui_travelers[i].finished = 0;
        gui_travelers[i].waiting = 0;
    }

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        for (int i = 0; i < NUM_TRAVELERS; i++) {
            if (!gui_travelers[i].active) continue;

            TravelerMessage receivedMsg;
            ssize_t bytesRead = read(pipes[i][0], &receivedMsg, sizeof(TravelerMessage));

            if (bytesRead > 0) {
                gui_travelers[i].current_node = receivedMsg.current_node;
                gui_travelers[i].next_node = receivedMsg.next_node;
                gui_travelers[i].waiting = receivedMsg.is_waiting;

                if (receivedMsg.is_finished) {
                    printf("[PID=%d] arrived at node %d | DESTINATION\n", receivedMsg.pid, receivedMsg.current_node);
                    gui_travelers[i].finished = 1;
                    gui_travelers[i].active = 0;
                    gui_travelers[i].posX = myGraph.x[receivedMsg.current_node];
                    gui_travelers[i].posY = myGraph.y[receivedMsg.current_node];
                } else {
                    printf("[PID=%d] arrived at node %d | next node: %d\n", receivedMsg.pid, receivedMsg.current_node, receivedMsg.next_node);
                }
            }
        }

        for (int i = 0; i < NUM_TRAVELERS; i++) {
            if (gui_travelers[i].active && !gui_travelers[i].finished && gui_travelers[i].next_node != -1) {
                int to = gui_travelers[i].next_node;

                float targetX = myGraph.x[to];
                float targetY = myGraph.y[to];
                gui_travelers[i].posX += (targetX - gui_travelers[i].posX) * deltaTime * 2.0f;
                gui_travelers[i].posY += (targetY - gui_travelers[i].posY) * deltaTime * 2.0f;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        // ציור קשתות
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

        // ציור צמתים
        for (int i = 0; i < myGraph.numNodes; i++) {
            DrawCircle(myGraph.x[i], myGraph.y[i], 25, BLUE);
            DrawText(TextFormat("%d", i), myGraph.x[i] - 5, myGraph.y[i] - 8, 20, WHITE);
        }

        // ציור נוסעים
        for (int i = 0; i < NUM_TRAVELERS; i++) {
            Color travelerColor;

            if (gui_travelers[i].waiting) {
                travelerColor = ORANGE;
            } else if (i == 0) {
                travelerColor = GREEN;
            } else if (i == 1) {
                travelerColor = PURPLE;
            } else {
                travelerColor = RED;
            }
            DrawCircle(gui_travelers[i].posX, gui_travelers[i].posY, 15, travelerColor);
            DrawText(TextFormat("P%d", i+1), gui_travelers[i].posX - 10, gui_travelers[i].posY - 6, 12, WHITE);
        }

        int allFinished = 1;
        for(int i=0; i<NUM_TRAVELERS; i++) { if(!gui_travelers[i].finished) allFinished = 0; }
        if (allFinished) {
            DrawText("ALL TRAVELERS REACHED DESTINATION!", 120, 40, 25, DARKGREEN);
        }

        EndDrawing();
    }

    // איסוף הבנים והדפסת finished בסיום
    for (int i = 0; i < NUM_TRAVELERS; i++) {
        close(pipes[i][0]);
        waitpid(child_pids[i], NULL, 0);
        printf("[PID=%d] finished\n", child_pids[i]);
    }

    CloseWindow();
    return 0;
}

