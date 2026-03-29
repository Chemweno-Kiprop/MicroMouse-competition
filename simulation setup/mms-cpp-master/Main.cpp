#include <iostream>
#include <string>
#include <queue>
#include "API.h"

using namespace std;

const int SIZE = 16;
int mazeDist[SIZE][SIZE];
bool walls[SIZE][SIZE][4] = {false}; 

int currX = 0;
int currY = 0;
int currDir = 0; // 0=North, 1=East, 2=South, 3=West

// Define the three phases of the run
enum State { SEARCHING, RETURNING, FAST_RUN, DONE };
State currentState = SEARCHING;

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

// Now accepts a target: true = center of maze, false = start square
void floodFill(bool targetIsCenter) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            mazeDist[i][j] = 255; 
        }
    }

    queue<pair<int, int>> q;
    
    if (targetIsCenter) {
        // Target the 2x2 destination area
        mazeDist[7][7] = 0; q.push({7, 7});
        mazeDist[7][8] = 0; q.push({7, 8});
        mazeDist[8][7] = 0; q.push({8, 7});
        mazeDist[8][8] = 0; q.push({8, 8});
    } else {
        // Target the start square
        mazeDist[0][0] = 0; q.push({0, 0});
    }

    while (!q.empty()) {
        int x = q.front().first;
        int y = q.front().second;
        q.pop();

        int nextDist = mazeDist[x][y] + 1;

        if (!walls[x][y][0] && y < SIZE - 1 && mazeDist[x][y + 1] == 255) {
            mazeDist[x][y + 1] = nextDist; q.push({x, y + 1});
        }
        if (!walls[x][y][1] && x < SIZE - 1 && mazeDist[x + 1][y] == 255) {
            mazeDist[x + 1][y] = nextDist; q.push({x + 1, y});
        }
        if (!walls[x][y][2] && y > 0 && mazeDist[x][y - 1] == 255) {
            mazeDist[x][y - 1] = nextDist; q.push({x, y - 1});
        }
        if (!walls[x][y][3] && x > 0 && mazeDist[x - 1][y] == 255) {
            mazeDist[x - 1][y] = nextDist; q.push({x - 1, y});
        }
        
        API::setText(x, y, to_string(mazeDist[x][y]));
    }
}

void moveRobot(int targetDir) {
    int turnDiff = targetDir - currDir;
    
    if (turnDiff == 1 || turnDiff == -3) {
        API::turnRight();
    } else if (turnDiff == -1 || turnDiff == 3) {
        API::turnLeft();
    } else if (turnDiff == 2 || turnDiff == -2) {
        API::turnRight();
        API::turnRight();
    }
    
    API::moveForward();
    
    currDir = targetDir;
    if (currDir == 0) currY++;
    if (currDir == 1) currX++;
    if (currDir == 2) currY--;
    if (currDir == 3) currX--;
}

int main(int argc, char* argv[]) {
    log("Phase 1: Searching for center...");
    
    for (int i = 0; i < SIZE; i++) {
        walls[i][0][2] = true;          
        walls[i][SIZE - 1][0] = true;   
        walls[0][i][3] = true;          
        walls[SIZE - 1][i][1] = true;   
    }

    while (currentState != DONE) {
        // Only update walls if we are exploring. During the fast run, trust the map!
        //if (currentState != FAST_RUN) {
        //    updateWalls();
        //}

        updateWalls(); // Always update walls to ensure we have the latest info for flood fill

        bool targetIsCenter = (currentState == SEARCHING || currentState == FAST_RUN);
        floodFill(targetIsCenter);

        // Check if we reached our current target
        if (mazeDist[currX][currY] == 0) {
            if (currentState == SEARCHING) {
                log("Center reached! Phase 2: Returning to start...");
                currentState = RETURNING;
                continue; // Recalculate flood fill immediately for the return trip
            } 
            else if (currentState == RETURNING) {
                log("Back at start! Phase 3: Initiating FAST RUN...");
                API::clearAllColor(); // Clear the map colors for the final sprint
                currentState = FAST_RUN;
                continue; 
            } 
            else if (currentState == FAST_RUN) {
                log("Fast run complete! Mission accomplished.");
                currentState = DONE;
                break;
            }
        }

        int bestDist = 255;
        int nextDir = currDir; 

        if (!walls[currX][currY][0] && mazeDist[currX][currY + 1] < bestDist) {
            bestDist = mazeDist[currX][currY + 1]; nextDir = 0;
        }
        if (!walls[currX][currY][1] && mazeDist[currX + 1][currY] < bestDist) {
            bestDist = mazeDist[currX + 1][currY]; nextDir = 1;
        }
        if (!walls[currX][currY][2] && mazeDist[currX][currY - 1] < bestDist) {
            bestDist = mazeDist[currX][currY - 1]; nextDir = 2;
        }
        if (!walls[currX][currY][3] && mazeDist[currX - 1][currY] < bestDist) {
            bestDist = mazeDist[currX - 1][currY]; nextDir = 3;
        }

        moveRobot(nextDir);
        
        // Color code the trail based on the current phase
        if (currentState == SEARCHING) API::setColor(currX, currY, 'Y');      // Yellow for search
        else if (currentState == RETURNING) API::setColor(currX, currY, 'B'); // Blue for return
        else if (currentState == FAST_RUN) API::setColor(currX, currY, 'G');  // Green for FAST RUN
    }
    
    return 0;
}