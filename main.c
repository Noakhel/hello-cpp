#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graph.h"
#include "raylib.h"
#include <math.h>
#include <signal.h> // נוסף עבור פקודת kill

// ספריות עבור ניהול תהליכים, צינורות ותקשורת לא חוסמת
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

#define NUM_TRAVELERS 3

// סוגי הודעות כדי שהאבא ידע מה הסטטוס המדויק
typedef enum {
    MSG_WAITING,   // הילד ממתין להיכנס לצומת
    MSG_TRAVELING, // הילד קיבל אישור ונכנס לצומת
    MSG_RELEASE    // הילד סיים ועוזב את הצומת
} MsgType;

// הגדרת מבנה ההודעה שהבנים ישלחו לאב
typedef struct {
    pid_t pid;
    int child_id;      // המזהה של הילד (0 עד NUM_TRAVELERS-1)
    int current_node;
    int next_node;
    int is_finished;
    int priority;      // העדיפות עבור אלגוריתם SJF
    MsgType type;
} TravelerMessage;

// מבנה פנימי של האבא כדי לשמור את מצב הנוסעים
typedef struct {
    pid_t pid;
    float posX, posY;
    int current_node;
    int next_node;
    int active;
    int finished;
    int waiting;
    int priority;
} GuiTraveler;

// פונקציית עזר פנימית שמחלצת את המסלול האמיתי
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

int main(int argc, char *argv[]) {
    // 1. קריאת הארגומנטים משורת הפקודה
    int is_sjf = 0; // 0 = FCFS, 1 = SJF
    char filename[256] = "input.txt"; // קובץ ברירת מחדל

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-schd") == 0 && i + 1 < argc) {
            if (strcmp(argv[i+1], "sjf") == 0) {
                is_sjf = 1;
            } else if (strcmp(argv[i+1], "fcfs") == 0) {
                is_sjf = 0;
            }
            i++; // דילוג על הארגומנט הבא (fcfs/sjf)
        } else {
            strcpy(filename, argv[i]);
        }
    }

    Graph myGraph;
    int dummyStart, dummyEnd;

    readGraph(filename, &myGraph, &dummyStart, &dummyEnd);
    AssignNodePositions(&myGraph);

    // 2. סמפור אישי לכל תהליך (ילד) במקום סמפור לצומת. מאותחל ל-0 כדי שיחסמו מיד.
    sem_t *traveler_sems = mmap(NULL, sizeof(sem_t) * NUM_TRAVELERS,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (traveler_sems == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    for (int i = 0; i < NUM_TRAVELERS; i++) {
        sem_init(&traveler_sems[i], 1, 0);
    }

    int sources[NUM_TRAVELERS] = {0, 1, 2};
    int dests[NUM_TRAVELERS] = {4, 4, 4};
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

            dijkstra(&myGraph, sources[i], dests[i]);

            int myPath[MAX_NODES];
            int myPathLength = getTravelerPath(&myGraph, sources[i], dests[i], myPath);

            // עדיפות לדוגמה: אפשר לשנות לקריאה מהקובץ. מספר קטן = עדיפות גבוהה
            int my_priority = (i % 3) + 1;

            for (int pathIdx = 0; pathIdx < myPathLength; pathIdx++) {
                TravelerMessage msg;
                msg.pid = getpid();
                msg.child_id = i;
                msg.current_node = myPath[pathIdx];
                msg.next_node = (pathIdx < myPathLength - 1) ? myPath[pathIdx + 1] : -1;
                msg.is_finished = (pathIdx == myPathLength - 1) ? 1 : 0;
                msg.priority = my_priority;

                // בקשת כניסה לצומת
                msg.type = MSG_WAITING;
                write(pipes[i][1], &msg, sizeof(TravelerMessage));

                // ממתין שהאבא (המתזמן) יעיר אותי
                sem_wait(&traveler_sems[i]);

                // אני בפנים! מעדכן את האבא שאני נוסע
                msg.type = MSG_TRAVELING;
                write(pipes[i][1], &msg, sizeof(TravelerMessage));

                if (msg.is_finished) {
                    msg.type = MSG_RELEASE; // הגעתי ליעד, משחרר את הצומת
                    write(pipes[i][1], &msg, sizeof(TravelerMessage));
                    break;
                }

                int weight = myGraph.weight[msg.current_node][msg.next_node];
                usleep(weight * 300000);
                usleep(1000000);

                // מסיים בצומת ומשחרר אותו כדי שהאבא יעיר את הבא בתור
                msg.type = MSG_RELEASE;
                write(pipes[i][1], &msg, sizeof(TravelerMessage));
            }

            close(pipes[i][1]);
            exit(0);
        } else { // --------- קוד האב ---------
            child_pids[i] = pid;
            close(pipes[i][1]);

            int flags = fcntl(pipes[i][0], F_GETFL, 0);
            fcntl(pipes[i][0], F_SETFL, flags | O_NONBLOCK);
        }
    }

    // --------- קוד האב (ניהול ה-GUI והמתזמן) ---------
    InitWindow(800, 600, "Graph Project - Milestone 7");
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
        gui_travelers[i].priority = 0;
    }

    // מבני הנתונים של המתזמן (האב)
    int node_busy[MAX_NODES] = {0}; // 0 = פנוי, 1 = תפוס
    int wait_queue[MAX_NODES][NUM_TRAVELERS]; // תור המתנה לכל צומת
    int wait_count[MAX_NODES] = {0}; // כמות הממתינים בכל צומת

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        // 1. קריאת הודעות מכל הבנים
        for (int i = 0; i < NUM_TRAVELERS; i++) {
            if (!gui_travelers[i].active) continue;

            TravelerMessage receivedMsg;
            ssize_t bytesRead = read(pipes[i][0], &receivedMsg, sizeof(TravelerMessage));

            if (bytesRead > 0) {
                int node = receivedMsg.current_node;
                gui_travelers[i].priority = receivedMsg.priority;

                if (receivedMsg.type == MSG_WAITING) {
                    gui_travelers[i].waiting = 1;
                    // הכנסה לתור ההמתנה של הצומת המבוקש
                    wait_queue[node][wait_count[node]] = i;
                    wait_count[node]++;
                }
                else if (receivedMsg.type == MSG_TRAVELING) {
                    gui_travelers[i].waiting = 0;
                    gui_travelers[i].current_node = receivedMsg.current_node;
                    gui_travelers[i].next_node = receivedMsg.next_node;

                    if (receivedMsg.is_finished) {
                        printf("[PID=%d] arrived at node %d | DESTINATION\n", receivedMsg.pid, receivedMsg.current_node);
                        gui_travelers[i].finished = 1;
                        gui_travelers[i].posX = myGraph.x[receivedMsg.current_node];
                        gui_travelers[i].posY = myGraph.y[receivedMsg.current_node];
                    } else {
                        printf("[PID=%d] arrived at node %d | next node: %d\n", receivedMsg.pid, receivedMsg.current_node, receivedMsg.next_node);
                    }
                }
                else if (receivedMsg.type == MSG_RELEASE) {
                    // שחרור הצומת
                    node_busy[node] = 0;

                    // הפסקת האזנה לילד רק אחרי שהצומת שוחרר בהצלחה
                    if (receivedMsg.is_finished) {
                        gui_travelers[i].active = 0;
                    }
                }
            }
        }

        // 2. לוגיקת המתזמן (Scheduler) - רץ על כל הצמתים ומעיר בנים במידת הצורך
        for (int n = 0; n < myGraph.numNodes; n++) {
            if (!node_busy[n] && wait_count[n] > 0) {
                int chosen_idx = 0; // ברירת מחדל לאינדקס הראשון (FCFS)

                if (is_sjf) {
                    // SJF: חיפוש הילד עם העדיפות הטובה ביותר (הערך הנמוך ביותר)
                    int min_prio = 999999;
                    for (int q = 0; q < wait_count[n]; q++) {
                        int cid = wait_queue[n][q];
                        if (gui_travelers[cid].priority < min_prio) {
                            min_prio = gui_travelers[cid].priority;
                            chosen_idx = q;
                        }
                    }
                }

                int chosen_child = wait_queue[n][chosen_idx];

                // הוצאת הילד מהתור והזזת שאר הממתינים שמאלה
                for (int q = chosen_idx; q < wait_count[n] - 1; q++) {
                    wait_queue[n][q] = wait_queue[n][q + 1];
                }
                wait_count[n]--;

                // סימון הצומת כתפוס והערת הילד הנבחר
                node_busy[n] = 1;
                sem_post(&traveler_sems[chosen_child]);
            }
        }

        // 3. עדכון מיקומי אנימציה ב-GUI
        for (int i = 0; i < NUM_TRAVELERS; i++) {
            if (gui_travelers[i].active && !gui_travelers[i].finished && gui_travelers[i].next_node != -1 && !gui_travelers[i].waiting) {
                int to = gui_travelers[i].next_node;
                float targetX = myGraph.x[to];
                float targetY = myGraph.y[to];
                gui_travelers[i].posX += (targetX - gui_travelers[i].posX) * deltaTime * 2.0f;
                gui_travelers[i].posY += (targetY - gui_travelers[i].posY) * deltaTime * 2.0f;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        // תצוגת המתזמן הפעיל בפינה העליונה
        DrawText(TextFormat("Current Scheduler: %s", is_sjf ? "SJF (Shortest Job First)" : "FCFS (First Come First Serve)"), 20, 20, 20, DARKBLUE);

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

            // הצגת עדיפות מעל הנוסע אם זה SJF
            if (is_sjf) {
                DrawText(TextFormat("pr:%d", gui_travelers[i].priority), gui_travelers[i].posX - 12, gui_travelers[i].posY - 22, 10, BLACK);
            }
        }

        int allFinished = 1;
        for(int i=0; i<NUM_TRAVELERS; i++) { if(!gui_travelers[i].finished) allFinished = 0; }
        if (allFinished) {
            DrawText("ALL TRAVELERS REACHED DESTINATION!", 120, 60, 25, DARKGREEN);
        }

        EndDrawing();
    }

    // איסוף הבנים והדפסת finished בסיום
    for (int i = 0; i < NUM_TRAVELERS; i++) {
        // מנגנון בטיחות: אם סגרת את החלון לפני שהם סיימו, נחסל את התהליכים כדי שלא ניתקע
        if (!gui_travelers[i].finished) {
            kill(child_pids[i], SIGKILL);
        }

        close(pipes[i][0]);
        waitpid(child_pids[i], NULL, 0);
        printf("[PID=%d] finished\n", child_pids[i]);
    }

    CloseWindow();
    return 0;
}