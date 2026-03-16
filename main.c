#include "primlib.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define SCREEN_W gfx_screenWidth()
#define SCREEN_H gfx_screenHeight()
#define N_POLES 3
#define N_DISKS 3
#define MARGIN 5
#define POLE_HEIGHT 400
#define FRAME_DELAY_MS 16
#define ANIMATION_STEP 14

typedef struct {
    int active;
    int diskSize;
    int fromPole;
    int toPole;
    int fromRow;
    float targetBottomY;
    float centerX;
    float bottomY;
    int phase;
} AnimatedDisk;


// --- GAMEPLAY ---

bool checkWon(int *game_state){
    for (int i = 0; i < N_DISKS; i++){
        if (*(game_state + (i+1) * N_POLES - 1) == 0){
            return false;
        }
    }
    return true;
}

// ---- POLE LOGIC ----

int poleCenterX(int poleIndex)
{
    int usableWidth = SCREEN_W - (2 * MARGIN);

    return MARGIN + ((poleIndex + 1) * usableWidth) / (N_POLES + 1);
}

int poleWidth(void)
{
    int usableWidth = SCREEN_W - (2 * MARGIN);
    int spacing = usableWidth / (N_POLES + 1);
    int width = spacing / 10;

    if (width < 8) {
        return 8;
    }

    return width;
}

void drawPoles(void){
    int poleBottomY = SCREEN_H - 11;
    int poleTopY = poleBottomY - POLE_HEIGHT;
    int width = poleWidth();

    for (int i = 0; i < N_POLES; i++) {
        int centerX = poleCenterX(i);
        int leftX = centerX - (width / 2);
        int rightX = leftX + width - 1;

        gfx_filledRect(leftX, poleTopY, rightX, poleBottomY, MAGENTA);
    }
}


// --- DISK LOGIC ---

int calculateDiskWidth(int diskSize) {

    int usableWidth = SCREEN_W - (2 * MARGIN);
    int spacing = usableWidth / (N_POLES+1) ;
    int minWidth = poleWidth() + 40;
    int maxWidth = spacing - 12;

    if (N_DISKS <= 1 || maxWidth <= minWidth) {
        return minWidth;
    }

    return minWidth + ((diskSize - 1) * (maxWidth - minWidth)) / (N_DISKS - 1);
}

int diskHeight(void) {
    int usableHeight = POLE_HEIGHT - 10;
    return usableHeight / (N_DISKS + 1);
}

int diskBottomYForRow(int row) {
    return SCREEN_H - 11 - ((N_DISKS - 1 - row) * diskHeight());
}

int animationLiftY(void) {
    return SCREEN_H - 11 - POLE_HEIGHT - diskHeight();
}

int getTopDisk(int k, int *game_state, int *n) {
    int i = 0;
    while (i < N_DISKS){
        int val = *(game_state + i * N_POLES + k);
        if (val != 0){
            *n = i;
            return val;
        }
        i++;
    }
    *n = -1;
    return 0;
}

void moveDisk(int first, int second, int *game_state){
    int fTopRowIndex = 0;
    int sTopRowIndex = 0;
    int targetRow = 0;
    int fTop = getTopDisk(first, game_state, &fTopRowIndex);
    int sTop = getTopDisk(second, game_state, &sTopRowIndex);

    if (first == second || fTop == 0) {
        return;
    }

    if (sTop != 0 && fTop > sTop) {
        return;
    }

    targetRow = (sTop == 0) ? (N_DISKS - 1) : (sTopRowIndex - 1);
    *(game_state + targetRow * N_POLES + second) = fTop;
    *(game_state + fTopRowIndex * N_POLES + first) = 0;
}

void drawDiskAtPosition(int diskSize, float centerX, float bottomY)
{
    int width = calculateDiskWidth(diskSize);
    int height = diskHeight();
    int leftX = (int)(centerX - (width / 2.0f));
    int rightX = leftX + width - 1;
    int diskBottomY = (int)bottomY;
    int diskTopY = diskBottomY - height + 1;

    gfx_filledRect(leftX, diskTopY, rightX, diskBottomY, RED);
    gfx_rect(leftX, diskTopY, rightX, diskBottomY, WHITE);
}

void drawDisks(int *game_state, const AnimatedDisk *animatedDisk){

    for(int i = 0; i < N_DISKS; i++){
        for (int j = 0; j < N_POLES; j++){
            if (*(game_state + i * N_POLES + j) == 0) continue;
            if (animatedDisk->active && i == animatedDisk->fromRow && j == animatedDisk->fromPole) {
                continue;
            }

            drawDiskAtPosition(
                *(game_state + i * N_POLES + j),
                (float)poleCenterX(j),
                (float)diskBottomYForRow(i)
            );
        }
    }
}

void drawAnimatedDisk(AnimatedDisk *animatedDisk)
{
    if (!animatedDisk->active) {
        return;
    }

    drawDiskAtPosition(animatedDisk->diskSize, animatedDisk->centerX, animatedDisk->bottomY);
}

void startDiskAnimation(int first, int second, int *game_state, AnimatedDisk *animatedDisk)
{
    int fromRow = 0;
    int targetTopRow = 0;
    int targetRow = 0;
    int movingDisk = getTopDisk(first, game_state, &fromRow);
    int targetTopDisk = getTopDisk(second, game_state, &targetTopRow);

    if (first == second || movingDisk == 0) {
        return;
    }

    if (targetTopDisk != 0 && movingDisk > targetTopDisk) {
        return;
    }

    animatedDisk->active = 1;
    animatedDisk->diskSize = movingDisk;
    animatedDisk->fromPole = first;
    animatedDisk->toPole = second;
    animatedDisk->fromRow = fromRow;
    targetRow = (targetTopDisk == 0) ? (N_DISKS - 1) : (targetTopRow - 1);
    animatedDisk->targetBottomY = (float)diskBottomYForRow(targetRow);
    animatedDisk->centerX = (float)poleCenterX(first);
    animatedDisk->bottomY = (float)diskBottomYForRow(fromRow);
    animatedDisk->phase = 0;

}

void updateAnimation(AnimatedDisk *animatedDisk, int *game_state)
{
    float distance = ANIMATION_STEP;
    float targetX = (float)poleCenterX(animatedDisk->toPole);
    float liftY = (float)animationLiftY();

    if (!animatedDisk->active) {
        return;
    }

    if (animatedDisk->phase == 0) {
        animatedDisk->bottomY -= distance;
        if (animatedDisk->bottomY <= liftY) {
            animatedDisk->bottomY = liftY;
            animatedDisk->phase = 1;
        }
        return;
    }

    if (animatedDisk->phase == 1) {
        if (animatedDisk->centerX < targetX) {
            animatedDisk->centerX += distance;
            if (animatedDisk->centerX >= targetX) {
                animatedDisk->centerX = targetX;
                animatedDisk->phase = 2;
            }
        } else {
            animatedDisk->centerX -= distance;
            if (animatedDisk->centerX <= targetX) {
                animatedDisk->centerX = targetX;
                animatedDisk->phase = 2;
            }
        }
        return;
    }

    animatedDisk->bottomY += distance;
    if (animatedDisk->bottomY >= animatedDisk->targetBottomY) {
        animatedDisk->bottomY = animatedDisk->targetBottomY;
        moveDisk(animatedDisk->fromPole, animatedDisk->toPole, game_state);
        animatedDisk->active = 0;
    }
}

void drawEndgameScreen(void){
    char *text = "You Won! Press ENTER to exit";
    gfx_filledRect(0, 0, SCREEN_W - 1, SCREEN_H - 1, BLACK);
    gfx_textout(SCREEN_W / 2  , SCREEN_H / 2 , text, YELLOW); 
}

void populateGameState(int *game_state){
 
    for (int i = 0; i < N_DISKS; i++) {
        for (int j = 0; j < N_POLES; j++) {
            if (j == 0) {
               *(game_state + i * N_POLES + j) = i+1;
            } else {
               *(game_state + i * N_POLES + j) = 0;
            }

            printf("%d", *(game_state + i * N_POLES + j) );
        }
        printf("\n");
    } 
}

void drawScreen(bool isEndgameScreen,
                bool *isRunning,
                AnimatedDisk *animatedDisk,
                int *game_state,
                int *move,
                int *mIndex){

    char key = gfx_pollkey();
    if (!isEndgameScreen){

        if (!animatedDisk->active) {
          

            if (key >= '1' && key <= '0' + N_POLES) {
                move[*mIndex] = key - '1';
                (*mIndex)++;
            } else if (key == '0') {
                move[*mIndex] = 9;
                (*mIndex)++;
            } else if (key == 'q'){
                *isRunning = 0;
            }

            if (*mIndex == 2){
                startDiskAnimation(move[0], move[1], game_state, animatedDisk);
                *mIndex = 0;
            }
        } else {
            updateAnimation(animatedDisk, game_state);
        }


        gfx_filledRect(0, 0, SCREEN_W- 1, SCREEN_H - 1, BLACK);
        gfx_filledRect(0, SCREEN_H - 10, SCREEN_W, SCREEN_H , YELLOW);
        drawPoles();
        drawDisks(game_state, animatedDisk);
        drawAnimatedDisk(animatedDisk);
    } else {
        drawEndgameScreen();
        if (key == 'q' || key == '\x0D'){
            *isRunning = 0;
        }
    }


}

int main(int argc, char *argv[])
{
    if (gfx_init()) {
        exit(3);
    }

    bool isEndgameScreen = 0;
    bool running = 1;
    int game_state[N_DISKS][N_POLES];


    int mIndex = 0;
    int move[2] = {-1, -1};
    AnimatedDisk animatedDisk = {0};

    populateGameState(*game_state);

    while (running) {

        drawScreen(isEndgameScreen, &running, &animatedDisk, *game_state , move, &mIndex);
        isEndgameScreen = checkWon(*game_state);  
        gfx_updateScreen();
        SDL_Delay(FRAME_DELAY_MS);
    }

    return 0;
}
