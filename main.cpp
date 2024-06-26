#include <iostream>
#include <vector>

constexpr int Height = 30;
constexpr int Width = 40;
constexpr int EmptyIdx = -1;
constexpr int SelfName = 2023202296;

constexpr double ValueOfScoreOne = 100;
constexpr double ValueOfScoreTwo = 110;
constexpr double ValueOfScoreThree = 120;
constexpr double ValueOfScoreFive = 130;
constexpr double valueOfLength = 100;

constexpr int ValueSpreadRounds = 70;
constexpr double ValueSpreadDecline = 0.75;  // percentage

enum Operation {
    Left = 0,
    Up = 1,
    Right = 2,
    Down = 3,
    Shield = 4,
};

Operation Reverse(Operation operation) {
    switch (operation) {
        case Operation::Left:
            return Operation::Right;
        case Operation::Right:
            return Operation::Left;
        case Operation::Up:
            return Operation::Down;
        case Operation::Down:
            return Operation::Up;
        default:
            return Operation::Shield;
    }
}

int DhOfOperation(Operation operation) {
    switch (operation) {
        case Operation::Left:
            return 0;
        case Operation::Right:
            return 0;
        case Operation::Up:
            return -1;
        case Operation::Down:
            return +1;
        default:
            return 0;
    }
}

int DwOfOperation(Operation operation) {
    switch (operation) {
        case Operation::Left:
            return -1;
        case Operation::Right:
            return +1;
        case Operation::Up:
            return 0;
        case Operation::Down:
            return 0;
        default:
            return 0;
    }
}

enum class ObjType {
    None,
    ScoreOne,
    ScoreTwo,
    ScoreThree,
    ScoreFive,
    Length,
    Trap,
    Wall,
};

struct Point {
    int h, w;
};

struct SnakeInfo {
    int Idx;
    int Name;
    int Length;
    int Score;
    Operation LastOperation;
    int ShieldCD;  // cold delay
    int ShieldET;  // effect time remaining
    std::vector<Point> Body;
};

struct Cell {
    int SnakeIdx;  // -1: Empty, 0: Self, 1~: Other Snake
    ObjType Obj;
};

typedef struct Game {
    int TimeRemain;
    int SelfIdx;
    std::vector<SnakeInfo> SnakeInfos;
    Cell Map[Height][Width];
}* PGame;

PGame CreateGame() {
    PGame pGame = new Game();
    std::cin >> pGame->TimeRemain;

    for (int h = 0; h < Height; h++) {
        for (int w = 0; w < Width; w++) {
            pGame->Map[h][w] = Cell{.SnakeIdx = -1, .Obj = ObjType::None};
        }
    }

    int obj_cnt;
    std::cin >> obj_cnt;
    for (int i = 0; i < obj_cnt; i++) {
        int h, w, type_idx;
        std::cin >> h >> w >> type_idx;
        ObjType type = ObjType::None;
        switch (type_idx) {
            case -4:
                type = ObjType::Wall;
                break;
            case -2:
                type = ObjType::Trap;
                break;
            case -1:
                type = ObjType::Length;
                break;
            case 1:
                type = ObjType::ScoreOne;
                break;
            case 2:
                type = ObjType::ScoreTwo;
                break;
            case 3:
                type = ObjType::ScoreThree;
                break;
            case 5:
                type = ObjType::ScoreFive;
                break;
        }
        pGame->Map[h][w].Obj = type;
    }

    int snake_cnt;
    std::cin >> snake_cnt;
    pGame->SnakeInfos.resize(snake_cnt);
    for (int snake_idx = 0; snake_idx < snake_cnt; snake_idx++) {
        int name, length, score, operation, shield_cd, shield_et;
        std::cin >> name >> length >> score >> operation >> shield_cd >> shield_et;
        if (name == SelfName) {
            pGame->SelfIdx = snake_idx;
        }
        pGame->SnakeInfos[snake_idx] = SnakeInfo{
            .Idx = snake_idx,
            .Name = name,
            .Length = length,
            .Score = score,
            .LastOperation = (Operation)operation,
            .ShieldCD = shield_cd,
            .ShieldET = shield_et,
        };
        for (int i = 0; i < length; i++) {
            int h, w;
            std::cin >> h >> w;
            pGame->SnakeInfos[snake_idx].Body.push_back(Point{.h = h, .w = w});
            pGame->Map[h][w].SnakeIdx = snake_idx;
        }
    }
    return pGame;
}

bool CanOperate(PGame pGame, int SnakeIdx, Operation operation) {
    if (operation == Operation::Shield) {
        return pGame->SnakeInfos[SnakeIdx].ShieldCD <= 0;
    }
    if (pGame->SnakeInfos[SnakeIdx].LastOperation == Reverse(operation)) {
        return false;
    }
    int dh, dw;
    dh = DhOfOperation(operation);
    dw = DwOfOperation(operation);
    int head_h, head_w, head_h_next, head_w_next;
    head_h = pGame->SnakeInfos[SnakeIdx].Body[0].h;
    head_w = pGame->SnakeInfos[SnakeIdx].Body[0].w;
    head_h_next = head_h + dh;
    head_w_next = head_w + dw;
    if (head_h_next < 0 || head_h_next >= Height || head_w_next < 0 || head_w_next >= Width) {
        return false;
    }
    if (pGame->Map[head_h_next][head_w_next].Obj == ObjType::Wall) {
        return false;
    }
    if (pGame->Map[head_h_next][head_w_next].SnakeIdx != EmptyIdx &&
        pGame->Map[head_h_next][head_w_next].SnakeIdx != SnakeIdx) {
        return false;
    }
    return true;
}

typedef struct ValueMap {
    double values[Height][Width];
}* PValueMap;

PValueMap CreateValueMap(PGame pGame) {
    // Initialize value
    PValueMap pMap = new ValueMap();
    for (int h = 0; h < Height; h++) {
        for (int w = 0; w < Width; w++) {
            pMap->values[h][w] = 0;
            if (pGame->Map[h][w].Obj == ObjType::ScoreOne) {
                pMap->values[h][w] = ValueOfScoreOne;
            }
            if (pGame->Map[h][w].Obj == ObjType::ScoreTwo) {
                pMap->values[h][w] = ValueOfScoreTwo;
            }
            if (pGame->Map[h][w].Obj == ObjType::ScoreThree) {
                pMap->values[h][w] = ValueOfScoreThree;
            }
            if (pGame->Map[h][w].Obj == ObjType::ScoreFive) {
                pMap->values[h][w] = ValueOfScoreFive;
            }
            if (pGame->Map[h][w].Obj == ObjType::Length) {
                pMap->values[h][w] = valueOfLength;
            }
        }
    }

    // Spread value
    for (int round = 0; round < ValueSpreadRounds; round++) {
        PValueMap pMapNext = new ValueMap();
        for (int h = 0; h < Height; h++) {
            for (int w = 0; w < Width; w++) {
                pMapNext->values[h][w] = pMap->values[h][w];
                for (int dh = -1; dh <= 1; dh += 2) {
                    for (int dw = -1; dw <= 1; dw += 2) {
                        int h_next = h + dh;
                        int w_next = w + dw;
                        if (h_next < 0 || h_next >= Height || w_next < 0 || w_next >= Width) {
                            continue;
                        }
                        double new_value = pMap->values[h_next][w_next] * ValueSpreadDecline;
                        if (new_value > pMapNext->values[h][w]) {
                            pMapNext->values[h][w] = new_value;
                        }
                    }
                }
            }
        }
        delete pMap;
        pMap = pMapNext;
    }
    return pMap;
}

int main() {
    PGame pGame = CreateGame();
    PValueMap pValueMap = CreateValueMap(pGame);
    const Operation move_operations[] = {Operation::Left, Operation::Up, Operation::Right, Operation::Down};
    Operation best_operation = Operation::Shield;
    double best_value = 0;
    for (Operation operation : move_operations) {
        if (!CanOperate(pGame, pGame->SelfIdx, operation)) {
            continue;
        }
        int dh, dw;
        dh = DhOfOperation(operation);
        dw = DwOfOperation(operation);
        int head_h, head_w, head_h_next, head_w_next;
        head_h = pGame->SnakeInfos[pGame->SelfIdx].Body[0].h;
        head_w = pGame->SnakeInfos[pGame->SelfIdx].Body[0].w;
        head_h_next = head_h + dh;
        head_w_next = head_w + dw;
        double value = pValueMap->values[head_h_next][head_w_next];
        if (value > best_value) {
            best_value = value;
            best_operation = operation;
        }
    }
    std::cout << best_operation;
}
