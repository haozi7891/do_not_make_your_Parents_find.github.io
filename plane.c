#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>
#include <wchar.h>

// 游戏状态枚举
typedef enum {
    MENU,
    PLAYING,
    PAUSED,
    WIN,
    LOST,
    MAP_SELECTION
} GameState;

// 地图类型枚举
typedef enum {
    EUROPE_US,
    VIENNA_HOTEL,
    JAPAN
} MapType;

// 玩家状态枚举
typedef enum {
    PLAYING_PLANE,
    HIDING,
    CAUGHT
} PlayerState;

// 结构体定义
typedef struct {
    int x, y;
    int score;
    int time_played;
    PlayerState state;
} Player;

typedef struct {
    int x, y;
    int active;
    int timer;
    int direction;  // 0:左, 1:右, 2:上, 3:下
    char symbol;
} Parent;

typedef struct {
    MapType type;
    int width, height;
    char **layout;
    int bed_x, bed_y;
    int hide_x, hide_y;
    int hide_width, hide_height;
    char *map_name;
} GameMap;

// 全局变量
GameState game_state = MENU;
MapType current_map = EUROPE_US;
Player player;
Parent parents[2];
GameMap maps[3];
int game_time = 0;
int total_time = 0;
int parent_check_timer = 0;
int warning_timer = 0;
int game_speed = 100000; // 微秒

// 颜色对定义
#define COLOR_PAIR_PLAYER 1
#define COLOR_PAIR_PARENT 2
#define COLOR_PAIR_BED 3
#define COLOR_PAIR_HIDE 4
#define COLOR_PAIR_WALL 5
#define COLOR_PAIR_TEXT 6
#define COLOR_PAIR_WARNING 7
#define COLOR_PAIR_MENU 8

// 函数声明
void init_ncurses();
void init_maps();
void init_game();
void draw_map();
void draw_player();
void draw_parents();
void draw_ui();
void update_game();
void handle_input(int ch);
void spawn_parent();
void move_parents();
void check_collisions();
void draw_menu();
void draw_map_selection();
void cleanup();

// 初始化ncurses
void init_ncurses() {
    setlocale(LC_ALL, "zh_CN.UTF-8");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(0);
    
    // 初始化颜色
    if (has_colors()) {
        start_color();
        init_pair(COLOR_PAIR_PLAYER, COLOR_GREEN, COLOR_BLACK);
        init_pair(COLOR_PAIR_PARENT, COLOR_RED, COLOR_BLACK);
        init_pair(COLOR_PAIR_BED, COLOR_YELLOW, COLOR_BLACK);
        init_pair(COLOR_PAIR_HIDE, COLOR_CYAN, COLOR_BLACK);
        init_pair(COLOR_PAIR_WALL, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_PAIR_TEXT, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_PAIR_WARNING, COLOR_RED, COLOR_BLACK);
        init_pair(COLOR_PAIR_MENU, COLOR_CYAN, COLOR_BLACK);
    }
}

// 初始化地图
void init_maps() {
    // 欧美地图
    maps[EUROPE_US].type = EUROPE_US;
    maps[EUROPE_US].width = 40;
    maps[EUROPE_US].height = 20;
    maps[EUROPE_US].map_name = "欧美卧室";
    maps[EUROPE_US].bed_x = 5;
    maps[EUROPE_US].bed_y = 5;
    maps[EUROPE_US].hide_x = 25;
    maps[EUROPE_US].hide_y = 8;
    maps[EUROPE_US].hide_width = 8;
    maps[EUROPE_US].hide_height = 4;
    
    // 分配内存并创建布局
    maps[EUROPE_US].layout = malloc(maps[EUROPE_US].height * sizeof(char*));
    for (int i = 0; i < maps[EUROPE_US].height; i++) {
        maps[EUROPE_US].layout[i] = malloc((maps[EUROPE_US].width + 1) * sizeof(char));
    }
    
    // 创建欧美地图布局
    strcpy(maps[EUROPE_US].layout[0],  "########################################");
    strcpy(maps[EUROPE_US].layout[1],  "#      门                    窗       #");
    strcpy(maps[EUROPE_US].layout[2],  "#                                    #");
    strcpy(maps[EUROPE_US].layout[3],  "#  #############      #############  #");
    strcpy(maps[EUROPE_US].layout[4],  "#  #    床     #      #  学习区    #  #");
    strcpy(maps[EUROPE_US].layout[5],  "#  #  ([ ])   #      #   [书桌]   #  #");
    strcpy(maps[EUROPE_US].layout[6],  "#  #   / \\    #      #   椅子     #  #");
    strcpy(maps[EUROPE_US].layout[7],  "#  #############      #############  #");
    strcpy(maps[EUROPE_US].layout[8],  "#                                    #");
    strcpy(maps[EUROPE_US].layout[9],  "#              衣柜                   #");
    strcpy(maps[EUROPE_US].layout[10], "#                                    #");
    strcpy(maps[EUROPE_US].layout[11], "########################################");

    // 维也纳艺术酒店地图
    maps[VIENNA_HOTEL].type = VIENNA_HOTEL;
    maps[VIENNA_HOTEL].width = 40;
    maps[VIENNA_HOTEL].height = 20;
    maps[VIENNA_HOTEL].map_name = "维也纳艺术酒店";
    maps[VIENNA_HOTEL].bed_x = 8;
    maps[VIENNA_HOTEL].bed_y = 6;
    maps[VIENNA_HOTEL].hide_x = 20;
    maps[VIENNA_HOTEL].hide_y = 10;
    maps[VIENNA_HOTEL].hide_width = 6;
    maps[VIENNA_HOTEL].hide_height = 3;
    
    maps[VIENNA_HOTEL].layout = malloc(maps[VIENNA_HOTEL].height * sizeof(char*));
    for (int i = 0; i < maps[VIENNA_HOTEL].height; i++) {
        maps[VIENNA_HOTEL].layout[i] = malloc((maps[VIENNA_HOTEL].width + 1) * sizeof(char));
    }
    
    // 创建维也纳酒店布局
    strcpy(maps[VIENNA_HOTEL].layout[0],  "########################################");
    strcpy(maps[VIENNA_HOTEL].layout[1],  "#   豪华套房 - 维也纳艺术酒店        #");
    strcpy(maps[VIENNA_HOTEL].layout[2],  "#                                    #");
    strcpy(maps[VIENNA_HOTEL].layout[3],  "#  #####        #####        #####  #");
    strcpy(maps[VIENNA_HOTEL].layout[4],  "#  #床#        #艺术#        #电视#  #");
    strcpy(maps[VIENNA_HOTEL].layout[5],  "#  #([ ])      #画 #        #椅子#  #");
    strcpy(maps[VIENNA_HOTEL].layout[6],  "#  # / \\       #####         ###  #");
    strcpy(maps[VIENNA_HOTEL].layout[7],  "#                                    #");
    strcpy(maps[VIENNA_HOTEL].layout[8],  "#      浴室              阳台        #");
    strcpy(maps[VIENNA_HOTEL].layout[9],  "#                                    #");
    strcpy(maps[VIENNA_HOTEL].layout[10], "########################################");

    // 日本地图
    maps[JAPAN].type = JAPAN;
    maps[JAPAN].width = 40;
    maps[JAPAN].height = 20;
    maps[JAPAN].map_name = "日本和室";
    maps[JAPAN].bed_x = 6;
    maps[JAPAN].bed_y = 8;
    maps[JAPAN].hide_x = 26;
    maps[JAPAN].hide_y = 7;
    maps[JAPAN].hide_width = 8;
    maps[JAPAN].hide_height = 4;
    
    maps[JAPAN].layout = malloc(maps[JAPAN].height * sizeof(char*));
    for (int i = 0; i < maps[JAPAN].height; i++) {
        maps[JAPAN].layout[i] = malloc((maps[JAPAN].width + 1) * sizeof(char));
    }
    
    // 创建日本地图布局
    strcpy(maps[JAPAN].layout[0],  "########################################");
    strcpy(maps[JAPAN].layout[1],  "#    日本和室 - 障子と畳             #");
    strcpy(maps[JAPAN].layout[2],  "#                                    #");
    strcpy(maps[JAPAN].layout[3],  "#   #####            #####          #");
    strcpy(maps[JAPAN].layout[4],  "#   #布団#           #学习#         #");
    strcpy(maps[JAPAN].layout[5],  "#   #([ ])           #区域#         #");
    strcpy(maps[JAPAN].layout[6],  "#   #####            机と椅子       #");
    strcpy(maps[JAPAN].layout[7],  "#                                    #");
    strcpy(maps[JAPAN].layout[8],  "#    押入れ              床の間       #");
    strcpy(maps[JAPAN].layout[9],  "#                                    #");
    strcpy(maps[JAPAN].layout[10], "########################################");
}

// 初始化游戏
void init_game() {
    player.x = maps[current_map].bed_x;
    player.y = maps[current_map].bed_y;
    player.score = 0;
    player.time_played = 0;
    player.state = PLAYING_PLANE;
    
    // 初始化父母
    for (int i = 0; i < 2; i++) {
        parents[i].active = 0;
        parents[i].symbol = 'P';
    }
    
    game_time = 0;
    total_time = 0;
    parent_check_timer = 0;
    warning_timer = 0;
    game_state = PLAYING;
}

// 绘制地图
void draw_map() {
    GameMap *map = &maps[current_map];
    
    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            char ch = map->layout[y][x];
            
            // 检查是否在隐藏区域
            int in_hide_area = (x >= map->hide_x && x < map->hide_x + map->hide_width &&
                               y >= map->hide_y && y < map->hide_y + map->hide_height);
            
            // 检查是否在床上区域
            int in_bed_area = (x >= map->bed_x - 1 && x <= map->bed_x + 1 &&
                              y >= map->bed_y - 1 && y <= map->bed_y + 1);
            
            // 设置颜色
            if (in_bed_area && ch != '#') {
                attron(COLOR_PAIR(COLOR_PAIR_BED));
            } else if (in_hide_area && ch != '#') {
                attron(COLOR_PAIR(COLOR_PAIR_HIDE));
            } else if (ch == '#') {
                attron(COLOR_PAIR(COLOR_PAIR_WALL));
            } else {
                attron(COLOR_PAIR(COLOR_PAIR_TEXT));
            }
            
            mvaddch(y + 2, x + 2, ch);
            attroff(COLOR_PAIR(COLOR_PAIR_TEXT));
        }
    }
}

// 绘制玩家
void draw_player() {
    attron(COLOR_PAIR(COLOR_PAIR_PLAYER));
    
    // 根据状态选择符号
    char symbol;
    switch (player.state) {
        case PLAYING_PLANE:
            symbol = 'A';  // 飞机
            break;
        case HIDING:
            symbol = 'S';  // 学习/看电视
            break;
        case CAUGHT:
            symbol = 'X';  // 被抓
            break;
    }
    
    mvaddch(player.y + 2, player.x + 2, symbol);
    attroff(COLOR_PAIR(COLOR_PAIR_PLAYER));
}

// 绘制父母
void draw_parents() {
    for (int i = 0; i < 2; i++) {
        if (parents[i].active) {
            attron(COLOR_PAIR(COLOR_PAIR_PARENT));
            mvaddch(parents[i].y + 2, parents[i].x + 2, parents[i].symbol);
            attroff(COLOR_PAIR(COLOR_PAIR_PARENT));
        }
    }
}

// 绘制UI
void draw_ui() {
    // 绘制边框
    attron(COLOR_PAIR(COLOR_PAIR_WALL));
    for (int i = 0; i < maps[current_map].width + 4; i++) {
        mvaddch(0, i, '#');
        mvaddch(maps[current_map].height + 3, i, '#');
    }
    for (int i = 0; i < maps[current_map].height + 4; i++) {
        mvaddch(i, 0, '#');
        mvaddch(i, maps[current_map].width + 3, '#');
    }
    attroff(COLOR_PAIR(COLOR_PAIR_WALL));
    
    // 绘制游戏信息
    attron(COLOR_PAIR(COLOR_PAIR_TEXT));
    mvprintw(1, maps[current_map].width + 6, "游戏: 不要让你的父母发现你在起飞");
    mvprintw(3, maps[current_map].width + 6, "地图: %s", maps[current_map].map_name);
    mvprintw(5, maps[current_map].width + 6, "状态: %s", 
             player.state == PLAYING_PLANE ? "玩飞机游戏中" :
             player.state == HIDING ? "躲避中" : "被抓了!");
    mvprintw(7, maps[current_map].width + 6, "游戏时间: %d秒", game_time);
    mvprintw(9, maps[current_map].width + 6, "总时间: %d/100秒", total_time);
    mvprintw(11, maps[current_map].width + 6, "剩余父母检查: %d", 
             15 - parent_check_timer);
    mvprintw(13, maps[current_map].width + 6, "得分: %d", player.score);
    
    // 绘制控制说明
    mvprintw(15, maps[current_map].width + 6, "控制:");
    mvprintw(16, maps[current_map].width + 6, "WASD/方向键 - 移动");
    mvprintw(17, maps[current_map].width + 6, "空格 - 开始/暂停");
    mvprintw(18, maps[current_map].width + 6, "M - 返回菜单");
    mvprintw(19, maps[current_map].width + 6, "Q - 退出游戏");
    
    // 绘制提示
    mvprintw(21, maps[current_map].width + 6, "提示:");
    mvprintw(22, maps[current_map].width + 6, "- 父母来时躲到%s区域",
             current_map == VIENNA_HOTEL ? "椅子看电视" : "学习区");
    mvprintw(23, maps[current_map].width + 6, "- 坚持100秒即可胜利!");
    
    // 警告信息
    if (warning_timer > 0) {
        attron(COLOR_PAIR(COLOR_PAIR_WARNING));
        mvprintw(25, maps[current_map].width + 6, "警告: 父母来了! 快躲起来!");
        warning_timer--;
        attroff(COLOR_PAIR(COLOR_PAIR_WARNING));
    }
    
    attroff(COLOR_PAIR(COLOR_PAIR_TEXT));
}

// 更新游戏逻辑
void update_game() {
    if (game_state != PLAYING) return;
    
    game_time++;
    
    // 每10秒增加游戏时间
    if (game_time % 10 == 0) {
        total_time++;
        player.score += 10;
        
        // 检查胜利条件
        if (total_time >= 100) {
            game_state = WIN;
            return;
        }
    }
    
    // 更新父母检查计时器
    parent_check_timer++;
    if (parent_check_timer >= 15) {  // 每15秒父母可能来检查
        if (rand() % 100 < 30) {  // 30%概率触发检查
            spawn_parent();
            warning_timer = 20;  // 显示警告2秒
        }
        parent_check_timer = 0;
    }
    
    // 移动父母
    move_parents();
    
    // 检查碰撞
    check_collisions();
    
    // 检查玩家是否在正确位置
    GameMap *map = &maps[current_map];
    int in_hide_area = (player.x >= map->hide_x && player.x < map->hide_x + map->hide_width &&
                       player.y >= map->hide_y && player.y < map->hide_y + map->hide_height);
    
    // 如果有活跃的父母，玩家应该在隐藏区域
    int active_parent = 0;
    for (int i = 0; i < 2; i++) {
        if (parents[i].active) {
            active_parent = 1;
            break;
        }
    }
    
    if (active_parent) {
        if (in_hide_area) {
            player.state = HIDING;
            player.score += 5;  // 成功躲避加分
        } else {
            player.state = PLAYING_PLANE;
        }
    } else {
        // 检查是否在床上玩飞机
        int in_bed_area = (player.x >= map->bed_x - 1 && player.x <= map->bed_x + 1 &&
                          player.y >= map->bed_y - 1 && player.y <= map->bed_y + 1);
        if (in_bed_area) {
            player.state = PLAYING_PLANE;
            player.score += 2;  // 在床上玩飞机加分
        } else {
            player.state = HIDING;
        }
    }
}

// 生成父母
void spawn_parent() {
    for (int i = 0; i < 2; i++) {
        if (!parents[i].active) {
            parents[i].active = 1;
            parents[i].x = rand() % maps[current_map].width;
            parents[i].y = rand() % maps[current_map].height;
            parents[i].timer = 50;  // 父母存在时间
            parents[i].direction = rand() % 4;
            break;
        }
    }
}

// 移动父母
void move_parents() {
    for (int i = 0; i < 2; i++) {
        if (parents[i].active) {
            parents[i].timer--;
            if (parents[i].timer <= 0) {
                parents[i].active = 0;
                continue;
            }
            
            // 随机移动
            if (rand() % 100 < 30) {  // 30%概率改变方向
                parents[i].direction = rand() % 4;
            }
            
            // 移动
            int new_x = parents[i].x;
            int new_y = parents[i].y;
            
            switch (parents[i].direction) {
                case 0: new_x--; break;  // 左
                case 1: new_x++; break;  // 右
                case 2: new_y--; break;  // 上
                case 3: new_y++; break;  // 下
            }
            
            // 检查边界和墙壁
            if (new_x >= 0 && new_x < maps[current_map].width &&
                new_y >= 0 && new_y < maps[current_map].height &&
                maps[current_map].layout[new_y][new_x] != '#') {
                parents[i].x = new_x;
                parents[i].y = new_y;
            }
        }
    }
}

// 检查碰撞
void check_collisions() {
    for (int i = 0; i < 2; i++) {
        if (parents[i].active) {
            // 检查父母是否看到玩家（在附近）
            int distance = abs(parents[i].x - player.x) + abs(parents[i].y - player.y);
            
            if (distance <= 3) {  // 如果距离小于等于3
                GameMap *map = &maps[current_map];
                int in_hide_area = (player.x >= map->hide_x && player.x < map->hide_x + map->hide_width &&
                                   player.y >= map->hide_y && player.y < map->hide_y + map->hide_height);
                
                // 如果玩家不在隐藏区域，就被抓到
                if (!in_hide_area) {
                    player.state = CAUGHT;
                    game_state = LOST;
                    return;
                }
            }
        }
    }
}

// 处理输入
void handle_input(int ch) {
    switch (game_state) {
        case MENU:
            if (ch == '1') {
                game_state = MAP_SELECTION;
            } else if (ch == '2') {
                // 显示游戏说明
                clear();
                attron(COLOR_PAIR(COLOR_PAIR_MENU));
                mvprintw(5, 20, "游戏说明:");
                mvprintw(7, 20, "1. 在床上玩开飞机游戏，坚持100秒即可胜利");
                mvprintw(8, 20, "2. 父母会随机来检查，听到警告后立即躲避");
                mvprintw(9, 20, "3. 欧美和日本地图：躲到学习区");
                mvprintw(10, 20, "4. 维也纳艺术酒店：躲到椅子看电视区域");
                mvprintw(11, 20, "5. 被抓到游戏结束");
                mvprintw(13, 20, "按任意键返回菜单");
                attroff(COLOR_PAIR(COLOR_PAIR_MENU));
                refresh();
                timeout(-1);
                getch();
                timeout(0);
            } else if (ch == '3' || ch == 'q' || ch == 'Q') {
                cleanup();
                exit(0);
            }
            break;
            
        case MAP_SELECTION:
            if (ch == '1') {
                current_map = EUROPE_US;
                init_game();
            } else if (ch == '2') {
                current_map = VIENNA_HOTEL;
                init_game();
            } else if (ch == '3') {
                current_map = JAPAN;
                init_game();
            } else if (ch == 'm' || ch == 'M') {
                game_state = MENU;
            }
            break;
            
        case PLAYING:
            if (ch == ' ' || ch == 'p' || ch == 'P') {
                game_state = PAUSED;
            } else if (ch == 'm' || ch == 'M') {
                game_state = MENU;
            } else if (ch == 'q' || ch == 'Q') {
                cleanup();
                exit(0);
            } else {
                // 移动玩家
                int new_x = player.x;
                int new_y = player.y;
                
                switch (ch) {
                    case 'w':
                    case 'W':
                    case KEY_UP:
                        new_y--;
                        break;
                    case 's':
                    case 'S':
                    case KEY_DOWN:
                        new_y++;
                        break;
                    case 'a':
                    case 'A':
                    case KEY_LEFT:
                        new_x--;
                        break;
                    case 'd':
                    case 'D':
                    case KEY_RIGHT:
                        new_x++;
                        break;
                }
                
                // 检查边界和墙壁
                if (new_x >= 0 && new_x < maps[current_map].width &&
                    new_y >= 0 && new_y < maps[current_map].height &&
                    maps[current_map].layout[new_y][new_x] != '#') {
                    player.x = new_x;
                    player.y = new_y;
                }
            }
            break;
            
        case PAUSED:
            if (ch == ' ' || ch == 'p' || ch == 'P') {
                game_state = PLAYING;
            } else if (ch == 'm' || ch == 'M') {
                game_state = MENU;
            }
            break;
            
        case WIN:
        case LOST:
            if (ch == 'm' || ch == 'M') {
                game_state = MENU;
            } else if (ch == 'r' || ch == 'R') {
                init_game();
            }
            break;
    }
}

// 绘制菜单
void draw_menu() {
    clear();
    
    attron(COLOR_PAIR(COLOR_PAIR_MENU));
    mvprintw(5, 30, "不要让你的父母发现你在起飞");
    mvprintw(7, 30, "==========================");
    mvprintw(9, 30, "1. 开始游戏");
    mvprintw(10, 30, "2. 游戏说明");
    mvprintw(11, 30, "3. 退出游戏");
    mvprintw(13, 30, "选择选项 (1-3):");
    attroff(COLOR_PAIR(COLOR_PAIR_MENU));
    
    refresh();
}

// 绘制地图选择界面
void draw_map_selection() {
    clear();
    
    attron(COLOR_PAIR(COLOR_PAIR_MENU));
    mvprintw(5, 30, "选择地图");
    mvprintw(7, 30, "==========");
    mvprintw(9, 30, "1. 欧美卧室");
    mvprintw(10, 30, "2. 维也纳艺术酒店");
    mvprintw(11, 30, "3. 日本和室");
    mvprintw(13, 30, "M. 返回菜单");
    mvprintw(15, 30, "选择地图 (1-3):");
    attroff(COLOR_PAIR(COLOR_PAIR_MENU));
    
    refresh();
}

// 绘制游戏结束界面
void draw_game_over() {
    clear();
    
    attron(COLOR_PAIR(COLOR_PAIR_MENU));
    if (game_state == WIN) {
        mvprintw(5, 30, "恭喜! 你成功起飞了!");
        mvprintw(6, 30, "坚持了100秒没被父母发现!");
    } else {
        mvprintw(5, 30, "游戏结束! 你被父母发现了!");
        mvprintw(6, 30, "下次要更快躲起来!");
    }
    
    mvprintw(8, 30, "得分: %d", player.score);
    mvprintw(9, 30, "游戏时间: %d秒", total_time);
    mvprintw(11, 30, "R. 重新开始");
    mvprintw(12, 30, "M. 返回菜单");
    attroff(COLOR_PAIR(COLOR_PAIR_MENU));
    
    refresh();
}

// 绘制暂停界面
void draw_pause() {
    attron(COLOR_PAIR(COLOR_PAIR_WARNING));
    mvprintw(maps[current_map].height + 5, 2, "游戏暂停 - 按空格继续");
    attroff(COLOR_PAIR(COLOR_PAIR_WARNING));
}

// 清理资源
void cleanup() {
    // 释放地图内存
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < maps[i].height; j++) {
            free(maps[i].layout[j]);
        }
        free(maps[i].layout);
    }
    
    endwin();
}

// 主函数
int main() {
    // 初始化随机种子
    srand(time(NULL));
    
    // 初始化ncurses
    init_ncurses();
    
    // 初始化地图
    init_maps();
    
    // 主游戏循环
    int ch;
    while (1) {
        clear();
        
        // 根据游戏状态绘制不同界面
        switch (game_state) {
            case MENU:
                draw_menu();
                break;
                
            case MAP_SELECTION:
                draw_map_selection();
                break;
                
            case PLAYING:
                draw_map();
                draw_player();
                draw_parents();
                draw_ui();
                update_game();
                break;
                
            case PAUSED:
                draw_map();
                draw_player();
                draw_parents();
                draw_ui();
                draw_pause();
                break;
                
            case WIN:
            case LOST:
                draw_game_over();
                break;
        }
        
        // 获取输入
        ch = getch();
        if (ch != ERR) {
            handle_input(ch);
        }
        
        // 控制游戏速度
        usleep(game_speed);
        refresh();
    }
    
    cleanup();
    return 0;
}
