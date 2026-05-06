#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

#define CELL_COUNT 25
#define CELL_SIZE 24
#define OFFSET 40
#define PLAY_ROWS 23
#define PLAY_TOP 0
#define CELL_MAX (CELL_COUNT*CELL_COUNT)

//Constants and Inits
//------------------------------------------------------
const int cellSize = CELL_SIZE;
const int cellCount = CELL_COUNT;
    
Color Background = {10, 147, 150, 255};
Color Apple = RED;
Color darkGreen = {43, 51, 24, 255};
bool running = true;
bool allowMove = true;

double lastUpdateTime = 0;

bool eventTrigger(double interval){
    double currentTime = GetTime();
    if(currentTime - lastUpdateTime >= interval){
        lastUpdateTime = currentTime;
        return true;
    }
    return false;
}

//------------------------------------------------------


// Structs and Stuff
//------------------------------------------------------
typedef struct{
    Vector2 position;
    int score;
} Food;
Food food;

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
    bool growSnake;
} Snake;
Snake snake;

typedef struct{
    bool active;
    float timer;
    float bounce;
    int index;
}TextPop;
TextPop textpop;

Snake MakeSnake(){
    Snake snake = {0};

    snake.body.segment[0] = (Vector2){6, 9};
    snake.body.segment[1] = (Vector2){5, 9};
    snake.body.segment[2] = (Vector2){4, 9};

    snake.body.head = 0;
    snake.body.tail = 2;
    snake.body.size = 3;

    snake.direction = (Vector2){1, 0};
    snake.growSnake = false;

    return snake;
}

//------------------------------------------------------

bool CheckFoodInSegment(Snake* snake, Vector2 point, int index);

//Food
//------------------------------------------------------
void UpdateTextPop(TextPop* textpop){
    if(textpop->active){
        textpop->timer -= GetFrameTime();
        textpop->bounce = 1.0f + sinf(textpop->timer * 10) * 0.1f;
        if(textpop->timer <= 0) textpop->active = false;
    }
}

void DrawTextPop(TextPop* textpop, Snake* snake){

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
}

void DrawFood(Food* food){
    DrawCircle(OFFSET+ food->position.x *cellSize +cellSize/2,
        OFFSET+60 + food->position.y *cellSize +cellSize/2,
        cellSize/2, Apple);
}

void RandomFoodPosition(Food* food){
    food->position = (Vector2){GetRandomValue(0, CELL_COUNT-1),
         GetRandomValue(1, PLAY_ROWS-1)};
} 

void FoodPosition(Snake* snake, Food* food){
    RandomFoodPosition(food);
    while(CheckFoodInSegment(snake, food->position, 0)){
        RandomFoodPosition(food);
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
    if(snake->growSnake == true){
        InsertFront(&snake->body, Vector2Add(snake->body.segment[snake->body.head], snake->direction));
        snake->growSnake = false;
    } else{
        InsertFront(&snake->body, Vector2Add(snake->body.segment[snake->body.head], snake->direction));
        DeleteRear(&snake->body);
    }
}
//------------------------------------------------------


// Collisions
//------------------------------------------------------

//Needs CLAUDE Check if it's same with the tutorial.
void Reset(Snake* snake){
    *snake = MakeSnake();

}

void GameOver(Snake* snake, Food* food){
    running = false;
    textpop.active = false;
    food->score = 0;
    Reset(snake);
    RandomFoodPosition(food);
}

void AnimGameOver(TextPop* textpop){
    if(!running){
        const char GameEnd[] = {"Game Over!"};
        int width = MeasureText(GameEnd, 30);
        DrawText(GameEnd, ((2*OFFSET + CELL_SIZE*CELL_COUNT)/2) - (width/2), 25, 30, YELLOW);
    }
}

void CheckFoodCollision(Snake* snake, Food* food, TextPop* textpop){
    if(Vector2Equals(snake->body.segment[snake->body.head], food->position)){
        textpop->active = true;
        textpop->timer = 3.0f;
        textpop->index = GetRandomValue(0, 12-1);
        FoodPosition(snake, food);
        snake->growSnake = true;
        food->score++;
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

void WallCollisionCheck(Snake* snake, Food* food){
    if(snake->body.segment[snake->body.head].x == CELL_COUNT || snake->body.segment[snake->body.head].x == -1){
        GameOver(snake, food);
    }
    if(snake->body.segment[snake->body.head].y == PLAY_ROWS || snake->body.segment[snake->body.head].y == 0){
        GameOver(snake, food);
    }
}

void BodyCollisionCheck(Snake* snake, Food* food){
    if(snake->body.size <= 3) return;  // skip when snake is small
    Vector2 head = snake->body.segment[snake->body.head];
    if(CheckFoodInSegment(snake, head, 1)){
        GameOver(snake, food);
    }
}
//------------------------------------------------------


int main(void)
{
    //Window
    SetConfigFlags(FLAG_WINDOW_TOPMOST);
    InitWindow(2* OFFSET + CELL_SIZE*CELL_COUNT, 2* OFFSET + CELL_SIZE*CELL_COUNT, "DSA Snake Game");
    SetWindowFocused();
    SetTargetFPS(60);

    Snake snake = MakeSnake();
    FoodPosition(&snake, &food);
    

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
            if(eventTrigger(0.2)){
                allowMove = true;
                CheckFoodCollision(&snake, &food, &textpop);
                MoveSnakeUpdate(&snake);
                WallCollisionCheck(&snake, &food);
                BodyCollisionCheck(&snake, &food);
            }
            UpdateTextPop(&textpop);
        }
        
        
        // Draw
        BeginDrawing();
            ClearBackground(Background);
            DrawRectangleLinesEx((Rectangle){(float)OFFSET-5, (float)OFFSET+70 + (CELL_SIZE * 0.4), (float)CELL_SIZE*CELL_COUNT+10, (float)CELL_SIZE*CELL_COUNT-62 - (CELL_SIZE*0.15)}, 5,darkGreen);
            DrawFood(&food);
            DrawSnake(&snake);
            int scoreWidth = MeasureText(TextFormat("%i", food.score), 20);
            DrawText(TextFormat("%i", food.score), (2*OFFSET + CELL_SIZE*CELL_COUNT) - scoreWidth - 25, 40, 40, YELLOW);
            DrawTextPop(&textpop, &snake);
            AnimGameOver(&textpop);
        EndDrawing();
    }
    CloseWindow();

    return 0;
}