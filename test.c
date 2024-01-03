//
// Created by Administrator on 2024/1/3.
//

#include "test.h"
#include <stdio.h>
#include <ctype.h>

int main() {
    // The character 'c' isalnum 8
    // char ch = 'c';

    // The character '1' isalnum 8
    // char ch = '1';

    // The character '╔' isalnum 0
    // char ch = 1;

    // The character '-' isalnum 0
    // char ch = '-';
    // char ch = '\uD83D\uDC79';
    char ch = '™';

    printf("The character '%c' isalnum %d \n", ch, isalnum(ch));

    if (isalnum(ch)) {
        printf("The character '%c' is alphanumeric\n", ch);
    } else {
        printf("The character '%c' is not alphanumeric\n", ch);
    }
    return 0;
}