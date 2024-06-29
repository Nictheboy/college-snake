#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <list>
#include <queue>
#include <stack>
#include <valarray>
#include <vector>

constexpr int Height = 30;
constexpr int Width = 40;
constexpr int EmptyIdx = -1;
constexpr int SelfName = 2023202296;

constexpr int TotalTime = 256;
constexpr int ShieldCost = 20;
constexpr int ShieldCD = 30;
constexpr int ShieldET = 5;
constexpr int ScorePerLength = 20;
constexpr int LengthOfLengthBean = 2;
constexpr int ScorePanaltyOfTrap = 10;
constexpr int ExecutionMillisecondLimit = 200 * 0.9;

constexpr int CenterH = Height / 2;
constexpr int CenterW = Width / 2;
constexpr int RadiusOfMap = Height / 2 + Width / 2;

constexpr double ValueSpreadDecline = 0.75;  // percentage
constexpr double BaseValueOfScore = 0;
constexpr double ValuePerScore = 100;
constexpr double ValueOfLengthAtBegin = 0;
constexpr double ValueOfLengthAtEnd = 0;
constexpr double ValueOfCenter = 3000;
constexpr double ValueOnlyInCenter = 0;
constexpr int TickCenterValueBegin = 100;
constexpr int TickCenterLockBegin = 256;

constexpr double BaseDeclineOfCompetitivity = 0.00;
constexpr double DeclinePerCompetitivity = 0.50;
constexpr double CorrectionForSpreadableFields = 0.15;

constexpr double ValueOfTrap = -1000;
constexpr double ValueOfWall = -1e10;
constexpr double ValueOfDeathPerRemainTime = -300;
constexpr double ValueOfOpponentWhenHaveShield = -2000;

constexpr double VerySmallValue = -1e20;
constexpr double VeryLargeValue = 1e20;

constexpr double UtilityPerScore = 200;
constexpr double UtilityPerValue = 1;
constexpr double UtilityOfShield = -4000;
constexpr double UtilityOfOpponentShield = 2000;
constexpr double UtilityOfOpponentDeath = 4000;
constexpr double DeclinePerDepth = 0.8;  // 1.0 := no decline

//
//  Generic Field
//

struct Point {
    int h, w;
};

template <typename T>
class Field {
   private:
    T* values;

   public:
    Field() {
        values = new T[Height * Width];
    }

    Field(T init_value) {
        values = new T[Height * Width];
        for (int h = 0; h < Height; h++) {
            for (int w = 0; w < Width; w++) {
                (*this)[h][w] = init_value;
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

    const T* operator[](int h) const {
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

    template <typename U>
    auto operator+(const Field<U>& field) const {
        Field<decltype(T() + U())> new_field;
        for (int h = 0; h < Height; h++) {
            for (int w = 0; w < Width; w++) {
                new_field[h][w] = (*this)[h][w] + field[h][w];
            }
        }
        return new_field;
    }

    template <typename U>
    auto operator-(const Field<U>& field) const {
        Field<decltype(T() - U())> new_field;
        for (int h = 0; h < Height; h++) {
            for (int w = 0; w < Width; w++) {
                new_field[h][w] = (*this)[h][w] - field[h][w];
            }
        }
        return new_field;
    }

    template <typename U>
    auto operator*(U scalar) const {
        Field<decltype(U() * T())> new_field;
        for (int h = 0; h < Height; h++) {
            for (int w = 0; w < Width; w++) {
                new_field[h][w] = (*this)[h][w] * scalar;
            }
        }
        return new_field;
    }

    template <typename F>
    auto Map(F f) const {
        Field<decltype(f(T()))> new_field;
        for (int h = 0; h < Height; h++) {
            for (int w = 0; w < Width; w++) {
                new_field[h][w] = f((*this)[h][w]);
            }
        }
        return new_field;
    }

    Field<T> MaxWith(const Field<T>& field) const {
        Field<T> new_field;
        for (int h = 0; h < Height; h++) {
            for (int w = 0; w < Width; w++) {
                new_field[h][w] = std::max((*this)[h][w], field[h][w]);
            }
        }
        return new_field;
    }

    Field<T> MinWith(const Field<T>& field) const {
        Field<T> new_field;
        for (int h = 0; h < Height; h++) {
            for (int w = 0; w < Width; w++) {
                new_field[h][w] = std::min((*this)[h][w], field[h][w]);
            }
        }
        return new_field;
    }

    Field<T> Standardlize(T standard) {
        T sum = 0;
        for (int h = 0; h < Height; h++) {
            for (int w = 0; w < Width; w++) {
                sum += (*this)[h][w];
            }
        }
        T avg = sum / (Height * Width);
        return (*this) * (standard / avg);
    }

    void DijkstrativeReduce(std::vector<std::pair<Point, T>> source,
                            std::function<void(Point, std::function<void(Point, T)>)> reducer) {
        std::queue<Point> queue;
        for (auto& item : source) {
            Point point = item.first;
            T value = item.second;
            queue.push(point);
            (*this)[point.h][point.w] = value;
        }
        while (!queue.empty()) {
            auto current = queue.front();
            queue.pop();
            reducer(current, [&](Point point, T new_value) {
                (*this)[point.h][point.w] = new_value;
                queue.push(point);
            });
        }
    }

    void PrintValuesNearby(Point point, int radius) const {
        for (int h = point.h - radius; h <= point.h + radius; h++) {
            for (int w = point.w - radius; w <= point.w + radius; w++) {
                if (h == point.h && w == point.w) {
                    std::cerr << "[";
                } else {
                    std::cerr << " ";
                }
                if (h < 0 || h >= Height || w < 0 || w >= Width) {
                    fprintf(stderr, "   X  ");
                } else {
                    fprintf(stderr, "%6.1g", (*this)[h][w]);
                }
                if (h == point.h && w == point.w) {
                    std::cerr << "]";
                } else {
                    std::cerr << " ";
                }
            }
            std::cerr << std::endl;
        }
    }
};

//
//  Basic Definitions and Game Rules
//

enum Operation {
    Invalid = -1,
    Left = 0,
    Up = 1,
    Right = 2,
    Down = 3,
    Shield = 4,
};

constexpr Operation AllOperations[] = {Operation::Left, Operation::Up, Operation::Right, Operation::Down, Operation::Shield};
constexpr int AllOperationCount = sizeof(AllOperations) / sizeof(Operation);

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

class NoTimeRemainException : public std::exception {};

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
            if (h < 0 || h >= Height || w < 0 || w >= Width) {
                continue;
            }
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
                    if (type_idx >= 1 && type_idx <= 20) {
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
                if (h < 0)
                    h = 0;
                if (h >= Height)
                    h = Height - 1;
                if (w < 0)
                    w = 0;
                if (w >= Width)
                    w = Width - 1;
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
        if (TimeRemain < 0) {
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

    void PrintMapNearby(Point point, int radius) const {
        for (int h = point.h - radius; h <= point.h + radius; h++) {
            for (int w = point.w - radius; w <= point.w + radius; w++) {
                if (h == point.h && w == point.w) {
                    std::cerr << "[";
                } else {
                    std::cerr << " ";
                }
                if (h < 0 || h >= Height || w < 0 || w >= Width) {
                    std::cerr << "X";
                } else {
                    if (Map[h][w].SnakeIdx == EmptyIdx) {
                        switch (Map[h][w].Obj) {
                            case None:
                                std::cerr << ".";
                                break;
                            case Length:
                                std::cerr << "L";
                                break;
                            case Trap:
                                std::cerr << "T";
                                break;
                            case Wall:
                                std::cerr << "W";
                                break;
                            default:
                                std::cerr << Map[h][w].Obj - ScoreZero;
                                break;
                        }
                    } else if (Map[h][w].SnakeIdx == SelfIdx) {
                        std::cerr << "S";
                    } else {
                        std::cerr << "O";
                    }
                }
                if (h == point.h && w == point.w) {
                    std::cerr << "]";
                } else {
                    std::cerr << " ";
                }
            }
            std::cerr << std::endl;
        }
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
        Operation direction = Invalid;
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

    void ImagineOperations(std::vector<SnakeIdxAndOperation> operations) {
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

Field<double> CreateDangerField(Game& game) {
    // Danger Field
    Field<double> DangerField(VeryLargeValue);
    std::vector<std::pair<Point, double>> dijkstra_source;
    bool i_have_shield = game.SnakeInfos[game.SelfIdx].ShieldET > 0;
    for (int h = 0; h < Height; h++) {
        for (int w = 0; w < Width; w++) {
            switch (game.Map[h][w].Obj) {
                case Trap:
                    dijkstra_source.push_back({
                        {h, w},
                        ValueOfTrap,
                    });
                    break;

                case Wall:
                    dijkstra_source.push_back({
                        {h, w},
                        ValueOfDeathPerRemainTime * game.TimeRemain,
                    });
                    break;

                default:
                    if (game.Map[h][w].SnakeIdx != EmptyIdx) {
                        SnakeInfo& snake = game.SnakeInfos[game.Map[h][w].SnakeIdx];
                        if (snake.Idx == game.SelfIdx) {
                            continue;
                        }
                        if (i_have_shield) {
                            dijkstra_source.push_back({
                                {h, w},
                                ValueOfOpponentWhenHaveShield,
                            });
                        } else {
                            dijkstra_source.push_back({
                                {h, w},
                                ValueOfDeathPerRemainTime * game.TimeRemain,
                            });
                        }
                    }
                    break;
            }
        }
    }
    DangerField.DijkstrativeReduce(dijkstra_source, [&](Point p, auto updater) {
        int h = p.h;
        int w = p.w;
        for (Operation direction : {Operation::Left, Operation::Up, Operation::Right, Operation::Down}) {
            const int h_next = h + DhOfOperation(direction);
            const int w_next = w + DwOfOperation(direction);
            if (h_next < 0 || h_next >= Height || w_next < 0 || w_next >= Width) {
                continue;
            }
            const int h_next_left = h_next + DhOfOperation(Operation::Left);
            const int w_next_left = w_next + DwOfOperation(Operation::Left);
            const int h_next_right = h_next + DhOfOperation(Operation::Right);
            const int w_next_right = w_next + DwOfOperation(Operation::Right);
            const int h_next_up = h_next + DhOfOperation(Operation::Up);
            const int w_next_up = w_next + DwOfOperation(Operation::Up);
            const int h_next_down = h_next + DhOfOperation(Operation::Down);
            const int w_next_down = w_next + DwOfOperation(Operation::Down);
            const double danger_left = (h_next_left < 0 || h_next_left >= Height || w_next_left < 0 || w_next_left >= Width)
                                           ? ValueOfDeathPerRemainTime * game.TimeRemain
                                           : DangerField[h_next_left][w_next_left];
            const double danger_right = (h_next_right < 0 || h_next_right >= Height || w_next_right < 0 || w_next_right >= Width)
                                            ? ValueOfDeathPerRemainTime * game.TimeRemain
                                            : DangerField[h_next_right][w_next_right];
            const double danger_up = (h_next_up < 0 || h_next_up >= Height || w_next_up < 0 || w_next_up >= Width)
                                         ? ValueOfDeathPerRemainTime * game.TimeRemain
                                         : DangerField[h_next_up][w_next_up];
            const double danger_down = (h_next_down < 0 || h_next_down >= Height || w_next_down < 0 || w_next_down >= Width)
                                           ? ValueOfDeathPerRemainTime * game.TimeRemain
                                           : DangerField[h_next_down][w_next_down];
            const double danger = std::min({
                std::max({danger_left, danger_right, danger_up}),
                std::max({danger_left, danger_right, danger_down}),
                std::max({danger_left, danger_up, danger_down}),
                std::max({danger_right, danger_up, danger_down}),
            });
            if (danger < DangerField[h_next][w_next]) {
                updater({h_next, w_next}, danger);
            }
        }
    });
    return DangerField;
}

Field<int> CreateDistanceField(Game& game, Point point) {
    Field<int> DistanceField(-1);
    DistanceField.DijkstrativeReduce(
        {{point, 0}}, [&](Point p, auto updater) {
            int h = p.h;
            int w = p.w;
            for (Operation direction : {Operation::Left, Operation::Up, Operation::Right, Operation::Down}) {
                int h_next = h + DhOfOperation(direction);
                int w_next = w + DwOfOperation(direction);
                if (h_next < 0 || h_next >= Height || w_next < 0 || w_next >= Width) {
                    continue;
                }
                if (game.Map[h_next][w_next].Obj == Wall ||
                    game.Map[h_next][w_next].Obj == Trap) {
                    continue;
                }
                if (game.Map[h_next][w_next].SnakeIdx != EmptyIdx && game.Map[h_next][w_next].SnakeIdx != game.SelfIdx) {
                    continue;
                }
                if (DistanceField[h_next][w_next] != -1) {
                    continue;
                }
                updater({h_next, w_next}, DistanceField[h][w] + 1);
            }
        });
    return DistanceField;
}

Field<double> CreateSpreadableField(Game& game, Point point, double spreadable_value, double spread_decline) {
    return CreateDistanceField(game, point).Map([spreadable_value, spread_decline](int distance) {
        if (distance == -1) {
            return 0.0;
        }
        return spreadable_value * std::pow(spread_decline, distance);
    });
}

Field<double> CreateObjectValueField(Game& game, const Field<double>& DangerField) {
    const int my_h = game.SnakeInfos[game.SelfIdx].Body.front().h;
    const int my_w = game.SnakeInfos[game.SelfIdx].Body.front().w;
    std::vector<Field<double>> RawObjectFields;
    for (int h = 0; h < Height; h++) {
        for (int w = 0; w < Width; w++) {
            double spreadable_value = 0;
            Cell cell = game.Map[h][w];
            if (DangerField[h][w] < 0)
                continue;
            if (cell.Obj > ScoreZero && cell.Obj < ScoreTooLarge) {
                spreadable_value = BaseValueOfScore + ValuePerScore * (cell.Obj - ScoreZero);
            }
            if (cell.Obj == Length) {
                spreadable_value = ValueOfLengthAtBegin * (double)game.TimeRemain / TotalTime + ValueOfLengthAtEnd * (1 - (double)game.TimeRemain / TotalTime);
            }
            if (spreadable_value != 0) {
                RawObjectFields.push_back(CreateSpreadableField(game, {.h = h, .w = w}, spreadable_value, ValueSpreadDecline));
            }
        }
    }

    Field<double> SumField(0);
    for (auto& Field : RawObjectFields) {
        SumField = SumField + Field;
    }
    Field<double> StandardlizedSumField = SumField.Standardlize(1.0);

    Field<double> ObjectValueField(0);
    for (auto& Field : RawObjectFields) {
        double field_weight = 1.0;
        double field_value_at_my_pos = Field[my_h][my_w];
        field_weight *= field_value_at_my_pos / (SumField[my_h][my_w] / RawObjectFields.size());
        field_weight *= StandardlizedSumField[my_h][my_w];
        for (SnakeInfo& snake : game.SnakeInfos) {
            if (!snake.Alive || snake.Idx == game.SelfIdx) {
                continue;
            }
            double field_value_at_opponent_pos = Field[snake.Body.front().h][snake.Body.front().w];
            if (field_value_at_opponent_pos >= field_value_at_my_pos) {
                field_weight *= BaseDeclineOfCompetitivity * (field_value_at_my_pos / field_value_at_opponent_pos);
            }
        }
        ObjectValueField = ObjectValueField.MaxWith(Field * field_weight);
    }
    ObjectValueField = ObjectValueField * CorrectionForSpreadableFields;
    for (int h = 0; h < Height; h++) {
        for (int w = 0; w < Width; w++) {
            Cell cell = game.Map[h][w];
            if (cell.Obj == Trap) {
                ObjectValueField[h][w] += ValueOfTrap;
            }
        }
    }
    return ObjectValueField;
}

Field<double> CreateCenterValueField(Game& game) {
    Field<double> CenterValueField;
    Field<int> DistanceField = CreateDistanceField(game, {.h = CenterH, .w = CenterW});
    const int radius_of_center = 5;
    for (int h = 0; h < Height; h++) {
        for (int w = 0; w < Width; w++) {
            int radius = DistanceField[h][w];
            double value = radius <= radius_of_center ? (ValueOfCenter + ValueOnlyInCenter) : ValueOfCenter * ((double)radius - RadiusOfMap) / (radius_of_center - RadiusOfMap);
            CenterValueField[h][w] = value;
        }
    }
    return CenterValueField;
}

Field<double> CreateValueField(Game& game) {
    const int h = game.SnakeInfos[game.SelfIdx].Body.front().h;
    const int w = game.SnakeInfos[game.SelfIdx].Body.front().w;
    const int longer = std::max(std::abs(h - CenterH), std::abs(w - CenterW));
    const int shorter = std::min(std::abs(h - CenterH), std::abs(w - CenterW));
    bool in_center = (longer <= 5 && shorter <= 2) || (longer <= 4 && shorter <= 4);
    const bool center_lock = (TotalTime - game.TimeRemain) >= TickCenterLockBegin;
    if (center_lock) {
        if (in_center) {
            game.Map[CenterH - 5][CenterW].Obj = Wall;
            game.Map[CenterH + 5][CenterW].Obj = Wall;
            game.Map[CenterH][CenterW - 5].Obj = Wall;
            game.Map[CenterH][CenterW + 5].Obj = Wall;
        }
    }

    Field<double> DangerField = CreateDangerField(game);
    Field<double> ObjectValueField = CreateObjectValueField(game, DangerField);
    Field<double> CenterValueField = (!in_center && (TotalTime - game.TimeRemain) >= TickCenterValueBegin) ? CreateCenterValueField(game) : Field<double>(0);
    Field<double> ValueField = (ObjectValueField + CenterValueField).MinWith(DangerField);

    if (center_lock) {
        if (in_center) {
            game.Map[CenterH - 5][CenterW].Obj = None;
            game.Map[CenterH + 5][CenterW].Obj = None;
            game.Map[CenterH][CenterW - 5].Obj = None;
            game.Map[CenterH][CenterW + 5].Obj = None;
        }
    }

    std::cerr.precision(2);
    std::cerr << "Danger Field:" << std::endl;
    DangerField.PrintValuesNearby(game.SnakeInfos[game.SelfIdx].Body.front(), 3);
    std::cerr << "Object Value Field:" << std::endl;
    ObjectValueField.PrintValuesNearby(game.SnakeInfos[game.SelfIdx].Body.front(), 3);
    std::cerr << "Center Value Field:" << std::endl;
    CenterValueField.PrintValuesNearby(game.SnakeInfos[game.SelfIdx].Body.front(), 3);
    std::cerr << "Value Field:" << std::endl;
    ValueField.PrintValuesNearby(game.SnakeInfos[game.SelfIdx].Body.front(), 3);
    std::cerr << "Map:" << std::endl;
    game.PrintMapNearby(game.SnakeInfos[game.SelfIdx].Body.front(), 10);

    return ValueField;
}

//
//  DFS Search
//

double UtilityOfMyMove(Game& game,
                       Operation operation,
                       const Field<double>& ValueField,
                       int depth,
                       std::chrono::system_clock::time_point should_finish_before) {
    if (std::chrono::high_resolution_clock::now() > should_finish_before) {
        throw NoTimeRemainException();
    }

    const int snake_cnt = game.SnakeInfos.size();
    const int my_h = game.SnakeInfos[game.SelfIdx].Body.front().h;
    const int my_w = game.SnakeInfos[game.SelfIdx].Body.front().w;

    // find all snakes in gambling radius
    const int GamblingRadius = 2 * depth;
    std::vector<int> gambling_snake_idxs;
    for (SnakeInfo& snake : game.SnakeInfos) {
        if (!snake.Alive || snake.Idx == game.SelfIdx) {
            continue;
        }
        const int distance = std::abs(snake.Body.front().h - my_h) + std::abs(snake.Body.front().w - my_w);
        if (distance <= GamblingRadius) {
            gambling_snake_idxs.push_back(snake.Idx);
        }
    }
    const int gambling_snake_cnt = gambling_snake_idxs.size();

    // enumerate all possible operations for gambling snakes, and use default operation for others
    std::list<std::vector<Operation>> operation_collections;
    if (gambling_snake_cnt == 0) {
        // no need to enumerate
        operation_collections.push_back(std::vector<Operation>(snake_cnt, Invalid));
    } else {
        std::vector<int> enumerate_stack(gambling_snake_cnt, 0);
        while (true) {
            // check if valid
            bool is_valid = true;
            for (int i = 0; i < gambling_snake_cnt; i++) {
                if (game.SnakeInfos[gambling_snake_idxs[i]].LastOperation == Reverse(AllOperations[enumerate_stack[i]])) {
                    is_valid = false;
                    break;
                }
            }

            // collect
            if (is_valid) {
                operation_collections.push_back(std::vector<Operation>(snake_cnt, Invalid));
                std::vector<Operation>& operations = operation_collections.back();
                for (int i = 0; i < gambling_snake_cnt; i++) {
                    operations[gambling_snake_idxs[i]] = AllOperations[enumerate_stack[i]];
                }
            }

            // next
            int i = gambling_snake_cnt - 1;
            while (i >= 0 && enumerate_stack[i] == AllOperationCount - 1) {
                enumerate_stack[i] = 0;
                i--;
            }
            if (i < 0) {
                break;
            }
            enumerate_stack[i]++;
        }
    }
    for (auto& item : operation_collections) {
        for (int snake_idx = 0; snake_idx < snake_cnt; snake_idx++) {
            if (snake_idx == game.SelfIdx) {
                item[snake_idx] = operation;
            } else {
                if (item[snake_idx] == Invalid)
                    item[snake_idx] = game.SnakeInfos[snake_idx].LastOperation;
            }
        }
    }

    // simulate
    std::valarray<double> utility_for_each_case(operation_collections.size());
    int case_idx = 0;
    for (std::vector<Operation>& operations : operation_collections) {
        std::vector<SnakeIdxAndOperation> snake_operations;
        for (int snake_idx = 0; snake_idx < snake_cnt; snake_idx++) {
            snake_operations.push_back({.Idx = snake_idx, .Op = operations[snake_idx]});
        }
        const int score_before = game.SnakeInfos[game.SelfIdx].Score;
        game.ImagineOperations(snake_operations);
        Field<double> ValueFieldWithNewDangerField = ValueField.Clone();
        Field<double> NewDangerField = CreateDangerField(game);
        ValueFieldWithNewDangerField = ValueFieldWithNewDangerField.MinWith(NewDangerField);
        int gambling_shield_count_before = 0;
        for (int snake_idx : gambling_snake_idxs) {
            if (game.SnakeInfos[snake_idx].ShieldET > 1 && game.SnakeInfos[snake_idx].Name != 2023202303) {
                gambling_shield_count_before++;
            }
        }

        // evaluate
        double utility = 0;
        SnakeInfo& self = game.SnakeInfos[game.SelfIdx];
        utility += UtilityPerScore * (self.Score - score_before);
        if (operation == Shield) {
            utility += UtilityOfShield;
        }
        if (!self.Alive) {
            utility += UtilityPerValue * ValueOfDeathPerRemainTime * game.TimeRemain;
        } else {
            utility += UtilityPerValue * ValueFieldWithNewDangerField[my_h][my_w];

            // shield
            int gambling_shield_count_after = 0;
            for (int snake_idx : gambling_snake_idxs) {
                if (game.SnakeInfos[snake_idx].ShieldET > 0 && game.SnakeInfos[snake_idx].Name != 2023202303) {
                    gambling_shield_count_after++;
                }
            }
            utility += UtilityOfShield * (gambling_shield_count_after - gambling_shield_count_before);

            // kill
            for (int idx : gambling_snake_idxs) {
                if (!game.SnakeInfos[idx].Alive && game.SnakeInfos[idx].Name != 2023202303) {
                    utility += UtilityOfOpponentDeath;
                }
            }

            // dfs
            if (depth > 0) {
                std::valarray<double> utilities(AllOperationCount);
                for (int i = 0; i < AllOperationCount; i++) {
                    if (game.CanOperate(game.SelfIdx, AllOperations[i]))
                        utilities[i] = UtilityOfMyMove(game, AllOperations[i], ValueFieldWithNewDangerField, depth - 1, should_finish_before);
                    else
                        utilities[i] = VerySmallValue;
                }
                utility += DeclinePerDepth * utilities.max();
            }
        }

        utility_for_each_case[case_idx] = utility;
        game.RevokeOperations();
        case_idx++;
    }
    return utility_for_each_case.min();
}

//
//  Main Function
//

int main() {
    auto start_time = std::chrono::high_resolution_clock::now();
    auto should_finish_before = start_time + std::chrono::milliseconds(ExecutionMillisecondLimit);
    std::ios::sync_with_stdio(false);

    Game game = Game();

    Field<double> ValueField = CreateValueField(game);
    std::vector<std::vector<Operation>> best_operations_by_depth;
    int depth = 0;
    try {
        while (depth <= game.TimeRemain) {
            best_operations_by_depth.push_back(std::vector<Operation>());
            double best_utility = VerySmallValue;
            for (Operation operation : AllOperations) {
                if (!game.CanOperate(game.SelfIdx, operation)) {
                    continue;
                }
                double utility = UtilityOfMyMove(game, operation, ValueField, depth, should_finish_before);
                if (utility > best_utility) {
                    best_utility = utility;
                    best_operations_by_depth[depth].clear();
                    best_operations_by_depth[depth].push_back(operation);
                } else if (utility == best_utility) {
                    best_operations_by_depth[depth].push_back(operation);
                }
            }
            depth++;
        }
    } catch (NoTimeRemainException& e) {
        best_operations_by_depth.pop_back();
    }

    std::vector<Operation> best_operations = best_operations_by_depth.size() > 0 ? best_operations_by_depth.back() : std::vector<Operation>();
    Operation best_operation = Shield;
    if (best_operations.empty()) {
        best_operation = Shield;
    } else if (best_operations.size() == 1) {
        best_operation = best_operations[0];
    } else {
        double best_value = VerySmallValue;
        for (Operation operation : best_operations) {
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
            double value = ValueField[head_h_next][head_w_next];
            if (value >= best_value) {
                best_value = value;
                best_operation = operation;
            }
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::cout << best_operation << " "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << "ms"
              << ", " << depth << " depth"
              << std::endl;
}
