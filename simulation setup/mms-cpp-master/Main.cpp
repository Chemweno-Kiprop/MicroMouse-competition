#include <iostream>
#include <string>
#include <queue>
#include "API.h"

using namespace std;

const int SIZE = 16;
int mazeDist[SIZE][SIZE];
bool walls[SIZE][SIZE][4] = {false}; 
int visited[SIZE][SIZE] = {0}; // Tracks how many times a cell is visited

int currX = 0;
int currY = 0;
int currDir = 0; // 0=North, 1=East, 2=South, 3=West

// Upgraded State Machine
enum State { SEARCH_CENTER, SEARCH_START, FAST_RUN, DONE };
State currentState = SEARCH_CENTER;

// Multi-run tracking
int runCount = 0;
const int MAX_EXPLORE_RUNS = 2; // 2 full round trips = 4 traversals before the final sprint

void log(const string& text) {
    cerr << text << endl;
}

void updateWalls() {
    bool wallF = API::wallFront();
    bool wallR = API::wallRight();
    bool wallL = API::wallLeft();

    if (wallF) walls[currX][currY][currDir] = true;
    if (wallR) walls[currX][currY][(currDir + 1) % 4] = true;
    if (wallL) walls[currX][currY][(currDir + 3) % 4] = true;

    // Symmetrically update neighbors
    if (wallF) {
        if (currDir == 0 && currY < SIZE - 1) walls[currX][currY + 1][2] = true;
        if (currDir == 1 && currX < SIZE - 1) walls[currX + 1][currY][3] = true;
        if (currDir == 2 && currY > 0)        walls[currX][currY - 1][0] = true;
        if (currDir == 3 && currX > 0)        walls[currX - 1][currY][1] = true;
    }
    if (wallR) {
        int rDir = (currDir + 1) % 4;
        if (rDir == 0 && currY < SIZE - 1) walls[currX][currY + 1][2] = true;
        if (rDir == 1 && currX < SIZE - 1) walls[currX + 1][currY][3] = true;
        if (rDir == 2 && currY > 0)        walls[currX][currY - 1][0] = true;
        if (rDir == 3 && currX > 0)        walls[currX - 1][currY][1] = true;
    }
    if (wallL) {
        int lDir = (currDir + 3) % 4;
        if (lDir == 0 && currY < SIZE - 1) walls[currX][currY + 1][2] = true;
        if (lDir == 1 && currX < SIZE - 1) walls[currX + 1][currY][3] = true;
        if (lDir == 2 && currY > 0)        walls[currX][currY - 1][0] = true;
        if (lDir == 3 && currX > 0)        walls[currX - 1][currY][1] = true;
    }
}

void floodFill(bool targetIsCenter) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            mazeDist[i][j] = 255; 
        }
    }

    queue<pair<int, int>> q;
    
    if (targetIsCenter) {
        mazeDist[7][7] = 0; q.push({7, 7});
        mazeDist[7][8] = 0; q.push({7, 8});
        mazeDist[8][7] = 0; q.push({8, 7});
        mazeDist[8][8] = 0; q.push({8, 8});
    } else {
        mazeDist[0][0] = 0; q.push({0, 0});
    }

    while (!q.empty()) {
        int x = q.front().first;
        int y = q.front().second;
        q.pop();

        int nextDist = mazeDist[x][y] + 1;

        if (!walls[x][y][0] && y < SIZE - 1 && mazeDist[x][y + 1] == 255) { mazeDist[x][y + 1] = nextDist; q.push({x, y + 1}); }
        if (!walls[x][y][1] && x < SIZE - 1 && mazeDist[x + 1][y] == 255) { mazeDist[x + 1][y] = nextDist; q.push({x + 1, y}); }
        if (!walls[x][y][2] && y > 0 && mazeDist[x][y - 1] == 255) { mazeDist[x][y - 1] = nextDist; q.push({x, y - 1}); }
        if (!walls[x][y][3] && x > 0 && mazeDist[x - 1][y] == 255) { mazeDist[x - 1][y] = nextDist; q.push({x - 1, y}); }
        
        API::setText(x, y, to_string(mazeDist[x][y]));
    }
}

void moveRobot(int targetDir) {
    int turnDiff = targetDir - currDir;
    
    if (turnDiff == 1 || turnDiff == -3) { API::turnRight(); } 
    else if (turnDiff == -1 || turnDiff == 3) { API::turnLeft(); } 
    else if (turnDiff == 2 || turnDiff == -2) { API::turnRight(); API::turnRight(); }
    
    API::moveForward();
    
    currDir = targetDir;
    if (currDir == 0) currY++;
    if (currDir == 1) currX++;
    if (currDir == 2) currY--;
    if (currDir == 3) currX--;

    // Log this cell as visited to satisfy the robot's curiosity
    visited[currX][currY]++;
}

int main(int argc, char* argv[]) {
    log("Starting Multi-Run Exploration Strategy...");
    
    for (int i = 0; i < SIZE; i++) {
        walls[i][0][2] = true;          
        walls[i][SIZE - 1][0] = true;   
        walls[0][i][3] = true;          
        walls[SIZE - 1][i][1] = true;   
    }

    // Mark start square as visited
    visited[0][0] = 1;

    while (currentState != DONE) {
        
        // Eyes open at all times to prevent optimistic crashing
        updateWalls();

        bool targetIsCenter = (currentState == SEARCH_CENTER || currentState == FAST_RUN);
        floodFill(targetIsCenter);

        // State Machine & Run Counter Logic
        if (mazeDist[currX][currY] == 0) {
            if (currentState == SEARCH_CENTER) {
                log("Center reached! Navigating back to start...");
                currentState = SEARCH_START;
                continue; 
            } 
            else if (currentState == SEARCH_START) {
                runCount++;
                log("Completed Round Trip: " + to_string(runCount));
                
                if (runCount >= MAX_EXPLORE_RUNS) {
                    log("Exploration phase complete. Initiating FAST RUN!");
                    API::clearAllColor(); 
                    currentState = FAST_RUN;
                } else {
                    log("Initiating secondary exploration run...");
                    currentState = SEARCH_CENTER;
                }
                continue; 
            } 
            else if (currentState == FAST_RUN) {
                log("Fast run complete! Mission accomplished.");
                currentState = DONE;
                break;
            }
        }

        // Selection Logic with Curiosity Tie-Breaker
        int bestDist = 255;
        int minVisited = 10000;
        int nextDir = currDir; 

        // North
        if (!walls[currX][currY][0]) {
            int d = mazeDist[currX][currY + 1];
            int v = visited[currX][currY + 1];
            if (d < bestDist || (d == bestDist && v < minVisited)) {
                bestDist = d; minVisited = v; nextDir = 0;
            }
        }
        // East
        if (!walls[currX][currY][1]) {
            int d = mazeDist[currX + 1][currY];
            int v = visited[currX + 1][currY];
            if (d < bestDist || (d == bestDist && v < minVisited)) {
                bestDist = d; minVisited = v; nextDir = 1;
            }
        }
        // South
        if (!walls[currX][currY][2]) {
            int d = mazeDist[currX][currY - 1];
            int v = visited[currX][currY - 1];
            if (d < bestDist || (d == bestDist && v < minVisited)) {
                bestDist = d; minVisited = v; nextDir = 2;
            }
        }
        // West
        if (!walls[currX][currY][3]) {
            int d = mazeDist[currX - 1][currY];
            int v = visited[currX - 1][currY];
            if (d < bestDist || (d == bestDist && v < minVisited)) {
                bestDist = d; minVisited = v; nextDir = 3;
            }
        }

        moveRobot(nextDir);
        
        // Visuals
        if (currentState == SEARCH_CENTER) API::setColor(currX, currY, 'Y'); 
        else if (currentState == SEARCH_START) API::setColor(currX, currY, 'B'); 
        else if (currentState == FAST_RUN) API::setColor(currX, currY, 'G'); 
    }
    
    return 0;
}