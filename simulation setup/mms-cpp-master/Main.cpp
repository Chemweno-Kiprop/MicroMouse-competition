#include <iostream>
#include <string>
#include <queue>
#include "API.h"

using namespace std;

const int SIZE = 16;
int mazeDist[SIZE][SIZE];
// Array to track walls: 0=North, 1=East, 2=South, 3=West
bool walls[SIZE][SIZE][4] = {false}; 

int currX = 0;
int currY = 0;
int currDir = 0; // 0=North, 1=East, 2=South, 3=West

void log(const string& text) {
    cerr << text << endl;
}

// Map the robot's local sensors to the global maze grid
void updateWalls() {
    bool wallF = API::wallFront();
    bool wallR = API::wallRight();
    bool wallL = API::wallLeft();

    if (wallF) walls[currX][currY][currDir] = true;
    if (wallR) walls[currX][currY][(currDir + 1) % 4] = true;
    if (wallL) walls[currX][currY][(currDir + 3) % 4] = true;

    // Symmetrically update neighboring cells so the water "blocks" from both sides
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

// Calculate the shortest path from the center to all reachable cells
void floodFill() {
    // Reset all distances to a high number
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            mazeDist[i][j] = 255; 
        }
    }

    queue<pair<int, int>> q;
    
    // The destination is the central 2x2 area (Rules Section 2.4)
    mazeDist[7][7] = 0; q.push({7, 7});
    mazeDist[7][8] = 0; q.push({7, 8});
    mazeDist[8][7] = 0; q.push({8, 7});
    mazeDist[8][8] = 0; q.push({8, 8});

    // Standard Breadth-First Search using std::queue
    while (!q.empty()) {
        int x = q.front().first;
        int y = q.front().second;
        q.pop();

        int nextDist = mazeDist[x][y] + 1;

        // Propagate North
        if (!walls[x][y][0] && y < SIZE - 1 && mazeDist[x][y + 1] == 255) {
            mazeDist[x][y + 1] = nextDist;
            q.push({x, y + 1});
        }
        // Propagate East
        if (!walls[x][y][1] && x < SIZE - 1 && mazeDist[x + 1][y] == 255) {
            mazeDist[x + 1][y] = nextDist;
            q.push({x + 1, y});
        }
        // Propagate South
        if (!walls[x][y][2] && y > 0 && mazeDist[x][y - 1] == 255) {
            mazeDist[x][y - 1] = nextDist;
            q.push({x, y - 1});
        }
        // Propagate West
        if (!walls[x][y][3] && x > 0 && mazeDist[x - 1][y] == 255) {
            mazeDist[x - 1][y] = nextDist;
            q.push({x - 1, y});
        }
        
        // Display the numbers on the simulator UI
        API::setText(x, y, to_string(mazeDist[x][y]));
    }
}

void moveRobot(int targetDir) {
    // Determine how many turns are needed
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
    
    // Update internal coordinates
    currDir = targetDir;
    if (currDir == 0) currY++;
    if (currDir == 1) currX++;
    if (currDir == 2) currY--;
    if (currDir == 3) currX--;
}

int main(int argc, char* argv[]) {
    log("Flood Fill Initiated...");
    
    // Add external borders so the robot doesn't drive off the map
    for (int i = 0; i < SIZE; i++) {
        walls[i][0][2] = true;          // South border
        walls[i][SIZE - 1][0] = true;   // North border
        walls[0][i][3] = true;          // West border
        walls[SIZE - 1][i][1] = true;   // East border
    }

    while (true) {
        updateWalls();
        floodFill();

        if (mazeDist[currX][currY] == 0) {
            log("Destination reached! Mapping complete.");
            API::setColor(currX, currY, 'G');
            break;
        }

        // Find the accessible neighbor with the absolute lowest distance
        int bestDist = 255;
        int nextDir = currDir; // Default to straight if distances tie

        if (!walls[currX][currY][0] && mazeDist[currX][currY + 1] < bestDist) {
            bestDist = mazeDist[currX][currY + 1];
            nextDir = 0;
        }
        if (!walls[currX][currY][1] && mazeDist[currX + 1][currY] < bestDist) {
            bestDist = mazeDist[currX + 1][currY];
            nextDir = 1;
        }
        if (!walls[currX][currY][2] && mazeDist[currX][currY - 1] < bestDist) {
            bestDist = mazeDist[currX][currY - 1];
            nextDir = 2;
        }
        if (!walls[currX][currY][3] && mazeDist[currX - 1][currY] < bestDist) {
            bestDist = mazeDist[currX - 1][currY];
            nextDir = 3;
        }

        moveRobot(nextDir);
        API::setColor(currX, currY, 'Y'); // Leave a yellow trail
    }
    
    return 0;
}