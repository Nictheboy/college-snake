#include <algorithm>
#include <iostream>
#include <list>
#include <stack>
#include <vector>

constexpr int Height = 30;
constexpr int Width = 40;
constexpr int EmptyIdx = -1;
constexpr int SelfName = 2023202296;

constexpr int ShieldCost = 20;
constexpr int ShieldCD = 30;
constexpr int ShieldET = 5;
constexpr int ScorePerLength = 20;
constexpr int LengthOfLengthBean = 2;
constexpr int ScorePanaltyOfTrap = 10;

constexpr double ValueBaseOfScore = 200;
constexpr double ValuePerScore = 50;
constexpr double ValueOfLength = 100;

constexpr int ValueSpreadRounds = 70;
constexpr double ValueSpreadDecline = 0.75;  // percentage

//
//  Generic Field
//

template <typename T>
class Field {
   private:
    T* values;

   public:
    Field() {
        values = new T[Height * Width];
        for (int h = 0; h < Height; h++) {
            for (int w = 0; w < Width; w++) {
                (*this)[h][w] = T();
            }
        }
    }

    ~Field() {
        if (values)
            delete[] values;
    }

    Field(Field& field) = delete;
    Field(Field&& field) {
        values = field.values;
        field.values = nullptr;
    };
    void operator=(Field& field) = delete;
    void operator=(Field&& field) {
        if (values)
            delete[] values;
        values = field.values;
        field.values = nullptr;
    };

    T* operator[](int h) {
        return values + h * Width;
    }

    Field<T> Clone() const {
        Field<T> new_field;
        for (int h = 0; h < Height; h++) {
            for (int w = 0; w < Width; w++) {
                new_field[h][w] = (*this)[h][w];
            }
        }
        return new_field;
    }
};

//
//  Basic Definitions and Game Rules
//

enum Operation {
    Left = 0,
    Up = 1,
    Right = 2,
    Down = 3,
    Shield = 4,
};

struct SnakeIdxAndOperation {
    int Idx;
    Operation Op;
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

enum ObjType {
    ScoreZero = 0,
    ScoreOne = 1,
    ScoreTwo = 2,
    ScoreThree = 3,
    ScoreFour = 4,
    ScoreFive = 5,
    ScoreSix = 6,
    ScoreSeven = 7,
    ScoreEight = 8,
    ScoreNine = 9,
    ScoreTen = 10,
    ScoreEleven = 11,
    ScoreTwelve = 12,
    ScoreThirteen = 13,
    ScoreFourteen = 14,
    ScoreFifteen = 15,
    ScoreSixteen = 16,
    ScoreSeventeen = 17,
    ScoreEighteen = 18,
    ScoreNineteen = 19,
    ScoreTwenty = 20,
    ScoreTooLarge = 21,
    None,
    Length,
    Trap,
    Wall,
};

struct Point {
    int h, w;
};

struct SnakeInfo {
    int Idx;
    bool Alive;
    int Name;
    int Score;
    Operation LastOperation;
    int ShieldCD;  // cold delay
    int ShieldET;  // effect time remaining
    std::list<Point> Body;
};

struct Cell {
    int SnakeIdx;  // -1: Empty, 0: Self, 1~: Other Snake
    ObjType Obj;
};

struct Game {
    int TimeRemain;
    int SelfIdx;
    std::vector<SnakeInfo> SnakeInfos;
    Cell Map[Height][Width];

    Game() {
        std::cin >> TimeRemain;

        for (int h = 0; h < Height; h++) {
            for (int w = 0; w < Width; w++) {
                Map[h][w] = Cell{.SnakeIdx = -1, .Obj = None};
            }
        }

        int obj_cnt;
        std::cin >> obj_cnt;
        for (int i = 0; i < obj_cnt; i++) {
            int h, w, type_idx;
            std::cin >> h >> w >> type_idx;
            ObjType type = None;
            switch (type_idx) {
                case -4:
                    type = Wall;
                    break;
                case -2:
                    type = Trap;
                    break;
                case -1:
                    type = Length;
                    break;
                default:
                    if (type > ScoreZero && type < ScoreTooLarge) {
                        type = (ObjType)(ScoreZero + type_idx);
                    }
                    break;
            }
            if (Map[h][w].Obj != Wall)
                Map[h][w].Obj = type;
        }

        int snake_cnt;
        std::cin >> snake_cnt;
        SnakeInfos.resize(snake_cnt);
        for (int snake_idx = 0; snake_idx < snake_cnt; snake_idx++) {
            int name, length, score, operation, shield_cd, shield_et;
            std::cin >> name >> length >> score >> operation >> shield_cd >> shield_et;
            if (name == SelfName) {
                SelfIdx = snake_idx;
            }
            SnakeInfos[snake_idx] = SnakeInfo{
                .Idx = snake_idx,
                .Alive = true,
                .Name = name,
                .Score = score,
                .LastOperation = (Operation)operation,
                .ShieldCD = shield_cd,
                .ShieldET = shield_et,
            };
            for (int i = 0; i < length; i++) {
                int h, w;
                std::cin >> h >> w;
                SnakeInfos[snake_idx].Body.push_back(Point{.h = h, .w = w});
                Map[h][w].SnakeIdx = snake_idx;
            }
        }
    }

    bool CanOperate(int SnakeIdx, Operation operation) {
        if (operation == Operation::Shield) {
            return SnakeInfos[SnakeIdx].ShieldCD <= 0 && SnakeInfos[SnakeIdx].Score > ShieldCost;
        }
        if (SnakeInfos[SnakeIdx].LastOperation == Reverse(operation)) {
            return false;
        }
        if (!SnakeInfos[SnakeIdx].Alive) {
            return false;
        }
        int dh, dw;
        dh = DhOfOperation(operation);
        dw = DwOfOperation(operation);
        int head_h, head_w, head_h_next, head_w_next;
        head_h = SnakeInfos[SnakeIdx].Body.front().h;
        head_w = SnakeInfos[SnakeIdx].Body.front().w;
        head_h_next = head_h + dh;
        head_w_next = head_w + dw;
        if (head_h_next < 0 || head_h_next >= Height || head_w_next < 0 || head_w_next >= Width) {
            return false;
        }
        if (Map[head_h_next][head_w_next].Obj == Wall) {
            return false;
        }
        if (Map[head_h_next][head_w_next].SnakeIdx != EmptyIdx &&
            Map[head_h_next][head_w_next].SnakeIdx != SnakeIdx) {
            return false;
        }
        return true;
    }

   private:
    struct RevokeEntry {
        struct SnakeInfoRevokeEntry {
            int SnakeIdx;
            SnakeInfo Last;
        };

        struct MapRevokeEntry {
            int H, W;
            Cell Last;
        };

        std::vector<SnakeInfoRevokeEntry> SnakeInfoRevokeList;
        std::vector<MapRevokeEntry> MapRevokeList;
    };

    std::stack<RevokeEntry> RevokeStack;

   public:
    void ImagineTailLengthen(int snake_idx, RevokeEntry& r_entry) {
        const int tail_h = SnakeInfos[snake_idx].Body.back().h;
        const int tail_w = SnakeInfos[snake_idx].Body.back().w;
        const int pre_tail_h = (++SnakeInfos[snake_idx].Body.rbegin())->h;
        const int pre_tail_w = (++SnakeInfos[snake_idx].Body.rbegin())->w;
        Operation direction;
        for (auto i : {Operation::Left, Operation::Up, Operation::Right, Operation::Down}) {
            if (pre_tail_h + DhOfOperation(i) == tail_h && pre_tail_w + DwOfOperation(i) == tail_w) {
                direction = i;
                break;
            }
        }
        if (direction == Operation::Right) {
            if (tail_w == 39) {
                if (tail_h == 29)
                    direction = Operation::Up;
                else
                    direction = Operation::Down;
            } else {
                direction = Operation::Right;
            }
        } else if (direction == Operation::Down) {
            if (tail_h == 29) {
                if (tail_w == 39)
                    direction = Operation::Left;
                else
                    direction = Operation::Right;
            } else {
                direction = Operation::Down;
            }
        } else if (direction == Operation::Left) {
            if (tail_w == 0) {
                if (tail_h == 29)
                    direction = Operation::Up;
                else
                    direction = Operation::Down;
            } else {
                direction = Operation::Left;
            }
        } else {
            if (tail_h == 0) {
                if (tail_w == 39)
                    direction = Operation::Left;
                else
                    direction = Operation::Right;
            } else {
                direction = Operation::Up;
            }
        }
        const int next_h = tail_h + DhOfOperation(direction);
        const int next_w = tail_w + DwOfOperation(direction);
        r_entry.MapRevokeList.push_back(RevokeEntry::MapRevokeEntry{
            .H = next_h,
            .W = next_w,
            .Last = Map[next_h][next_w],
        });
        Map[next_h][next_w].SnakeIdx = snake_idx;
        SnakeInfos[snake_idx].Body.push_back(Point{.h = next_h, .w = next_w});
    }

    void ImagineDeath(int snake_idx, RevokeEntry& r_entry) {
        int s_score = SnakeInfos[snake_idx].Score;
        for (const auto& point : SnakeInfos[snake_idx].Body) {
            r_entry.MapRevokeList.push_back(RevokeEntry::MapRevokeEntry{
                .H = point.h,
                .W = point.w,
                .Last = Map[point.h][point.w],
            });
            Map[point.h][point.w] = {.SnakeIdx = EmptyIdx, .Obj = None};
            if (s_score <= 0) {
                break;
            }
            if (Map[point.h][point.w].Obj != None) {
                continue;
            }
            if (s_score >= 20) {
                Map[point.h][point.w].Obj = ScoreTwenty;
                s_score -= 20;
            } else {
                Map[point.h][point.w].Obj = (ObjType)(ScoreZero + s_score);
                s_score = 0;
            }
        }
        SnakeInfos[snake_idx].Alive = false;
        SnakeInfos[snake_idx].Body.clear();
    }

    void ImagineOperations(std::vector<SnakeIdxAndOperation> operations, bool lucky_tail) {
        RevokeEntry r_entry;
        TimeRemain--;
        std::sort(operations.begin(), operations.end(), [](const SnakeIdxAndOperation& a, const SnakeIdxAndOperation& b) {
            return a.Idx < b.Idx;
        });
        for (const auto& op : operations) {
            SnakeInfo& snake = SnakeInfos[op.Idx];
            r_entry.SnakeInfoRevokeList.push_back(RevokeEntry::SnakeInfoRevokeEntry{
                .SnakeIdx = op.Idx,
                .Last = snake,
            });
            if (op.Op == Operation::Shield) {
                if (snake.ShieldCD > 0) {
                    ImagineDeath(op.Idx, r_entry);
                } else {
                    snake.ShieldCD = ShieldCD;
                    snake.ShieldET = ShieldET;
                    snake.Score -= ShieldCost;
                }
            }
        }
        for (const auto& op : operations) {
            SnakeInfo& snake = SnakeInfos[op.Idx];
            const Operation operation = op.Op;
            if (op.Op != Operation::Shield) {
                if (snake.ShieldET > 0) {
                    snake.ShieldET--;
                }
                if (snake.ShieldCD > 0) {
                    snake.ShieldCD--;
                }
                const int dh = DhOfOperation(operation);
                const int dw = DwOfOperation(operation);
                const int head_h = snake.Body.front().h;
                const int head_w = snake.Body.front().w;
                const int head_h_next = head_h + dh;
                const int head_w_next = head_w + dw;
                if (head_h_next < 0 || head_h_next >= Height || head_w_next < 0 || head_w_next >= Width) {
                    ImagineDeath(op.Idx, r_entry);
                    continue;
                }
                const Cell head_next_cell = Map[head_h_next][head_w_next];
                r_entry.MapRevokeList.push_back(RevokeEntry::MapRevokeEntry{
                    .H = head_h_next,
                    .W = head_w_next,
                    .Last = head_next_cell,
                });
                const int tail_h = snake.Body.back().h;
                const int tail_w = snake.Body.back().w;
                r_entry.MapRevokeList.push_back(RevokeEntry::MapRevokeEntry{
                    .H = tail_h,
                    .W = tail_w,
                    .Last = Map[tail_h][tail_w],
                });

                // Move Logic
                snake.Body.push_front(Point{.h = head_h_next, .w = head_w_next});
                snake.Body.pop_back();
                Map[head_h_next][head_w_next].SnakeIdx = op.Idx;  // do not change Obj
                Map[tail_h][tail_w] = Cell{.SnakeIdx = EmptyIdx, .Obj = None};
            }
        }
        for (const auto& op : operations) {
            SnakeInfo& snake = SnakeInfos[op.Idx];
            const int head_h = snake.Body.front().h;
            const int head_w = snake.Body.front().w;
            const int old_score = snake.Score;
            int lengthen = 0;
            switch (Map[head_h][head_w].Obj) {
                case Length:
                    lengthen += 2;
                    break;

                case Trap:
                    snake.Score -= ScorePanaltyOfTrap;
                    break;

                case Wall:
                    ImagineDeath(op.Idx, r_entry);
                    continue;

                default:
                    if (Map[head_h][head_w].Obj > ScoreZero && Map[head_h][head_w].Obj < ScoreTooLarge) {
                        snake.Score += Map[head_h][head_w].Obj - ScoreZero;
                    }
            }
            Map[head_h][head_w].Obj = None;
            lengthen += snake.Score / ScorePerLength - old_score / ScorePerLength;
            lengthen = lucky_tail ? LengthOfLengthBean : lengthen;
            if (lengthen-- > 0) {
                ImagineTailLengthen(op.Idx, r_entry);
            }
        }
        for (const auto& op : operations) {
            if (!SnakeInfos[op.Idx].Alive || SnakeInfos[op.Idx].ShieldET > 0) {
                continue;
            }
            SnakeInfo& snake = SnakeInfos[op.Idx];
            for (auto& other_snake : SnakeInfos) {
                if (other_snake.Idx == snake.Idx) {
                    continue;
                }
                for (auto& point : other_snake.Body) {
                    if (point.h == snake.Body.front().h && point.w == snake.Body.front().w) {
                        if (point.h == other_snake.Body.front().h && point.w == other_snake.Body.front().w) {
                            ImagineDeath(other_snake.Idx, r_entry);
                        }
                        ImagineDeath(op.Idx, r_entry);
                        break;
                    }
                }
            }
        }
        RevokeStack.push(r_entry);
    }

    void RevokeOperations() {
        RevokeEntry r_entry = RevokeStack.top();
        RevokeStack.pop();
        TimeRemain++;
        for (int i = r_entry.SnakeInfoRevokeList.size() - 1; i >= 0; i--) {
            auto& item = r_entry.SnakeInfoRevokeList[i];
            SnakeInfos[item.SnakeIdx] = item.Last;
        }
        for (int i = r_entry.MapRevokeList.size() - 1; i >= 0; i--) {
            auto& item = r_entry.MapRevokeList[i];
            Map[item.H][item.W] = item.Last;
        }
    }
};

//
//  Value System
//

Field<double> CreateValueMap(Game& game) {
    // Initialize value
    Field<double> Map;
    for (int h = 0; h < Height; h++) {
        for (int w = 0; w < Width; w++) {
            Map[h][w] = 0;
            if (game.Map[h][w].Obj > ScoreZero && game.Map[h][w].Obj < ScoreTooLarge) {
                Map[h][w] = ValuePerScore * (game.Map[h][w].Obj - ScoreZero) + ValueBaseOfScore;
            }
            if (game.Map[h][w].Obj == Length) {
                Map[h][w] = ValueOfLength;
            }
        }
    }

    // Spread value
    for (int round = 0; round < ValueSpreadRounds; round++) {
        Field<double> MapNext;
        for (int h = 0; h < Height; h++) {
            for (int w = 0; w < Width; w++) {
                MapNext[h][w] = Map[h][w];
                for (int dh = -1; dh <= 1; dh += 2) {
                    for (int dw = -1; dw <= 1; dw += 2) {
                        int h_next = h + dh;
                        int w_next = w + dw;
                        if (h_next < 0 || h_next >= Height || w_next < 0 || w_next >= Width) {
                            continue;
                        }
                        double new_value = Map[h_next][w_next] * ValueSpreadDecline;
                        if (new_value > MapNext[h][w]) {
                            MapNext[h][w] = new_value;
                        }
                    }
                }
            }
        }
        Map = std::move(MapNext);
    }
    return Map;
}

//
//  Main Logic
//

int main() {
    Game game = Game();
    Field<double> ValueMap = CreateValueMap(game);
    const Operation move_operations[] = {Operation::Left, Operation::Up, Operation::Right, Operation::Down};
    Operation best_operation = Operation::Shield;
    double best_value = 0;
    for (Operation operation : move_operations) {
        if (!game.CanOperate(game.SelfIdx, operation)) {
            continue;
        }
        int dh, dw;
        dh = DhOfOperation(operation);
        dw = DwOfOperation(operation);
        int head_h, head_w, head_h_next, head_w_next;
        head_h = game.SnakeInfos[game.SelfIdx].Body.front().h;
        head_w = game.SnakeInfos[game.SelfIdx].Body.front().w;
        head_h_next = head_h + dh;
        head_w_next = head_w + dw;
        double value = ValueMap[head_h_next][head_w_next];
        if (value > best_value) {
            best_value = value;
            best_operation = operation;
        }
    }
    std::cout << best_operation;
}
