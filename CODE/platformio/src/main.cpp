/*
 * @author            Joshua Page
 * @date_of_creation  Jan 26th, 2022
 * @date_last_edit    ?? ??, 2021
 * @version           v9
 * @board             MEGA 2560
 * @description       Start from scratch
 * @global            WORKING AI!!
*/

/*
  *@game convention overview
  *           X == 1, O == 0, empty == -1
  *           user can chose which color of X and O
  *           led board works on an x,y coordinate system
  *                 |-> e.g. the first led -> led0 == (0,0)
  *
*/

#include <Arduino.h>

// RGB pins # corresponds to row
#define PWM1RED     13
#define PWM1GREEN   10
#define PWM1BLUE    7
#define PWM2RED     12
#define PWM2BLUE    9
#define PWM2GREEN   6
#define PWM3RED     11
#define PWM3BLUE    8
#define PWM3GREEN   5

// Column grounds
#define ROW1GND    4
#define ROW2GND    3
#define ROW3GND    2

// Buttons
#define rightB     A1
#define leftB      A3
#define upB        A2
#define downB      A4
#define selectB    A0

// game constants
const int LEDS = 9;
const int GNDS = 3;
const int ROW = 3;
const int COL = 3;
const int RGB = 3;
const int NUM_BUTTONS = 5;
const int NUM_COLORS = 7;
const int PAIR = 2;

const int RGBPINS[LEDS] = {13, 12, 11, 10, 9, 8, 7, 6, 5};
const int GNDPINS[GNDS] = {4, 3, 2};

const int BOARDPINS[ROW][COL][RGB+1] = {
       {{4,13,12,11},{3,13,12,11},{2,13,12,11}},
       {{4,10,9,8}, {3,10,9,8}, {2,10,9,8}},
       {{4,7,6,5}, {3,7,6,5}, {2,7,6,5}}
  };

// {on/off, r, g, b}
const int board_zerOcolor[ROW][COL][RGB+1] = {
        {{0,0,0,0},{0,0,0,0},{0,0,0,0}},
        {{0,0,0,0},{0,0,0,0},{0,0,0,0}},
        {{0,0,0,0},{0,0,0,0},{0,0,0,0}}
  };
int board_color[ROW][COL][RGB+1];
//
const int game_board_zero[ROW][COL] = {
        {-1,-1,-1},
        {-1,-1,-1},
        {-1,-1,-1}
  };
int game_board[ROW][COL];
int winningLineZero[RGB][PAIR] = {{-1,-1}, {-1,-1}, {-1,-1}};
int winningLine[RGB][PAIR];

// COLORS
char color_str[RGB] = {'r','g','b'};
int red[RGB]        = {1,0,0};        // idx = 0
int green[RGB]      = {0,1,0};        // idx = 1
int blue[RGB]       = {0,0,1};        // idx = 2
int light_blue[RGB] = {0,1,1};        // idx = 3
int lime_green[RGB] = {1,1,0};        // idx = 4
int pink[RGB]       = {1,0,1};        // idx = 5
int white[RGB]      = {1,1,1};        // idx = 6
int black[RGB]      = {0,0,0};
int black_all[RGB+1]= {0,0,0,0};

const int allColor[NUM_COLORS][RGB] = {{1,0,0}, {0,1,0}, {0,0,1},
                                        {0,1,1}, {1,1,0}, {1,0,1},
                                        {1,1,1}};
int idxXcolor; // ->  0 < const RGB
int idxOcolor; // ->  0 < const RGB
int randomColorInt;

int Xcolor[RGB]; // stores color variable for X
int Ocolor[RGB]; // stores color variable for O
int randomColor[RGB]; // stores the color for random mode indicator
                      // pink; beacuse why not

// button setup
// up(0), right(1), down(2), left(3), select(4)
const int BUTTONS[NUM_BUTTONS] = {A2, A0, A4, A3, A1};
bool button_state[NUM_BUTTONS] = {1,1,1,1,1};
bool button_select_state[NUM_BUTTONS] = {1,1,1,1,1};
bool button_event[NUM_BUTTONS] = {0,0,0,0,0}; // this is the one to check events

// event timing
int currentTime;
int previouseTime;
int buttonTime;
int pauseTime;
const int blinkSpeed = 250;
const int buttonSpeed = 25;
const int pauseSpeed = 1350;

// game variables
int gameMode; // determine the game screen -> start/game/end
int setupXpos[PAIR];
int setupOpos[PAIR];
int setupRandomPos[PAIR] = {2,1};
int pos[PAIR]; // current position (x,y)
int test_pos[PAIR]; // test possition (x,y) -> to check is pos can be placed
bool XO_turn; // -> X = true, O = false
bool XO_ai;   // -> ai = X(true), ai = O (false)
bool randomXO_ai;
bool user2; // 2user game ->
bool firstAiMove;
bool instSwitch;
bool isOnOff;
bool new_turn;
bool new_game;
int partyTimeCounter;
const int partyTimeCount = 12;
bool instPartyTime;
bool instPauseTime;
int winner;

void printBoard(int board[ROW][COL]){
  for(int i = 0; i < ROW; i++){
    for(int j = 0; j < COL; j++){
      Serial.print(board[i][j]);
    }
    Serial.print("\n");
  }
  Serial.print("\n");
}
void printPos(int _pos[2]){
  Serial.print(_pos[0]);
  Serial.print(",");
  Serial.print(_pos[1]);
  Serial.print("  \n");
}

void checkButton(bool checkCancel){
  if(checkCancel){
    for(int i = 0; i < NUM_BUTTONS; i++){
      button_state[i] = digitalRead(BUTTONS[i]);
      if (!button_state[i] && button_select_state[i]){
        button_select_state[i] = 0;
        button_event[i] = 1;
      }else if (button_state[i] && !button_select_state[i]){
        button_select_state[i] = 1;
      }
    }
  }else{
    for(int i = 0; i < NUM_BUTTONS; i++){
        button_event[i] = 0;
    }
  }
}

void allOff(){
  // turn all RGBPINS pins off
  for(int i = 0; i < LEDS; i++)
    digitalWrite(RGBPINS[i], 0);
  for(int i = 0; i < GNDS; i++)
    digitalWrite(GNDPINS[i], 0);
}

void drawBoard(){
  for(int i = ROW; i >= 0; i--){
    for(int j = COL; j >= 0; j--){
      allOff();
      //delayMicroseconds(50);
      if(board_color[i][j][0] == 1){
        // for(int q = RGB+1; q >= 0; q--){
        //   digitalWrite(BOARDPINS[i][j][q], board_color[i][j][q]);
        // }
        for(int q = 0; q < RGB+1; q++){
          digitalWrite(BOARDPINS[i][j][q], board_color[i][j][q]);
          if(q==0)
            delayMicroseconds(50);
        }
      }
    }
  }
}

void colorLED(int *rgb, int *pos, bool OnOff){
  for(int i = 0; i < RGB; i++){
    board_color[pos[0]][pos[1]][i+1] = rgb[i];
  }
  if(OnOff){
    board_color[pos[0]][pos[1]][0] = 1;
  }else{
    board_color[pos[0]][pos[1]][0] = 0;
  }
}
void placeTicOrToe(int *pos, bool XO){
  if(XO){
    game_board[pos[0]][pos[1]] = 1;
  }else{
    game_board[pos[0]][pos[1]] = 0;
  }
}

void XOsetupColor(){
  for(int i = 0; i < RGB ; i++){
    Xcolor[i] = allColor[idxXcolor][i];
    Ocolor[i] = allColor[idxOcolor][i];
  }
}
void zeroBoards(){
  // zero the game board and the board color
  for(int w = 0; w < RGB; w++){
    winningLine[w][0] = winningLineZero[w][0];
    winningLine[w][1] = winningLineZero[w][1];
  }
  for(int i = 0; i < ROW; i++){
    for(int j = 0; j < COL; j++){
      game_board[i][j] = game_board_zero[i][j];
      for(int k = 0; k < RGB+1; k++){
        board_color[i][j][k] = board_zerOcolor[i][j][k];
      }
    }
  }
}

// Game Function
int checkWinner(int board[ROW][COL]){
  // check row winner or col winner
  for(int i = 0; i < ROW; i++){
    if (board[i][0] != -1 && board[i][0] == board[i][1] && board[i][1] == board[i][2]){
      return board[i][0];
    }
    if (board[0][i] != -1 && board[0][i] == board[1][i] && board[1][i] == board[2][i]){
      return board[0][i];
    }
  }
  // check diagonal winner
  if(board[0][0] != -1 && board[0][0] == board[1][1] && board[1][1] == board[2][2]){
    return board[0][0];
  }
  if(board[0][2] != -1 && board[0][2] == board[1][1] && board[1][1] == board[2][0]){
    return board[0][2];
  }

  // if nothing has been returned yet then there is no winner
  return -1;


}
void getWinnerLine(int board[ROW][COL]){
  // assue input will have a winning line -> i.e. No cats game
  for(int i = 0; i < ROW; i++){
    if (board[i][0] != -1 && board[i][0] == board[i][1] && board[i][1] == board[i][2]){
      winningLine[0][0] = i; winningLine[0][1] = 0;
      winningLine[1][0] = i; winningLine[1][1] = 1;
      winningLine[2][0] = i; winningLine[2][1] = 2;
    }
    if (board[0][i] != -1 && board[0][i] == board[1][i] && board[1][i] == board[2][i]){
      winningLine[0][0] = 0; winningLine[0][1] = i;
      winningLine[1][0] = 1; winningLine[1][1] = i;
      winningLine[2][0] = 2; winningLine[2][1] = i;
    }
  }
  // check diagonal winner
  if(board[0][0] != -1 && board[0][0] == board[1][1] && board[1][1] == board[2][2]){
    winningLine[0][0] = 0; winningLine[0][1] = 0;
    winningLine[1][0] = 1; winningLine[1][1] = 1;
    winningLine[2][0] = 2; winningLine[2][1] = 2;
  }
  if(board[0][2] != -1 && board[0][2] == board[1][1] && board[1][1] == board[2][0]){
    winningLine[0][0] = 0; winningLine[0][1] = 2;
    winningLine[1][0] = 1; winningLine[1][1] = 1;
    winningLine[2][0] = 2; winningLine[2][1] = 0;
  }
}
int utility(int board[ROW][COL]){
  int util = checkWinner(board);
  if(util == 1){
      return 10;
  }else if(util == 0){
      return -10;
  }else{
    return 0;
  }
}
bool terminal(int board[ROW][COL]){
  // return if the game is over
  // Returns True if game is over, False otherwise.
  int util = utility(board);
  for(int i = 0; i < ROW; i++){
    for(int j = 0; j < COL; j++){
      if(board[i][j] == -1 && util == 0){
        return false;
      }
    }
  }
  return true;
}
int userTurn(int board[ROW][COL]){

  int Xcount = 0;
  int Ocount = 0;
  for(int i  = 0; i < ROW; i++){
    for(int j = 0; j < COL; j++){
      if(board[i][j] == 1){
        Xcount++;
      }else if(board[i][j] == 0){
        Ocount++;
      }
    }
  }
  // Serial.print(Xcount);
  // Serial.print(" : ");
  // Serial.print(Ocount);
  // Serial.print("\n");
  if(Xcount > Ocount){
    return 0;
  }else{
    return 1;
  }
}
void getFirstPos(int board[ROW][COL]){
  // return id cats game and first open place
  for(int i = 0; i < ROW; i++){
    for(int j = 0; j < COL; j++){
      if(board[i][j] == -1){
        pos[0] = i; pos[1] = j;
        return;
      }
    }
  }
}
void move(int board[ROW][COL], int direction, bool XO){
  test_pos[0] = pos[0]; test_pos[1] = pos[1];
  do{
    switch(direction){
      case(0):
        test_pos[0]--;
        if(test_pos[0] < 0){
          test_pos[0] = ROW-1; test_pos[1]++;
          if(test_pos[1] > COL-1){
            test_pos[1] = 0;
          }
        }
        break;
      case(1):
        test_pos[1]++;
        if(test_pos[1] > COL-1){
          test_pos[0]++; test_pos[1] = 0;
          if(test_pos[0] > ROW-1){
            test_pos[0] = 0;
          }
        }
        break;
      case(2):
        test_pos[0]++;
        if(test_pos[0] > ROW-1){
          test_pos[0] = 0; test_pos[1]++;
          if(test_pos[1] > COL-1){
            test_pos[1] = 0;
          }
        }
        break;
      case(3):
        test_pos[1]--;
        if(test_pos[1] < 0){
          test_pos[0]++; test_pos[1] = COL-1;
          if(test_pos[0] > ROW-1){
            test_pos[0] = 0;
          }
        }
        break;
    }
  }while(board[test_pos[0]][test_pos[1]] != -1);

  colorLED(black, pos, false);
  pos[0] = test_pos[0]; pos[1] = test_pos[1];
  if(XO){
    colorLED(Xcolor, pos, true);
  }else{
    colorLED(Ocolor, pos, true);
  }

}

void maxi(int board[ROW][COL], int alpha, int beta, int depth, int* result);
void mini(int board[ROW][COL], int alpha, int beta, int depth, int* result);
void maxi(int board[ROW][COL], int alpha, int beta, int depth, int* result){
  result[0] = -1; result[1] = -1; result[2] = -1;
  int move[PAIR] = {-1,-1};
  //int result[ROW];
  if(terminal(board)){
    result[0] = utility(board);
    result[1] = move[0];
    result[2] = move[1];
    return;
  }
  for(int i = 0; i < ROW; i++){
    for(int j = 0; j < COL; j++){
      if(board[i][j] == -1){
        board[i][j] = userTurn(board);
        mini(board, alpha, beta, depth+1, result);
        board[i][j] = -1;
        if(result[0] >= beta){
          result[0] = beta;
          result[1] = move[0];
          result[2] = move[1];
          return;
        }
        if(result[0] > alpha){
          alpha = result[0];
          move[0] = i; move[1] = j;
        }
      }
    }
  }
  result[0] = alpha;
  result[1] = move[0];
  result[2] = move[1];
  return;
}
void mini(int board[ROW][COL], int alpha, int beta, int depth, int* result){
  result[0] = -1; result[1] = -1; result[2] = -1;
  int move[PAIR] = {-1,-1};
  //int result[ROW];
  if(terminal(board)){
    result[0] = utility(board);
    result[1] = move[0];
    result[2] = move[1];
    return;
  }
  //int score[ROW];
  for(int i = 0; i < ROW; i++){
    for(int j = 0; j < COL; j++){
      if(board[i][j] == -1){
        board[i][j] = userTurn(board);
        maxi(board, alpha, beta, depth+1, result);
        board[i][j] = -1;
        if(result[0] <= alpha){
          result[0] = alpha;
          result[1] = move[0];
          result[2] = move[1];
          return;
        }
        if(result[0] < beta){
          beta = result[0];
          move[0] = i; move[1] = j;
        }
      }
    }
  }
  result[0] = beta;
  result[1] = move[0];
  result[2] = move[1];
  return;
}
void smartAi(int board[ROW][COL]){
  int alpha = -30000;
  int beta = 30000;
  int ans[ROW];

  if(userTurn(board)){
    maxi(board, alpha, beta, 0, ans);
  }else{
    mini(board, alpha, beta, 0, ans);
  }
  pos[0] = ans[1]; pos[1] = ans[2];
}

void randomAi(int board[ROW][COL]){
  // return id cats game and random open place
  int possible_moves[LEDS][2] = {{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1}};
  int idx = 0;
  for(int i = 0; i < ROW; i++){
    for(int j = 0; j < COL; j++){
      if(board[i][j] == -1){
        possible_moves[idx][0] = i; possible_moves[idx][1] = j;
        idx++;
      }
    }
  }
  idx = random(0,idx);
  pos[0] = possible_moves[idx][0]; pos[1] = possible_moves[idx][1];
}

void gameSetup(){

  allOff();
  zeroBoards();

  setupXpos[0] = 0; setupXpos[1] = 0;
  setupOpos[0] = 0; setupOpos[1] = 2;
  pos[0] = 0; pos[1] = 0;

  idxXcolor = 0; idxOcolor = 2;
  XOsetupColor();
  colorLED(Ocolor, setupOpos, true);
  colorLED(Xcolor, setupXpos, true);

  randomColorInt = 5; // pinf; because why not?
  for(int i = 0; i < RGB; i++){
    randomColor[i] = allColor[randomColorInt][i];
  }

  currentTime = millis();
  previouseTime = millis();
  buttonTime = millis();
  pauseTime = millis();

  instSwitch = false;
  isOnOff = false;
  randomXO_ai = false;
  instPartyTime = false;
  instPauseTime = false;

  partyTimeCounter = 0;

  // START GAME
  gameMode = 0;

}
void setup(){
  // setup hardware -> only need to be done once on device power up!
  for(int i = 0; i < LEDS; i++)
    pinMode(RGBPINS[i], OUTPUT);

  for(int i = 0; i < GNDS; i++)
    pinMode(GNDPINS[i], OUTPUT);

  for(int button_pin = 0; button_pin < NUM_BUTTONS; button_pin++)
    pinMode(BUTTONS[button_pin], INPUT_PULLUP);

  randomSeed(analogRead(0)); // need to create a truly random number generator for random();

  Serial.begin(9600);

  Serial.println("Program Start");

  // sets up all variables for a new game
  gameSetup();

}

int startScreen(){

  if(button_event[1] && button_event[3]){
    instSwitch = true;
    user2 = true;
    Serial.println("Two Player Game");
  }else if((pos[1] == 2 || user2) && button_event[3]){
    // left
    pos[1] = 0;
    instSwitch = true;
    isOnOff = false;
    user2 = false;
  }else if((pos[1] == 0 || user2) && button_event[1]){
    // right
    pos[1] = 2;
    instSwitch = true;
    isOnOff = false;
    user2 = false;
  }else if(button_event[0] && button_event[2]){
    randomXO_ai = !randomXO_ai;
    colorLED(randomColor, setupRandomPos, randomXO_ai);
  }else if(button_event[2]){
    // down
    instSwitch = true;
    isOnOff = true;
    if(pos[1]==0){
      do{
        idxXcolor--;
        if(idxXcolor == -1){
          idxXcolor = NUM_COLORS-1;
        }
      }while(idxXcolor == idxOcolor);
    }else{
      do{
        idxOcolor--;
        if(idxOcolor == -1){
          idxOcolor = NUM_COLORS-1;
        }
      }while(idxXcolor == idxOcolor);
    }
    XOsetupColor();
  }else if(button_event[0]){
    //up
    instSwitch = true;
    isOnOff = true;
    if(pos[1]==0){
      do{
        idxXcolor++;
        if(idxXcolor == NUM_COLORS){
          idxXcolor = 0;
        }
      }while(idxXcolor == idxOcolor);
    }else{
      do{
        idxOcolor++;
        if(idxOcolor == NUM_COLORS){
          idxOcolor = 0;
        }
      }while(idxXcolor == idxOcolor);
    }
    XOsetupColor();
  }else if(button_event[4]){


    if(pos[1] == 0 || user2){
      XO_ai = false;
    }else{
      XO_ai = true;
    }

    instSwitch = true;
    firstAiMove = true;
    new_game = true;
    new_turn = true;
    pos[0] = 0; pos[1] = 0;

    allOff();
    zeroBoards();

    return 1;
  }

  // blink indexed led
  if(currentTime - previouseTime > blinkSpeed || instSwitch){
    previouseTime = millis();

    if(instSwitch){
      instSwitch = false;
      if(!user2){
        if(pos[1] == 0){
          colorLED(Ocolor, setupOpos, true);
        }else{
          colorLED(Xcolor, setupXpos, true);
        }
      }
    }

    if(user2){
      colorLED(Xcolor, setupXpos, isOnOff);
      colorLED(Ocolor, setupOpos, isOnOff);
    }else{
      if(pos[1] == 0){
        colorLED(Xcolor, setupXpos, isOnOff);
      }else{
        colorLED(Ocolor, setupOpos, isOnOff);
      }
    }

    if(isOnOff){
      isOnOff = false;
    }else{
      isOnOff = true;
    }
  }

  return 0;
}
int gameScreen(){

  if(new_turn){
    new_turn = false;
    //printBoard(game_board);
    if(terminal(game_board)){
      winner = utility(game_board);
      if(winner != 0){
        getWinnerLine(game_board);
      }
      instSwitch = true;
      return 3;
    }
    // instPauseTime = true;
    // pauseTime = millis();

    XO_turn = userTurn(game_board);

    if(XO_ai){
      if(randomXO_ai){
        randomAi(game_board);
      }else{
        smartAi(game_board);
      }
    }else{
      getFirstPos(game_board);
    }

  }

  if(XO_ai){
    if(XO_turn){
      colorLED(Xcolor, pos, true);
    }else{
      colorLED(Ocolor, pos, true);
    }

    placeTicOrToe(pos, XO_turn);
    new_turn = true;
    instSwitch = true;
    XO_ai = false;

  }else{
    // up(0), right(1), down(2), left(3), select(4)
    if(button_event[3] && button_event[1]){
      // secret party screen ->   EASTER EGG
      Serial.println("party mode");
      instSwitch = true;
      return 4;
    }else if(button_event[2]){
      move(game_board, 2, XO_turn);
      instSwitch = true;
    }else if(button_event[0]){
      move(game_board, 0, XO_turn);
      instSwitch = true;
    }else if(button_event[1]){
      move(game_board, 1, XO_turn);
      instSwitch = true;
    }else if(button_event[3]){
      move(game_board, 3, XO_turn);
      instSwitch = true;
    }else if(button_event[4]){
      if(XO_turn){
        colorLED(Xcolor, pos, true);
      }else{
        colorLED(Ocolor, pos, true);
      }
      placeTicOrToe(pos, XO_turn);
      new_turn = true;
      instSwitch = true;
      if(!user2){
        XO_ai = true;
      }

    }
  }

  if(currentTime - previouseTime > blinkSpeed || instSwitch){
    previouseTime = millis();
    if(instSwitch){
      instSwitch = false;
      isOnOff = true;
    }

    if(XO_turn){
      colorLED(Xcolor, pos, isOnOff);
    }else{
      colorLED(Ocolor, pos, isOnOff);
    }

    if(isOnOff){
      isOnOff = false;
    }else{
      isOnOff = true;
    }

  }

  return 1; // gameScreen

}
int endScreen(){

  if(winner == 0){
    if(currentTime - previouseTime > blinkSpeed*2 || instSwitch){
      previouseTime = millis();
      partyTimeCounter++;
      if(instSwitch){
        instSwitch = false;
        isOnOff = true;
      }
      if(partyTimeCounter > partyTimeCount){
        allOff();
        zeroBoards();
        int rand_bool;
        for(int i = 0; i < ROW; i++){
          for(int j = 0; j < COL; j++){
            pos[0] = i; pos[1] = j;
            rand_bool = random(-32768,32768);
            if(rand_bool>0){
              colorLED(Xcolor, pos, true);
            }else{
              colorLED(Ocolor, pos, true);
            }
          }
        }
      }else{

        for(int i = 0; i < ROW; i++){
          for(int j = 0; j < COL; j++){
            pos[0] = i; pos[1] = j;
            if(game_board[i][j] == 1){
              colorLED(Xcolor, pos, isOnOff);
            }else{
              colorLED(Ocolor, pos, isOnOff);
            }
          }
        }


        if(isOnOff){
          isOnOff = false;
        }else{
          isOnOff = true;
        }

      }

    }
  }else{
    if(currentTime - previouseTime > blinkSpeed*2 || instSwitch){
      previouseTime = millis();
      if(instSwitch){
        instSwitch = false;
        isOnOff = true;
      }

      for(int i = 0; i < RGB; i++){
        pos[0] = winningLine[i][0]; pos[1] = winningLine[i][1];
        if(winner == 10){
          colorLED(Xcolor, pos, isOnOff);
        }else if(winner == -10){
          colorLED(Ocolor, pos, isOnOff);
        }
      }

      if(isOnOff){
        isOnOff = false;
      }else{
        isOnOff = true;
      }

    }
  }


  if(button_event[4]){
    gameSetup();
    return 0;
  }
  return 3;
}
int partyScreen(){

  if(currentTime - previouseTime > blinkSpeed*2 || instSwitch){
    previouseTime = millis();
    if(instSwitch){
      instSwitch = false;
      isOnOff = true;
    }

    for(int i = 0; i < ROW; i++){
      for(int j = 0; j < COL; j++){
        randomColorInt = random(0,NUM_COLORS-1);
        for(int k = 0; k < RGB; k++){
          randomColor[k] = allColor[randomColorInt][k];
        }
        pos[0] = i; pos[1] = j;
        colorLED(randomColor, pos, isOnOff);
      }
    }
  }


  Serial.println("Pizza party");
  if(button_event[4]){
    gameSetup();
    return 0;
  }
  return 4;
}
void pauseScreen(){
  // this function is not a game screen but rather a fuction the pauses the
  // game without disrupting the screen animation
  if(currentTime - pauseTime > pauseSpeed){
    pauseTime = millis();
    instPauseTime = false;
  }
}

void loop() {
  // check for button events
  if(currentTime - buttonTime > buttonSpeed){
    buttonTime = millis();
    checkButton(true);
  }
  //Serial.println(gameMode);
  // main game screens
  switch(gameMode){
    // start screen player select X or O
    case(0):
      gameMode = startScreen();
      break;

    case(1):
      if(instPauseTime){
        pauseScreen();
      }else{
        gameMode = gameScreen();
      }
      break;

    case(3):
      gameMode = endScreen();
      break;

    case(4):
      gameMode = partyScreen();
      break;

    default:
      Serial.print("An error has occured in the game mode screen switch\n");
      gameMode = 0;
      break;
  }

  // update draw board
  drawBoard();
  allOff();

  // cancel all button events
  checkButton(false);
  currentTime = millis();
}
