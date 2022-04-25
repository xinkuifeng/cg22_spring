#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
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
const int kMidCircle = 4400;
const int kOutterCircle = 6000;
const int kMonsterSpeed = 400;
const int kHeroSpeed = 800;
const int kRadiusOfWind = 1280;
const int kHerosPerPlayer = 3;
const int kHeroPhysicAttackRange = 800;
const int kHeroViewRange = 2200;
const int kMagicManaCost = 10;

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

vector<Monster> discover_in_range(const vector<Monster> & monsters, Point pos, int range);
vector<int> discover_in_range(const vector<Hero> & heros, Point pos, int range);
double convert_degree_to_radian(int degree);
int convert_radian_to_degree(double theta);
Point convert_radian_to_cartesian(const RadianPoint & rp);
int calc_degree_between(const Point & ref, const Point & other);
bool is_bottom_lane(const Point & ref, const Point & other);
bool is_mid_lane(const Point & ref, const Point & other);
bool is_up_lane(const Point & ref, const Point & other);
bool is_lower_area(const Point & ref, const Point & other);
bool is_upper_area(const Point & ref, const Point & other);

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

bool is_up_lane(const Point & ref, const Point & other) {
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
    int controlled; // Equals 1 when this entity is under a control spell
    int hp; // Remaining health of this monster
    int vx; // Trajectory of this monster
    int vy;
    int target; // 0=monster with no target yet, 1=monster targeting a base
    int threat; // Given this monster's trajectory, is it a threat to 1=your base, 2=your opponent's base, 0=neither
};

class Monster : public Entity {
public:
    Monster(Entity e): Entity(e), v(vx, vy) {}

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
    Point v;
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
    // sort from the nearest to the farest
    sort(ans.begin(), ans.end(), [&](const auto & a, const auto & b) {
        return distance(a.pos, pos) < distance(b.pos, pos);
    });
    return ans;
}

int find_max_hp(const vector<Monster> & monsters) {
    int ans = 0;
    for (const auto & m : monsters) {
        ans = std::max(ans, m.hp);
    }
    return ans;
}

bool is_ending_game(const vector<Monster> & monsters) {
    int maxHp = find_max_hp(monsters);
    return maxHp > 24;
}

// risk of this entity based on the distance
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
        m_cmd << "SPELL CONTROL " << id << " " << toward.x << " " << toward.y << " Help";
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

private:
    void undo() {
        m_cmd.str("");
        m_spellingWind = false;
    }

    std::stringstream m_cmd;
    bool m_spellingWind;
};

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

class Brain {
public:
    Brain(const Base & ours, const Base & theirs) :
        m_ourBase(ours), m_theirBase(theirs), m_turns(0)
    {
        //Point delta = Point(565, 565);
        Point delta = Point(1500, 1500);
        m_guardianPos = m_ourBase.pos.x == 0 ?
            m_ourBase.pos + delta : m_ourBase.pos - delta;

        m_midFieldPos = Point(kWidth / 2, kHeight / 2);

        delta = Point(4243, 4243);
        m_attackerPos = m_ourBase.pos.x == 0 ?
            m_theirBase.pos - delta : m_theirBase.pos + delta;

        delta = Point(3100, 3100);
        m_goalAreaPos = m_ourBase.pos.x == 0 ?
            m_theirBase.pos - delta : m_theirBase.pos + delta;
    }

    void updateOurBase(int hp, int mp) {
        m_ourBase.update(hp, mp);
    }
    void updateTheirBase(int hp, int mp) {
        m_theirBase.update(hp, mp);
    }

    void parse(const vector<Entity> & units) {
        vector<Hero> heros;
        vector<Monster> monsters;
        vector<Hero> opponents;

        for (const auto & e : units) {
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

    void play() {
        ++m_turns;

        // planning phase
        //idle();
        //strategy_full_defence();
        strategy_one_attacker();

        // commit phase
        for (auto & h : m_heros) {
            h.confirmOrder();
        }
    }

    // for debug purpose
    void showBases() {
        cerr << "our base: " << m_ourBase << endl;
        cerr << "their base: " << m_theirBase << endl;
    }

    void showMonsters() {
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

private:
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
        //m_heros[0].move(m_ourBase, kOutterCircle, 10);
        //m_heros[1].move(m_ourBase, kMidCircle, 30);
        //m_heros[2].move(m_ourBase, kOutterCircle, 75);
        for (int i = 0; i < kHerosPerPlayer; i++) {
            m_heros[i].wait();
        }
    }

    void strategy_full_defence() {
        for (int i = 0; i < kHerosPerPlayer; i++) {
            auto & hero = m_heros[i];
            if (i == 2) {
                if (m_enemies.size() != 0) {
                    const auto & monster = m_enemies[0];
                    auto l = distance(hero.pos, monster.pos);
                    if (l <= kRadiusOfWind) {
                        hero.wind(m_theirBase.pos);
                    } else {
                        hero.move(m_guardianPos);
                    }
                } else {
                    hero.move(m_guardianPos);
                }
                continue;
            }

            auto dist = distance(hero.pos, m_ourBase.pos);
            if (dist > 6000) {
                // do not go away
                hero.move(m_ourBase.pos);
            } else {
                if (m_enemies.size() != 0) {
                    // fully focused
                    const auto & monster = m_enemies[0];
                    hero.move(monster.pos);
                } else {
                    hero.move(m_guardianPos);
                }
            }
        }
    }

    void strategy_one_attacker() {
        static bool midFieldReached = false;
        static bool frontLineReached = false;
        static bool endingGame = false;
        for (int i = 0; i < kHerosPerPlayer; i++) {
            auto & hero = m_heros[i];
            // attacker
            if (i == 2) {
                //// step1: go to the mid field
                //if (!midFieldReached) {
                //    if (distance(hero.pos, m_midFieldPos) < 20) {
                //        midFieldReached = true;
                //    } else {
                //        hero.move(m_midFieldPos);
                //    }
                //}
                //if (hero.orderReceived()) continue;

                //// step2: hunt wild mana
                //if (midFieldReached && !frontLineReached && m_ourBase.mp < 120) {
                //    auto monstersNearBy = hero.discover(m_monsters);
                //    if (!monstersNearBy.empty()) {
                //        hero.move(monstersNearBy[0].pos);
                //    } else {
                //        hero.move(m_midFieldPos);
                //    }
                //}
                //if (hero.orderReceived()) continue;

                //endingGame = is_ending_game(m_monsters);
                //if (endingGame) {
                //    endingGame = true;
                //    // bypass step3 and step4
                //    frontLineReached = true;
                //}

                //// step3: move to the front line
                //cerr << "Let the attack begin!" << endl;
                //if (!frontLineReached) {
                //    if (distance(hero.pos, m_attackerPos) < 20) {
                //        frontLineReached = true;
                //    } else {
                //        hero.move(m_attackerPos);
                //        continue;
                //    }
                //}

                //// step4: attack the farthest allies
                //if (!endingGame) {
                //    auto monstersNearBy = hero.discover(m_allies);
                //    if (!monstersNearBy.empty()) {
                //        hero.move(monstersNearBy.back().pos);
                //    } else {
                //        auto monstersNearBy = hero.discover(m_neutral);
                //        for (const auto & m : monstersNearBy) {
                //            if (!m.controlled) {
                //                hero.control(monstersNearBy.front().id, m_theirBase.pos);
                //                break;
                //            }
                //        }
                //        if (!hero.orderReceived()) hero.move(m_attackerPos);
                //    }
                //}
                //if (hero.orderReceived()) continue;

                //// step5: ending game
                //cerr << "Ending game!" << endl;
                //auto monstersNearBy = hero.discover(m_monsters);
                //sort(monstersNearBy.begin(), monstersNearBy.end(), [&](const auto & a, const auto & b) {
                //    return a.eta(m_theirBase) < b.eta(m_theirBase);
                //});
                //if (monstersNearBy.empty()) {
                //    hero.move(m_attackerPos);
                //} else {
                //    auto opponentsNearBy = hero.discover(m_opponents);
                //    if (opponentsNearBy.empty() || m_theirBase.mp <= kMagicManaCost) {
                //        auto throwables = hero.estimateWindAttackVictims(m_monsters);
                //        if (throwables.size() != 0 && m_ourBase.mp >= kMagicManaCost) {
                //            hero.wind(m_theirBase.pos);
                //        } else {
                //            // do nothing
                //        }
                //    } else {
                //        for (const auto & m : monstersNearBy) {
                //            if (m.hp >= 20 && m.shield == 0) {
                //                hero.protect(m.id);
                //                break;
                //            }
                //        }
                //    }
                //    if (!hero.orderReceived()) hero.move(monstersNearBy.back().pos);
                //}
                continue;
            }

            // goal-keeper
            //if (i == 0) {
            //    auto dist = distance(hero.pos, m_ourBase.pos);
            //    if (dist > 3000) {
            //        hero.move(m_guardianPos);
            //    } else if (m_enemies.size() != 0) {
            //        auto throwables = hero.estimateWindAttackVictims(m_enemies);
            //        if (throwables.size() != 0 && m_ourBase.mp >= kMagicManaCost) {
            //            hero.wind(m_theirBase.pos);
            //        } else {
            //            hero.move(m_enemies.front().pos);
            //        }
            //    } else {
            //        hero.move(m_guardianPos);
            //    }
            //    continue;
            //}

            // defenders
            auto dist = distance(hero.pos, m_ourBase.pos);
            int radiusOfDefence;
            if (endingGame) {
                radiusOfDefence = kInnerCircle;
            } else if (frontLineReached) {
                radiusOfDefence = kMidCircle;
            } else {
                radiusOfDefence = kOutterCircle;
            }
            if (m_enemies.empty()) {
                auto monstersNearBy = hero.discover(m_monsters);
                if (monstersNearBy.size() != 0) {
                    hero.move(monstersNearBy.front().pos);
                    hero.say("Faralë");
                } else {
                    // go to the defence post
                    hero.move(m_ourBase, radiusOfDefence, i * 60 + 15);
                    hero.say("Glories");
                }
            } else {
                auto distBaseMonster = distance(m_ourBase.pos, m_enemies.front().pos);
                if (distBaseMonster <= kInnerCircle) {
                    if (  canUseWindSpell(hero, m_enemies.front())
                       && !m_heros[0].isWinding()) {
                        hero.wind(m_theirBase.pos);
                    } else {
                        hero.move(m_enemies.front().pos);
                        hero.say("Focus!");
                    }
                } else if (distBaseMonster > kInnerCircle && distBaseMonster <= kMidCircle) {
                    hero.move(m_enemies.front().pos);
                    hero.say("Catch");
                } else if (distBaseMonster > kMidCircle && distBaseMonster <= kOutterCircle) {
                    vector<Monster> monstersInArea;
                    for (const auto & m : m_enemies) {
                        int deg = calc_degree_between(m_ourBase.pos, m.pos);
                        if (deg >= (i * 45) && deg < ((i + 1) * 45)) {
                            monstersInArea.push_back(m);
                        }
                    }
                    if (monstersInArea.size() != 0) {
                        hero.move(monstersInArea.front().pos);
                        hero.say("Invaders!");
                    } else {
                        hero.move(m_ourBase, radiusOfDefence - 400, i * 60 + 15);
                        hero.say("Withdraw");
                    }
                } else {
                    auto monstersNearBy = hero.discover(m_monsters);
                    if (monstersNearBy.size() != 0) {
                        hero.move(monstersNearBy.front().pos);
                        hero.say("Faralë");
                    }
                }
            }

            //if (dist > 6000) {
            //    // do not go away
            //    hero.move(m_ourBase.pos);
            //} else {
            //    if (m_enemies.size() != 0) {
            //        // fully focused
            //        const auto & monster = m_enemies[0];
            //        hero.move(monster.pos);
            //    } else {
            //        hero.move(m_guardianPos);
            //    }
            //}
        }
    }

    bool canUseWindSpell(const Hero & hero, const Monster & monster) {
        auto dist = distance(hero.pos, monster.pos);
        if (dist <= kRadiusOfWind && m_ourBase.mp >= kMagicManaCost && monster.shield == 0) {
            return true;
        }
        return false;
    }

    bool canUseWindSpell(const Hero & hero, const vector<Monster> & monsters) {
        for (const auto & m : monsters) {
            if (canUseWindSpell(hero, m)) return true;
        }
        return false;
    }

    Base m_ourBase;
    Base m_theirBase;

    Point m_guardianPos;
    Point m_midFieldPos;
    Point m_attackerPos;
    Point m_goalAreaPos;

    int m_turns;

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

    Base myBase, hisBase;
    myBase.pos = Point(base_x, base_y);
    hisBase.pos = Point(kWidth - base_x, kHeight - base_y);

    Point delta = Point(565, 565);
    Point post = base_x == 0 ? myBase.pos + delta : myBase.pos - delta;

    Brain brain(myBase, hisBase);

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
        brain.showBases();
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
            e.controlled = is_controlled;
            e.hp = health;
            e.vx = vx;
            e.vy = vy;
            e.target = near_base;
            e.threat = threat_for;
            units.push_back(e);
        }
        brain.parse(units);
        brain.showMonsters();
        brain.play();
    }
}
