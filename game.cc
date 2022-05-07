#include <algorithm>
#include <cmath>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;

/*****************************************************************************
 * Constants
 ****************************************************************************/
const double pi = std::acos(-1);
const int k15Degree = 15;
const int k30Degree = 30;
const int k45Degree = 45;
const int k60Degree = 60;
const int k75Degree = 75;

const int kWidth = 17630;
const int kHeight = 9000;
const int kRadiusOfBase = 5000;
const int kInnerCircle = 2800;
const int kMidCircle = 6000; // at the outskirt of the base
const int kOutterCircle = 7000;
const int kMonsterSpeed = 400;
const int kHeroSpeed = 800;
const int kRadiusOfWind = 1280;
const int kHerosPerPlayer = 3;
const int kHeroPhysicAttackRange = 800;
const int kHeroViewRange = 2200;
const int kBaseViewRange = 6000;
const int kMagicManaCost = 10;
const int kHeroPhysicAttackDmg = 2;
const int kNumberOfDefenders = 2;
const int kVeryBigDistance = 40000;

/*****************************************************************************
 * Forward declarations
 ****************************************************************************/
struct Point;
struct RadianPoint;
struct Base;
class Entity;
class Monster;
class Hero;
class Brain;
struct Action;
class NaiveOptimiser;

vector<Monster> discover_in_range(const vector<Monster> & monsters, Point pos, int range);
vector<int> discover_in_range(const vector<Hero> & heros, Point pos, int range);
double convert_degree_to_radian(int degree);
int convert_radian_to_degree(double theta);
Point convert_radian_to_cartesian(const RadianPoint & rp);
int calc_degree_between(const Point & ref, const Point & other);
bool is_bottom_lane(const Point & ref, const Point & other);
bool is_mid_lane(const Point & ref, const Point & other);
bool is_top_lane(const Point & ref, const Point & other);
bool is_lower_area(const Point & ref, const Point & other);
bool is_upper_area(const Point & ref, const Point & other);
Point compute_cartesian_point(const Base & base, int r, int angle);
int other_defencer(int idx);
vector<Point> find_the_centers(const Point & p, const Point q, int r);
void cruise_between_angles(Hero & hero, const Base & ref, int radius, int low, int high);

/*****************************************************************************
 * Types
 ****************************************************************************/
struct Point {
    int x;
    int y;

    Point() : x(0), y(0) {}
    Point(int m, int n) : x(m), y(n) {}

    bool valid() const {
        if (x >= 0 && y >= 0 && x <= kWidth && y <= kHeight) {
            return true;
        }
        return false;
    }

    Point & operator+=(const Point & other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    Point & operator-=(const Point & other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    void display(std::ostream & os) const {
        os << "(" << x << "," << y << ")";
    }
};

float distance(const Point & a, const Point & b) {
    return std::hypot(a.x - b.x, a.y - b.y);
}

Point operator+(const Point & p1, const Point & p2) {
    return Point(p1.x + p2.x, p1.y + p2.y);
}

Point operator-(const Point & p1, const Point & p2) {
    return Point(p1.x - p2.x, p1.y - p2.y);
}

std::ostream & operator<<(std::ostream & os, const Point & p) {
    p.display(os);
    return os;
}

Point operator/(const Point & p, int div) {
    if (div == 0) {
        cerr << "Divide by zero exception: " << p << "/" << div << endl;
        return p;
    }
    return Point(p.x / div, p.y / div);
}

// Given two different points P=(x1,x2) and Q=(y1,y2) and a real number r,
// we want to compute the center of circle that pass through both points with radius r.
vector<Point> find_the_centers(const Point & p, const Point q, int r) {
    float dist = distance(p, q);
    if (dist == 0) return { p };

    if (dist > 2 * r) return {};

    int half = dist / 2;
    Point mid = (p + q) / 2;
    if (half == r) return { mid };

    // normalized direction
    Point dir = Point(p.y - q.y, q.x - p.x) / (int)dist;

    double lambda = std::sqrt(r * r - half * half);
    double delta_x = dir.x * lambda;
    double delta_y = dir.y * lambda;
    Point delta(delta_x, delta_y);

    return { mid + delta, mid - delta };
}

class NaiveOptimiser {
public:
    // find the maximum points enclosed in a circle of r
    static vector<pair<Point, int>> solve(const vector<Point> & points, int r) {
        if (points.empty()) return {};

        vector<pair<Point, int>> ans;
        int n = points.size();

        // debug
        cerr << "Optimizer number of input points: " << n << endl;
        for (const auto & p : points) {
            cerr << "point = " << p << endl;
        }

        vector<vector<bool>> visited(n, vector<bool>(n, false));
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (i == j || visited[i][j]) continue;

                visited[i][j] = true;
                visited[j][i] = true;
                auto centers = find_the_centers(points[i], points[j], r);
                for (const auto & c : centers) {
                    int cnt = 0;

                    for (const auto & p : points) {
                        int dist = distance(c, p);
                        if (dist <= r) ++cnt;
                    }
                    ans.push_back({c, cnt});
                }
            }
        }
        sort(ans.begin(), ans.end(), [] (const auto & p1, const auto & p2) {
            return p1.second > p2.second;
        });
        return ans;
    }

private:

};

enum Command {
    WAIT,
    MOVE,
    WIND,
    PROTECT,
    CONTROL,
};

struct Action {
    int subject; // index of the subject
    Command verb;
    int object; // id of the object
    Point dest; // destination
    string msg;

    Action() : subject(0), verb(WAIT), object(0), dest(), msg() {}

    void display(std::ostream & os) const {
        string verbName;
        switch (verb) {
            case WAIT:
                verbName = "WAIT";
                break;

            case MOVE:
                verbName = "MOVE";
                break;

            case PROTECT:
                verbName = "PROTECT";
                break;

            case CONTROL:
                verbName = "CONTROL";
                break;

            case WIND:
                verbName = "WIND";
                break;

            default:
                verbName = "???";
                break;
        }
        cerr << "Action: subject(idx)=" << subject;
        cerr << "; verb=" << verbName;
        cerr << "; object(id)=" << object;
        cerr << "; dest=" << dest;
        cerr << "; msg=" << msg;
    }
};

std::ostream & operator<<(std::ostream & os, const Action & a) {
    a.display(os);
    return os;
}

struct RadianPoint {
    Point orig;
    int radius;
    int angle;

    RadianPoint(Point o, int r, int deg) : orig(o), radius(r), angle(deg) {}

    void display(std::ostream & os) const {
        os << "orig=" << orig << "; r=" << radius << " ; deg=" << angle;
    }
};

std::ostream & operator<<(std::ostream & os, const RadianPoint & rp) {
    rp.display(os);
    return os;
}

double convert_degree_to_radian(int degree) {
    return pi * degree / 180;
}

int convert_radian_to_degree(double radian) {
    return 180 * radian / pi;
}

bool is_bottom_lane(const Point & ref, const Point & other) {
    int degree = calc_degree_between(ref, other);
    if (degree < k30Degree) return true;
    return false;
}

bool is_mid_lane(const Point & ref, const Point & other) {
    int degree = calc_degree_between(ref, other);
    if (degree > k30Degree && degree < k60Degree) return true;
    return false;
}

bool is_top_lane(const Point & ref, const Point & other) {
    int degree = calc_degree_between(ref, other);
    if (degree >= k60Degree) return true;
    return false;
}

bool is_upper_area(const Point & ref, const Point & other) {
    return !is_lower_area(ref, other);
}

bool is_lower_area(const Point & ref, const Point & other) {
    int degree = calc_degree_between(ref, other);
    if (degree <= k45Degree) return true;
    return false;
}

int calc_degree_between(const Point & ref, const Point & other) {
    int delta_x = other.x - ref.x;
    int delta_y = other.y - ref.y;
    if (delta_x == 0) {
        return delta_y >= 0 ? 90 : 0;
    }
    double theta = std::atan((double) delta_y / delta_x);
    return convert_radian_to_degree(theta);
}

Point convert_radian_to_cartesian(const RadianPoint & rp) {
    double theta = convert_degree_to_radian(rp.angle);
    double delta_x = rp.radius * std::cos(theta);
    double detta_y = rp.radius * std::sin(theta);
    Point p = rp.orig + Point(delta_x, detta_y);
    return p;
}

struct Base {
    int hp;
    int mp;
    Point pos;

    Base() : hp(0), mp(0), pos(0, 0) {}

    void update(int health, int mana) {
        hp = health;
        mp = mana;
    }

    void display(std::ostream & os) const {
        os << "hp=" << hp;
        os << "; mp=" << mp;
        os << "; pos=" << pos;
    }
};

std::ostream & operator<<(std::ostream & os, const Base & b) {
    b.display(os);
    return os;
}

class Entity {
public:
    int id; // Unique identifier
    int type; // 0=monster, 1=your hero, 2=opponent hero
    Point pos;
    int shield; // Count down until shield spell fades
    bool mad; // Equals 1 when this entity is under a control spell
    int hp; // Remaining health of this monster
    Point v;
    int target; // 0=monster with no target yet, 1=monster targeting a base
    int threat; // Given this monster's trajectory, is it a threat to 1=your base, 2=your opponent's base, 0=neither
};

class Monster : public Entity {
public:
    Monster(Entity e): Entity(e) {}

    void display(std::ostream & os) const {
        os << "Monster " << id << ": ";
        os << "hp=" << hp;
        os << "; pos=" << pos;
        os << "; v=" << v;
    }

    // How many turns to reach to the destination point
    int eta(const Base & base) const {
        auto dest = base.pos;
        auto curr = pos;
        int ans = 0;
        while (curr.valid()) {
            float dist = distance(dest, curr);
            if (dist <= kRadiusOfBase) {
                ans += dist / kMonsterSpeed;
                break;
            } else {
                ++ans;
                curr += v;
            }
        }
        return curr.valid() ? ans : -1;
    }

private:
};

std::ostream & operator<<(std::ostream & os, const Monster & m) {
    m.display(os);
    return os;
}

// find the monsters in the range
vector<Monster> discover_in_range(const vector<Monster> & monsters, Point pos, int range) {
    vector<Monster> ans;
    for (const auto & m : monsters) {
        if (distance(m.pos, pos) <= range) {
            ans.push_back(m);
        }
    }
    return ans;
}

int find_max_hp(const vector<Monster> & monsters) {
    int ans = 0;
    for (const auto & m : monsters) {
        ans = std::max(ans, m.hp);
    }
    return ans;
}

int other_defencer(int idx) {
    if (idx < 0 || idx >= kNumberOfDefenders) {
        cerr << "Wrong Hero index: " << idx << endl;
        return 0;
    }

    return kNumberOfDefenders - 1 - idx;
}

// eval the risk of this monster based on its eta to the base
int eval_risk(const Base & ref, const Monster & m) {
    int ans = 0;
    int eta = m.eta(ref);

    if (m.target != 0) {
        // this unit is directing to our base
        if (m.threat == 1) {
            ans = 100 - eta;
        } else {
            // do nothing
        }
    } else if (eta > 0) {
        // this unit can eventually reach to the base
        ans = 70 - eta;
    }
    return ans > 0 ? ans : 0;
}

Point compute_cartesian_point(const Base & base, int r, int angle) {
    bool mirrow = base.pos.x == 0 ? false : true;

    if (mirrow) angle += 180;
    RadianPoint rp(base.pos, r, angle);
    return convert_radian_to_cartesian(rp);
}

class Hero : public Entity {
public:
    Hero(Entity e): Entity(e), m_cmd(""), m_spellingWind(false)
    {
    }

    void move(const Point & p) {
        undo();
        m_cmd << "MOVE " << p.x << " " << p.y;
    }

    void move(const Point & p, int r, int angle, bool mirrow = false) {
        if (mirrow) angle += 180;

        RadianPoint rp(p, r, angle);
        move(convert_radian_to_cartesian(rp));
    }

    void move(const Base & base, int r, int angle) {
        bool mirrow = base.pos.x == 0 ? false : true;
        move(base.pos, r, angle, mirrow);
    }

    void say(string words) {
        m_cmd << " " << words;
    }

    void wait() {
        undo();
        m_cmd << "WAIT Ndorē";
    }

    // enter a dummy action (a placeholder) which can be overriden
    void end() {
        undo();
        m_cmd << "WAIT End";
    }

    void wind(const Point & toward) {
        undo();
        m_spellingWind = true;
        m_cmd << "SPELL WIND " << toward.x << " " << toward.y << " Súrë";
    }

    bool isWinding() const { return m_spellingWind; }

    void protect(int id) {
        undo();
        m_cmd << "SPELL SHIELD " << id << " May force be with you";
    }

    void control(int id, const Point & toward) {
        undo();
        // elvish: this place
        m_cmd << "SPELL CONTROL " << id << " " << toward.x << " " << toward.y << " sinomë";
    }

    // find the monsters in the range
    vector<Monster> discover(const vector<Monster> & monsters) const {
        return discover_in_range(monsters, pos, kHeroViewRange);
    }

    // find the heros in the range
    vector<int> discover(const vector<Hero> & heros) const {
        return discover_in_range(heros, pos, kHeroViewRange);
    }

    vector<Monster> estimateWindAttackVictims(const vector<Monster> & monsters) const {
        auto objects = discover_in_range(monsters, pos, kRadiusOfWind);
        vector<Monster> ans;
        for (const auto & o : objects) {
            if (o.shield == 0) ans.push_back(o);
        }
        return ans;
    }

    bool orderReceived() const { return !m_cmd.str().empty(); }

    void confirmOrder() {
        if (!orderReceived()) {
            wait();
        }
        cout << m_cmd.str() << endl;
    }

    void display(std::ostream & os) const {
        os << "Hero " << id << ": ";
        os << "pos=" << pos;
        os << "; shield=" << shield;
        os << "; mad=" << mad;
    }

private:
    void undo() {
        m_cmd.str("");
        m_spellingWind = false;
    }

    std::stringstream m_cmd;
    bool m_spellingWind;
};

std::ostream& operator<<(std::ostream & os, const Hero & hero) {
    hero.display(os);
    return os;
}

// find Hero (its id) in the range
vector<int> discover_in_range(const vector<Hero> & heros, Point pos, int range) {
    vector<int> ans;
    for (const auto & h : heros) {
        if (distance(h.pos, pos) <= range) {
            ans.push_back(h.id);
        }
    }
    return ans;
}

// produce the index of this hero
int find_nearest_hero(const Monster & monster, const vector<Hero> & heros) {
    int ans;
    int min_dist = kVeryBigDistance;
    for (int i = 0; i < heros.size(); ++i) {
        auto & hero = heros[i];
        int dist = distance(hero.pos, monster.pos);
        if (dist < min_dist) {
            min_dist = dist;
            ans = i;
        }
    }
    return ans;
}

// produce the index of this defender
int find_nearest_defender(const Monster & monster, const vector<Hero> & heros) {
    int ans;
    int min_dist = kVeryBigDistance;
    for (int i = 0; i < kNumberOfDefenders; ++i) {
        auto & hero = heros[i];
        int dist = distance(hero.pos, monster.pos);
        if (dist < min_dist) {
            min_dist = dist;
            ans = i;
        }
    }
    return ans;
}

void cruise_between_angles(Hero & hero, const Base & ref, int radius, int low, int high) {
    static bool goHighPos = false;
    auto deg = calc_degree_between(ref.pos, hero.pos);
    if (deg < low + 1) {
        goHighPos = true;
    } else if (deg > high - 1) {
        goHighPos = false;
    }
    if (goHighPos) {
        hero.move(ref, radius, high);
    } else {
        hero.move(ref, radius, low);
    }
}

class Brain {
public:
    Brain(const Base & ours, const Base & theirs) :
        m_ourBase(ours), m_theirBase(theirs), m_turns(0), m_allIn(false), m_madness(0), m_queue()
    {
        m_phase = StartingGame;
        // the blue team
        m_blue = m_ourBase.pos.x == 0 ? true : false;
        m_startPos = m_blue ? Point(2200, 6800) : Point(kWidth - 2200, kHeight - 6800);
        m_endPos = m_blue ? Point(11130, 6800) : Point(kWidth - 11130, kHeight - 6800);
        //m_attackPos = m_blue ? Point(11130, 6800) : Point(kWidth - 11130, kHeight - 6800);
        m_attackPos = m_blue ? Point(12549, 6800) : Point(kWidth - 12549, kHeight - 6800);

        // three default positions
        Point p = compute_cartesian_point(m_ourBase, kMidCircle, 30);
        m_defaultPos.push_back(p);
        p = compute_cartesian_point(m_ourBase, kMidCircle, 60);
        m_defaultPos.push_back(p);
        p = Point(kWidth / 2, kHeight / 2);
        m_defaultPos.push_back(p);
    }

    void updateOurBase(int hp, int mp) {
        m_ourBase.update(hp, mp);
    }
    void updateTheirBase(int hp, int mp) {
        m_theirBase.update(hp, mp);
    }

    enum Phase {
        StartingGame,
        MiddleGame,
        EndingGame
    };

    void parse(const vector<Entity> & units) {
        vector<Hero> heros;
        vector<Monster> monsters;
        vector<Hero> opponents;

        for (const auto & e : units) {
            // index the world
            m_world[e.id] = e;
            switch (e.type) {
                case 0:
                    monsters.push_back(e);
                    break;

                case 1:
                    heros.push_back(e);
                    break;

                case 2:
                    opponents.push_back(e);
                    break;

                default:
                    throw("unknown type");
            }
        }
        swap(m_heros, heros);
        swap(m_monsters, monsters);
        swap(m_opponents, opponents);
        classification(m_monsters);
    }

    bool is_opponent_all_in() {
        if (m_opponents.size() == kHerosPerPlayer) {
            for (const auto & op : m_opponents) {
                int dist = distance(op.pos, m_ourBase.pos);
                if (dist > kOutterCircle) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    void play() {
        ++m_turns;

        if (!m_allIn) {
            m_allIn = is_opponent_all_in();
        }

        int maxHp = find_max_hp(m_enemies);
        if (maxHp >= 24 && m_phase == MiddleGame) {
            m_phase = EndingGame;
        } else if (maxHp >= 17 && m_ourBase.mp >= 200) {
            m_phase = MiddleGame;
        } else if (m_allIn) {
            m_phase = MiddleGame;
        }

        // planning phase
        //idle();
        strategy_one_attacker();

        // commit phase
        commit_my_commands();
    }

    void commit_my_commands() {
        if (m_queue.size() > 2) {
            cerr << "Warning: more than 2 commands for the defenders." << endl;
        }

        vector<bool> seen = { false, false };
        while (!m_queue.empty()) {
            auto a = m_queue.front();
            m_queue.pop();
            int idx = a.subject;
            if (seen[idx]) {
                cerr << "Warning: discard one command for Hero " << m_heros[idx].id << endl;
                cerr << "Raw " << a << endl;
                continue;
            } else {
                seen[idx] = true;
                cerr << "Debug: " << a << endl;
            }

            auto & hero = m_heros[idx];
            switch (a.verb) {
                case MOVE:
                    hero.move(a.dest);
                    break;

                case WIND:
                    hero.wind(a.dest);
                    break;

                case PROTECT:
                    hero.protect(a.object);
                    break;

                case CONTROL:
                    hero.control(a.object, a.dest);
                    break;

                case WAIT:
                default:
                    hero.wait();
                    break;
            }
            if (!a.msg.empty()) hero.say(a.msg);
        }

        for (auto & h : m_heros) {
            h.confirmOrder();
        }
    }

    // for debug purpose
    void showGameInfo() const {
        showStage();
        showBases();
        showHeros();
        showMonsters();
    }

private:
    void showBases() const {
        cerr << "our base: " << m_ourBase << endl;
        cerr << "their base: " << m_theirBase << endl;
    }

    void showStage() const {
        switch (m_phase) {
            case StartingGame:
                cerr << "=== stage: Starting (" << m_turns << ") ===" << endl;
                break;
            case MiddleGame:
                cerr << "=== stage: Middle (" << m_turns << ") ===" << endl;
                break;
            case EndingGame:
                cerr << "=== stage: EndingGame (" << m_turns << ") ===" << endl;
                break;

            default:
                cerr << "=== stage: Unknown (" << m_turns << ") ===" << endl;
                break;
        }
    }

    void showMonsters() const {
        // highest risk to our base
        cerr << "=== our enemies (" << m_enemies.size() << ") ===" << endl;
        for (int i = 0; i < kHerosPerPlayer && i < m_enemies.size(); ++i) {
            const auto & m = m_enemies[i];
            cerr << m << "; ETA=" << m.eta(m_ourBase) << endl;
        }
        // neutral
        cerr << "=== passengers (" << m_neutral.size() << ") ===" << endl;
        for (int i = 0; i < kHerosPerPlayer && i < m_neutral.size(); ++i) {
            const auto & m = m_neutral[i];
            cerr << m << "; ETA=" << m.eta(m_theirBase) << endl;
        }
        cerr << "=== our allies (" << m_allies.size() << ") ===" << endl;
        // highest risk to their base
        for (int i = 0; i < kHerosPerPlayer && i < m_allies.size(); ++i) {
            const auto & m = m_allies[i];
            cerr << m << "; ETA=" << m.eta(m_theirBase) << endl;
        }
    }

    void showHeros() const {
        cerr << "=== our heros (" << m_heros.size() << ") ===" << endl;
        for (const auto & hero : m_heros) {
            cerr << hero << endl;
        }
        cerr << "=== their heros (" << m_opponents.size() << ") ===" << endl;
        for (const auto & hero : m_opponents) {
            cerr << hero << endl;
        }
    }

    void classification(const vector<Monster> & monsters) {
        vector<Monster> enemies;
        vector<Monster> allies;
        vector<Monster> neutral;
        for (const auto & m : monsters) {
            if (m.eta(m_ourBase) >= 0) {
                // they can reach to our base
                enemies.push_back(m);
            } else if (m.eta(m_theirBase) >= 0) {
                // they are our friends
                allies.push_back(m);
            } else {
                // they can be very useful
                neutral.push_back(m);
            }
        }
        // sort by risk
        sort(enemies.begin(), enemies.end(), [&](const auto & a, const auto & b) {
            return eval_risk(m_ourBase, a) > eval_risk(m_ourBase, b);
        });
        sort(neutral.begin(), neutral.end(), [&](const auto & a, const auto & b) {
            return eval_risk(m_theirBase, a) > eval_risk(m_theirBase, b);
        });
        sort(allies.begin(), allies.end(), [&](const auto & a, const auto & b) {
            return eval_risk(m_theirBase, a) > eval_risk(m_theirBase, b);
        });
        swap(m_enemies, enemies);
        swap(m_neutral, neutral);
        swap(m_allies, allies);
    }

    void idle() {
        //for (int i = 0; i < kHerosPerPlayer; i++) {
        //    m_heros[i].wait();
        //}
        Action a;
        m_queue.push(a);
        a.subject = 1;
        m_queue.push(a);
        m_heros[2].wait();
        //for (int i = 0; i < kHerosPerPlayer; i++) {
        //    m_heros[i].wait();
        //}
    }

    void command_the_attacker() {
        switch (m_phase) {
            case StartingGame: go_hunting(); break;
            case MiddleGame: summon_allies(); break;
            case EndingGame: range_and_protect(); break;
            default: throw("unknown phase.");
        }
    }

    void command_the_attacker_new() {
        static int step = 0;

        // short-cut: against all soccers
        if (m_allIn && step <= 2) {
            if (rush_to_the_position(m_attackPos, true)) {
                step = 3;
            }
            return;
        }

        if (m_phase == StartingGame) {
            go_hunting();
            return;
        }

        if (step == 0) {
            if (rush_to_the_position(m_startPos)) {
                ++step;
            }
            return;
        }

        if (step == 1) {
            if (rush_to_the_position(m_endPos, true)) {
                ++step;
            }
            return;
        }

        if (step == 2) {
            if (wait_and_protect(m_endPos)) {
                ++step;
            }
            return;
        }

        if (m_ourBase.mp < 30) {
            go_hunting();
        } else {
            wait_and_protect(m_attackPos);
        }
    }

    // rush to a given position
    bool rush_to_the_position(const Point & pos, bool summon = false) {
        auto & hero = m_heros[2];
        int dist = distance(hero.pos, pos);
        if (dist < 400) {
            // arrived
            return true;
        }

        // could summon
        if (summon && m_ourBase.mp >= 3 * kMagicManaCost) {
            auto monstersNearBy = hero.discover(m_monsters);
            if (monstersNearBy.size() != 0) {
                for (const auto & m : monstersNearBy) {
                    if (m.eta(m_theirBase) < 0 && m.shield == 0 && (m.hp >= 16 || m_allIn)) {
                        hero.control(m.id, m_theirBase.pos);
                        return false;
                    }
                }
            }
        }

        // elvish: rush
        hero.move(pos);
        hero.say("Alco");
        return false;
    }

    // true if protected one ally
    bool wait_and_protect(const Point & pos) {
        auto & hero = m_heros[2];
        auto monstersNearBy = hero.discover(m_monsters);
        // sort by health and by eta
        sort(monstersNearBy.begin(), monstersNearBy.end(), [&](const auto & a, const auto & b) {
            if (a.hp > b.hp) {
                return true;
            } else if (a.hp == b.hp) {
                return eval_risk(m_theirBase, a) > eval_risk(m_theirBase, b);
            } else {
                return false;
            }
        });
        // have enough mana
        if (m_ourBase.mp >= 3 * kMagicManaCost) {
            int dh = distance(m_theirBase.pos, hero.pos);
            if (dh < kMidCircle) {
                for (const auto & m : monstersNearBy) {
                    if (m.shield) continue;

                    // Use wind to boost the perf
                    int dm = distance(m_theirBase.pos, m.pos);
                    int mh = distance(hero.pos, m.pos);
                    int diff = dm - dh;
                    if (diff > 0 && mh < kRadiusOfWind && (m.hp >= 17 || m_allIn)) {
                        hero.wind(m_theirBase.pos);
                        return true;
                    }
                }
            }
            // protect first
            for (const auto & m : monstersNearBy) {
                if (shouldUseShieldSpell(hero, m)) {
                    hero.protect(m.id);
                    return true;
                }
            }
            // then control
            for (const auto & m : monstersNearBy) {
                if (m.eta(m_theirBase) < 0 && m.shield == 0 && m.hp >= 16) {
                    hero.control(m.id, m_theirBase.pos);
                    return false;
                }
            }
        }
        hero.move(pos);
        // elvish: there
        hero.say("Sanomë");
        return false;
    }

    // the attacker goes hunting
    void go_hunting() {
        auto & hero = m_heros[2];

        auto monstersNearBy = hero.discover(m_monsters);
        sort(monstersNearBy.begin(), monstersNearBy.end(), [&](const auto & a, const auto & b) {
            int da = distance(a.pos, hero.pos);
            int db = distance(b.pos, hero.pos);
            if (da < db) {
                return true;
            } else if (da == db) {
                return eval_risk(m_ourBase, a) > eval_risk(m_ourBase, b);
            } else {
                return false;
            }
        });
        if (monstersNearBy.empty()) {
            cruise_between_angles(hero, m_theirBase, 8500, 30, 60);
            hero.say("Faralë");
        } else {
            //hero.move(monstersNearBy.front().pos);
            //hero.say("Faralë");
            // may optimize the attack
            auto monster = monstersNearBy.front();
            if (monstersNearBy.size() >= 2) {
                vector<Point> points;
                for (const auto & m : monstersNearBy) {
                    points.push_back(m.pos);
                }

                // position and counts
                vector<pair<Point, int>> res = NaiveOptimiser::solve(points, kHeroPhysicAttackRange);
                if (!res.empty()) {
                    sort(res.begin(), res.end(), [&](const auto & p1, const auto & p2) {
                        if (p1.second > p2.second) {
                            return true;
                        } else if (p1.second == p2.second) {
                            return distance(hero.pos, p1.first) < distance(hero.pos, p2.first);
                        } else {
                            return false;
                        }
                    });
                    auto originalTargets = discover_in_range(m_monsters, monster.pos, kHeroPhysicAttackRange);
                    auto plan = res.front();
                    cerr << "Optimizer ON: init plan=" << monster.pos << "; cnt=" << originalTargets.size() << endl;
                    cerr << "Optimizer ON: corrected plan=" << plan.first << "; cnt=" << plan.second << endl;
                    hero.move(plan.first);
                    hero.say("Aragorn");
                }
            }

            if (!hero.orderReceived()) {
                hero.move(monster.pos);
                hero.say("Faralë");
            }
        }
    }

    void summon_allies() {
        auto & hero = m_heros[2];

        auto dist = distance(hero.pos, m_theirBase.pos);
        auto monstersNearBy = hero.discover(m_monsters);
        if (monstersNearBy.empty()) {
            // switch area
            cruise_between_angles(hero, m_theirBase, kOutterCircle, 15, 75);
            hero.say("Attack");
        } else {
            if (m_ourBase.mp >= 4 * kMagicManaCost) {
                // when I'm far from their base
                if (dist >= kOutterCircle) {
                    // the most important thing
                    for (const auto & m : monstersNearBy) {
                        if (m.eta(m_theirBase) < 0 && m.shield == 0 && m.hp >= 18) {
                            hero.control(m.id, m_theirBase.pos);
                            return;
                        }
                    }
                    auto throwables = hero.estimateWindAttackVictims(monstersNearBy);
                    if (shouldUseWindSpell(hero, monstersNearBy) && throwables.size() > 2) {
                        hero.wind(m_theirBase.pos);
                        return;
                    }
                }
                // when I'm near enough
                if (shouldUseWindSpell(hero, monstersNearBy)) {
                    hero.wind(m_theirBase.pos);
                    return;
                }
                // sort from the highest risk to the lowest
                sort(monstersNearBy.begin(), monstersNearBy.end(), [&](const auto & a, const auto & b) {
                    return eval_risk(m_theirBase, a) > eval_risk(m_theirBase, b);
                });
                for (const auto & m : monstersNearBy) {
                    if (shouldUseShieldSpell(hero, m)) {
                        hero.protect(m.id);
                        return;
                    }
                }
                // pull the monster back
                for (const auto & m : monstersNearBy) {
                    if (m.eta(m_theirBase) < 0 && m.shield == 0) {
                        hero.control(m.id, m_theirBase.pos);
                        return;
                    }
                }
            }
            if (!hero.orderReceived()) {
                // switch area
                cruise_between_angles(hero, m_theirBase, kOutterCircle, 15, 75);
                hero.say("Attack");
            }
        }
    }

    void range_and_protect() {
        auto & hero = m_heros[2];

        auto monstersNearBy = hero.discover(m_monsters);
        if (monstersNearBy.empty()) {
            // switch area
            cruise_between_angles(hero, m_theirBase, kMidCircle, 15, 75);
            hero.say("Focus");
        } else {
            if (m_ourBase.mp >= 3 * kMagicManaCost) {
                // sort from the highest risk to the lowest
                sort(monstersNearBy.begin(), monstersNearBy.end(), [&](const auto & a, const auto & b) {
                    return eval_risk(m_theirBase, a) > eval_risk(m_theirBase, b);
                });
                for (const auto & m : monstersNearBy) {
                    if (shouldUseShieldSpell(hero, m)) {
                        hero.protect(m.id);
                        break;
                    }
                }
                if (!hero.orderReceived()) {
                    for (const auto & m : monstersNearBy) {
                        if (m.eta(m_theirBase) < 0 && m.hp >= 20 && m.shield == 0) {
                            hero.control(m.id, m_theirBase.pos);
                            break;
                        }
                    }
                }
                if (!hero.orderReceived() && canUseWindSpell(hero, m_enemies)) {
                    hero.wind(m_theirBase.pos);
                }
            }
            if (!hero.orderReceived()) {
                //hero.move(m_theirBase, kInnerCircle, 45);
                cruise_between_angles(hero, m_theirBase, kMidCircle, 15, 75);
                hero.say("Heru");
            }
        }
    }

    // discover the monsters near by and find the optimized point to attack
    bool optimized_range_attack(int idx, const Monster & monster) {
        auto & hero = m_heros[idx];
        if (hero.orderReceived()) {
            cerr << "Warning: " << hero  << "not available" << endl;
            cerr << "cannot attack " << monster << endl;
            return false;
        }

        auto monstersNearBy = discover_in_range(m_monsters, monster.pos, kHeroViewRange);
        auto originalTargets = discover_in_range(m_monsters, monster.pos, kHeroPhysicAttackRange);
        // may optimize the attack
        if (monstersNearBy.size() >= 2) {
            vector<Point> points;
            for (const auto & m : monstersNearBy) {
                points.push_back(m.pos);
            }

            // position and counts
            vector<pair<Point, int>> res = NaiveOptimiser::solve(points, kHeroPhysicAttackRange);
            if (!res.empty()) {
                sort(res.begin(), res.end(), [&](const auto & p1, const auto & p2) {
                    if (p1.second > p2.second) {
                        return true;
                    } else if (p1.second == p2.second) {
                        return distance(m_ourBase.pos, p1.first) < distance(m_ourBase.pos, p2.first);
                    } else {
                        return false;
                    }
                });
                auto plan = res.front();
                cerr << "Optimizer ON: init plan=" << monster.pos << "; cnt=" << originalTargets.size() << endl;
                cerr << "Optimizer ON: corrected plan=" << plan.first << "; cnt=" << plan.second << endl;
                Action a;
                a.subject = idx;
                a.verb = MOVE;
                a.dest = plan.first;
                a.msg = "Aragorn";
                m_queue.push(a);
                hero.end();
                int dist = distance(a.dest, monster.pos);
                // the init target is enclosed in the circle
                return dist < kHeroPhysicAttackRange;
            }
        }

        if (!hero.orderReceived()) {
            Action a;
            a.subject = idx;
            a.verb = MOVE;
            a.dest = monster.pos;
            a.object = monster.id;
            a.msg = "Focus!";
            m_queue.push(a);
            hero.end();
            return true;
        }
        return false;
    }

    // For defencer only: can update the state of the monster
    void attack_the_monster(int idx, Monster & monster) {
        auto & hero = m_heros[idx];
        if (hero.orderReceived()) {
            cerr << "Warning: " << hero  << "not available" << endl;
            cerr << "cannot attack " << monster << endl;
            return;
        }

        // magic attack first
        if (m_ourBase.mp >= kMagicManaCost) {
            // Wind attack
            if (canUseWindSpell(hero, monster)) {
                // find on the opponents near our base
                auto opponentsNearOurBase =
                    discover_in_range(m_opponents, m_ourBase.pos, kMidCircle);
                // sort by the distance to my base (nearest to farest)
                sort(opponentsNearOurBase.begin(), opponentsNearOurBase.end(), [&](int id1, int id2) {
                    auto pos1 = m_world[id1].pos;
                    auto pos2 = m_world[id2].pos;
                    return distance(pos1, m_ourBase.pos) < distance(pos2, m_ourBase.pos);
                });
                bool shallUseWind = opponentsNearOurBase.size() != 0 &&
                    (distance(m_world[opponentsNearOurBase.front()].pos, monster.pos) <= kHeroViewRange);
                if (shallUseWind || !canEliminateMonster(hero, monster)) {
                    Action a;
                    a.subject = idx;
                    a.verb = WIND;
                    a.dest = m_theirBase.pos;
                    a.object = monster.id;
                    m_queue.push(a);
                    hero.end();
                    m_ourBase.mp -= kMagicManaCost;
                    // no need to handle this monster for a while
                    monster.hp = -1;
                }
            }
        }

        // physic attack
        if (!hero.orderReceived()) {
            //Action a;
            //a.subject = idx;
            //a.verb = MOVE;
            //a.dest = monster.pos;
            //a.object = monster.id;
            //a.msg = "Focus!";
            //m_queue.push(a);
            //hero.end();
            bool attacked = optimized_range_attack(idx, monster);
            if (attacked && canEliminateMonster(hero, monster)) {
                monster.hp = -1;
            }
        }
    }

    // Hero back to the position
    void back_to_the_position(int idx, const Point & pos) {
        auto & hero = m_heros[idx];

        Action a;
        a.subject = idx;
        a.verb = MOVE;
        a.dest = pos;
        // elvish: runsh
        a.msg = "Alco";
        m_queue.push(a);
        hero.end();
    }

    void update_the_default_positions() {
        // find on the opponents near our base
        auto opponentsNearOurBase =
            discover_in_range(m_opponents, m_ourBase.pos, kMidCircle);
        // sort by the distance to my base (nearest to farest)
        sort(opponentsNearOurBase.begin(), opponentsNearOurBase.end(), [&](int id1, int id2) {
            auto pos1 = m_world[id1].pos;
            auto pos2 = m_world[id2].pos;
            return distance(pos1, m_ourBase.pos) < distance(pos2, m_ourBase.pos);
        });
        for (int idx = 0; idx < kNumberOfDefenders && idx < opponentsNearOurBase.size(); ++idx) {
            auto & opponent = m_world[opponentsNearOurBase[idx]];
            int dist = distance(opponent.pos, m_ourBase.pos);
            int radius = kMidCircle;
            int degree = calc_degree_between(m_ourBase.pos, opponent.pos);
            Point p = compute_cartesian_point(m_ourBase, radius, degree);
            m_defaultPos[idx] = p;
            if (opponentsNearOurBase.size() == 1) {
                degree += 30;
                if (degree < 60) {
                    p = compute_cartesian_point(m_ourBase, radius, degree);
                    m_defaultPos[1] = p;
                }
            }
        }
    }

    void self_protections() {
        for (int i = 0; i < kNumberOfDefenders; ++i) {
            if (m_heros[i].mad)
                ++m_madness;
        }
        if (m_queue.size() >= kNumberOfDefenders) return;

        for (int idx = 0; idx < kNumberOfDefenders; ++idx) {
            auto & hero = m_heros[idx];
            if (hero.orderReceived()) continue;

            // protect myself frist
            if (shouldSelfProtect(hero) && m_ourBase.mp >= kMagicManaCost) {
                Action a;
                a.subject = idx;
                a.verb = PROTECT;
                a.object = hero.id;
                m_queue.push(a);
                m_ourBase.mp -= kMagicManaCost;
                hero.end();
                continue;
            }

            int j = other_defencer(idx);
            auto & other = m_heros[j];
            int dist = distance(hero.pos, other.pos);
            // try to protect each other
            if (hero.mad && dist < kHeroViewRange && m_ourBase.mp >= kMagicManaCost) {
                Action a;
                a.subject = idx;
                a.verb = MOVE;
                a.dest = other.pos;
                a.object = other.id;
                // elvish: help
                a.msg = "Sáme";
                m_queue.push(a);
                hero.end();

                if (!other.orderReceived()) {
                    a.subject = j;
                    a.verb = PROTECT;
                    a.object = idx;
                    m_queue.push(a);
                    m_ourBase.mp -= kMagicManaCost;
                    other.end();
                }
            }
        }
    }

    void pull_it_back() {
        if (m_queue.size() >= kNumberOfDefenders) return;
        if (m_ourBase.mp < kMagicManaCost) return;

        vector<Monster> enemiesNearOurBase;
        for (const auto & m : m_enemies) {
            int dist = distance(m.pos, m_ourBase.pos);
            if (dist <= kMidCircle) {
                enemiesNearOurBase.push_back(m);
            }
        }
        if (enemiesNearOurBase.empty()) return;

        auto & monster = enemiesNearOurBase.front();
        // cannot pull it back
        if (monster.shield > 0) return;

        int idx = find_nearest_defender(monster, m_heros);
        auto & hero = m_heros[idx];
        int j = other_defencer(idx);
        auto & other = m_heros[j];
        int eta = monster.eta(m_ourBase);
        if (!hero.orderReceived()) {
            int dist = distance(hero.pos, monster.pos);
            if (dist <= kHeroViewRange) {
                // round up
                int turns = (dist + kHeroSpeed) / kHeroSpeed;
                if (turns >= eta - 1) {
                    Action a;
                    a.subject = idx;
                    a.verb = CONTROL;
                    a.object = monster.id;
                    a.dest = hero.pos;
                    // elvish: this
                    a.msg = "Ike";
                    m_queue.push(a);
                    m_ourBase.mp -= kMagicManaCost;
                    hero.end();
                    return;
                }
            }
        } else {
            int dist = distance(other.pos, monster.pos);
            if (dist <= kHeroViewRange) {
                int turns = (dist + kHeroSpeed) / kHeroSpeed;
                if (turns >= eta - 1) {
                    Action a;
                    a.subject = j;
                    a.verb = CONTROL;
                    a.object = monster.id;
                    a.dest = other.pos;
                    // elvish: this
                    a.msg = "Ike";
                    m_queue.push(a);
                    m_ourBase.mp -= kMagicManaCost;
                    hero.end();
                    return;
                }
            }
        }
    }

    void command_the_defenders_new() {
        update_the_default_positions();
        // step 1
        self_protections();
        // step2
        pull_it_back();
        if (m_queue.size() >= kNumberOfDefenders) return;

        vector<Monster> enemiesNearOurBase;
        for (const auto & m : m_enemies) {
            int dist = distance(m.pos, m_ourBase.pos);
            if (dist <= kMidCircle) {
                enemiesNearOurBase.push_back(m);
            }
        }

        // highest priority (per monster)
        if (enemiesNearOurBase.size() != 0) {
            // must handle the first monster
            auto & monster = enemiesNearOurBase.front();
            int idx = find_nearest_defender(monster, m_heros);
            auto & hero = m_heros[idx];
            attack_the_monster(idx, monster);

            int j = other_defencer(idx);
            auto & other = m_heros[j];
            if (monster.hp > 0) {
                attack_the_monster(j, monster);
                return;
            }

            auto monstersNearBy = other.discover(enemiesNearOurBase);
            // sort by distance (to the hero) and by risk
            sort(monstersNearBy.begin(), monstersNearBy.end(), [&](const auto & a, const auto & b) {
                int da = distance(a.pos, other.pos);
                int db = distance(b.pos, other.pos);
                if (da < db) {
                    return true;
                } else if (da == db) {
                    return eval_risk(m_ourBase, a) > eval_risk(m_ourBase, b);
                } else {
                    return false;
                }
            });

            // attack the nearest monster
            for (auto & m : monstersNearBy) {
                if (m.hp < 0) continue;
                attack_the_monster(j, m);
                return;
            }

            // attack the second enemy
            for (auto & m : enemiesNearOurBase) {
                if (m.hp < 0) continue;
                attack_the_monster(j, m);
                return;
            }
        }
        if (m_queue.size() >= kNumberOfDefenders) return;

        // lower priority: farm the monsters in the wild (per monster)
        vector<Monster> monstersInTheWild;
        for (const auto & m : m_monsters) {
            int dist = distance(m.pos, m_ourBase.pos);
            if (dist > kMidCircle && dist <= kOutterCircle) {
                monstersInTheWild.push_back(m);
            }
        }
        for (int idx = 0; idx < monstersInTheWild.size() && m_queue.size() < kNumberOfDefenders; ++idx) {
            auto & monster = monstersInTheWild[idx];
            if (monster.hp < 0) continue;

            int i = find_nearest_defender(monster, m_heros);
            auto & hero = m_heros[i];
            if (hero.orderReceived()) continue;

            attack_the_monster(i, monster);
        }

        // default operation
        for (int i = 0; i < kNumberOfDefenders; ++i) {
            auto & hero = m_heros[i];
            if (!hero.orderReceived()) back_to_the_position(i, m_defaultPos[i]);
        }
        return;
    }

    void command_the_defenders() {
        // stage1: use shield to protect ourselves
        for (int i = 0; i < kNumberOfDefenders; ++i) {
            if (m_heros[i].mad)
                ++m_madness;
        }
        for (int idx = 0; idx < kNumberOfDefenders; ++idx) {
            auto & hero = m_heros[idx];
            if (shouldSelfProtect(hero) && m_ourBase.mp >= ((idx + 1) * kMagicManaCost)) {
                Action a;
                a.subject = idx;
                a.verb = PROTECT;
                a.object = hero.id;
                m_queue.push(a);
                // dummy thing; placeholder
                hero.wait();
            }
        }
        if (m_queue.size() >= kNumberOfDefenders) return;

        int radiusOfDefence;
        int defaultAngle;
        vector<int> defaultAngles = { 0, 0 };
        switch (m_phase) {
            case StartingGame:
                radiusOfDefence = kOutterCircle;
                defaultAngles[0] = 15;
                defaultAngles[1] = 75;
                break;
            case MiddleGame:
            case EndingGame:
                radiusOfDefence = kMidCircle;
                defaultAngles[0] = 45;
                defaultAngles[1] = 60;
                break;
            default: throw("unknow phase");
        }
        // find on the opponents near our base
        auto opponentsNearOurBase =
            discover_in_range(m_opponents, m_ourBase.pos, kOutterCircle);
        // sort by the distance to my base (nearest to farest)
        sort(opponentsNearOurBase.begin(), opponentsNearOurBase.end(), [&](int id1, int id2) {
            auto pos1 = m_world[id1].pos;
            auto pos2 = m_world[id2].pos;
            return distance(pos1, m_ourBase.pos) < distance(pos2, m_ourBase.pos);
        });
        bool alert = false;
        if (opponentsNearOurBase.size() != 0) {
            alert = true;

            int id = opponentsNearOurBase.front();
            auto & opponent = m_world[id];
            int dist = distance(opponent.pos, m_ourBase.pos);
            radiusOfDefence = std::min(kMidCircle, dist);
            defaultAngles[0] = calc_degree_between(m_ourBase.pos, opponent.pos);
            defaultAngles[1] = defaultAngles[0] + 30;
        }
        if (opponentsNearOurBase.size() > 1) {
            int id = opponentsNearOurBase[1];
            auto & opponent = m_world[id];
            defaultAngles[1] = calc_degree_between(m_ourBase.pos, opponent.pos);
        }

        // per hero
        for (int i = 0; i < kNumberOfDefenders; ++i) {
            auto & hero = m_heros[i];
            auto dist = distance(hero.pos, m_ourBase.pos);
            // stage2: do not go too far
            if (dist > radiusOfDefence + 1500) {
                // back to the default position
                Action a;
                a.subject = i;
                a.verb = MOVE;
                a.dest = compute_cartesian_point(m_ourBase, radiusOfDefence, defaultAngles[i]);
                a.msg = "Back";
                m_queue.push(a);
                // dummy thing; placeholder
                hero.wait();
            }
            if (m_queue.size() >= kNumberOfDefenders) return;

            // stage3: no enemies at all
            if (m_enemies.empty() && opponentsNearOurBase.empty()) {
                auto monstersNearBy = hero.discover(m_monsters);
                // sort from the nearest to the farest
                sort(monstersNearBy.begin(), monstersNearBy.end(), [&](const auto & a, const auto & b) {
                    return distance(a.pos, hero.pos) < distance(b.pos, hero.pos);
                });
                Action a;
                a.subject = i;
                a.verb = MOVE;
                if (monstersNearBy.size() != 0) {
                    // farm
                    a.dest = monstersNearBy.front().pos;
                    a.msg = "Faralë";
                } else {
                    // go to the default position
                    a.dest = compute_cartesian_point(m_ourBase, radiusOfDefence, defaultAngles[i]);
                    a.msg = "Glories";
                }
                m_queue.push(a);
                // dummy thing; placeholder
                hero.wait();
            }
            if (m_queue.size() >= kNumberOfDefenders) return;
        }
        if (m_queue.size() >= kNumberOfDefenders) return;

        // stage4: focus on the enemies in our base
        auto monstersInOurBase = discover_in_range(m_monsters, m_ourBase.pos, kRadiusOfBase);
        // sort by risk (highest to lowest)
        sort(monstersInOurBase.begin(), monstersInOurBase.end(), [&](const auto & a, const auto & b) {
            return eval_risk(m_ourBase, a) > eval_risk(m_ourBase, b);
        });
        // per monster now
        int x = 0;
        while (m_queue.size() < 2 && x < monstersInOurBase.size()) {
            auto & monster = monstersInOurBase[x];
            if (monster.hp < 0) {
                ++x;
                continue;
            }

            int idx = find_nearest_defender(monster, m_heros);
            // this hero is not available
            if (m_heros[idx].orderReceived()) {
                // must handle the first monster
                if (x == 0) {
                    idx = kNumberOfDefenders - 1 - idx;
                } else {
                    ++x;
                    continue;
                }
            }
            auto & hero = m_heros[idx];

            if (canUseWindSpell(hero, monster) && m_ourBase.mp >= ((idx + 1) * kMagicManaCost)) {
                if (opponentsNearOurBase.size() != 0 || !canEliminateMonster(hero, monster)) {
                    Action a;
                    a.subject = idx;
                    a.verb = WIND;
                    a.dest = m_theirBase.pos;
                    m_queue.push(a);
                    // dummy thing; placeholder
                    hero.wait();
                    // no need to handle this monster for a while
                    monster.hp = -1;
                }
            }
            if (!hero.orderReceived()) {
                Action a;
                a.subject = idx;
                a.verb = MOVE;
                a.dest = monster.pos;
                a.msg = "Focus!";
                m_queue.push(a);
                // dummy thing; placeholder
                hero.wait();
                if (canEliminateMonster(hero, monster)) {
                    monster.hp = -1;
                }
            }
        }
        if (m_queue.size() >= kNumberOfDefenders) return;

        // stage5: enemies not far from our base
        for (int idx = 0; idx < kNumberOfDefenders; ++idx) {
            auto & hero = m_heros[idx];
            if (hero.orderReceived())
                continue;

            // no alert => go farm
            if (!alert) {
                auto monstersNearBy = hero.discover(m_monsters);
                // sort by distance to my hero (nearest to farest)
                sort(monstersNearBy.begin(), monstersNearBy.end(), [&](const auto & a, const auto & b) {
                    return distance(a.pos, hero.pos) < distance(b.pos, hero.pos);
                });

                if (monstersNearBy.size() != 0) {
                    Action a;
                    a.subject = idx;
                    a.verb = MOVE;
                    a.dest = monstersNearBy.front().pos;
                    a.msg = "Faralë";
                    m_queue.push(a);
                    // dummy thing; placeholder
                    hero.wait();
                }
            }
            if (hero.orderReceived()) continue;

            // no enemies. back to the position
            Action a;
            a.subject = idx;
            a.verb = MOVE;
            a.dest = compute_cartesian_point(m_ourBase, radiusOfDefence, defaultAngles[idx]);
            a.msg = "Glories";
            m_queue.push(a);
            // dummy thing; placeholder
            hero.wait();
        }
    }

    void strategy_one_attacker() {
        //command_the_defenders();
        command_the_defenders_new();
        command_the_attacker_new();
    }

    bool canEliminateMonster(const Hero & hero, const Monster & monster) {
        if (monster.hp < 0) {
            cerr << "Warning [Negative HP]: " << monster << endl;
            return true;
        }
        auto eta = monster.eta(m_ourBase);
        if (eta < 0) return true;

        if (monster.hp < eta * kHeroPhysicAttackDmg) {
            return true;
        }
        return false;
    }

    bool canUseWindSpell(const Hero & hero, const Monster & monster) const {
        int dist = distance(hero.pos, monster.pos);
        if (dist <= kRadiusOfWind && m_ourBase.mp >= kMagicManaCost && monster.shield == 0) {
            return true;
        }
        return false;
    }

    bool canUseWindSpell(const Hero & hero, const vector<Monster> & monsters) const {
        for (const auto & m : monsters) {
            if (canUseWindSpell(hero, m)) return true;
        }
        return false;
    }

    bool shouldSelfProtect(const Hero & hero) const {
        int maxHp = find_max_hp(m_monsters);
        if (maxHp >= 20 && hero.shield == 0 && m_madness > 1) {
            return true;
        } else {
            return false;
        }
    }

    // for attacker only
    bool shouldUseWindSpell(const Hero & hero, const Monster & monster) const {
        int dh = distance(monster.pos, hero.pos);
        int db = distance(monster.pos, m_theirBase.pos);
        if (  dh <= kRadiusOfWind
           && db <= 7000
           && monster.shield == 0) {
            return true;
        }
        return false;
    }

    bool shouldUseWindSpell(const Hero & hero, const vector<Monster> & monsters) const {
        for (const auto & m : monsters) {
            if (shouldUseWindSpell(hero, m)) return true;
        }
        return false;
    }

    bool shouldUseShieldSpell(const Hero & hero, const Monster & monster) const {
        auto eta = monster.eta(m_theirBase);
        if (  monster.shield == 0
           && eta >= 0
           && eta <= 13) {
            return true;
        }
        return false;
    }

    Base m_ourBase;
    Base m_theirBase;

    int m_turns;
    int m_madness;
    bool m_allIn;
    Phase m_phase;
    bool m_blue;

    Point m_startPos;
    Point m_endPos;
    Point m_attackPos;
    vector<Point> m_defaultPos;

    queue<Action> m_queue;

    unordered_map<int, Entity> m_world;


    vector<Hero> m_heros;
    vector<Monster> m_monsters;
    vector<Hero> m_opponents;
    vector<Monster> m_enemies;
    vector<Monster> m_neutral;
    vector<Monster> m_allies;
};

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/

int main()
{
    int base_x; // The corner of the map representing your base
    int base_y;
    cin >> base_x >> base_y; cin.ignore();
    int heroes_per_player; // Always 3
    cin >> heroes_per_player; cin.ignore();

    Base ourBase, theirBase;
    ourBase.pos = Point(base_x, base_y);
    theirBase.pos = Point(kWidth - base_x, kHeight - base_y);
    Brain brain(ourBase, theirBase);

    // game loop
    while (1) {
        for (int i = 0; i < 2; i++) {
            int health; // Your base health
            int mana; // Spend ten mana to cast a spell
            cin >> health >> mana; cin.ignore();

            if (i == 0) {
                brain.updateOurBase(health, mana);
            } else {
                brain.updateTheirBase(health, mana);
            }
        }
        int entity_count; // Amount of heros and monsters you can see
        cin >> entity_count; cin.ignore();
        vector<Entity> units;
        for (int i = 0; i < entity_count; i++) {
            int id; // Unique identifier
            int type; // 0=monster, 1=your hero, 2=opponent hero
            int x; // Position of this entity
            int y;
            int shield_life; // Count down until shield spell fades
            int is_controlled; // Equals 1 when this entity is under a control spell
            int health; // Remaining health of this monster
            int vx; // Trajectory of this monster
            int vy;
            int near_base; // 0=monster with no target yet, 1=monster targeting a base
            int threat_for; // Given this monster's trajectory, is it a threat to 1=your base, 2=your opponent's base, 0=neither
            cin >> id >> type >> x >> y >> shield_life >> is_controlled >> health >> vx >> vy >> near_base >> threat_for; cin.ignore();

            Entity e;
            e.id = id;
            e.type = type;
            e.pos = Point(x, y);
            e.shield = shield_life;
            e.mad = is_controlled ? true : false;
            e.hp = health;
            e.v = Point(vx, vy);
            e.target = near_base;
            e.threat = threat_for;
            units.push_back(e);
        }
        brain.parse(units);
        brain.showGameInfo();
        brain.play();
    }
}
