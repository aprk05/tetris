#include <iostream>
#include <vector>
#include <conio.h>
#include <windows.h>
#include <ctime>
#include <fstream>
#include <unordered_map>
#include <algorithm>
#include <mmsystem.h>

using namespace std;

// Constants
const int WIDTH = 10;
const int HEIGHT = 20;
const char EMPTY = '.';
const char BLOCK = '#';

//padding
    int consoleWidth;
    int padding;
    string pad(padding, ' ');

// Grid and Score
vector<vector<char>> grid(HEIGHT, vector<char>(WIDTH, EMPTY));
int score = 0;
int linesCleared = 0;
int level = 1;
bool powerUpUsed = false; // Track if the power-up has been used

// Structures
struct Tetromino {
    vector<pair<int, int>> shape;
    int x, y;
    int color;
};
Tetromino currentPiece;
Tetromino nextPiece;

// Tetromino Definitions and Colors
vector<vector<pair<int, int>>> tetrominoes = {
    {{0, 0}, {1, 0}, {2, 0}, {3, 0}}, // I
    {{0, 0}, {1, 0}, {0, 1}, {1, 1}}, // O
    {{0, 0}, {1, 0}, {2, 0}, {1, 1}}, // T
    {{0, 0}, {1, 0}, {1, 1}, {2, 1}}, // S
    {{1, 0}, {2, 0}, {0, 1}, {1, 1}}, // Z
    {{0, 0}, {0, 1}, {1, 1}, {2, 1}}, // L
    {{2, 0}, {0, 1}, {1, 1}, {2, 1}}  // J
};
vector<int> colors = {9, 14, 13, 10, 12, 6, 11}; // blue, yellow, purple, green, red, orange, cyan

// Color Grid
vector<vector<int>> colorGrid(HEIGHT, vector<int>(WIDTH, 7)); // Default color

// High Score System
unordered_map<string, pair<string, int>> userHighScores; // Map student ID to (name, score)
vector<pair<string, int>> leaderboard; // Leaderboard for all users
string currentUser;
string userId;

// Function Prototypes
void loadHighScores();
void saveHighScores();
void displayHighScore();
void displayLeaderboard();
void drawGrid();
void drawStats();
void startMenu();
void gameLoop();
bool isCollision(int dx, int dy);
void clearFullRows();
void lockPiece();
void rotatePiece();
void spawnPiece();
void movePiece(int dx, int dy);
void hardDrop();
void adjustDifficulty();
void beep();
void usePowerUp();
void playMusic();
void confirmRestart();
void resetGame();

// Load High Scores from a File
void loadHighScores() {
    ifstream file("highscores.txt");
    if (file.is_open()) {
        string id, name;
        int highScore;
        while (file >> id >> name >> highScore) {
            userHighScores[id] = {name, highScore};
        }
        file.close();
    }
}

// Save High Scores to a File
void saveHighScores() {
    ofstream file("highscores.txt");
    if (file.is_open()) {
        for (const auto &entry : userHighScores) {
            file << entry.first << " " << entry.second.first << " " << entry.second.second << "\n";
        }
        file.close();
    }
}

// Display Leaderboard
void displayLeaderboard() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int consoleWidth = 80;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        consoleWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
    int padding = (consoleWidth - 30) / 2;
    string pad(padding, ' ');

    cout << "\n" << pad << "LEADERBOARD:\n";
    cout << pad << "----------------\n";
    leaderboard.clear();
    for (const auto &entry : userHighScores) {
        leaderboard.push_back({entry.second.first, entry.second.second});
    }
    sort(leaderboard.begin(), leaderboard.end(), [](const pair<string, int> &a, const pair<string, int> &b) {
        return b.second < a.second;
    });
    for (size_t i = 0; i < leaderboard.size() && i < 10; ++i) {
        cout << pad << i + 1 << ". " << leaderboard[i].first << " - " << leaderboard[i].second << "\n";
    }
}

// Display In-Game Stats
void drawStats() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int consoleWidth = 80;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        consoleWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
    int padding = (consoleWidth - 30) / 2;
    string pad(padding, ' ');

    cout << pad <<"     STATS:\n";
    cout << pad <<"     Level: " << level << "\n";
    cout << pad <<"     Score: " << score << "\n";
    cout << pad <<"     Lines Cleared: " << linesCleared << "\n";
    cout << pad <<"     Power-Up:           \r"; // Overwrite previous text with spaces
    cout << pad <<"     Power-Up: " << (powerUpUsed ? "Used" : "Available") << "\n";
}

void drawGrid() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD cursorPosition = {0, 0};
    SetConsoleCursorPosition(hConsole, cursorPosition);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int originalAttributes = 7; // Default fallback if attributes can't be retrieved.
    if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        originalAttributes = csbi.wAttributes;
    }

    int consoleWidth = 80;
    if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        consoleWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
    int padding = (consoleWidth - (WIDTH * 2)) / 2;
    string pad(padding, ' ');

    cout << "\n\n";
    cout << pad << "Score: " << score << "\n\n";

    // Display the next piece preview
    cout << pad << "Next Piece:\n";
    for (int i = 0; i < 4; i++) { // The maximum size of a tetromino is 4x4.
        cout << pad; // Maintain the same padding for alignment.
        for (int j = 0; j < 4; j++) {
            bool isBlock = false;
            for (auto &block : nextPiece.shape) {
                if (block.second == i && block.first == j) {
                    isBlock = true;
                    break;
                }
            }
            if (isBlock) {
                SetConsoleTextAttribute(hConsole, nextPiece.color);
                cout << BLOCK << " ";
                SetConsoleTextAttribute(hConsole, originalAttributes); // Reset to default
            } else {
                cout << "  "; // Empty space
            }
        }
        cout << "\n";
    }
    drawStats();
    cout << "\n"; // Add some spacing below the preview.

    // Create a temporary grid to render the current piece.
    vector<vector<char>> tempGrid = grid;

    for (auto &block : currentPiece.shape) {
        tempGrid[currentPiece.y + block.second][currentPiece.x + block.first] = BLOCK;
    }

    // Render the grid.
    for (int i = 0; i < HEIGHT; i++) {
        cout << pad;
        for (int j = 0; j < WIDTH; j++) {
            if (tempGrid[i][j] == BLOCK && grid[i][j] != BLOCK) { // Active piece.
                SetConsoleTextAttribute(hConsole, currentPiece.color);
                cout << BLOCK << " ";
                SetConsoleTextAttribute(hConsole, originalAttributes); // Reset to original color.
            } else if (grid[i][j] == BLOCK) { // Locked blocks.
                SetConsoleTextAttribute(hConsole, colorGrid[i][j]);
                cout << BLOCK << " ";
                SetConsoleTextAttribute(hConsole, originalAttributes); // Reset to original color.
            } else {
                cout << tempGrid[i][j] << " ";
            }
        }
        cout << "\n";
    }
}


bool isCollision(int dx, int dy) {
    for (auto &block : currentPiece.shape) {
        int newX = currentPiece.x + block.first + dx;
        int newY = currentPiece.y + block.second + dy;
        if (newX < 0 || newX >= WIDTH || newY >= HEIGHT || grid[newY][newX] == BLOCK) {
            return true;
        }
    }
    return false;
}

void clearFullRows() {
    for (int i = 0; i < HEIGHT; i++) {
        bool fullRow = true;
        for (int j = 0; j < WIDTH; j++) {
            if (grid[i][j] == EMPTY) {
                fullRow = false;
                break;
            }
        }
        if (fullRow) {
            for (int k = i; k > 0; k--) {
                grid[k] = grid[k - 1];
            }
            grid[0] = vector<char>(WIDTH, EMPTY);
            score += 10; // Increase score when a row is cleared
            userHighScores[userId] = {currentUser, max(userHighScores[userId].second, score)};
            saveHighScores(); // Save before exit
            beep();
        }
    }
    
}

void lockPiece() {
    for (auto &block : currentPiece.shape) {
        int x = currentPiece.x + block.first;
        int y = currentPiece.y + block.second;
        grid[y][x] = BLOCK;               // Lock the block in the grid.
        colorGrid[y][x] = currentPiece.color; // Save the color in the color grid.
    }
    clearFullRows();
}

void rotatePiece() {
    vector<pair<int, int>> rotatedShape;
    for (auto &block : currentPiece.shape) {
        rotatedShape.push_back({-block.second, block.first});
    }
    Tetromino rotatedPiece = {rotatedShape, currentPiece.x, currentPiece.y};
    bool collision = false;
    for (auto &block : rotatedPiece.shape) {
        int newX = rotatedPiece.x + block.first;
        int newY = rotatedPiece.y + block.second;
        if (newX < 0 || newX >= WIDTH || newY >= HEIGHT || grid[newY][newX] == BLOCK) {
            collision = true;
            break;
        }
    }
    if (!collision) {
        currentPiece.shape = rotatedPiece.shape;
    }
}


void spawnPiece() {
    currentPiece = nextPiece; // The next piece becomes the current piece.
    int index = rand() % tetrominoes.size();
    nextPiece.shape = tetrominoes[index]; // Generate a new next piece.
    nextPiece.color = colors[index];
    nextPiece.x = WIDTH / 2 - 1;
    nextPiece.y = 0;

    // Check for collisions when spawning the current piece.
    if (isCollision(0, 0)) {
        cout << "Game Over! Final Score: " << score << "\n";
        userHighScores[userId] = {currentUser, max(userHighScores[userId].second, score)};
        saveHighScores(); // Save before exit
        confirmRestart();
        exit(0);
    }
}


void movePiece(int dx, int dy) {
    if (!isCollision(dx, dy)) {
        currentPiece.x += dx;
        currentPiece.y += dy;
    } else if (dy > 0) {
        lockPiece();
        spawnPiece();
    }
}

void hardDrop() {
    while (!isCollision(0, 1)) {
        currentPiece.y++; // Keep moving the piece down until it collides.
    }
    lockPiece(); // Lock the piece at its final position.
    spawnPiece(); // Spawn a new piece.
}



// Beep Sound
void beep() {
    Beep(750, 300); // Beep sound when a row is cleared
}

// Use Power-Up to Clear Last Row
void usePowerUp() {
    if (!powerUpUsed) { // Ensure the power-up can only be used once
        for (int x = 0; x < WIDTH; ++x) {
            grid[HEIGHT - 1][x] = EMPTY;      // Clear the last row
            colorGrid[HEIGHT - 1][x] = 7;     // Reset color to default
        }

        // Shift rows down
        for (int i = HEIGHT - 1; i > 0; --i) {
            grid[i] = grid[i - 1];
            colorGrid[i] = colorGrid[i - 1];
        }

        // Reset the top row to empty
        grid[0] = vector<char>(WIDTH, EMPTY);
        colorGrid[0] = vector<int>(WIDTH, 7);

        powerUpUsed = true; // Mark power-up as used
        beep(); // Play a sound to indicate power-up activation
    }
}


void startMenu() {
    system("cls");
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int consoleWidth = 80;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        consoleWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
    int padding = (consoleWidth - 30) / 2;
    string pad(padding, ' ');


    cout << "Enter your name: ";
    cin >> currentUser;
    cout << "Enter your user ID: ";
    cin >> userId;
    loadHighScores(); // Load scores before checking
    displayLeaderboard(); // Show global leaderboard
    cout <<"\n"<< pad << "TETRIS GAME\n";
    cout << pad << "------------------------\n";
    cout << pad << "Instructions:\n";
    cout << pad << "- Use 'A' to move left\n";
    cout << pad << "- Use 'D' to move right\n";
    cout << pad << "- Use 'S' to speed up descent\n";
    cout << pad << "- Use 'W' to rotate the piece\n";
    cout << pad << "- Use 'Spacebar' to hard drop piece\n";
    cout << pad << "- Use 'P' to to clear last row\n";
    cout << pad << "- Press 'ESC' to exit during gameplay\n";
    cout << pad << "------------------------\n";
    cout << pad << "1. Start Game\n";
    cout << pad << "2. Exit\n";
    cout << pad << "Select an option: ";
}

void playMusic() {
    PlaySound(TEXT("Tetris.wav"), NULL, SND_ASYNC | SND_LOOP); // Loops music asynchronously
}

void confirmRestart() {
    saveHighScores();
    cout << "\nDo you want to restart the game? (Y/N): ";
    char choice;
    cin >> choice;
    if (choice == 'Y' || choice == 'y') {
        system("cls");
        resetGame();
        gameLoop();
    }
    else {
        cout << "Thanks for playing" << endl;
        Sleep(2000);
        system("cls");
        exit(0);
    }
}

void resetGame() {
    grid.assign(HEIGHT, vector<char>(WIDTH, EMPTY));
    colorGrid.assign(HEIGHT, vector<int>(WIDTH, 7));
    score = 0;
    powerUpUsed = false;
    spawnPiece();
}


void gameLoop() {
    spawnPiece();
    int dropSpeed = 500;

    while (true) {
        drawGrid();
        dropSpeed = max(100, 300 - (score / 10) * 50);

        if (_kbhit()) {
            char key = _getch();
            if (key == 27) { // ESC key
                userHighScores[userId] = {currentUser, max(userHighScores[userId].second, score)};
                saveHighScores(); // Save before exit
                confirmRestart();
            } else if (key == 'a') movePiece(-1, 0);
            else if (key == 'd') movePiece(1, 0);
            else if (key == 's') movePiece(0, 1);
            else if (key == 'w') rotatePiece();
            else if (key == ' ') hardDrop();
            else if (key == 'p') usePowerUp();
        }

        movePiece(0, 1);
        Sleep(dropSpeed);
    }
}


int main() {
    srand(time(0));      // Initialize the first nextPiece
    playMusic();
    int index = rand() % tetrominoes.size();
    nextPiece.shape = tetrominoes[index];
    nextPiece.color = colors[index];
    nextPiece.x = WIDTH / 2 - 1;
    nextPiece.y = 0;
    while (true) {
        startMenu();
        char choice = _getch();
        if (choice == '1') {
            system("cls");
            gameLoop();
            userHighScores[userId] = {currentUser, max(userHighScores[userId].second, score)};
            saveHighScores(); // Save before exit
            confirmRestart();
        } else if (choice == '2') {
            userHighScores[userId] = {currentUser, max(userHighScores[userId].second, score)};
            saveHighScores(); // Save before exit
            cout << "\nExiting game...";
            return 0;
        }
    }
    return 0;
}
