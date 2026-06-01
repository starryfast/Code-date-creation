#include <cstdio>
#include <iostream>
#include <string>
#include <windows.h>  
#include <cstdlib>     
#include <ctime>      
using namespace std;
HANDLE hConsole;
void delay(int ms) {
    Sleep(ms);
}
void gengxin(){
   string text = "欢迎使用3.0.0版本。与2.0.0版本相比，它还更新了随机字符串";
	for (size_t i = 0; i < text.length(); i++) {
		cout << text[i] << flush;
		delay(50);
	}cout<<endl; 
	string text1 = "开发工作已基本完成。如果您有任何建议，我们会相应地进行修改";
	for (size_t i = 0; i < text1.length(); i++) {
		cout << text1[i] << flush;
		delay(50);
	}cout<<endl; 
	string text2 = "你可以点个小小的赞吗？";
	for (size_t i = 0; i < text2.length(); i++) {
		cout << text2[i] << flush;
		delay(50);
	}cout<<endl; 
	delay(300);
} 
enum Color {
    BLACK       = 0,
    DARK_BLUE   = 1,
    DARK_GREEN  = 2,
    DARK_CYAN   = 3,
    DARK_RED    = 4, 
    DARK_MAGENTA= 5,
    DARK_YELLOW = 6,
    LIGHT_GRAY  = 7,
    GRAY        = 8,
    BLUE        = 9,
    GREEN       = 10,
    CYAN        = 11,
    RED         = 12,
    MAGENTA     = 13,
    YELLOW      = 14,
    WHITE       = 15
};
void setColor(int color) {
    SetConsoleTextAttribute(hConsole, color);
}
void clearScreen() {
    system("cls");
}

void gotoxy(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(hConsole, coord);
}
void hideCursor() {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}
void showCursor() {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = true;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}
int randomInt(int min, int max) {
    return min + rand() % (max - min + 1);
}
void drawGun(int type, int x, int y) {
    gotoxy(x, y);
    switch(type) {
        case 0:
            cout << "    ____";
            gotoxy(x, y+1); cout << "   |____|>";
            gotoxy(x, y+2); cout << "   |    |";
            gotoxy(x, y+3); cout << "   ︻┳═一";
            break;
        case 1: 
            setColor(YELLOW);
            cout << "    ____";
            gotoxy(x, y+1); cout << "   |####|>";
            gotoxy(x, y+2); cout << "   |    |";
            gotoxy(x, y+3); cout << "   ︻┳═一";
            setColor(LIGHT_GRAY);
            break;
        case 2: 
            setColor(CYAN);
            cout << "    ____";
            gotoxy(x, y+1); cout << "   |■■■■|>";
            gotoxy(x, y+2); cout << "   |    |";
            gotoxy(x, y+3); cout << "   ︻┳═一";
            setColor(LIGHT_GRAY);
            break;
        case 3: 
            cout << "    ____    ";
            gotoxy(x, y+1); cout << "   |____|>  ";
            gotoxy(x, y+2); cout << "   |    |   ";
            gotoxy(x, y+3); cout << "   ︻┳═一    ";
            break;
        case 4: 
            cout << "    ____";
            gotoxy(x, y+1); cout << "   |____|/";
            gotoxy(x, y+2); cout << "   |    |";
            gotoxy(x, y+3); cout << "   ︻┳═一";
            break;
        default: break;
    }
}
void drawTarget(int state, int x, int y, int flicker) {
    gotoxy(x, y);
    switch(state) {
        case 0:
            setColor(WHITE);
            cout << "   .-------.";
            gotoxy(x, y+1); cout << "  /         \\";
            gotoxy(x, y+2); cout << " |   (◎)    |";
            gotoxy(x, y+3); cout << "  \\         /";
            gotoxy(x, y+4); cout << "   '-------'";
            break;
        case 1: 
            setColor(WHITE);
            cout << "   .-------.";
            gotoxy(x, y+1); cout << "  /    *    \\";
            gotoxy(x, y+2); cout << " |    (X)    |";
            gotoxy(x, y+3); cout << "  \\         /";
            gotoxy(x, y+4); cout << "   '-------'";
            break;
        case 2: 
            setColor(RED);
            cout << "   .-------.";
            gotoxy(x, y+1); cout << "  /   * *   \\";
            gotoxy(x, y+2); cout << " |   (╬)    |";
            gotoxy(x, y+3); cout << "  \\   * *   /";
            gotoxy(x, y+4); cout << "   '-------'";
            break;
        case 3: 
            if (flicker % 2 == 0) setColor(RED);
            else setColor(YELLOW);
            cout << "   .-------.";
            gotoxy(x, y+1); cout << "  /   ~ ~   \\";
            gotoxy(x, y+2); cout << " |   (█)    |";
            gotoxy(x, y+3); cout << "  \\   ~ ~   /";
            gotoxy(x, y+4); cout << "   '-------'";
            break;
        case 4:
            setColor(GRAY);
            cout << "     .-------.";
            gotoxy(x, y+1); cout << "    /   *   \\";
            gotoxy(x, y+2); cout << "   |   (X)   |";
            gotoxy(x, y+3); cout << "    \\       /";
            gotoxy(x, y+4); cout << "     '-----'";
            break;
        default: break;
    }
    setColor(LIGHT_GRAY);
}
void drawBullet(int x, int y, int pos, bool tracer) {
    gotoxy(x + pos, y);
    if (tracer) setColor(RED);
    else setColor(YELLOW);
    cout << "?";
    setColor(LIGHT_GRAY);
}
void drawMuzzleFlash(int x, int y, int intensity) {
    gotoxy(x + 8, y + 3);
    if (intensity > 2) setColor(RED);
    else setColor(YELLOW);
    cout << "★";
    if (intensity > 1) cout << "★";
    setColor(LIGHT_GRAY);
}
void drawCasing(int x, int y, int step) {
    if (step <= 0) return;
    int cx = x + 10 + step;
    int cy = y + 2 - step/2;
    gotoxy(cx, cy);
    setColor(YELLOW);
    cout << "$";
    setColor(LIGHT_GRAY);
}
void drawSmoke(int x, int y, int step) {
    for (int i = 0; i < 3; i++) {
        int sx = x + 6 + randomInt(-2, 2) + step/2;
        int sy = y + 3 + randomInt(-1, 1) - step/3;
        if (sx < 80 && sy < 25) {
            gotoxy(sx, sy);
            setColor(GRAY);
            cout << ".";
            setColor(LIGHT_GRAY);
        }
    }
}
void drawMessage(const string& msg) {
    gotoxy(2, 20);
    setColor(CYAN);
    cout << msg;
    for (int i = (int)msg.length(); i < 60; i++) cout << " ";
    setColor(LIGHT_GRAY);
}
long long randomBetween(long long a, long long b) {
    if (a > b) {
        long long temp = a;
        a = b;
        b = temp;
    }
    unsigned long long range = (unsigned long long)(b - a) + 1;
    unsigned long long r = ((unsigned long long)rand() << 30) |
                           ((unsigned long long)rand() << 15) |
                           (unsigned long long)rand();
    return a + (long long)(r % range);
}
string randomString(int length) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    string result;
    result.reserve(length);
    for (int i = 0; i < length; ++i) {
        result += charset[rand() % (sizeof(charset) - 1)];
    }
    return result;
}

signed main() {
	SetConsoleTitleA("CDC3 数据生成器 - 3.0.0");
    srand((unsigned)time(NULL));
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    hideCursor();
    for (int frame = 0; frame < 10; frame++) {
        clearScreen();
        drawGun(1, 10, 10);
        drawTarget(0, 50, 10, 0);
        drawMessage("← 正在上膛...");
        delay(100);
    }
    for (int frame = 0; frame < 20; frame++) {
        clearScreen();
        int sway = (frame % 4) - 2;
        drawGun(2, 10 + sway, 10);
        drawTarget(0, 50, 10, 0);
        drawMessage("● 瞄准中...");
        delay(80);
    }
    for (int bulletPos = 0; bulletPos <= 30; bulletPos++) {
        clearScreen();
        int gunType = 0;
        if (bulletPos == 0) gunType = 3;
        else if (bulletPos == 1) gunType = 2;
        else gunType = 0;
        drawGun(gunType, 10, 10);
        if (bulletPos < 3) drawMuzzleFlash(10, 10, 3 - bulletPos);
        if (bulletPos >= 1 && bulletPos <= 5) drawCasing(10, 10, bulletPos - 1);
        if (bulletPos >= 1 && bulletPos <= 10) drawSmoke(10, 10, bulletPos);
        drawBullet(18, 13, bulletPos, bulletPos > 20);
        drawTarget(0, 50, 10, 0);
        drawMessage("→ 子弹飞行中...");
        delay(60);
    }
    for (int flicker = 0; flicker < 30; flicker++) {
        clearScreen();
        drawGun(0, 10, 10);
        gotoxy(48, 13);
        setColor(YELLOW);
        cout << "?";
        setColor(LIGHT_GRAY);
        if (flicker < 15) drawTarget(1, 50, 10, flicker);
        else drawTarget(2, 50, 10, flicker);
        if (flicker % 3 == 0) {
            gotoxy(55, 11); setColor(RED); cout << "★";
            gotoxy(54, 12); cout << "★";
            gotoxy(56, 12); cout << "★";
            setColor(LIGHT_GRAY);
        }
        drawMessage("◆ 击中靶心！ ◆");
        delay(90);
    }
    for (int frag = 0; frag < 30; frag++) {
        clearScreen();
        drawGun(0, 10, 10);
        drawTarget(3, 50, 10, frag);
        for (int piece = 0; piece < 8; piece++) {
            int px = 52 + randomInt(-3, 3);
            int py = 11 + randomInt(-2, 2);
            gotoxy(px, py);
            setColor(YELLOW);
            cout << "·";
            setColor(LIGHT_GRAY);
        }
        if (frag > 20) {
            gotoxy(55, 12);
            setColor(RED);
            cout << "~~";
            setColor(LIGHT_GRAY);
        }
        drawMessage("※ 靶子碎裂燃烧...");
        delay(80);
    }
    for (int fall = 0; fall < 15; fall++) {
        clearScreen();
        drawGun(4, 10, 10);
        int yOffset = fall / 5;
        drawTarget(4, 50, 10 + yOffset, 0);
        drawMessage("〓 靶子倒下...");
        delay(100);
    }
    for (int cheers = 0; cheers < 10; cheers++) {
        clearScreen();
        drawGun(0, 10, 10);
        gotoxy(45, 13);
        setColor(YELLOW);
        cout << "★★★  PERFECT  ★★★";
        setColor(LIGHT_GRAY);
        for (int i = 0; i < 20; i++) {
            gotoxy(randomInt(5, 75), randomInt(10, 18));
            setColor(randomInt(10, 15));
            cout << "*";
            setColor(LIGHT_GRAY);
        }
        drawMessage("★ 射击完成！ ★");
        delay(120);
    }
    clearScreen();
    gotoxy(28, 12);
    setColor(GREEN);
    cout << "完美命中！";
    delay(1500);

    clearScreen();
    gotoxy(28, 14);
    setColor(MAGENTA);
    string text = "CDC官方出品";
    for (size_t i = 0; i < text.length(); i++) {
        cout << text[i] << flush;
        delay(150);
    }
    cout << endl << endl;
    gotoxy(30, 16);
    setColor(CYAN);
    delay(2000);
    showCursor();
    gotoxy(0, 24); system("cls");
    cout<<"是否介绍？(1/0)";
    int n;
    cin>>n;
    if(n==1)
    gengxin(); 
    system("cls");
    cout << "能点个关注吗？作者免费小工具starryfast出品，作者也不能维持数年orz"<<endl;
	ShellExecuteW(NULL, L"open", L"https://www.luogu.com.cn/user/2070675", NULL, NULL, SW_SHOWNORMAL);
	ShellExecuteW(NULL, L"open", L"https://space.bilibili.com/3546973651077402", NULL, NULL, SW_SHOWNORMAL);
    Sleep(300);
    gengxin();
    int a11, b11;
    cout << "请输入起始文件编号和结束文件编号（两个整数）: ";
    cin >> a11 >> b11;

    int dataType;
    cout << "请选择数据类型 (1-整数, 2-字符串): ";
    cin >> dataType;

    long long x11 = 0, y11 = 0;
    int strLen = 0;
    if (dataType == 1) {
        cout << "请输入随机数据的范围（两个整数）: ";
        cin >> x11 >> y11;
    } else {
        cout << "请输入字符串长度: ";
        cin >> strLen;
    }

    int cdc;
    cout << "请输入每个数据的数量: ";
    cin >> cdc;
    cout << "是否输入每个数据的数量?(1/0)";
    bool pd;
    cin >> pd;
    for (int i = a11; i <= b11; ++i) {
        char filename[20];
        sprintf(filename, "%d.in", i);
        FILE* fp = fopen(filename, "w");
        if (pd == 1) fprintf(fp, "%d\n", cdc);
        for (int j = 1; j <= cdc; ++j) {
            if (dataType == 1) {
                fprintf(fp, "%lld ", randomBetween(x11, y11));
            } else {
                string s = randomString(strLen);
                fprintf(fp, "%s ", s.c_str());
            }
        }
        fclose(fp);
    }

    for (int i = a11; i <= b11; ++i) {
        char filename[20];
        sprintf(filename, "%d.out", i);
        FILE* fp = fopen(filename, "w");
        fclose(fp);
    }
    cout << "\n所有文件已生成完毕，按回车键退出...";
	cin.ignore(); cin.get();
    return 0;
}
