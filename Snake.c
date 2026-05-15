#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>

#define CELL_COUNT 25
#define CELL_SIZE 24
#define OFFSET 40
#define PLAY_ROWS 23
#define PLAY_TOP 0
#define CELL_MAX (CELL_COUNT*CELL_COUNT)
#define MAX_WALL_CELLS 82
#define INPUT_QUEUE_SIZE 4

//Constants and Inits
//------------------------------------------------------//------------------------------------------------------
const int cellSize = CELL_SIZE;
const int cellCount = CELL_COUNT;

Color Background = {30, 32, 40, 255};
Color Apple = RED;
Color darkGreen = {86, 102, 48, 255};
Color darkRed = {139, 0, 0, 255};

bool paused = false;
bool running = true;

double lastUpdateTime = 0;
double gameInterval = 0.2;
int speedTimer = 0;
int scoreMulti = 1;
int scoreMultiTimer = 0;

int wallScoreAdd = 10;
int nextWallScore = 10;
int pendingWall = -1;
float flashWallTimer = 0;
bool isWallFlashing = true;

bool reversed = false;
int reversedTimer = 0;


bool eventTrigger(double interval){
    double currentTime = GetTime();
    if(currentTime - lastUpdateTime >= interval){
        lastUpdateTime = currentTime;
        return true;
    }
    return false;
}

//------------------------------------------------------//------------------------------------------------------

// Structs and Stuff
//------------------------------------------------------//------------------------------------------------------
//Snake Structs____________________________________________________________________________________________________________________
typedef struct{
    Vector2 segment[CELL_MAX];
    int head;
    int tail;
    int size;
} Deque;
Deque deque;

typedef struct{
    Deque body;
    Vector2 direction;
    int score;
    int growSnake;
} Snake;
Snake snake;

Snake MakeSnake(){
    Snake snake = {0};

    snake.body.segment[0] = (Vector2){6, 9};
    snake.body.segment[1] = (Vector2){5, 9};
    snake.body.segment[2] = (Vector2){4, 9};

    snake.body.head = 0;
    snake.body.tail = 2;
    snake.body.size = 3;

    snake.direction = (Vector2){1, 0};
    snake.growSnake = 0;

    return snake;
}

typedef struct{
    Vector2 sinput[INPUT_QUEUE_SIZE];
    int front;
    int rear;
    int size;
}InputSnake;
InputSnake Input;
//_____________________________________________________________________________________________________________________

//Food Structs______________________________________________________________________________________________________________________
typedef enum{       //Description:      //Color:    //Chance:
    NormalFood,     // The generic food // Red      //Very Common
    ZaApplo,        // 0.8x speed       // Yellow   //Epic
    FoodInHeaven,   // 2x speed         // White    //Uncommon
    BigApple,       // +3 segments      // Dark Red //Uncommon
    Minimize,       // -2 segments      // Purple   //Epic
    FoodRush,       // 2x points        // Orange   //Epic
    Reverse         // head on tail     //Gray      //Epic
}FoodType;

Color foodColors[] = {
    RED,                        // NormalFood
    YELLOW,                     // ZaApplo
    WHITE,                      // FoodInHeaven
    {139, 0, 0, 255},           // BigApple (dark red)
    PURPLE,                     // Minimize
    ORANGE,                     // FoodRush
    PINK                        // Reverse
};

int foodChance[] = {
    50,     //Normal Food
    5,      //Za Applo
    15,     //Food In Heaven
    15,     //Big Apple
    5,      //Minimize
    5,      //FoodRush
    5       //Reverse
};

typedef struct FoodNode{
    Vector2 position;
    FoodType type;
    struct FoodNode* next;
}FoodNode;

typedef struct{
    bool active;
    float timer;
    float bounce;
    int index;
    FoodType type;
}TextPop;
TextPop textpop;
//_____________________________________________________________________________________________________________________

//Wall Structs_____________________________________________________________________________________________________________________
typedef struct{
    Vector2 cell[MAX_WALL_CELLS];
    int count;
    int activeWalls;

} WallPattern;
WallPattern wall;



WallPattern MakePattern(){
    // wall 1: Center-top vertical
    wall.cell[0] = (Vector2){12, 1};
    wall.cell[1] = (Vector2){12, 2};
    wall.cell[2] = (Vector2){12, 3};
    wall.cell[3] = (Vector2){12, 4};
    wall.cell[4] = (Vector2){12, 5};
    wall.cell[5] = (Vector2){12, 6};

    // wall 2: Right-side vertical + hook
    wall.cell[6] = (Vector2){22, 3};
    wall.cell[7] = (Vector2){22, 4};
    wall.cell[8] = (Vector2){22, 5};
    wall.cell[9] = (Vector2){22, 6};
    wall.cell[10] = (Vector2){22, 7};
    wall.cell[11] = (Vector2){22, 8};
    wall.cell[12] = (Vector2){21, 8};
    wall.cell[13] = (Vector2){20, 8};
    wall.cell[14] = (Vector2){19, 8};
    wall.cell[15] = (Vector2){18, 8};

    // wall 3: Center-right horizontal arm
    wall.cell[16] = (Vector2){13, 4};
    wall.cell[17] = (Vector2){14, 4};
    wall.cell[18] = (Vector2){15, 4};
    wall.cell[19] = (Vector2){16, 4};
    wall.cell[20] = (Vector2){17, 4};

    // wall 4: Center-right vertical
    wall.cell[21] = (Vector2){15, 13};
    wall.cell[22] = (Vector2){15, 12};
    wall.cell[23] = (Vector2){15, 11};
    wall.cell[24] = (Vector2){15, 10};
    wall.cell[25] = (Vector2){15, 9};
    wall.cell[26] = (Vector2){14, 9};
    wall.cell[27] = (Vector2){13, 9};
    wall.cell[28] = (Vector2){12, 9};
    wall.cell[29] = (Vector2){11, 9};

    // wall 5: Top-left L
    wall.cell[30] = (Vector2){2, 3};
    wall.cell[31] = (Vector2){3, 3};
    wall.cell[32] = (Vector2){4, 3};
    wall.cell[33] = (Vector2){5, 3};
    wall.cell[34] = (Vector2){6, 3};
    wall.cell[35] = (Vector2){7, 3};
    wall.cell[36] = (Vector2){2, 4};
    wall.cell[37] = (Vector2){2, 5};
    wall.cell[38] = (Vector2){2, 6};
    wall.cell[39] = (Vector2){2, 7};
    wall.cell[40] = (Vector2){2, 8};

    // wall 6: Center L-shape
    wall.cell[41] = (Vector2){8, 10};
    wall.cell[42] = (Vector2){8, 11};
    wall.cell[43] = (Vector2){8, 12};
    wall.cell[44] = (Vector2){8, 13};
    wall.cell[45] = (Vector2){8, 14};
    wall.cell[46] = (Vector2){9, 14};
    wall.cell[47] = (Vector2){10, 14};
    wall.cell[48] = (Vector2){11, 14};
    wall.cell[49] = (Vector2){12, 14};

    // wall 7: Left T-bar + stem
    wall.cell[50] = (Vector2){0, 13};
    wall.cell[51] = (Vector2){1, 13};
    wall.cell[52] = (Vector2){2, 13};
    wall.cell[53] = (Vector2){3, 13};
    wall.cell[54] = (Vector2){4, 13};
    wall.cell[55] = (Vector2){5, 13};
    wall.cell[56] = (Vector2){3, 14};
    wall.cell[57] = (Vector2){3, 15};
    wall.cell[58] = (Vector2){3, 16};
    wall.cell[59] = (Vector2){3, 17};
    wall.cell[60] = (Vector2){3, 18};

    // wall 8: Bottom horizontal + stem down
    wall.cell[61] = (Vector2){8, 18};
    wall.cell[62] = (Vector2){9, 18};
    wall.cell[63] = (Vector2){10, 18};
    wall.cell[64] = (Vector2){11, 18};
    wall.cell[65] = (Vector2){12, 18};
    wall.cell[66] = (Vector2){13, 18};
    wall.cell[67] = (Vector2){14, 18};
    wall.cell[68] = (Vector2){15, 18};
    wall.cell[69] = (Vector2){16, 18};
    wall.cell[70] = (Vector2){12, 19};
    wall.cell[71] = (Vector2){12, 20};
    wall.cell[72] = (Vector2){12, 21};
    wall.cell[73] = (Vector2){12, 22};

    // wall 9: Right-bottom tall vertical
    wall.cell[74] = (Vector2){21, 15};
    wall.cell[75] = (Vector2){21, 16};
    wall.cell[76] = (Vector2){21, 17};
    wall.cell[77] = (Vector2){21, 18};
    wall.cell[78] = (Vector2){21, 19};
    wall.cell[79] = (Vector2){21, 20};
    wall.cell[80] = (Vector2){21, 21};
    wall.cell[81] = (Vector2){21, 22};

    wall.count = 82;
    wall.activeWalls = 0;
    return wall;
}
//_____________________________________________________________________________________________________________________
//------------------------------------------------------//------------------------------------------------------

//Functions---------------------------------------------------------------------------------------------------------
bool CheckFoodInSegment(Snake* snake, Vector2 point, int index);
void UpdateTextPop(TextPop* textpop);
void DrawTextPop(TextPop* textpop, Snake* snake);
void InsertFront(Deque* deque, Vector2 pos);
void DeleteRear(Deque* deque);
void DeleteFront(Deque* deque);
void DrawSnake(Snake* snake);
void MoveSnakeUpdate(Snake* snake);
void Reset(Snake* snake);
void AnimGameOver(TextPop* textpop);
void Link_Clear(struct FoodNode** head);
void Link_Dele_End(struct FoodNode** head);
void Link_Dele_Beg(struct FoodNode** head);
void Link_Add_End(struct FoodNode** head, Vector2 position, FoodType type);
void Link_Add_Beg(struct FoodNode** head, Vector2 position, FoodType type);
struct FoodNode* CreateFoodNode(Vector2 position, FoodType type);
struct FoodNode* Check_Food_Collision_V2(struct FoodNode* head, Snake* snake);
Vector2 Random_Food_Position();
FoodType FoodTypeChance();
void SpawnFood(struct FoodNode** head, Snake* snake, WallPattern* wall);
void Apply_Food_Effect(struct FoodNode* food, Snake* snake, TextPop* textpop);
void Link_Dele_Node(struct FoodNode** head, struct FoodNode* target);
void Draw_Food(struct FoodNode* head);
void Draw_Text_Pop_Special(char* message, TextPop* textpop, Color color);
void Draw_Food_Effect_Text();
bool CheckFoodInSegment(Snake* snake, Vector2 point, int index);
void WallCollisionCheck(Snake* snake, struct FoodNode** food, WallPattern* wall);
void BodyCollisionCheck(Snake* snake, struct FoodNode** food, WallPattern* wall);
void GameOver(Snake* snake, struct FoodNode** food, WallPattern* wall);
void InsertRear(Deque* deque, Vector2 pos);
Vector2 Reverse_Or_Not(Snake* snake);
bool Check_Food_In_Wall(WallPattern* wall, Vector2 point, int index);
void Check_Snake_Wall_Collision(WallPattern* wall, Snake* snake, FoodNode** food);
Vector2 Reverse_Or_Not(Snake* snake);
void Init_Input(InputSnake* input);
void Input_Enqueue(InputSnake* input, Vector2 point);
Vector2 Input_Dequeue(InputSnake* input);
void Draw_Wall(WallPattern* wall);
void Show_Wall(WallPattern* wall);
void Wall_Flash_Update(WallPattern* wall);
bool Check_Food_In_Food(FoodNode* head, Vector2 point);
//------------------------------------------------------------------------------------------------------------------------------------

void Game_Paused(){
    if(paused){
        int measure = MeasureText("Game Paused! Move to Continue", 20);
        DrawText("Game Paused! Move to Continue", ((2*OFFSET + CELL_SIZE*CELL_COUNT) - measure) / 2, (2*OFFSET + CELL_SIZE*CELL_COUNT) / 2, 20, darkRed);
    }
}

int main(void)
{   
    //Window
    SetConfigFlags(FLAG_WINDOW_TOPMOST);
    InitWindow(2* OFFSET + CELL_SIZE*CELL_COUNT, 2* OFFSET + CELL_SIZE*CELL_COUNT, "DSA Snake Game");
    SetWindowFocused();
    SetTargetFPS(60);

    Snake snake = MakeSnake();
    wall = MakePattern();
    struct FoodNode* head = NULL;
    Init_Input(&Input);
    for(int i = 0; i < 4; i++){
        SpawnFood(&head, &snake, &wall);
    }
    
    while (!WindowShouldClose())
    {
        //Event handling
        if(IsKeyPressed(KEY_UP)){
            Input_Enqueue(&Input, (Vector2){0, -1});
            running = true;
            paused = false;
        }
        if(IsKeyPressed(KEY_DOWN)){
            Input_Enqueue(&Input, (Vector2){0, 1});
            running = true;
            paused = false;
        }
            if(IsKeyPressed(KEY_LEFT)){
            Input_Enqueue(&Input, (Vector2){-1, 0});
            running = true;
            paused = false;
        }
        if(IsKeyPressed(KEY_RIGHT)){
            Input_Enqueue(&Input, (Vector2){1, 0});
            running = true;
            paused = false;
        }
        if(IsKeyPressed(KEY_P)){
            paused = true;
        }
        


        // Update Positions
        if(running){
            if(!paused){
                if(eventTrigger(gameInterval)){
                if(Input.size > 0){
                    Vector2 next = Input_Dequeue(&Input);
                    if(reversed){
                        next.x *= -1;
                        next.y *= -1;
                    }
                    if(!(next.x == -snake.direction.x && next.y == -snake.direction.y)){
                        snake.direction = next;
                        running = true;
                    }
                }
                if(speedTimer > 0){
                    speedTimer--;
                    if(speedTimer == 0){
                        gameInterval = 0.2;
                    }
                }
                if(scoreMultiTimer > 0){
                    scoreMultiTimer--;
                    if(scoreMultiTimer == 0){
                        scoreMulti = 1;
                    }
                }
                if(reversedTimer > 0){
                    reversedTimer--;
                    if(reversedTimer == 0){
                        reversed = false;
                    }
                }
                struct FoodNode* eaten = Check_Food_Collision_V2(head, &snake);
                if(eaten){
                    Apply_Food_Effect(eaten, &snake, &textpop);
                    Link_Dele_Node(&head, eaten);
                    SpawnFood(&head, &snake, &wall);
                }
                MoveSnakeUpdate(&snake);
                WallCollisionCheck(&snake, &head, &wall);
                BodyCollisionCheck(&snake, &head, &wall);
                Check_Snake_Wall_Collision(&wall, &snake, &head);
            }
            Wall_Flash_Update(&wall);
            UpdateTextPop(&textpop);
            }
        }
        
        // Draw
        BeginDrawing();
            ClearBackground(Background);
            DrawRectangleLinesEx((Rectangle){(float)OFFSET-5, (float)OFFSET+70 + (CELL_SIZE * 0.4), (float)CELL_SIZE*CELL_COUNT+10, (float)CELL_SIZE*CELL_COUNT-62 - (CELL_SIZE*0.15)}, 5,darkGreen);
            Draw_Food(head);
            DrawSnake(&snake);
            int scoreWidth = MeasureText(TextFormat("%i", snake.score), 20);
            DrawText(TextFormat("%i", snake.score), (2*OFFSET + CELL_SIZE*CELL_COUNT) - scoreWidth - 25, 40, 40, YELLOW);
            Draw_Food_Effect_Text();
            DrawTextPop(&textpop, &snake);
            AnimGameOver(&textpop);
            Draw_Wall(&wall);
            Game_Paused();
        EndDrawing();
    }
    CloseWindow();

    return 0;
}







//{{{Snake Start}}}
//deque start---------------------------------------
void InsertFront(Deque* deque, Vector2 pos){
    if(deque->head == 0){
        deque->head = CELL_MAX -1;
    } else{
        deque->head = deque->head -1;
    }
    deque->segment[deque->head] = pos;
    deque->size++;
}

void InsertRear(Deque* deque, Vector2 pos){
    if(deque->tail == CELL_MAX - 1){
        deque->tail = 0;
    } else {
        deque->tail = deque->tail + 1;
    }
    deque->segment[deque->tail] = pos;
    deque->size++;
}

void DeleteRear(Deque* deque){
    if(deque->tail == 0){
        deque->tail = CELL_MAX-1;
    } else{
        deque->tail = deque->tail-1;
    }
    deque->size--;
}

void DeleteFront(Deque* deque){
    if(deque->head == CELL_MAX -1){
        deque->head = 0;
    } else{
        deque->head = deque->head +1;
    }
    deque->size--;
}
//deque end---------------------------------------

//Draw Snake and Movement start---------------------------
void DrawSnake(Snake* snake){
    for(int i = 0; i < snake->body.size; i++){
        int index = (snake->body.head + i) % CELL_MAX;
        float x = snake->body.segment[index].x;
        float y = snake->body.segment[index].y;
        Rectangle snakeBody = (Rectangle){OFFSET+ x*cellSize, OFFSET+60 + y*cellSize, (float)cellSize, (float)cellSize};
        DrawRectangleRounded(snakeBody, 0.5, 6, WHITE);    
    }

    float headx = snake->body.segment[snake->body.head].x;
    float heady = snake->body.segment[snake->body.head].y;
    float pixelx = OFFSET + headx * cellSize;
    float pixely = OFFSET + 60 + heady * cellSize;

    
    float eyeOffsetX = snake->direction.y * 5;
    float eyeOffsetY = snake->direction.x * 5;

    DrawCircle(pixelx + cellSize/2 + eyeOffsetX, pixely + cellSize/2 - eyeOffsetY, 3, BLACK);
    DrawCircle(pixelx + cellSize/2 - eyeOffsetX, pixely + cellSize/2 + eyeOffsetY, 3, BLACK);
}

void MoveSnakeUpdate(Snake* snake){
    if(reversed){
        if(snake->growSnake > 0){
            InsertRear(&snake->body, Vector2Subtract(snake->body.segment[snake->body.tail], snake->direction));
            snake->growSnake--;
        } else{
            InsertRear(&snake->body, Vector2Subtract(snake->body.segment[snake->body.tail], snake->direction));
            DeleteFront(&snake->body);
        }
    } else{
        if(snake->growSnake > 0){
            InsertFront(&snake->body, Vector2Add(snake->body.segment[snake->body.head], snake->direction));
            snake->growSnake--;
        } else{
            InsertFront(&snake->body, Vector2Add(snake->body.segment[snake->body.head], snake->direction));
            DeleteRear(&snake->body);
        }
    }
}
//Draw Snake and Movement end---------------------------

//Collision Checks Start---------------------------------
void WallCollisionCheck(Snake* snake, struct FoodNode** food, WallPattern* wall){
    Vector2 reversedHead = Reverse_Or_Not(snake);
    if(reversedHead.x == CELL_COUNT || reversedHead.x == -1){
        GameOver(snake, food, wall);
    }
    if(reversedHead.y == PLAY_ROWS || reversedHead.y == 0){
        GameOver(snake, food, wall);
    }
}

Vector2 Reverse_Or_Not(Snake* snake){
    if(reversed){
        return snake->body.segment[snake->body.tail];
    }
    return snake->body.segment[snake->body.head];
}


void BodyCollisionCheck(Snake* snake, struct FoodNode** food, WallPattern* wall){
    if(snake->body.size <= 3){
         return; 
    }
    if(reversed){
        Vector2 tail = snake->body.segment[snake->body.tail];
        for(unsigned int i = 0; i < snake->body.size - 1; i++){
            int index = (snake->body.head + i) % CELL_MAX;
            if(Vector2Equals(snake->body.segment[index], tail)){
                GameOver(snake, food, wall);
                return;
            }
        }

    } else{
        Vector2 head = snake->body.segment[snake->body.head];
        if(CheckFoodInSegment(snake, head, 1)){
            GameOver(snake, food, wall);
        }
    }
    
}

void Check_Snake_Wall_Collision(WallPattern* wall, Snake* snake, FoodNode** food){
    Vector2 reversedHead = Reverse_Or_Not(snake);
    for(unsigned int i = 0; i < wall->activeWalls; i++){
        if(Vector2Equals(reversedHead, wall->cell[i])){
                GameOver(snake, food, wall);
        }
    }
}
//Collision Checks End----------------------------------
//{{{Snake End}}}


//{{{Input Queue Start}}}
//------------------------------------------------------------------------------------------------------------------------------------
void Init_Input(InputSnake* input){
    input->front = 0;
    input->rear = 0;
    input->size = 0;
}

void Input_Enqueue(InputSnake* input, Vector2 point){
    if(input->size >= INPUT_QUEUE_SIZE){
        return;
    }
    input->sinput[input->rear] = point;
    input->rear = (input->rear + 1) % INPUT_QUEUE_SIZE;
    input->size++;
}

Vector2 Input_Dequeue(InputSnake* input){
    if(input->size <= 0){
        return (Vector2){0,0};
    }
    Vector2 point = input->sinput[input->front];
    input->front = (input->front + 1) % INPUT_QUEUE_SIZE;
    input->size--;
    return point;
}
//------------------------------------------------------------------------------------------------------------------------------------
//{{{Input Queue End}}}


//{{{Wall Start}}}
//Walls--------------------------------------------------------------------------------------------------------------------------------
void Draw_Wall(WallPattern* wall){
    if(pendingWall != -1 && isWallFlashing){
        for(unsigned int i = wall->activeWalls; i < pendingWall+1; i++){
            float x = wall->cell[i].x;
            float y = wall->cell[i].y;
            Rectangle r = (Rectangle){OFFSET + x*cellSize, OFFSET+60 + y*cellSize, (float)cellSize, (float)cellSize};
            DrawRectangleRounded(r, 0.3, 4, RED);
        }
    }

    for(int i = 0; i <= wall->activeWalls - 1; i++){
        float x = wall->cell[i].x;
        float y = wall->cell[i].y;
        Rectangle wallPat = (Rectangle){OFFSET+ x*cellSize, OFFSET+60 + y*cellSize, (float)cellSize, (float)cellSize};
        DrawRectangleRounded(wallPat, 0.3, 4, darkGreen);
    }
}

void Show_Wall(WallPattern* wall){
    if(wall->activeWalls >= wall->count){
        return;
    }

    int actualWalls[] = {
    5,      //wall 1
    15,     //wall 2
    20,     //wall 3
    29,     //wall 4
    40,     //wall 5
    49,     //wall 6
    60,     //wall 7
    73,     //wall 8
    81      //wall 9
    };
    int wallAmmount = 9;
    int currentGroup = 0;

    for(unsigned int i = 0; i < wallAmmount; i++){
        if(wall->activeWalls <= actualWalls[i]){
            pendingWall = actualWalls[i];
            flashWallTimer = 3.0;
            isWallFlashing = true;
            return;
        }
    }
}

void Wall_Flash_Update(WallPattern* wall){
    if(pendingWall == -1){
        return;
    }

    flashWallTimer -= GetFrameTime();
    isWallFlashing = (int)(flashWallTimer * 6) % 2 == 0;

    if(flashWallTimer <= 0){
        wall->activeWalls = pendingWall + 1;
        pendingWall = -1;
        isWallFlashing = false;
    }
}
//------------------------------------------------------------------------------------------------------------------------------------
//{{{Wall End}}}


//{{{Food Start}}}
//Linked List Start-----------------------------------------
struct FoodNode* CreateFoodNode(Vector2 position, FoodType type){
    struct FoodNode* newNode = (struct FoodNode*)malloc(sizeof(struct FoodNode));
    newNode->position = position;
    newNode->type = type;
    newNode->next = NULL;
    return newNode;
}

void Link_Add_Beg(struct FoodNode** head, Vector2 position, FoodType type){
    struct FoodNode* newNode = CreateFoodNode(position, type);
    newNode->next = *head;
    *head = newNode;
}

void Link_Add_End(struct FoodNode** head, Vector2 position, FoodType type){
    struct FoodNode* newNode = CreateFoodNode(position, type);
    if(*head == NULL){
        *head = newNode;
        return;
    }
    struct FoodNode* walker = *head;
    while(walker->next != NULL){
        walker = walker->next;
    }
    walker->next = newNode;
}

void Link_Dele_Beg(struct FoodNode** head){
    struct FoodNode* walker = *head;
    if(*head ==NULL){
        return;
    }
    *head = (*head)->next;
    free(walker);
}

void Link_Dele_End(struct FoodNode** head){
    struct FoodNode* walker = *head;
    if(*head == NULL){
        return;
    }
    if((*head)->next == NULL){
        free(*head);
        *head = NULL;
        return;
    }
    while(walker->next->next != NULL){
        walker = walker->next;
    }
    free(walker->next);
    walker->next = NULL;
}

void Link_Clear(struct FoodNode** head){
    struct FoodNode* walker = *head;
    while(walker != NULL){
        struct FoodNode* temp = walker->next;
        free(walker);
        walker = temp;
    }
    *head = NULL;
}

void Link_Dele_Node(struct FoodNode** head, struct FoodNode* target){
    struct FoodNode* walker = *head;
    if(*head == NULL){
        return;
    }
    if(*head == target){
        *head = target->next;
        free(target);
        return;
    }

    while(walker->next !=NULL && walker->next != target){
        walker = walker->next;
    }

    walker->next = target->next;
    free(target);
}

//Linked List End-----------------------------------------

//Collisions And Spawning-------------------------------------------------------------------------------
void Draw_Food(struct FoodNode* head){
    struct FoodNode* walker = head;
    while(walker != NULL){
        DrawCircle(OFFSET+ walker->position.x *cellSize +cellSize/2,
        OFFSET+60 + walker->position.y *cellSize +cellSize/2,
        cellSize/2, foodColors[walker->type]);
        walker = walker->next;
    }
}
struct FoodNode* Check_Food_Collision_V2(struct FoodNode* head, Snake* snake){
    struct FoodNode* walker = head;
    if(reversed){
        while(walker != NULL){
            if(Vector2Equals(walker->position, snake->body.segment[snake->body.tail])){
                return walker;
            }
            walker = walker->next;
        }
    } else{
        while(walker != NULL){
            if(Vector2Equals(walker->position, snake->body.segment[snake->body.head])){
                return walker;
            }
            walker = walker->next;
        }
    }
    return NULL;
}

void SpawnFood(struct FoodNode** head, Snake* snake, WallPattern* wall){
    Vector2 position = Random_Food_Position();
    while(CheckFoodInSegment(snake, position, 0) || Check_Food_In_Wall(wall, position, 0) || Check_Food_In_Food(*head, position)){
        position = Random_Food_Position();
    }
    FoodType type = FoodTypeChance();
    Link_Add_Beg(head, position, type);
}

Vector2 Random_Food_Position(){
    return (Vector2){GetRandomValue(0, CELL_COUNT-1),
         GetRandomValue(1, PLAY_ROWS-1)};
} 

FoodType FoodTypeChance(){
    int roll = GetRandomValue(0, 99);
    int cumulative = 0;
    for(int i = 0; i < Reverse + 1; i++){
        cumulative += foodChance[i];
        if(roll < cumulative){
            return (FoodType)i;
        }
    }
    return NormalFood;
}

bool CheckFoodInSegment(Snake* snake, Vector2 point, int index){
    for(unsigned int i = index; i < snake->body.size; i++ ){
        int bodyIndex = (snake->body.head + i) % CELL_MAX;
        if(Vector2Equals(snake->body.segment[bodyIndex], point)){
            return true;
        }
    }
    return false;
}

bool Check_Food_In_Wall(WallPattern* wall, Vector2 point, int index){
    for(unsigned int i = index; i < wall->count; i++){
        if(Vector2Equals(point, wall->cell[i])){
            return true;
        }
    }
    return false;
}

bool Check_Food_In_Food(FoodNode* head, Vector2 point){
    struct FoodNode* walker = head;
    while(walker != NULL){
        if(Vector2Equals(walker->position, point)){
            return true;
        }
        walker = walker->next;
    }
    return false;
}


void Apply_Food_Effect(struct FoodNode* food, Snake* snake, TextPop* textpop){
    switch(food->type){
        case NormalFood:
            snake->growSnake = 1;
            snake->score += 1 * scoreMulti;
            if(snake->score >= nextWallScore){
                Show_Wall(&wall);
                nextWallScore += wallScoreAdd;
            }
            textpop->active = true;
            textpop->timer = 2.5f;
            textpop->type = NormalFood;
            textpop->index = GetRandomValue(0, 11);
            break;
        case ZaApplo:
            snake->growSnake = 1;
            gameInterval = 0.30;
            snake->score += 1 * scoreMulti;
            if(snake->score >= nextWallScore){
                Show_Wall(&wall);
                nextWallScore += wallScoreAdd;
            }
            speedTimer = 60;
            textpop->active = true;
            textpop->timer = 2.5f;
            textpop->type = ZaApplo;
            break;
        case FoodInHeaven:
            gameInterval = 0.12;
            snake->score += 1 * scoreMulti;
            if(snake->score >= nextWallScore){
                Show_Wall(&wall);
                nextWallScore += wallScoreAdd;
            }
            speedTimer = 75;
            textpop->active = true;
            textpop->timer = 2.5f;
            textpop->type = FoodInHeaven;
            break;
        case BigApple:
            snake->growSnake = 3;
            snake->score += 3 * scoreMulti;
            if(snake->score >= nextWallScore){
                Show_Wall(&wall);
                nextWallScore += wallScoreAdd;
            }
            textpop->active = true;
            textpop->timer = 2.5f;
            textpop->type = BigApple;
            break;
        case Minimize:
            if(snake->body.size > 5){
                int removeCount = 4 * scoreMulti;
                for(int i = 0; i < removeCount; i++){
                    if(snake->body.size <= 3) break;
                    DeleteRear(&snake->body);
                }
                snake->score += 1 * scoreMulti;
            } else {
                snake->score += 1 * scoreMulti;
            }
            if(snake->score >= nextWallScore){
                Show_Wall(&wall);
                nextWallScore += wallScoreAdd;
            }
            textpop->active = true;
            textpop->timer = 2.5f;
            textpop->type = Minimize;
            break;
        case FoodRush:
            snake->growSnake = 1;
            if(scoreMulti >= 2){
                scoreMulti += 2;
                scoreMultiTimer = 90;
            } else{
                scoreMultiTimer = 90;
                scoreMulti = 2;
            }
            snake->score += 1 * scoreMulti;
            if(snake->score >= nextWallScore){
                Show_Wall(&wall);
                nextWallScore += wallScoreAdd;
            }
            textpop->active = true;
            textpop->timer = 2.5f;
            textpop->type = FoodRush;
            break;
        case Reverse:
            snake->growSnake = 1;
            snake->score += 1 * scoreMulti;
            if(snake->score >= nextWallScore){
                Show_Wall(&wall);
                nextWallScore += wallScoreAdd;
            }
            reversed = true;
            reversedTimer = 60;
            textpop->active = true;
            textpop->timer = 2.5f;
            textpop->type = Reverse;
            textpop->index = GetRandomValue(0, 11);
            break;
    }
}
//{{{Food End}}}

//{{{Game and Drawing Functions Start}}}
void Draw_Text_Pop_Special(char* message, TextPop* textpop, Color color){
    if(textpop->active){
        int fontsize = (int)(30*textpop->bounce);
        int width = MeasureText(message, fontsize);
        DrawText(message, ((2*OFFSET + CELL_SIZE*CELL_COUNT)/2) - (width/2), 25, fontsize, color);
    }
}

void Draw_Food_Effect_Text(){
    int y = OFFSET + 60;

    if(speedTimer > 0){
        if(gameInterval > 0.2){
            DrawText("+SLOWED!", OFFSET, y, 16, RED);
        } else if(gameInterval < 0.2){
            DrawText("+SPEEDING!", OFFSET, y, 16, RED);
        }
    }
    if(scoreMulti > 1){
        DrawText(TextFormat("+%dxMULTIPLIER!", scoreMulti), OFFSET, y-15, 16, GOLD);
    }
    if(reversedTimer > 0){
        DrawText(TextFormat("+REVERSED! %d", reversedTimer), OFFSET, y-30, 16, PINK);
    }
}

void DrawTextPop(TextPop* textpop, Snake* snake){
    if(!textpop->active){
        return;
    }
    
    switch(textpop->type){
        case NormalFood:
            const char messages[12][30] = {
            "Wow... food. Shocking!",
            "Oh look, I'm eating again.",
            "Didn't see that coming.",
            "This again?",
            "Another snack, another day.",
            "Give me more!",
            "Lami Ka-ayo!",
            "Me Carnivore, Yes",
            "Need more",
            "Crunch",
            "Scrumptious",
            "Tasty"
            };
            if(textpop->active){
                int fontsize = (int)(30*textpop->bounce);
                int width = MeasureText(messages[textpop->index], fontsize);
                DrawText(messages[textpop->index], ((2*OFFSET + CELL_SIZE*CELL_COUNT)/2) - (width/2), 25, fontsize, YELLOW);
            }
            break;
        case ZaApplo:
            Draw_Text_Pop_Special("Toki Yo Tomare!", textpop, GOLD);
        break;
        case FoodInHeaven:
            Draw_Text_Pop_Special("Food In Heaven!", textpop, WHITE);
        break;
        case BigApple:
            Draw_Text_Pop_Special("Getting Bigger", textpop, darkRed);
        break;
        case Minimize:
            Draw_Text_Pop_Special("Snake used Minimize", textpop, PURPLE);
        break;
        case FoodRush:
            Draw_Text_Pop_Special("Make It Rain!", textpop, ORANGE);
        break;
        case Reverse:
            Draw_Text_Pop_Special("Bites The Apple!", textpop, PINK);
        break;
    }
    
}

void UpdateTextPop(TextPop* textpop){
    if(textpop->active){
        textpop->timer -= GetFrameTime();
        textpop->bounce = 1.0f + sinf(textpop->timer * 10) * 0.1f;
        if(textpop->timer <= 0) textpop->active = false;
    }
}

void Reset(Snake* snake){
    *snake = MakeSnake();

}

void GameOver(Snake* snake, struct FoodNode** food, WallPattern* wall){
    running = false;
    paused = false;
    textpop.active = false;
    snake->score = 0;
    Reset(snake);
    speedTimer = 0;
    gameInterval = 0.2;
    scoreMulti = 1;
    scoreMultiTimer = 0;
    wall->activeWalls = 0;
    nextWallScore = wallScoreAdd;
    pendingWall = -1;
    flashWallTimer = 0;
    isWallFlashing = false;
    reversed = false;
    reversedTimer = 0;
    Link_Clear(food);
    for(int i = 0; i < 4; i++){
        SpawnFood(food, snake, wall);
    }
}

void AnimGameOver(TextPop* textpop){
    if(!running){
        const char GameEnd[] = {"Game Over!"};
        int width = MeasureText(GameEnd, 30);
        DrawText(GameEnd, ((2*OFFSET + CELL_SIZE*CELL_COUNT)/2) - (width/2), 25, 30, YELLOW);
    }
}
//{{{Game and Drawing Functions End}}}