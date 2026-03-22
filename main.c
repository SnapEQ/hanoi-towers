#include "primlib.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#define POLES_NO 10
#define DISKS_NO 3
#define MARGIN 5
#define POLE_HEIGHT 400
#define FRAME_DELAY_MS 16
#define ANIMATION_STEP 14

typedef struct
{
    bool active;
    int diskSize;
    int fromPole;
    int toPole;
    int fromRow;
    float targetBottomY;
    float centerX;
    float bottomY;
    int phase;
} AnimatedDisk;

int screenH = 0;
int screenW = 0;

bool checkWon(int *game_state)
{
    for (int i = 0; i < DISKS_NO; i++)
    {
        if (*(game_state + (i + 1) * POLES_NO - 1) == 0)
        {
            return false;
        }
    }
    return true;
}

int poleCenterX(int poleIndex)
{
    int usableWidth = screenW - (2 * MARGIN);

    return MARGIN + ((poleIndex + 1) * usableWidth) / (POLES_NO + 1);
}

int poleWidth(void)
{
    int usableWidth = screenW - (2 * MARGIN);
    int spacing = usableWidth / (POLES_NO + 1);
    int width = spacing / 10;

    if (width < 8)
    {
        return 8;
    }

    return width;
}

void drawPoles(void)
{
    int poleBottomY = screenH - 11;
    int poleTopY = poleBottomY - POLE_HEIGHT;
    int width = poleWidth();

    for (int i = 0; i < POLES_NO; i++)
    {
        int centerX = poleCenterX(i);
        int leftX = centerX - (width / 2);
        int rightX = leftX + width - 1;

        gfx_filledRect(leftX, poleTopY, rightX, poleBottomY, MAGENTA);
    }
}

int calculateDiskWidth(int diskSize)
{

    int usableWidth = screenW - (2 * MARGIN);
    int spacing = usableWidth / (POLES_NO + 1);
    int minWidth = poleWidth() + 40;
    int maxWidth = spacing - 12;

    if (DISKS_NO <= 1 || maxWidth <= minWidth)
    {
        return minWidth;
    }

    return minWidth + ((diskSize - 1) * (maxWidth - minWidth)) / (DISKS_NO - 1);
}

int diskHeight(void)
{
    int usableHeight = POLE_HEIGHT - 10;
    return usableHeight / (DISKS_NO + 1);
}

int diskBottomYForRow(int row)
{
    return screenH - 11 - ((DISKS_NO - 1 - row) * diskHeight());
}

int animationLiftY(void)
{
    return screenH - 11 - POLE_HEIGHT - diskHeight();
}

int getTopDisk(int k, int *game_state, int *n)
{
    int i = 0;
    while (i < DISKS_NO)
    {
        int val = *(game_state + i * POLES_NO + k);
        if (val != 0)
        {
            *n = i;
            return val;
        }
        i++;
    }
    *n = -1;
    return 0;
}

void moveDisk(int first, int second, int *game_state)
{
    int fTopRowIndex = 0;
    int sTopRowIndex = 0;
    int targetRow = 0;
    int fTop = getTopDisk(first, game_state, &fTopRowIndex);
    int sTop = getTopDisk(second, game_state, &sTopRowIndex);

    if (first == second || fTop == 0)
    {
        return;
    }

    if (sTop != 0 && fTop > sTop)
    {
        return;
    }

    targetRow = (sTop == 0) ? (DISKS_NO - 1) : (sTopRowIndex - 1);
    *(game_state + targetRow * POLES_NO + second) = fTop;
    *(game_state + fTopRowIndex * POLES_NO + first) = 0;
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

void drawDisks(int *game_state, const AnimatedDisk *animatedDisk)
{

    for (int i = 0; i < DISKS_NO; i++)
    {
        for (int j = 0; j < POLES_NO; j++)
        {
            if (*(game_state + i * POLES_NO + j) == 0)
                continue;
            if (animatedDisk->active && i == animatedDisk->fromRow && j == animatedDisk->fromPole)
            {
                continue;
            }

            drawDiskAtPosition(
                *(game_state + i * POLES_NO + j),
                (float)poleCenterX(j),
                (float)diskBottomYForRow(i));
        }
    }
}

void drawAnimatedDisk(AnimatedDisk *animatedDisk)
{
    if (!animatedDisk->active)
    {
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

    if ((first == second || movingDisk == 0) || (targetTopDisk != 0 && movingDisk > targetTopDisk))
    {
        return;
    }

    animatedDisk->active = true;
    animatedDisk->diskSize = movingDisk;
    animatedDisk->fromPole = first;
    animatedDisk->toPole = second;
    animatedDisk->fromRow = fromRow;
    targetRow = (targetTopDisk == 0) ? (DISKS_NO - 1) : (targetTopRow - 1);
    animatedDisk->targetBottomY = (float)diskBottomYForRow(targetRow);
    animatedDisk->centerX = (float)poleCenterX(first);
    animatedDisk->bottomY = (float)diskBottomYForRow(fromRow);
    animatedDisk->phase = 0;
}

void animatedDiskPhase0(AnimatedDisk *animatedDisk, float distance, float liftY)
{
    if (animatedDisk->phase == 0)
    {
        animatedDisk->bottomY -= distance;
        if (animatedDisk->bottomY <= liftY)
        {
            animatedDisk->bottomY = liftY;
            animatedDisk->phase = 1;
        }
        return;
    }
}

void animatedDiskPhase1(AnimatedDisk *animatedDisk, float distance, float targetX)
{
    if (animatedDisk->phase == 1)
    {
        if (animatedDisk->centerX < targetX)
        {
            animatedDisk->centerX += distance;
            if (animatedDisk->centerX >= targetX)
            {
                animatedDisk->centerX = targetX;
                animatedDisk->phase = 2;
            }
        }
        else
        {
            animatedDisk->centerX -= distance;
            if (animatedDisk->centerX <= targetX)
            {
                animatedDisk->centerX = targetX;
                animatedDisk->phase = 2;
            }
        }
        return;
    }
}

void updateAnimation(AnimatedDisk *animatedDisk, int *game_state)
{
    float distance = ANIMATION_STEP;
    float targetX = (float)poleCenterX(animatedDisk->toPole);
    float liftY = (float)animationLiftY();

    if (!animatedDisk->active)
    {
        return;
    }
    if (animatedDisk->phase == 0)
    {
        animatedDiskPhase0(animatedDisk, distance, liftY);
        return;
    }
    if (animatedDisk->phase == 1)
    {
        animatedDiskPhase1(animatedDisk, distance, targetX);
        return;
    }

    animatedDisk->bottomY += distance;
    if (animatedDisk->bottomY >= animatedDisk->targetBottomY)
    {
        animatedDisk->bottomY = animatedDisk->targetBottomY;
        moveDisk(animatedDisk->fromPole, animatedDisk->toPole, game_state);
        animatedDisk->active = false;
    }
}

void drawEndgameScreen(void)
{
    char *text = "You Won! Press Q to exit";
    gfx_filledRect(0, 0, screenW - 1, screenH - 1, BLACK);
    gfx_textout(screenW / 2, screenH / 2, text, YELLOW);
}

void populateGameState(int *game_state)
{

    for (int i = 0; i < DISKS_NO; i++)
    {
        for (int j = 0; j < POLES_NO; j++)
        {
            if (j == 0)
            {
                *(game_state + i * POLES_NO + j) = i + 1;
            }
            else
            {
                *(game_state + i * POLES_NO + j) = 0;
            }

            printf("%d", *(game_state + i * POLES_NO + j));
        }
        printf("\n");
    }
}

int keyToPoleIndex(int key)
{
    if (key >= '1' && key <= '9')
    {
        int index = key - '1';
        if (index < POLES_NO)
        {
            return index;
        }
    }

    if (key == '0' && POLES_NO == 10)
    {
        return 9;
    }

    return -1;
}

void drawScreen(bool isEndgameScreen,
                bool *isRunning,
                AnimatedDisk *animatedDisk,
                int *game_state,
                int *move,
                int *mIndex)
{

    int key = gfx_pollkey();
    if (!isEndgameScreen)
    {

        if (!animatedDisk->active)
        {
            int poleIndex = keyToPoleIndex(key);

            if (poleIndex >= 0 && *mIndex < 2)
            {
                move[*mIndex] = poleIndex;
                (*mIndex)++;
            }
            else if (key == 'q')
            {
                *isRunning = 0;
            }

            if (*mIndex == 2)
            {
                startDiskAnimation(move[0], move[1], game_state, animatedDisk);
                *mIndex = 0;
            }
        }
        else
        {
            updateAnimation(animatedDisk, game_state);
        }

        gfx_filledRect(0, 0, screenW - 1, screenH - 1, BLACK);
        gfx_filledRect(0, screenH - 10, screenW, screenH, YELLOW);
        drawPoles();
        drawDisks(game_state, animatedDisk);
        drawAnimatedDisk(animatedDisk);
    }
    else
    {
        drawEndgameScreen();
        if (key == 'q')
        {
            *isRunning = 0;
        }
    }
}

int main(int argc, char *argv[])
{
    const int initStatus = gfx_init();

    if (initStatus != 0)
    {
        fprintf(stderr, "Failed to init gfx\n");
        return 3;
    }

    static_assert(POLES_NO >= 3 && DISKS_NO >= 3, "Not sufficient constants");
    screenH = gfx_screenHeight();
    screenW = gfx_screenWidth();

    bool isEndgameScreen = 0;
    bool running = 1;
    int game_state[DISKS_NO][POLES_NO];

    int mIndex = 0;
    int move[2] = {-1, -1};
    AnimatedDisk animatedDisk = {0};

    populateGameState(*game_state);

    while (running)
    {

        drawScreen(isEndgameScreen, &running, &animatedDisk, *game_state, move, &mIndex);
        isEndgameScreen = checkWon(*game_state);
        gfx_updateScreen();
        SDL_Delay(FRAME_DELAY_MS);
    }

    return 0;
}
