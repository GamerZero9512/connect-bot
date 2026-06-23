/*
    +-----------+
    | connect.c |
    +-----------+

    ANSI C Connect 4 engine with Game Review
*/

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

/* tried to use INT_MIN/MAX from limits.h
   but the bot played worse?              */
#define SCORE_WIN  1000000
#define SCORE_LOSS -1000000
#define SCORE_DRAW 0

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

typedef enum {
  HL_OFF,
  HL_ON
} HlMode;

typedef enum {
  REV_ILLEGAL,
  REV_BEST,
  REV_GREAT,
  REV_GOOD,
  REV_INACCURACY,
  REV_MISTAKE,
  REV_BLUNDER
} RevClass;

/* ==================== VARIABLES ==================== */

const char *rev_names[7] = {
  "Illegal",
  "Best",
  "Great",
  "Good",
  "Inaccuracy",
  "Mistake",
  "Blunder"
};

static const int move_order[7] = {
  3, 2, 4, 1, 5, 0, 6
};

/* ==================== FUNCTIONS ==================== */

int is_legal_move(Board *board, Column x) {
  return !(((board->red >> (x + 35)) & 1) || ((board->yellow >> (x + 35)) & 1));
}

void get_moves(Board *board, Moves *out) {
  int i, x;
  out->count = 0;
  for(i = 0; i < 7; i++) {
    x = move_order[i];
    /* check if the top cell of the column is empty */
    if(!(((board->red >> (x + 35)) & 1) || ((board->yellow >> (x + 35)) & 1)))
      out->columns[out->count++] = x;
  }
}

void render_board(Board *board) {
  int x, y;
  printf("\x1b[44m                      \x1b[0m\n");
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

/* to reduce code verbosity */
#define GET_ANSI(c) ((c) && ((hl / 7 == y) || (hl / 7 == y - 1)) ? "\x1b[104m" : "\x1b[44m")

void render_board_hl(Board *board, int hl) {
  int x, y = 6;
  printf("\x1b[44m%s %s  %s %s  %s %s  %s %s  %s %s  %s %s  %s %s  %s \x1b[0m\n",
         GET_ANSI(hl % 7 == 0), /* can't be -1 so can simplify */
         GET_ANSI(hl % 7 == 0),
         GET_ANSI(hl % 7 == 0 || hl % 7 == 1),
         GET_ANSI(hl % 7 == 1),
         GET_ANSI(hl % 7 == 1 || hl % 7 == 2),
         GET_ANSI(hl % 7 == 2),
         GET_ANSI(hl % 7 == 2 || hl % 7 == 3),
         GET_ANSI(hl % 7 == 3),
         GET_ANSI(hl % 7 == 3 || hl % 7 == 4),
         GET_ANSI(hl % 7 == 4),
         GET_ANSI(hl % 7 == 4 || hl % 7 == 5),
         GET_ANSI(hl % 7 == 5),
         GET_ANSI(hl % 7 == 5 || hl % 7 == 6),
         GET_ANSI(hl % 7 == 6),
         GET_ANSI(hl % 7 == 6));
  for(y = 5; y >= 0; y--) {
    printf("\x1b[%sm ", (hl / 7 == y) && (hl % 7 == 0) ? "104" : "44");
    for(x = 0; x < 7; x++) {
      int i = x + y * 7;
      if(((board->red >> i) & 1) && ((board->yellow >> i) & 1)) printf("\x1b[41m \x1b[43m ");
      else if((board->red >> i) & 1) printf("\x1b[41m  ");
      else if((board->yellow >> i) & 1) printf("\x1b[43m  ");
      else fputs("\x1b[40m  ", stdout);
      printf("\x1b[%sm ", (hl / 7 == y) && (hl % 7 == x || hl % 7 == x + 1) ? "104" : "44");
    }
    printf("\x1b[0m\n\x1b[44m%s %s  %s %s  %s %s  %s %s  %s %s  %s %s  %s %s  %s \x1b[0m\n",
           GET_ANSI(hl % 7 == 0),
           GET_ANSI(hl % 7 == 0),
           GET_ANSI(hl % 7 == 0 || hl % 7 == 1),
           GET_ANSI(hl % 7 == 1),
           GET_ANSI(hl % 7 == 1 || hl % 7 == 2),
           GET_ANSI(hl % 7 == 2),
           GET_ANSI(hl % 7 == 2 || hl % 7 == 3),
           GET_ANSI(hl % 7 == 3),
           GET_ANSI(hl % 7 == 3 || hl % 7 == 4),
           GET_ANSI(hl % 7 == 4),
           GET_ANSI(hl % 7 == 4 || hl % 7 == 5),
           GET_ANSI(hl % 7 == 5),
           GET_ANSI(hl % 7 == 5 || hl % 7 == 6),
           GET_ANSI(hl % 7 == 6),
           GET_ANSI(hl % 7 == 6));
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

void win_game(const char *msg, char *game) {
  puts(msg);
  printf("game code: %s\n", game);
}

void eval_print(int eval, Turn turn, int err) {
  FILE *stream;
  if(err) stream = stdout;
  else stream = stderr;
  if(eval == SCORE_WIN) fprintf(stream, "+%s", turn == TURN_RED ? "RED" : "YELLOW");
  else if(eval == SCORE_LOSS) fprintf(stream, "+%s", turn == TURN_RED ? "YELLOW" : "RED");
  else fprintf(stream, "%s%d", eval > 0 ? "+" : "", eval);
}

/* ==================== ENGINE/SEARCH ==================== */

/* not sure if this actually works -
   I don't have any non-GNU compilers */

#if defined(__GNUC__) || defined(__clang__)
  #define popcountll __builtin_popcountll
#else
static int portable_popcountll(uint64_t x) {
  x = x - ((x >> 1) & (uint64_t)0x5555555555555555);
  x = (x & (uint64_t)0x3333333333333333) + ((x >> 2) & (uint64_t)0x3333333333333333);
  x = (x + (x >> 4)) & (uint64_t)0x0F0F0F0F0F0F0F0F;
  x = x + (x >> 8);
  x = x + (x >> 16);
  x = x + (x >> 32);
  return (int)(x & 0x7F);
}
#define popcountll portable_popcountll
#endif

int evaluate(Board *board, Turn turn) {
  int score = 0;
  uint64_t b;
  WinState won;
  if(turn == TURN_RED)
    b = board->red;
  else
    b = board->yellow;
  won = check_won(board);
  switch(turn) {
    case TURN_RED:
      if(won == WIN_RED) return SCORE_WIN;
      if(won == WIN_YELLOW) return SCORE_LOSS;
      break;
    case TURN_YELLOW:
      if(won == WIN_RED) return SCORE_LOSS;
      if(won == WIN_YELLOW) return SCORE_WIN;
      break;
  }
  score += popcountll(b & (b >> 1));
  score += popcountll(b & (b >> 7));
  score += popcountll(b & (b >> 8));
  score += popcountll(b & (b >> 6));
  if(turn == TURN_RED)
    b = board->yellow;
  else
    b = board->red;
  score -= popcountll(b & (b >> 1));
  score -= popcountll(b & (b >> 7));
  score -= popcountll(b & (b >> 8));
  score -= popcountll(b & (b >> 6));
  return score;
}

int search(Board *board, Turn turn, int depth, int alpha, int beta) {
  Moves moves;
  int i, score;
  WinState won = check_won(board);
  switch(turn) {
    case TURN_RED:
      if(won == WIN_RED) return SCORE_WIN;
      if(won == WIN_YELLOW) return SCORE_LOSS;
      break;
    case TURN_YELLOW:
      if(won == WIN_RED) return SCORE_LOSS;
      if(won == WIN_YELLOW) return SCORE_WIN;
      break;
  }
  if(depth == 0) return evaluate(board, turn);
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

Column get_best_move(Board board, Turn turn, int depth, int print_eval) {
  Moves moves;
  int i, d, best_score, score;
  Column best_move;
  get_moves(&board, &moves);
  best_move = moves.columns[0];
  for(d = 1; d <= depth; d++) {
    best_score = SCORE_LOSS;
    for(i = 0; i < moves.count; i++) {
      Column move = moves.columns[i];
      do_move(&board, move, turn);
      score = -search(&board, 1 - turn, d - 1, SCORE_LOSS, SCORE_WIN);
      if(score > best_score) {
        best_score = score;
        best_move = move;
      }
      undo_move(&board, move);
    }
  }
  if(print_eval) {
    int eval = search(&board, turn, depth, SCORE_LOSS, SCORE_WIN);
    fputs("eval: ", stderr);
    eval_print(eval, turn, 0);
    fputc('\n', stderr);
  }
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
    puts("invalid input (out of bounds)\n");
    return 1;
  }
  if((board->red | board->yellow) & ((uint64_t)1 << (35 + selection))) {
    puts("invalid input (column is full)\n");
    return 1;
  }
  *last_move = do_move(board, (Column)selection, turn);
  return 0;
}

void turn_bot(Board *board, Turn turn, int depth, int *last_move) {
  clock_t start, end;
  double elapsed;
  start = clock();
  *last_move = do_move(board, get_best_move(*board, turn, depth, 1), turn);
  end = clock();
  elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  fprintf(stderr, "took %.3f seconds\n", elapsed);
}

/* ==================== GAME REVIEW ==================== */

/*
  REV_ILLEGAL,
  REV_BEST,
  REV_GREAT,
  REV_GOOD,
  REV_INACCURACY,
  REV_MISTAKE,
  REV_BLUNDER
*/

RevClass review_move(Board *board, Column move, Turn turn, int depth, Column *best, int *new) {
  int loss, old;
  old = search(board, turn, depth, SCORE_LOSS, SCORE_WIN);
  if(!is_legal_move(board, move)) return REV_ILLEGAL;
  *best = get_best_move(*board, turn, depth, 0);
  do_move(board, move, turn);
  *new = -search(board, 1 - turn, depth, SCORE_LOSS, SCORE_WIN);
  loss = old - *new;
  if(move == *best) return REV_BEST;
  if(loss <= 0) return REV_GREAT;
  if(loss <= 1) return REV_GOOD;
  if(loss <= 3) return REV_INACCURACY;
  if(loss <= 6) return REV_MISTAKE;
  return REV_BLUNDER;
}

void review_game(const char *game, int depth, Board board, Turn turn) {
  int i, len = strlen(game);
  RevClass rev;
  const char *col;
  Column best;
  int eval;

  for(i = 0; i < len; i++) {
    if(game[i] < 49 || game[i] > 55) {
      fprintf(stderr, "invalid move (invalid character)\n");
      continue;
    }
    rev = review_move(&board, game[i] - 49, turn, depth, &best, &eval);
    if(rev == REV_ILLEGAL) {
      fprintf(stderr, "invalid move (column is full)\n");
      continue;
    }
    switch(rev) {
      case REV_ILLEGAL:
        col = "40;37";
        break;
      case REV_BEST:
        col = "44;37";
        break;
      case REV_GREAT:
        col = "46;30";
        break;
      case REV_GOOD:
        col = "42;37";
        break;
      case REV_INACCURACY:
        col = "43;30";
        break;
      case REV_MISTAKE:
        col = "48;5;208;30";
        break;
      case REV_BLUNDER:
        col = "41;37";
        break;
    }
    printf("\x1b[4%cm%d:\x1b[0m \x1b[%sm%-10s\x1b[0m (", turn == TURN_RED ? '1' : '3', game[i] - 48, col, rev_names[rev]);
    eval_print(eval, turn, 1);
    fputc(')', stdout);
    if(rev != REV_BEST) printf(" best: %d", best + 1);
    fputc('\n', stdout);
    if(i % 2 == 1) fputc('\n', stdout);
    turn = 1 - turn;
  }
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
  HlMode hl = HL_OFF;
  char game[43] = {0};
  int game_idx = 0;

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
          fprintf(stderr, "invalid character '%c' in parameter for '%s'\n", *end, argv[i - 1]);
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
          fprintf(stderr, "expected prameter for flag '%s'\n", argv[i]);
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
      } else if(strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--review") == 0) {
        if(i >= argc - 1) {
          fprintf(stderr, "expected parameter for flag '%s'\n", argv[i]);
          return 1;
        }
        review_game(argv[++i], depth, board, turn);
        return 0;
      } else if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--highlight") == 0) hl = HL_ON;
      else if(strcmp(argv[i], "-?") == 0 || strcmp(argv[i], "--help") == 0) {
        printf(
          "usage: %s [options]\n"
          "\n"
          "options:\n"
          "  --help,      -?              Show this help message.\n"
          "  --depth,     -d <depth>      Specify custom engine search depth.\n"
          "  --player1,   -1 <human|bot>  Specify whether Player 1 is a bot or human.\n"
          "  --player2,   -2 <human|bot>  Specify whether Player 2 is a bot or human.\n"
          "  --start,     -s <red|yellow> Specify which colour starts first.\n"
          "  --highlight, -h              Highlight the latest move.\n"
          "  --review,    -r <game>       Review a game from its game code.\n"
        , argv[0]);
        return 0;
      } else {
        fprintf(stderr, "unrecognised flag: '%s'\n", argv[i]);
        return 1;
      }
    }
  }

  while(1) {
    if(hl) render_board_hl(&board, last_move);
    else render_board(&board);
    if(check_won(&board) == WIN_RED) {
      win_game("red wins!", game);
      break;
    }
    if(check_won(&board) == WIN_YELLOW) {
      win_game("yellow wins!", game);
      break;
    }
    get_moves(&board, &moves);
    if(moves.count == 0) {
      win_game("draw!", game);
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
    game[game_idx++] = (last_move % 7) + 49;
  }
  return 0;
}
