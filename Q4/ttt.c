#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BOARD_SIZE 9

void print_board(char* board) {
    for (int i = 0; i < 3; ++i) {
        printf(" %c | %c | %c \n", board[i * 3 + 0], board[i * 3 + 1], board[i * 3 + 2]);
        if (i < 2) {
            printf("---+---+---\n");
        }
    }
    printf("\n");
}

int check_win(char* board, char player) {
    const int WINNING_COMBOS[8][3] = {
        {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, // rows
        {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, // columns
        {0, 4, 8}, {2, 4, 6}  // diagonals
    };

    for (int i = 0; i < 8; ++i) {
        if (board[WINNING_COMBOS[i][0]] == player &&
            board[WINNING_COMBOS[i][1]] == player &&
            board[WINNING_COMBOS[i][2]] == player) {
            return 1;
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2 || strlen(argv[1]) != BOARD_SIZE) {
        printf("Error\n");
        exit(1);
    }

    char* strategy = argv[1];
    int digit_count[10] = {0};
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (strategy[i] < '1' || strategy[i] > '9' || ++digit_count[strategy[i] - '0'] > 1) {
            printf("Error\n");
            exit(1);
        }
    }

    char board[BOARD_SIZE];
    for (int i = 0; i < BOARD_SIZE; i++) {
        board[i] = ' ';
    }
    int move_count = 0;

    while (1) {
        int move;
        do {
            if (move_count >= BOARD_SIZE) {
                printf("DRAW\n");
                return 0;
            }
            move = strategy[move_count++] - '0' - 1;
        } while (board[move] != ' ');

        printf("Computer's turn: %d\n", move + 1);
        board[move] = 'X';
        print_board(board);

        if (check_win(board, 'X')) {
            printf("\033[1;32mI win \033[0m\n");
            break;
        }

        printf("Human's turn: ");
        scanf("%d", &move);
        printf("\n");
        move--;

        if (move < 0 || move >= BOARD_SIZE || board[move] != ' ') {
            printf("Error\n");
            exit(1);
        }

        board[move] = 'O';
        print_board(board);

        if (check_win(board, 'O')) {
            printf("\033[1;31mI lost \033[0m\n");
            break;
        }
    }

    return 0;
}
