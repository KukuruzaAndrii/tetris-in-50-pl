#define _GNU_SOURCE

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "core.h"

unsigned randomInt(void) {
  unsigned rand;
  FILE *fp;
  fp = fopen("/dev/urandom", "r");
  fread(&rand, sizeof(unsigned), 1, fp);
  fclose(fp);
  return rand;
}

unsigned getRandomIntInclusive(unsigned min, unsigned max) {
  return (randomInt() % (max - min + 1)) + min;
}

void initState(struct state *state) {
  state->move = MOVE_DOWN;
  memset(state->board, 0, sizeof(state->board));
  state->figIndex = getRandomIntInclusive(0, FIG_COUNT - 1);
  state->rotateIndex = 0;
  state->color = getRandomIntInclusive(1, COLORS_COUNT - 1);
  state->offsetX = state->figIndex == 0 ? 3 : 4;
  state->offsetY = -1 * (int) FIGURES[state->figIndex].rotations[0].ofy;
  state->nextFigIndex = getRandomIntInclusive(0, FIG_COUNT - 1);
  state->nextFigColor = getRandomIntInclusive(1, COLORS_COUNT - 1);
  state->score = 0;
}

void printState(const struct state *state) {
  char strBoard[BOARD_H * BOARD_W + 1];
  for (unsigned y = 0; y < BOARD_H; ++y) {
    for (unsigned x = 0; x < BOARD_W; ++x) {
      strBoard[y * BOARD_W + x] = (char) ((unsigned) '0' + state->board[y][x]);
    }
  }
  strBoard[BOARD_H * BOARD_W] = 0;
  printf("%u %s %u %u %u %d %d %u %u %u\n", state->move, strBoard, state->figIndex, state->rotateIndex, state->color,
         state->offsetX, state->offsetY, state->nextFigIndex, state->nextFigColor, state->score);
}

void parseState(char **argv, struct state *dest) {
  dest->move = (enum MOVES) (atoi(argv[1]));
  for (unsigned i = 0; i < BOARD_H * BOARD_W; ++i) {
    unsigned y = i / BOARD_W;
    unsigned x = i % BOARD_W;
    dest->board[y][x] = argv[2][i] - '0';
  }
  dest->figIndex = atoi(argv[3]);
  dest->rotateIndex = atoi(argv[4]);
  dest->color = atoi(argv[5]);
  dest->offsetX = atoi(argv[6]);
  dest->offsetY = atoi(argv[7]);
  dest->nextFigIndex = atoi(argv[8]);
  dest->nextFigColor = atoi(argv[9]);
  dest->score = atoi(argv[10]);
}

void getFigCoords(struct coords *coords, unsigned figIndex, unsigned rotateIndex, int offsetX, int offsetY) {
  unsigned count = 0;
  for (unsigned i = 0; i < 4; ++i) {
    // check overflow !
    unsigned y = FIGURES[figIndex].rotations[rotateIndex].squares[i][1] + offsetY +
                 FIGURES[figIndex].rotations[rotateIndex].ofy;
    unsigned x = FIGURES[figIndex].rotations[rotateIndex].squares[i][0] + offsetX +
                 FIGURES[figIndex].rotations[rotateIndex].ofx;
    if (y > 20)
      continue;

    coords->squares[count][0] = x;
    coords->squares[count][1] = y;
    count += 1;
  }
  coords->count = count;
}

unsigned needNewFigure(const struct state *state) {
  struct coords coords;
  getFigCoords(&coords, state->figIndex, state->rotateIndex, state->offsetX, state->offsetY);
  for (unsigned i = 0; i < coords.count; ++i) {
    unsigned x = coords.squares[i][0];
    unsigned y = coords.squares[i][1];
    if (y + 1 == BOARD_H || state->board[y + 1][x] != 0) {
      return 1;
    }
  }
  return 0;
}

void removeFullLines(struct state *state) {
  unsigned countFullLines = 0;
  for (unsigned y = 0; y < BOARD_H; ++y) {
    unsigned full = 1;
    for (unsigned x = 0; x < BOARD_W; ++x) {
      if (state->board[y][x] == 0) {
        full = 0;
        break;
      }
    }
    if (full) {
      memmove(state->board[countFullLines + 1], state->board[countFullLines],
              (y - countFullLines) * (sizeof state->board[y]));
      countFullLines++;
    }
  }

  if (countFullLines > 0) {
    // update score
    state->score += SCORES[countFullLines - 1];

    // add new empty lines
    memset(state->board[0], 0, countFullLines * (sizeof state->board[0]));
  }
}

void createNewFig(struct state *state) {
  // replace known things
  state->figIndex = state->nextFigIndex;
  state->color = state->nextFigColor;

  state->offsetX = state->figIndex == 0 ? 3 : 4;
  state->offsetY = -1 * FIGURES[state->figIndex].rotations[0].ofy;
  state->rotateIndex = 0;

  // calculate new 
  state->nextFigIndex = getRandomIntInclusive(0, FIG_COUNT - 1);
  state->nextFigColor = getRandomIntInclusive(1, COLORS_COUNT - 1);
}

void checkEndGame(const struct state *state) {
  struct coords newCoords;
  getFigCoords(&newCoords, state->figIndex, state->rotateIndex, state->offsetX, state->offsetY);

  for (unsigned i = 0; i < newCoords.count; ++i) {
    unsigned x = newCoords.squares[i][0];
    unsigned y = newCoords.squares[i][1];
    if (state->board[y][x] != 0) {
      printf("%s\n", "Game over!");
      exit(0);
    }
  }
}

unsigned canMoveLeft(const struct state *state) {
  if (state->offsetX + FIGURES[state->figIndex].rotations[state->rotateIndex].ofx <= 0)
    return 0;

  struct coords oldCoords;
  getFigCoords(&oldCoords, state->figIndex, state->rotateIndex, state->offsetX, state->offsetY);
  for (unsigned i = 0; i < oldCoords.count; ++i) {
    unsigned x = oldCoords.squares[i][0];
    unsigned y = oldCoords.squares[i][1];
    if (state->board[y][x - 1] != 0) {
      return 0;
    }
  }
  return 1;
}

unsigned canMoveRight(const struct state *state) {
  if (state->offsetX + FIGURES[state->figIndex].rotations[state->rotateIndex].w +
      FIGURES[state->figIndex].rotations[state->rotateIndex].ofx >= BOARD_W)
    return 0;

  struct coords oldCoords;
  getFigCoords(&oldCoords, state->figIndex, state->rotateIndex, state->offsetX, state->offsetY);
  for (unsigned i = 0; i < oldCoords.count; ++i) {
    unsigned x = oldCoords.squares[i][0];
    unsigned y = oldCoords.squares[i][1];
    if (state->board[y][x + 1] != 0) {
      return 0;
    }
  }
  return 1;
}

unsigned canRotate(const unsigned board[BOARD_H][BOARD_W], const unsigned figIndex, const unsigned newRotIndex,
                   const int offsetX, const int offsetY) {
  struct coords rotateFigCoords;
  getFigCoords(&rotateFigCoords, figIndex, newRotIndex, offsetX, offsetY);
  for (unsigned i = 0; i < rotateFigCoords.count; ++i) {
    unsigned x = rotateFigCoords.squares[i][0];
    unsigned y = rotateFigCoords.squares[i][1];
    if (x < 0 || x >= BOARD_W || y >= BOARD_H || board[y][x] != 0) {
      return 0;
    }
  }
  return 1;
}

int getOffsetAtDrop(struct state *state) {
  while (!needNewFigure(state)) {
    state->offsetY += 1;
  }
  return state->offsetY;
}

void update(struct state *state) {
  switch (state->move) {
    case MOVE_DOWN:
      if (needNewFigure(state)) {
        struct coords oldCoords;
        getFigCoords(&oldCoords, state->figIndex, state->rotateIndex, state->offsetX, state->offsetY);
        for (unsigned i = 0; i < oldCoords.count; ++i) {
          unsigned x = oldCoords.squares[i][0];
          unsigned y = oldCoords.squares[i][1];
          state->board[y][x] = state->color;
        }

        removeFullLines(state);
        createNewFig(state);
        checkEndGame(state);
        break;
      }
      state->offsetY += 1;
      break;
    case MOVE_LEFT:
      if (canMoveLeft(state)) {
        state->offsetX -= 1;
      }
      break;
    case MOVE_RIGHT:
      if (canMoveRight(state)) {
        state->offsetX += 1;
      }
      break;
    case MOVE_ROTATE_CLOCKWISE: {
      unsigned newRotIndex = (state->rotateIndex == FIGURES[state->figIndex].count - 1) ? 0 : state->rotateIndex + 1;
      if (canRotate(state->board, state->figIndex, newRotIndex, state->offsetX, state->offsetY)) {
        state->rotateIndex = newRotIndex;
      }
      break;
    }
    case MOVE_ROTATE_COUNTER_CLOCKWISE: {
      unsigned newRotIndex = (state->rotateIndex == 0) ? FIGURES[state->figIndex].count - 1 : state->rotateIndex - 1;
      if (canRotate(state->board, state->figIndex, newRotIndex, state->offsetX, state->offsetY)) {
        state->rotateIndex = newRotIndex;
      }
      break;
    }
    case MOVE_DROP: {
      int newOffsetY = getOffsetAtDrop(state);
      struct coords coords;
      getFigCoords(&coords, state->figIndex, state->rotateIndex, state->offsetX, newOffsetY);
      for (unsigned i = 0; i < coords.count; ++i) {
        unsigned x = coords.squares[i][0];
        unsigned y = coords.squares[i][1];
        state->board[y][x] = state->color;
      }

      removeFullLines(state);
      createNewFig(state);
      checkEndGame(state);
      break;
    }
  }
}

char *renderLine(char *bucket, const unsigned y, const unsigned board[BOARD_H][BOARD_W]) {
  for (unsigned x = 0; x < BOARD_W; x++) {
    if (board[y][x] != 0) {
      bucket = stpcpy(bucket, COLORS[board[y][x]]);
      bucket = stpcpy(bucket, " ");
      bucket = stpcpy(bucket, RESET);
    } else {
      bucket = stpcpy(bucket, (x % 2 == 0) ? " " : SPACER);
    }
  }
  return bucket;
}

char *
renderNextPieceLine(char *bucket, const unsigned y, const struct coords coords, unsigned nextFigColor, unsigned score) {
  if (y > NEXT_P_BOARD_H + 2) return bucket;
  if (y == 0) {
    bucket = stpcpy(bucket, " ");
    sprintf(bucket, "%06d", score);
    return bucket + 6;
  }
  if (y == 1) {
    bucket = stpcpy(bucket, " ");
    for (unsigned x = 0; x < NEXT_P_BOARD_W; x++) {
      bucket = stpcpy(bucket, CEIL);
    }
    bucket = stpcpy(bucket, " ");
    return bucket;
  }
  if (y == NEXT_P_BOARD_H + 2) {
    bucket = stpcpy(bucket, " ");
    for (unsigned x = 0; x < NEXT_P_BOARD_W; x++) {
      bucket = stpcpy(bucket, FLOOR);
    }
    bucket = stpcpy(bucket, " ");
    return bucket;
  }

  bucket = stpcpy(bucket, LEFT);

  for (unsigned x = 0; x < NEXT_P_BOARD_W; x++) {
    unsigned isPiece = 0;
    for (unsigned i = 0; i < coords.count; ++i) {
      unsigned xc = coords.squares[i][0];
      unsigned yc = coords.squares[i][1];
      if (xc == x && yc == y - 2) {
        isPiece = 1;
        break;
      }
    }
    if (isPiece) {
      bucket = stpcpy(bucket, COLORS[nextFigColor]);
      bucket = stpcpy(bucket, " ");
      bucket = stpcpy(bucket, RESET);
    } else {
      bucket = stpcpy(bucket, " ");
    }
  }
  bucket = stpcpy(bucket, RIGHT);

  return bucket;
}

void render(char *res, struct state *state) {

  // add piece to board for simplifying render
  struct coords coords;
  getFigCoords(&coords, state->figIndex, state->rotateIndex, state->offsetX, state->offsetY);
  for (unsigned i = 0; i < coords.count; ++i) {
    unsigned x = coords.squares[i][0];
    unsigned y = coords.squares[i][1];
    state->board[y][x] = state->color;
  }

  char *bucket = stpcpy(res, " ");
  for (unsigned x = 0; x < BOARD_W; x++) {
    bucket = stpcpy(bucket, CEIL);
  }
  bucket = stpcpy(bucket, " \n");
  struct coords nextFigCoords;
  getFigCoords(&nextFigCoords, state->nextFigIndex, 0, state->nextFigIndex == 5 ? 2 : 1,
               state->nextFigIndex == 5 ? 2 : 1);

  for (unsigned y = 0; y < BOARD_H; y++) {
    bucket = stpcpy(bucket, LEFT);
    bucket = renderLine(bucket, y, state->board);
    bucket = stpcpy(bucket, RIGHT);
    bucket = renderNextPieceLine(bucket, y, nextFigCoords, state->nextFigColor, state->score);
    bucket = stpcpy(bucket, "\n");
  }
  bucket = stpcpy(bucket, " ");
  for (unsigned x = 0; x < BOARD_W; x++) {
    bucket = stpcpy(bucket, FLOOR);
  }
  bucket = stpcpy(bucket, " ");

  for (unsigned i = 0; i < coords.count; ++i) {
    unsigned x = coords.squares[i][0];
    unsigned y = coords.squares[i][1];
    state->board[y][x] = 0;
  }

}

int main(int argc, char **argv) {
  struct state state;
  char res[3000];
//  printf("argc - %d\n", argc);
  if (argc == 11) {
    parseState(argv, &state);
    update(&state);
    printState(&state);
    render(res, &state);
    printf("%s\n", res);
  } else if (argc == 2 && !strcmp(argv[1], "INIT_STATE")) {
    initState(&state);
    printState(&state);
    render(res, &state);
    printf("%s\n", res);
  } else {
    printf("argv[0] - %s\n", argv[0]);
    printf("argv[1] - %s\n", argv[1]);

    puts("incorrect arguments");
    exit(1);
  }
  return 0;
}

