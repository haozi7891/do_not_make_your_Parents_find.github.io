编译方式：
第一种：gcc -o plane_game plane_game.c -lncurses
第二种：gcc -o game plane_game.c -lncursesw
第三种：gcc -o game plane_game.c -lncurses -ltermcap
第四种：gcc -o game plane_game.c -lncurses -ltinfo -ltermcap
第五种：gcc -Wall -Wextra -o plane_game plane_game.c -lncursesw -ltinfo
