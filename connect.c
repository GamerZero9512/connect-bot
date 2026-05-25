/*
    +-----------+
    | connect.c |
    +-----------+

    Strong ANSI C Connect 4 engine
*/

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

/* ==================== TYPEDEFS ==================== */

typedef enum {
  COL_A,
  COL_B,
  COL_C,
  COL_D,
  COL_E,
  COL_F,
  COL_G
} Column;

typedef struct {
  Column columns[7];
  int count;
} Moves;

/* bitboard */
typedef struct {
  uint64_t red;    /* only uses 42 bits */
  uint64_t yellow;
} Board;

typedef enum {
  WIN_NONE,
  WIN_RED,
  WIN_YELLOW
} WinState;

typedef enum {
  TURN_RED,
  TURN_YELLOW
} Turn;

typedef enum {
  PT_BOT,
  PT_HUMAN
} PlayerType;

/* ==================== FUNCTIONS ==================== */

void get_moves(Board *board, Moves *out) {
  int x;
  out->count = 0;
  for(x = 0; x < 7; x++) {
    /* check if the top cell of the column is empty */
    if(!(((board->red >> (x + 35)) & 1) || ((board->yellow >> (x + 35)) & 1))) {
      out->columns[out->count++] = x;
    }
  }
}

void render_board(Board *board, int hl) {
  int x, y;
  (void)hl; /* not using hl yet */
  puts("\x1b[44m                      \x1b[0m");
  for(y = 5; y >= 0; y--) {
    fputs("\x1b[44m ", stdout);
    for(x = 0; x < 7; x++) {
      int i = x + y * 7;
      if(((board->red >> i) & 1) && ((board->yellow >> i) & 1)) printf("\x1b[41m \x1b[43m ");
      else if((board->red >> i) & 1) printf("\x1b[41m  ");
      else if((board->yellow >> i) & 1) printf("\x1b[43m  ");
      else fputs("\x1b[40m  ", stdout);
      fputs("\x1b[44m ", stdout);
    }
    puts("\x1b[0m\n\x1b[44m                      \x1b[0m");
  }
  fputc('\n', stdout);
}

WinState check_won(Board *board) {
    int x, y;
    for(y = 0; y < 6; y++) {
        for(x = 0; x < 7; x++) {
            int i = x + y * 7;
            uint8_t red = (board->red >> i) & 1;
            uint8_t yellow = (board->yellow >> i) & 1;
            if(!red && !yellow) continue;

            /* horizontal */
            if(x <= 3) {
                if(red && ((board->red >> (i+1)) & 1) && ((board->red >> (i+2)) & 1) && ((board->red >> (i+3)) & 1))
                    return WIN_RED;
                if(yellow && ((board->yellow >> (i+1)) & 1) && ((board->yellow >> (i+2)) & 1) && ((board->yellow >> (i+3)) & 1))
                    return WIN_YELLOW;
            }

            /* vertical */
            if(y <= 2) {
                if(red && ((board->red >> (i+7)) & 1) && ((board->red >> (i+14)) & 1) &&((board->red >> (i+21)) & 1))
                    return WIN_RED;
                if(yellow && ((board->yellow >> (i+7)) & 1) && ((board->yellow >> (i+14)) & 1) && ((board->yellow >> (i+21)) & 1))
                    return WIN_YELLOW;
            }

            /* diagonal / */
            if(x <= 3 && y <= 2) {
                if(red && ((board->red >> (i+8)) & 1) && ((board->red >> (i+16)) & 1) && ((board->red >> (i+24)) & 1))
                    return WIN_RED;
                if(yellow && ((board->yellow >> (i+8)) & 1) && ((board->yellow >> (i+16)) & 1) && ((board->yellow >> (i+24)) & 1))
                    return WIN_YELLOW;
            }

            /* diagonal \ */
            if(x >= 3 && y <= 2) {
                if(red && ((board->red >> (i+6)) & 1) && ((board->red >> (i+12)) & 1) && ((board->red >> (i+18)) & 1))
                    return WIN_RED;
                if(yellow && ((board->yellow >> (i+6)) & 1) && ((board->yellow >> (i+12)) & 1) && ((board->yellow >> (i+18)) & 1))
                    return WIN_YELLOW;
            }
        }
    }
    return WIN_NONE;
}

int do_move(Board *board, Column col, Turn turn) {
  int row;
  for(row = 0; row < 6; row++) {
    int i = col + (row * 7);
    if(!((board->red >> i) & 1) && !((board->yellow >> i) & 1)) {
      if(turn == TURN_RED) board->red |= ((uint64_t)1 << i);
      else board->yellow |= ((uint64_t)1 << i);
      return i;
    }
  }
  return -1;
}

void undo_move(Board *board, Column col) {
  int row;
  for(row = 5; row >= 0; row--) {
    int i = col + (row * 7);
    if((board->red >> i) & 1) {
      board->red &= ~((uint64_t)1 << i);
      return;
    }
    if((board->yellow >> i) & 1) {
      board->yellow &= ~((uint64_t)1 << i);
      return;
    }
  }
}

/* ==================== ENGINE ==================== */

/* tried to use INT_MIN/MAX from limits.h
   but the bot played worse?              */

#define SCORE_WIN  1000000
#define SCORE_LOSS -1000000
#define SCORE_DRAW 0

/* not sure if this actually works -
   I don't have any non-GNU compilers */

#ifndef __GNUC__
#define __builtin_popcountll(x) \
({ \
    uint64_t i = x; \
    i = i - ((i >> 1) & 0x5555555555555555); \
    i = (i & 0x3333333333333333) + ((i >> 2) & 0x3333333333333333); \
    i = (i + (i >> 4)) & 0x0f0f0f0f0f0f0f0f; \
    i = i + (i >> 8); \
    i = i + (i >> 16); \
    i = i + (i >> 32); \
    (int)(i & 0x7f); \
})
#endif

int evaluate(Board *board) {
  int score = 0;
  uint64_t b = board->red;
  score += __builtin_popcountll(b & (b >> 1));
  score += __builtin_popcountll(b & (b >> 7));
  score += __builtin_popcountll(b & (b >> 8));
  score += __builtin_popcountll(b & (b >> 6));
  b = board->yellow;
  score -= __builtin_popcountll(b & (b >> 1));
  score -= __builtin_popcountll(b & (b >> 7));
  score -= __builtin_popcountll(b & (b >> 8));
  score -= __builtin_popcountll(b & (b >> 6));
  return score;
}

int search(Board *board, Turn turn, int depth, int alpha, int beta) {
  Moves moves;
  int i, score;
  WinState won = check_won(board);
  if (won != WIN_NONE) return SCORE_LOSS;
  if(depth == 0) return evaluate(board);
  get_moves(board, &moves);
  if(moves.count == 0) return SCORE_DRAW;
  for(i = 0; i < moves.count; i++) {
    Column move = moves.columns[i];
    do_move(board, move, turn);
    score = -search(board, 1 - turn, depth - 1, -beta, -alpha);
    undo_move(board, move);
    if(score >= beta) return beta;
    if(score > alpha) alpha = score;
  }
  return alpha;
}

Column get_best_move(Board board, Turn turn, int depth) {
  Moves moves;
  int i, d, best_score, score;
  Column best_move;
  get_moves(&board, &moves);
  best_move = moves.columns[0];
  for(d = 1; d <= depth; d++) {
    best_score = -1000000;
    for(i = 0; i < moves.count; i++) {
      Column move = moves.columns[i];
      do_move(&board, move, turn);
      score = -search(&board, 1 - turn, d - 1, -1000000, 1000000);
      if(score >= best_score) {
        best_score = score;
        best_move = move;
      }
      undo_move(&board, move);
    }
  }
  fprintf(stderr, "eval: %d\n", evaluate(&board));
  return best_move;
}

/* ==================== TURNS ==================== */

uint8_t turn_human(Board *board, Turn turn, int *last_move) {
  int selection;
  printf(": ");
  fflush(stdout);
  scanf("%d", &selection);
  selection--;
  if(selection < 0 || selection > 6) {
    puts("Invalid input");
    return 1;
  }
  if((board->red | board->yellow) & ((uint64_t)1 << (35 + selection))) {
    puts("Invalid input");
    return 1;
  }
  *last_move = do_move(board, (Column)selection, turn);
  return 0;
}
void turn_bot(Board *board, Turn turn, int depth, int *last_move) {
  clock_t start, end;
  double elapsed;
  start = clock();
  *last_move = do_move(board, get_best_move(*board, turn, depth), turn);
  end = clock();
  elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  fprintf(stderr, "took %.3f seconds\n", elapsed);

}

/* ==================== MAIN ==================== */

int main(int argc, char *argv[]) {
  Board board = {0};
  Turn turn = TURN_RED;
  Moves moves;
  int depth = 10;
  int last_move = -1;
  PlayerType p1 = PT_BOT;
  PlayerType p2 = PT_HUMAN;

  int i;
  for(i = 1; i < argc; i++) {
    if(argv[i][0] == '-') {
      if(strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--depth") == 0) {
        char *end;
        if(i >= argc - 1) {
          fprintf(stderr, "expected parameter for flag '%s'\n", argv[i]);
          return 1;
        }
        i++;
        depth = strtol(argv[i], &end, 0);
        if(*end != '\0') {
          fprintf(stderr, "invalid character '%c' in parameter for '%s'", *end, argv[i - 1]);
          return 1;
        }
      } else if(strcmp(argv[i], "-1") == 0 || strcmp(argv[i], "--player1") == 0) {
        if(i >= argc - 1) {
          fprintf(stderr, "expected parameter for flag '%s'\n", argv[i]);
          return 1;
        }
        i++;
        if(strcmp(argv[i], "human") == 0) p1 = PT_HUMAN;
        else if(strcmp(argv[i], "bot") == 0) p1 = PT_BOT;
        else {
          fprintf(stderr, "expected one of 'human' or 'bot' for flag '%s'\n", argv[i - 1]);
          return 1;
        }
      } else if(strcmp(argv[i], "-2") == 0 || strcmp(argv[i], "--player2") == 0) {
        if(i >= argc - 1) {
          fprintf(stderr, "expected parameter for flag '%s'\n", argv[i]);
          return 1;
        }
        i++;
        if(strcmp(argv[i], "human") == 0) p2 = PT_HUMAN;
        else if(strcmp(argv[i], "bot") == 0) p2 = PT_BOT;
        else {
          fprintf(stderr, "expected one of 'human' or 'bot' for flag '%s'\n", argv[i - 1]);
          return 1;
        }
      } else if(strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--start") == 0) {
        if(i >= argc - 1) {
          fprintf(stderr, "expected parameter for flag '%s'\n", argv[i]);
          return 1;
        }
        i++;
        if(strcmp(argv[i], "red") == 0) turn = TURN_RED;
        else if(strcmp(argv[i], "yellow") == 0) turn = TURN_YELLOW;
        else {
          fprintf(stderr, "expected one of 'red' or 'yellow' for flag '%s'\n", argv[i - 1]);
          return 1;
        }
      } else if(strcmp(argv[i], "-?") == 0 || strcmp(argv[i], "--help") == 0) {
        printf(
          "usage: %s [options]\n"
          "\n"
          "options:\n"
          "  --help,   -?              Show this help message\n"
          "  --depth,  -d <depth>      Specify custom engine search depth\n"
          "  --red,    -1 <human|bot>  Specify whether Red is a bot or human\n"
          "  --yellow, -2 <human|bot>  Specify whether Yellow is a bot or human\n"
          "  --start,  -s <red|yellow> Specify which colour starts first\n"
        , argv[0]);
        return 0;
      } else {
        fprintf(stderr, "unrecognised flag: '%s'\n", argv[i]);
        return 1;
      }
    }
  }

  while(1) {
    render_board(&board, last_move);
    if(check_won(&board) == WIN_RED) {
      puts("Red wins!");
      break;
    }
    if(check_won(&board) == WIN_YELLOW) {
      puts("Yellow wins!");
      break;
    }
    get_moves(&board, &moves);
    if(moves.count == 0) {
      puts("Draw!");
      break;
    }
    if(turn == TURN_RED) {
      if(p1 == PT_BOT) turn_bot(&board, turn, depth, &last_move);
      else if(turn_human(&board, turn, &last_move)) continue;
      turn = TURN_YELLOW;
    } else {
      if(p2 == PT_BOT) turn_bot(&board, turn, depth, &last_move);
      else if(turn_human(&board, turn, &last_move)) continue;
      turn = TURN_RED;
    }
  }
  return 0;
}

/*
    Example usage:
    ./connect -d 20 -1 human -2 bot  # set engine depth 20, p1 is human, p2 is bot

    Recommend engine depths 5-7 for 'easy mode'
*/
