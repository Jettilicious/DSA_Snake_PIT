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

//Constants and Inits
//------------------------------------------------------//------------------------------------------------------
const int cellSize = CELL_SIZE;
const int cellCount = CELL_COUNT;
    
Color Background = {10, 147, 150, 255};
Color Apple = RED;
Color darkGreen = {43, 51, 24, 255};
Color darkRed = {139, 0, 0, 255};
bool running = true;
bool allowMove = true;

double lastUpdateTime = 0;
double gameInterval = 0.2;
int speedTimer = 0;
int scoreMulti = 1;
int scoreMultiTimer = 0;

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
//------------------------------------------------------//------------------------------------------------------

//Food
//------------------------------------------------------//------------------------------------------------------
typedef enum{       //Description:      //Color:    //Chance:
    NormalFood,     // The generic food // Red      //Very Common
    ZaApplo,        // 0.8x speed       // Yellow   //Epic
    FoodInHeaven,   // 2x speed         // White    //Uncommon
    BigApple,       // +3 segments      // Dark Red //Uncommon
    Minimize,       // -2 segments      // Blue     //Epic
    FoodRush,       // 2x points        // Orange   //Epic
}FoodType;

Color foodColors[] = {
    RED,                        // NormalFood
    YELLOW,                     // ZaApplo
    WHITE,                      // FoodInHeaven
    {139, 0, 0, 255},           // BigApple (dark red)
    PURPLE,                       // Minimize
    ORANGE,                     // FoodRush
};

int foodChance[] = {
    50,     //Normal Food
    7,      //Za Applo
    15,     //Food In Heaven
    15,     //Big Apple
    8,      //Minimize
    5       //FoodRush
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

//Collisions-And-Spawning-------------------------------------------------------------------------------
struct FoodNode* Check_Food_Collision_V2(struct FoodNode* head, Snake* snake){
    struct FoodNode* walker = head;
    while(walker != NULL){
        if(Vector2Equals(walker->position, snake->body.segment[snake->body.head])){
            return walker;
        }
        walker = walker->next;
    }
    return NULL;
}

Vector2 Random_Food_Position(){
    return (Vector2){GetRandomValue(0, CELL_COUNT-1),
         GetRandomValue(1, PLAY_ROWS-1)};
} 

FoodType FoodTypeChance(){
    int roll = GetRandomValue(0, 99);
    int cumulative = 0;
    for(int i = 0; i < FoodRush + 1; i++){
        cumulative += foodChance[i];
        if(roll < cumulative){
            return (FoodType)i;
        }
    }
    return NormalFood;
}

void SpawnFood(struct FoodNode** head, Snake* snake){
    Vector2 position = Random_Food_Position();
    while(CheckFoodInSegment(snake, position, 0)){
        position = Random_Food_Position();
    }
    FoodType type = FoodTypeChance();
    Link_Add_Beg(head, position, type);
}

void Apply_Food_Effect(struct FoodNode* food, Snake* snake, TextPop* textpop){
    switch(food->type){
        case NormalFood:
            snake->growSnake = 1;
            snake->score += 1 * scoreMulti;
            textpop->active = true;
            textpop->timer = 2.5f;
            textpop->type = NormalFood;
            textpop->index = GetRandomValue(0, 11);
            break;
        case ZaApplo:
            snake->growSnake = 1;
            gameInterval = 0.40;
            snake->score += 1 * scoreMulti;
            speedTimer = 60;
            textpop->active = true;
            textpop->timer = 2.5f;
            textpop->type = ZaApplo;
            break;
        case FoodInHeaven:
            gameInterval = 0.10;
            snake->score += 1 * scoreMulti;
            speedTimer = 75;
            textpop->active = true;
            textpop->timer = 2.5f;
            textpop->type = FoodInHeaven;
            break;
        case BigApple:
            snake->growSnake = 3;
            snake->score += 3 * scoreMulti;
            textpop->active = true;
            textpop->timer = 2.5f;
            textpop->type = BigApple;
            break;
        case Minimize:
            if(snake->body.size <= 3){
                snake->score += 1 * scoreMulti;
            } else{
                DeleteRear(&snake->body);
                DeleteRear(&snake->body);
                snake->score += 1 * scoreMulti;
            }
            textpop->active = true;
            textpop->timer = 2.5f;
            textpop->type = Minimize;
            break;
        case FoodRush:
            snake->growSnake = 1;
            scoreMulti = 2;
            scoreMulti = 2;
            scoreMultiTimer = 200;
            snake->score += 1 * scoreMulti;
            textpop->active = true;
            textpop->timer = 2.5f;
            textpop->type = FoodRush;
            break;
    }
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

//Draw---------------------------------------------------------------------
void Draw_Food(struct FoodNode* head){
    struct FoodNode* walker = head;
    while(walker != NULL){
        DrawCircle(OFFSET+ walker->position.x *cellSize +cellSize/2,
        OFFSET+60 + walker->position.y *cellSize +cellSize/2,
        cellSize/2, foodColors[walker->type]);
        walker = walker->next;
    }
}

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
        DrawText("+2xMULTIPLIER!", OFFSET, y-15, 16, GOLD);
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
            Draw_Text_Pop_Special("Muda Muda!", textpop, GOLD);
        break;
        case FoodInHeaven:
            Draw_Text_Pop_Special("1..3..5..7..9..11...", textpop, WHITE);
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
    }
    
}
//------------------------------------------------------//------------------------------------------------------


//Food
//------------------------------------------------------
void UpdateTextPop(TextPop* textpop){
    if(textpop->active){
        textpop->timer -= GetFrameTime();
        textpop->bounce = 1.0f + sinf(textpop->timer * 10) * 0.1f;
        if(textpop->timer <= 0) textpop->active = false;
    }
}
//------------------------------------------------------



//Snake and Movement
//------------------------------------------------------
void InsertFront(Deque* deque, Vector2 pos){
    if(deque->head == 0){
        deque->head = CELL_MAX -1;
    } else{
        deque->head = deque->head -1;
    }
    deque->segment[deque->head] = pos;
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

void DrawSnake(Snake* snake){
    for(int i = 0; i < snake->body.size; i++){
        int index = (snake->body.head + i) % CELL_MAX;
        float x = snake->body.segment[index].x;
        float y = snake->body.segment[index].y;
        Rectangle snakeBody = (Rectangle){OFFSET+ x*cellSize, OFFSET+60 + y*cellSize, (float)cellSize, (float)cellSize};
        DrawRectangleRounded(snakeBody, 0.5, 6, WHITE);    
    }
}

void MoveSnakeUpdate(Snake* snake){
    if(snake->growSnake > 0){
        InsertFront(&snake->body, Vector2Add(snake->body.segment[snake->body.head], snake->direction));
        snake->growSnake--;
    } else{
        InsertFront(&snake->body, Vector2Add(snake->body.segment[snake->body.head], snake->direction));
        DeleteRear(&snake->body);
    }
}
//------------------------------------------------------


// Collisions
//------------------------------------------------------
void Reset(Snake* snake){
    *snake = MakeSnake();

}

void GameOver(Snake* snake, struct FoodNode** food){
    running = false;
    textpop.active = false;
    snake->score = 0;
    Reset(snake);
    speedTimer = 0;
    gameInterval = 0.2;
    scoreMulti = 1;
    scoreMultiTimer = 0;
    Link_Clear(food);
    for(int i = 0; i < 3; i++){
        SpawnFood(food, snake);
    }
}

void AnimGameOver(TextPop* textpop){
    if(!running){
        const char GameEnd[] = {"Game Over!"};
        int width = MeasureText(GameEnd, 30);
        DrawText(GameEnd, ((2*OFFSET + CELL_SIZE*CELL_COUNT)/2) - (width/2), 25, 30, YELLOW);
    }
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

void WallCollisionCheck(Snake* snake, struct FoodNode** food){
    if(snake->body.segment[snake->body.head].x == CELL_COUNT || snake->body.segment[snake->body.head].x == -1){
        GameOver(snake, food);
    }
    if(snake->body.segment[snake->body.head].y == PLAY_ROWS || snake->body.segment[snake->body.head].y == 0){
        GameOver(snake, food);
    }
}

void BodyCollisionCheck(Snake* snake, struct FoodNode** food){
    if(snake->body.size <= 3) return;  // skip when snake is small
    Vector2 head = snake->body.segment[snake->body.head];
    if(CheckFoodInSegment(snake, head, 1)){
        GameOver(snake, food);
    }
}
//------------------------------------------------------


int main(void)
{   
    struct FoodNode* head = NULL;

    //Window
    SetConfigFlags(FLAG_WINDOW_TOPMOST);
    InitWindow(2* OFFSET + CELL_SIZE*CELL_COUNT, 2* OFFSET + CELL_SIZE*CELL_COUNT, "DSA Snake Game");
    SetWindowFocused();
    SetTargetFPS(60);

    Snake snake = MakeSnake();
    for(int i = 0; i < 3; i++){
        SpawnFood(&head, &snake);
    }
    
    while (!WindowShouldClose())
    {
        //Event handling
        if(IsKeyPressed(KEY_UP) && snake.direction.y != 1 && allowMove){
            snake.direction = (Vector2){0, -1};
            running = true;
            allowMove = false;
        }
        if(IsKeyPressed(KEY_DOWN) && snake.direction.y != -1 && allowMove){
            snake.direction = (Vector2){0, 1}; 
            running = true;
            allowMove = false;
        }
        if(IsKeyPressed(KEY_LEFT) && snake.direction.x != 1 && allowMove){
            snake.direction = (Vector2){-1, 0};
            running = true;
            allowMove = false;
        }
        if(IsKeyPressed(KEY_RIGHT) && snake.direction.x != -1 && allowMove){
            snake.direction = (Vector2){1, 0};
            running = true;
            allowMove = false;
        }


        // Update Positions
        if(running){
            if(eventTrigger(gameInterval)){
                if(speedTimer > 0){
                    speedTimer--;
                    if(speedTimer == 0) gameInterval = 0.2;
                }
                if(scoreMultiTimer > 0){
                    scoreMultiTimer--;
                    if(scoreMultiTimer == 0){
                        scoreMulti = 1;
                    }
                }
                allowMove = true;
                struct FoodNode* eaten = Check_Food_Collision_V2(head, &snake);
                if(eaten){
                    Apply_Food_Effect(eaten, &snake, &textpop);
                    Link_Dele_Node(&head, eaten);
                    SpawnFood(&head, &snake);
                }
                MoveSnakeUpdate(&snake);
                WallCollisionCheck(&snake, &head);
                BodyCollisionCheck(&snake, &head);
            }
            UpdateTextPop(&textpop);
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
        EndDrawing();
    }
    CloseWindow();

    return 0;
}