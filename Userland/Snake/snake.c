#include <sys.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>


// <----------------------------------------------------------------------- DEFINES ----------------------------------------------------------------------->

#define VALID_DIFFICULTY(c) (c == 'E' || c == 'e' || c == 'M' || c == 'm'|| c == 'H' || c == 'h' || c == 'D' || c == 'd')

#define DIRECTIONS 4
#define BEGIN_GAME_KEY '\n'
#define PLAY_AGAIN_KEY '\n'
#define QUIT_KEY 'X'

// end_of_game values
#define ENDED_BY_CRASH 1
#define ENDED_BY_QUIT 2
#define ENDED_BY_MAX_SIZE 3

#define INITIAL_BODY_SIZE 3
#define MAX_BODY_SIZE 100 + INITIAL_BODY_SIZE + 1   // so that the snake eats 100 foods
#define SQUARE_DIM 32
#define MAX_SNAKES 2
#define HEAD 0

// sleeping times
#define DEFAULT_GAME_SLEEPING_TIME 500
#define DEFAULT_WELCOME_SLEEPING_TIME 700
#define DEFAULT_INSTRUCTIONS_SLEEPING_TIME 1000
#define DEFAULT_GOODBYE_SLEEPING_TIME 1000
#define DEMON 65
#define HARD 120
#define MEDIUM 170
#define EASY 220

// sounds
#define EATING_SOUND_FREQUENCY 1400
#define CRASHING_SOUND_FREQUENCY 300
#define CRASHING_SOUND_DURATION 550

// hues
#define HUE_1 120
#define HUE_2 0

// initial positions
#define INITIAL_POS_1_X (SQUARE_DIM * 28)
#define INITIAL_POS_1_Y (SQUARE_DIM * 21)
#define INITIAL_POS_2_X (SQUARE_DIM * 3)
#define INITIAL_POS_2_Y (SQUARE_DIM * 4)

// initial directions
#define INITIAL_DIR_1_X -1
#define INITIAL_DIR_1_Y 0
#define INITIAL_DIR_2_X 1
#define INITIAL_DIR_2_Y 0

// drawing
#define DEFAULT_BACKGROUND_COLOR 0x2F2F2F
#define DEFAULT_FOOD_COLOR 0xA933DC
#define ANSI_1 "\e[0;96m"
#define ANSI_2 "\e[0;31m"
#define OFFSET 4


// <----------------------------------------------------------------------- DATA TYPES ----------------------------------------------------------------------->

typedef struct {
    int width;
    int height;
} Square;

typedef struct {
    int x;
    int y;
} Position;

typedef struct {
    int x;
    int y;
} Direction;

typedef struct {
    Position position;
} SnakeBodySquare;

typedef struct {
    int initial_hue;
    char * main_ansi;
    Direction direction;
    int size;
    SnakeBodySquare body[MAX_BODY_SIZE];
    int lost;
} SnakeHeadSquare;

typedef struct {
    int hue;
    Position position;
} Food;


// <----------------------------------------------------------------------- FUNCTION PROTOTYPES ----------------------------------------------------------------------->

static void setSquareDimensions(void);
static void setDefaultFeatures(void);

static void welcomePlayers(void);
static void checkMaxBodySize(void);
static void endGameByMaxSize(void);
static void endGameByQuit(void);
static void endGameByCrash(void);
static void endGame(int code);
static void setDifficulty(char difficulty);
static void showWinners(void);

static void registerKeys(void);
static void movingTo(int snake, int dir_x, int dir_y);
static void setDirection(enum REGISTERABLE_KEYS scancode);
static void moveSnakes(void);
static void checkCrash(void);
static int outOfBounds(int x, int y);

static void drawBackground(void);
static void drawSnakes(void);
static void drawFood(void);
static void printScore(void);

static void setRandomSeed(void);
static int overSnakeBody(int x, int y);
static void randomizeFoodPosition(void);
static void checkFoodEaten(void);

static uint32_t hsv2rgb(uint8_t h, uint8_t s, uint8_t v);


// <----------------------------------------------------------------------- STATIC VARS ----------------------------------------------------------------------->

// key-movement for each snake
static char * movement_directions[] = {"left", "right", "up\t", "down"};
static char * movement_keys[][DIRECTIONS] = { {"left arrow", "right arrow", "up arrow", "down arrow"}, {"A", "D", "W", "S"} };

static SnakeHeadSquare snakes[MAX_SNAKES] = {};
static Square square;
static Food food;

// hues
static int hues[] = {HUE_1, HUE_2};

// main colors
static char * main_colors[] = {ANSI_1, ANSI_2};

// initial positions
static Position initial_positions[MAX_SNAKES] = { { .x = INITIAL_POS_1_X, .y = INITIAL_POS_1_Y } , { .x = INITIAL_POS_2_X, .y = INITIAL_POS_2_Y } };

// initial directions
static Direction initial_directions[MAX_SNAKES] = { { .x = INITIAL_DIR_1_X, .y = INITIAL_DIR_1_Y } , { .x = INITIAL_DIR_2_X, .y = INITIAL_DIR_2_Y } };

// screen dimensions
static int window_height, window_width;
static int border_y = SQUARE_DIM * 2;

static int end_of_game;   // 1 if the game finished because somebody crashed, 2 if it was because X was pressed
static int snakes_amount;
static int difficulty_level;
static int food_eaten;
static int first_round;


// ================================================================================ GAME ================================================================================

int main(void) {
    end_of_game = 0;
    window_height = getWindowHeight();
    window_width = getWindowWidth();
    food_eaten = 0;

    welcomePlayers();

    setRandomSeed();
    registerKeys();
    setSquareDimensions();

    do{
        clearScreen();
        printf("\n\n");
        setDefaultFeatures();

        first_round = 1;
        drawBackground();
        printScore();

        while(!end_of_game) {
            drawSnakes();
            drawFood();
        
            sleep(difficulty_level);

            stopBeep();

            moveSnakes();
            checkFoodEaten();
            checkCrash();
            checkMaxBodySize();

            first_round = 0;
        }

        showWinners();

    } while(!end_of_game);
    
    return 0;
}


// ================================================================================ GAME SETUP ================================================================================

static void setSquareDimensions(void) {
    square.width = SQUARE_DIM;
    square.height = SQUARE_DIM;
}

static void setDefaultFeatures(void) {
    for(int i = 0; i < snakes_amount; i++){
        snakes[i].initial_hue = hues[i];
        snakes[i].main_ansi = main_colors[i];
        snakes[i].direction.x = initial_directions[i].x;
        snakes[i].direction.y = initial_directions[i].y;
        snakes[i].size = INITIAL_BODY_SIZE + 1;
        for(int k = 0; k < INITIAL_BODY_SIZE + 1; k++){
            snakes[i].body[k].position.x = initial_positions[i].x - k * snakes[i].direction.x * SQUARE_DIM;
            snakes[i].body[k].position.y = initial_positions[i].y - k * snakes[i].direction.y * SQUARE_DIM;
        }
        snakes[i].lost = 0;
    }
    food.hue = DEFAULT_FOOD_COLOR;
    randomizeFoodPosition();
}


// ================================================================================ GAME START/END ================================================================================

static void welcomePlayers(void) {
    int players_result = 0;

    setFontSize(3);
    printf("\n\t\t\e[0;32mWelcome to the snake game!\e[0m\n\n");
    setFontSize(2);

    sleep(DEFAULT_WELCOME_SLEEPING_TIME);

    do{
        do{
            printf("\tEnter the number of players (maximum %d):", MAX_SNAKES);
            players_result = scanf("%d", &snakes_amount);
        } while(players_result != 1);

    } while(snakes_amount > MAX_SNAKES || snakes_amount <= 0);


    int dif_result = 0;
    char difficulty[1] = {0};

    do{
        do{
            printf("\n\tEnter difficulty\n");
            printf("\t(E)asy, (M)edium, (H)ard, (D)emon: ");   
            dif_result = scanf("%s", difficulty);
            
        } while(dif_result != 1);

    } while(!VALID_DIFFICULTY(*difficulty));

    setDifficulty(*difficulty);

    clearScreen();

    for(int i = 0; i < snakes_amount; i++){
        printf("%sPlayer %d keys:\e[0m\n\n", main_colors[i], i + 1);
        for(int k = 0; k < DIRECTIONS; k++){
            printf("\t\t%s\t \e[0;35m->\e[0m %s\n", movement_directions[k], movement_keys[i][k]);
        }
        printf("\n\n");
    }

    sleep(DEFAULT_INSTRUCTIONS_SLEEPING_TIME);

    printf("If any player eats %d foods, becomes the winner.\n\nDon't crash against the walls!\n\nAt any time, press X to quit.\n\nPress ENTER to begin", MAX_BODY_SIZE - INITIAL_BODY_SIZE - 1);

    while(getCharacterWithoutDisplay() != BEGIN_GAME_KEY);

    clearScreen();
}

static void checkMaxBodySize(void) {
    for(int i = 0; i < snakes_amount; i++){
        if(snakes[i].size == MAX_BODY_SIZE){
            endGameByMaxSize();
        }
    }
    if(end_of_game == ENDED_BY_MAX_SIZE){
        for(int i = 0; i < snakes_amount; i++){
            if(snakes[i].size != MAX_BODY_SIZE){
                snakes[i].lost = 1;
            }
        }
    }
    
}

static void endGameByMaxSize(void) {
    endGame(ENDED_BY_MAX_SIZE);
}

static void endGameByQuit(void) {
    endGame(ENDED_BY_QUIT);
}

static void endGameByCrash(void) {
    startBeep(CRASHING_SOUND_FREQUENCY);
    sleep(CRASHING_SOUND_DURATION);
    stopBeep();
    endGame(ENDED_BY_CRASH);
}

static void endGame(int code) {
    end_of_game = code;
}

static void showWinners(void) {
    if(end_of_game == ENDED_BY_CRASH || end_of_game == ENDED_BY_MAX_SIZE){
        clearScreen();

        int c;

        printf("\n\n\n");

        if(snakes_amount == 1){
            printf("\t");
            if(!snakes[0].lost){
                printf("You won, congratulations!\n");
            } else{
                printf("You lost.\n");
            }
            printf("\tScore: %d\n", snakes[0].size - (INITIAL_BODY_SIZE + 1));
            printf("\n\n");
        } else{
            for(int i = 0; i < snakes_amount; i++){
                printf("\t");
                if(!snakes[i].lost){
                    printf("%sPlayer %d won, congratulations!\e[0m\n", snakes[i].main_ansi, i + 1);
                    printf("\tScore: %d\n", snakes[i].size - (INITIAL_BODY_SIZE + 1));
                } else{
                    printf("%sPlayer %d lost :( \e[0m\n", snakes[i].main_ansi, i + 1);
                    printf("\tScore: %d\n", snakes[i].size - (INITIAL_BODY_SIZE + 1));
                }
                printf("\n");
            }
        }

        sleep(DEFAULT_GOODBYE_SLEEPING_TIME);

        printf("\n\e[0;32m----------------------------------------------------------------\e[0m \
        \n\n\t\t\tPress ENTER to play again, X to finish");

        while((c = getCharacterWithoutDisplay()) != PLAY_AGAIN_KEY && toupper(c) != QUIT_KEY);

        if(c == PLAY_AGAIN_KEY){
            end_of_game = 0;
        }
    }
}


// ================================================================================ MOVING LOGIC ================================================================================

static void registerKeys(void) {
    // player 1 keys
    registerKey(KP_UP_KEY, setDirection);
    registerKey(KP_DOWN_KEY, setDirection);
    registerKey(KP_LEFT_KEY, setDirection);
    registerKey(KP_RIGHT_KEY, setDirection);

    // player 2 keys
    registerKey(W_KEY, setDirection);
    registerKey(S_KEY, setDirection);
    registerKey(A_KEY, setDirection);
    registerKey(D_KEY, setDirection);

    registerKey(X_KEY, (void (*)(enum REGISTERABLE_KEYS scancode)) endGameByQuit);
}

static void movingTo(int snake, int dir_x, int dir_y) {
    snake -= 1;
    
    int forbiddenX = -(snakes[snake].body[HEAD].position.x - snakes[snake].body[1].position.x) / SQUARE_DIM;
    int forbiddenY = -(snakes[snake].body[HEAD].position.y - snakes[snake].body[1].position.y) / SQUARE_DIM;

    if (!(dir_x == forbiddenX && dir_y == forbiddenY)) {
        snakes[snake].direction.x = dir_x;
        snakes[snake].direction.y = dir_y;
    }
}

static void setDirection(enum REGISTERABLE_KEYS scancode) {
    switch (scancode) {
        case KP_UP_KEY:
            movingTo(1, 0, -1);
            break;
        case KP_DOWN_KEY:
            movingTo(1, 0, 1);
            break;
        case KP_LEFT_KEY:
            movingTo(1, -1, 0);
            break;
        case KP_RIGHT_KEY:
            movingTo(1, 1, 0);
            break;
        
        case W_KEY:
            movingTo(2, 0, -1);
            break;
        case S_KEY:
            movingTo(2, 0, 1);
            break;
        case A_KEY:
            movingTo(2, -1, 0);
            break;
        case D_KEY:
            movingTo(2, 1, 0);
            break;
        default:
            break;
    }
}

static void moveSnakes(void) {
    for(int i = 0; i < snakes_amount; i++){
        // move each square, except head
        for(int k = snakes[i].size - 1; k > 0; k--) {
            snakes[i].body[k].position.x = snakes[i].body[k - 1].position.x;
            snakes[i].body[k].position.y = snakes[i].body[k - 1].position.y;
        }

        // move head
        snakes[i].body[HEAD].position.x += snakes[i].direction.x * SQUARE_DIM;
        snakes[i].body[HEAD].position.y += snakes[i].direction.y * SQUARE_DIM;
    }
}

static void checkCrash(void) {
    int somebody_crashed = 0;
    for(int i = 0; i < snakes_amount; i++){
        // checks whether the snake crashed with a border
        if(outOfBounds(snakes[i].body[0].position.x, snakes[i].body[0].position.y)){
            snakes[i].lost = 1;
            somebody_crashed = 1;
        }

        // checks whether the snake crashed with itself
        for(int k = 1; k < snakes[i].size - 1 && !end_of_game; k++){
            if(snakes[i].body[0].position.x == snakes[i].body[k].position.x && snakes[i].body[0].position.y == snakes[i].body[k].position.y){
                snakes[i].lost = 1;
                somebody_crashed = 1;
            }
        }

        // checks whether snakes crashed with each other
        for(int k = 0; k < snakes_amount; k++){
            if(i != k){
                for(int j = 0; j < snakes[k].size - 1; j++){
                    if(snakes[i].body[0].position.x == snakes[k].body[j].position.x && snakes[i].body[0].position.y == snakes[k].body[j].position.y){
                        snakes[i].lost = 1;
                        // if both heads crash, both players lose
                        if(j == 0){
                            snakes[k].lost = 1;
                        }
                        somebody_crashed = 1;
                    }
                }
            }
        }
    }
    if(somebody_crashed){
        endGameByCrash();
    }
}

static int outOfBounds(int x, int y) {
    return (x < 0 || x > (window_width - SQUARE_DIM) || y < border_y || y > (window_height - SQUARE_DIM));
}


// ================================================================================ DRAWING LOGIC ================================================================================

static void drawBackground(void) {
    for(int y = border_y; y < window_height; y += square.height){
        for(int x = 0; x < window_width; x += square.width){
            if(first_round || !(x == food.position.x && y == food.position.y)){
                drawRectangle(DEFAULT_BACKGROUND_COLOR, square.width - OFFSET, square.height - OFFSET, x + OFFSET, y + OFFSET);
            }
        }
    }
}

static void drawSnakes(void) {
    for(int i = 0; i < snakes_amount; i++){
        //set last (phantom) snake body's rectangle to black
        drawRectangle(DEFAULT_BACKGROUND_COLOR, square.width - OFFSET, square.height - OFFSET, snakes[i].body[snakes[i].size - 1].position.x + OFFSET, snakes[i].body[snakes[i].size - 1].position.y + OFFSET);

        for(int k = 0; k < snakes[i].size - 1; k++){
            drawRectangle(hsv2rgb(k * 10 + snakes[i].initial_hue, 255, 255), square.width - OFFSET, square.height - OFFSET, snakes[i].body[k].position.x + OFFSET, snakes[i].body[k].position.y + OFFSET);
        }
    }
}

static void drawFood(void) {
    drawCircle(food.hue, food.position.x + OFFSET, food.position.y + OFFSET, SQUARE_DIM - OFFSET);
}

static void printScore(void) {
    printf("\r");

    if(snakes_amount == 1){
        printf("\t\t\t\t\t\t");
        printf("%sPlayer %d score: %d\e[0m", snakes[0].main_ansi, 1, snakes[0].size - (INITIAL_BODY_SIZE + 1));
    } else{
        for(int i = 0 ; i < snakes_amount; i++){
            printf("\t\t  ");
            printf("%sPlayer %d score: %d\e[0m", snakes[i].main_ansi, i + 1, snakes[i].size - (INITIAL_BODY_SIZE + 1));
        }
    }
}


// ================================================================================ FOOD LOGIC ================================================================================

static void setRandomSeed(void) {
    int hours, minutes, seconds;
    getDate(&hours, &minutes, &seconds);
    srand(seconds);
}

static void  setDifficulty(char difficulty) {
    switch (toupper(difficulty)) {
        case 'E':
            difficulty_level = EASY;
            break;
        case 'M':
            difficulty_level = MEDIUM;
            break;
        case 'H':
            difficulty_level = HARD;
            break;
        case 'D':
            difficulty_level = DEMON;
            break;
        default:
            break;
    }
}

static int overSnakeBody(int x, int y) {
    int over_snake = 0;

    for(int i = 0; i < snakes_amount && !over_snake; i++){
        for(int k = 0; k < snakes[i].size - 1 && !over_snake; k++){
            if(snakes[i].body[k].position.x == x && snakes[i].body[k].position.y == y) {
                over_snake = 1;
            }
        }
    }

    return over_snake;
}

static void randomizeFoodPosition(void) {
    int x, y;

    do {
        x = (rand() % SQUARE_DIM) * SQUARE_DIM;
        x = x % (window_width);

        y = (rand() % SQUARE_DIM) * SQUARE_DIM;
        y = y % (window_height - border_y) + border_y;

    } while(overSnakeBody(x, y));

    food.position.x = x;
    food.position.y = y;
}

static void checkFoodEaten(void) {
    for(int i = 0 ; i < snakes_amount; i++){
        if(snakes[i].body[HEAD].position.x == food.position.x && snakes[i].body[HEAD].position.y == food.position.y){
            int s = snakes[i].size++;
            snakes[i].body[s].position.x = snakes[i].body[s - 1].position.x;
            snakes[i].body[s].position.y = snakes[i].body[s - 1].position.y;
            food_eaten = 1;
        }
    }

    if(food_eaten){
        startBeep(EATING_SOUND_FREQUENCY);
        randomizeFoodPosition();
        printScore();
        food_eaten = 0;
    }
}


// ================================================================================ COLORED SNAKE LOGIC ================================================================================

// Hand transpiled from https://github.com/hughsk/glsl-hsv2rgb/blob/master/index.glsl
#define clamp(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
#define abs(a) ((a) < 0 ? -(a) : (a))
#define fract256(x) ((x) % 256)
// Hand transpiled from https://github.com/hughsk/glsl-hsv2rgb/blob/master/index.glsl
static uint32_t hsv2rgb(uint8_t h, uint8_t s, uint8_t v) {
//   vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
//   vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
//   return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
    uint16_t pr = abs(fract256( h ) * 6 - 255*3);
    uint16_t pg = abs(fract256( h + (85*2) ) * 6 - 255*3);
    uint16_t pb = abs(fract256( h + 85 ) * 6 - 255*3);
    return (((v * clamp(pr-256, 0, 255)) / 255) << 16) | (((v * clamp(pg-256, 0, 255)) / 255) << 8) | (v * clamp(pb-256, 0, 255)) / 255;
}