#include <iostream>
#include <raylib.h>
#include <raymath.h>
#include <deque>

using namespace std;

static bool allowMove = false;
constexpr double UPDATE_INTERVAL = 0.2;

Color green = {173, 204, 96, 255};
Color darkGreen = {43, 51, 24, 255};

//30*25=750
const int CELL_SIZE = 30;
const int CELL_COUNT = 25;
const int OFFSET = 75;

double lastUpdateTime = 0;

int globalScore = 0; // Global score variable

bool elementInDeque(const Vector2& element, const deque<Vector2>& deque)
{
    for (const auto& item : deque)
    {
        if (Vector2Equals(item, element))
        {
            return true;
        }
    }
    return false;
}

bool eventTriggered(const double interval)
{
    const double currentTime = GetTime();

    if((currentTime - lastUpdateTime) >= interval)
    {
        lastUpdateTime = currentTime;
        return true;
    }

    return false;
}

class Snake
{
public:
    deque<Vector2> body = {Vector2{6, 9}, Vector2{5, 9}, Vector2{4, 9}};
    Vector2 direction = {1, 0};
    bool addSegment = false;

    void draw()
    {
        for (const auto& segment : body)
        {
            Rectangle segmentRect = Rectangle{(OFFSET + segment.x * CELL_SIZE), (OFFSET + segment.y * CELL_SIZE), (float)CELL_SIZE, (float)CELL_SIZE};
            DrawRectangleRounded(segmentRect, 0.5, 6, darkGreen);
        }
    }

    void update()
    {
        body.push_front(Vector2Add(body[0], direction));
        if(addSegment)
        {
            addSegment = false;
        }
        else
        {
            body.pop_back();
        }
    }

    void reset()
    {
        body = {Vector2{6, 9}, Vector2{5, 9}, Vector2{4, 9}};
        direction = {1, 0};
    }

};

class Food
{
public:
    Vector2 position;
    Texture2D texture;

    Food(deque<Vector2> snakeBody) : position(generateRandomPos(snakeBody)) {
        Image image = LoadImage("Graphics/food.png");
        texture = LoadTextureFromImage(image);
        UnloadImage(image);
    }

    ~Food()
    {
        UnloadTexture(texture);
    }

    void draw()
    {
        DrawTexture(texture, (OFFSET + position.x*CELL_SIZE), (OFFSET + position.y*CELL_SIZE), WHITE);
    }

    Vector2 generateRandomCell()
    {
        float x = GetRandomValue(0, CELL_COUNT-1);
        float y = GetRandomValue(0, CELL_COUNT-1);

        return Vector2{x, y};
    }

    Vector2 generateRandomPos(const deque<Vector2>& snakeBody)
    {
        Vector2 position;
        do
        {
            position = generateRandomCell();
        } while (elementInDeque(position, snakeBody));

        return position;
    }

};

class Menu
{
public:
    int selectedOption = 0;

    void draw()
    {
        DrawText("Snake Classic", OFFSET - 5, 20, 40, darkGreen);
        DrawText("Credit: HelloKindGit", OFFSET + 455, 30, 30, darkGreen);
        DrawText("1. New Game", OFFSET + 10, OFFSET + 100, 30, selectedOption == 0 ? DARKGRAY : darkGreen);
        DrawText("2. Main Menu", OFFSET + 10, OFFSET + 150, 30, selectedOption == 1 ? DARKGRAY : darkGreen);
        DrawText("3. Exit", OFFSET + 10, OFFSET + 200, 30, selectedOption == 2 ? DARKGRAY : darkGreen);
        DrawText(("High Score: " + to_string(globalScore)).c_str(), OFFSET + 10, OFFSET + 250, 30, darkGreen);
    }

    int update()
    {
        if (IsKeyPressed(KEY_UP))
        {
            selectedOption = (selectedOption - 1 + 3) % 3;
        }

        if (IsKeyPressed(KEY_DOWN))
        {
            selectedOption = (selectedOption + 1) % 3;
        }

        if (IsKeyPressed(KEY_ENTER))
        {
            return selectedOption + 1; // Return the selected option
        }

        return 0; // No option selected yet
    }
};

class Game
{
public:
    Snake snake = Snake();
    Food food = Food(snake.body);
    bool running = true;
    bool paused = false;
    Menu pauseMenu;
    int score = 0;

    Sound eatSound;
    Sound wallSound;

    Game()
    {
        InitAudioDevice();
        eatSound = LoadSound("Sounds/eat.mp3");
        wallSound = LoadSound("Sounds/wall.mp3");
    }

    ~Game()
    {
        UnloadSound(eatSound);
        UnloadSound(wallSound);
        CloseAudioDevice();
    }

    void update()
    {
        if (running && !paused)
        {
            snake.update();
            checkCollisionWithFood();
            checkCollisionWithTail();
            checkCollisionWithEdge();
        }
    }

    void togglePause()
    {
        paused = !paused;
    }

    void drawPauseMenu()
    {
        pauseMenu.draw();
    }

    int updatePauseMenu()
    {
        return pauseMenu.update();
    }

    void draw()
    {
        if (paused)
        {
            drawPauseMenu();
        }
        else
        {
            food.draw();
            snake.draw();
        }
    }

    void checkCollisionWithFood()
    {
        if (Vector2Equals(snake.body[0], food.position))
        {
            food.position = food.generateRandomPos(snake.body);
            snake.addSegment = true;
            score++;
            PlaySound(eatSound);
        }
    }

    void checkCollisionWithTail()
    {
        const Vector2& head = snake.body.front();
        const deque<Vector2>& bodyWithoutHead = {snake.body.begin() + 1, snake.body.end()};

        if (elementInDeque(head, bodyWithoutHead))
        {
            gameOver();
        }
    }

    void checkCollisionWithEdge()
    {
        if (snake.body[0].x == CELL_COUNT || snake.body[0].x == -1 ||
            snake.body[0].y == CELL_COUNT || snake.body[0].y == -1)
        {
            gameOver();
        }
    }

    void gameOver()
    {
        snake.reset();
        food.position = food.generateRandomPos(snake.body);
        running = false;
        globalScore = max(globalScore, score);
        score = 0;        
        PlaySound(wallSound);
    }
};

int main()
{
    puts("Game Started");
    InitWindow((2 * OFFSET + CELL_SIZE * CELL_COUNT), (2 * OFFSET + CELL_SIZE * CELL_COUNT), "Snake Classic");
    SetTargetFPS(60);

    Menu menu;
    Game game;

    bool inMenu = true;

    while (!WindowShouldClose())
    {
        if (inMenu)
        {
            BeginDrawing();
            ClearBackground(green);

            int selectedOption = menu.update();
            menu.draw();

            EndDrawing();

            if (selectedOption == 1)
            {
                inMenu = false;
            }
            else if (selectedOption == 3)
            {
                CloseWindow();
                puts("Game Closed");
                return 0;
            }
        }
        else
        {
            BeginDrawing();
            ClearBackground(green);

            if (IsKeyPressed(KEY_P)) // Press 'P' to pause/unpause
            {
                game.togglePause();
            }

            if (!game.paused)
            {
                if (eventTriggered(UPDATE_INTERVAL))
                {
                    game.update();
                    allowMove = true;
                }

                if (IsKeyPressed(KEY_UP) && game.snake.direction.y != 1 && allowMove)
                {
                    game.snake.direction = {0, -1};
                    game.running = true;
                    allowMove = false;
                }

                if(IsKeyPressed(KEY_UP) && game.snake.direction.y != 1 && allowMove)
                {
                    game.snake.direction = {0, -1};
                    game.running = true;
                    allowMove = false;
                }

                if(IsKeyPressed(KEY_DOWN) && game.snake.direction.y != -1 && allowMove)
                {
                    game.snake.direction = {0, 1};
                    game.running = true;
                    allowMove = false;
                }

                if(IsKeyPressed(KEY_LEFT) && game.snake.direction.x != 1 && allowMove)
                {
                    game.snake.direction = {-1, 0};
                    game.running = true;
                    allowMove = false;
                }

                if(IsKeyPressed(KEY_RIGHT) && game.snake.direction.x != -1 && allowMove)
                {
                    game.snake.direction = {1, 0};
                    game.running = true;
                    allowMove = false;
                }
                
            }
            else
            {
                int pauseMenuOption = game.updatePauseMenu();

                if (pauseMenuOption == 1)
                {
                    game.togglePause();
                    game.gameOver();
                    
                }
                else if (pauseMenuOption == 2) // Main Menu
                {
                    inMenu = true;
                    game.togglePause();
                    game.gameOver();
                }
                else if (pauseMenuOption == 3) // Exit
                {
                    CloseWindow();
                    puts("Game Closed");
                    return 0;
                }
            }

            // Drawing
            DrawRectangleLinesEx(Rectangle{(float)OFFSET - 5, (float)OFFSET - 5, (float)CELL_SIZE * CELL_COUNT + 10, (float)CELL_SIZE * CELL_COUNT + 10}, 5, darkGreen);
            DrawText("Snake Classic", OFFSET - 5, 20, 40, darkGreen);
            DrawText(TextFormat("%i", game.score), OFFSET - 5, OFFSET + CELL_SIZE * CELL_COUNT + 10, 40, darkGreen);
            game.draw();

            EndDrawing();
        }
    }

    CloseWindow();

    puts("Game Closed");
    return 0;
}
