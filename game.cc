#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

/*****************************************************************************
 * Constants
 ****************************************************************************/
const int kWidth = 17630;
const int kHeight = 9000;
const int kRadiusOfBase = 5000;
const int kMonsterSpeed = 400;
const int kHeroSpeed = 800;
const int kRadiusOfWind = 1280;
const int kHerosPerPlayer = 3;
const int kHeroPhysicAttackRange = 800;

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

int find_max_hp(const vector<Monster> & enemies) {
    int ans = 0;
    for (const auto & m : enemies) {
        ans = std::max(ans, m.hp);
    }
    return ans;
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
    Hero(Entity e): Entity(e) {}

    void move(const Point & p) const {
        cout << "MOVE " << p.x << " " << p.y << endl;
    }

    void wait() const {
        cout << "WAIT Ndorē" << endl;
    }

    void wind(const Point & toward) const {
        cout << "SPELL WIND " << toward.x << " " << toward.y
             << " Súrë" << endl;
    }

    void protect(int id) const {
        cout << "SPELL SHIELD " << id
             << " May force be with you" << endl;
    }

    // find the enemies in the range
    vector<Monster> discover(const vector<Monster> & enemies) const {
        vector<Monster> ans;
        for (const auto & m : enemies) {
            if (distance(pos, m.pos) <= kHeroPhysicAttackRange) {
                ans.push_back(m);
            }
        }
        return ans;
    }

private:
};

class Brain {
public:
    Brain(const Base & ours, const Base & theirs) :
        m_ourBase(ours), m_theirBase(theirs), m_turns(0)
    {
        Point delta = Point(565, 565);
        m_guardianPos = m_ourBase.pos.x == 0 ?
            m_ourBase.pos + delta : m_ourBase.pos - delta;

        delta = Point(4243, 4243);
        m_attackerPos = m_ourBase.pos.x == 0 ?
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
        //strategy_full_defence();
        strategy_one_attacker();
    }

    // for debug purpose
    void showBases() {
        cerr << "our base: " << m_ourBase << endl;
        cerr << "their base: " << m_theirBase << endl;
    }

    void showMonsters() {
        // highest risk to our base
        cerr << "=== our enemies ===" << endl;
        for (int i = 0; i < kHerosPerPlayer && i < m_enemies.size(); ++i) {
            const auto & m = m_enemies[i];
            cerr << m << "; ETA=" << m.eta(m_ourBase) << endl;
        }
        // neutral
        cerr << "=== passengers ===" << endl;
        for (int i = 0; i < kHerosPerPlayer && i < m_neutral.size(); ++i) {
            const auto & m = m_neutral[i];
            cerr << m << "; ETA=" << m.eta(m_theirBase) << endl;
        }
        cerr << "=== our allies  ===" << endl;
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
            } else if (m.target == 0) {
                // they are not near a base
                neutral.push_back(m);
            } else {
                // they could be useful to us
                allies.push_back(m);
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
        for (int i = 0; i < kHerosPerPlayer; i++) {
            m_heros[i].wait();
        }
    }

    void strategy_full_defence() {
        for (int i = 0; i < kHerosPerPlayer; i++) {
            const auto & hero = m_heros[i];
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
                    hero.wait();
                }
            }
        }
    }

    void strategy_one_attacker() {
        static bool attackerIsReady = false;
        for (int i = 0; i < kHerosPerPlayer; i++) {
            const auto & hero = m_heros[i];
            // attacker
            if (i == 2) {
                // init
                if (!attackerIsReady) {
                    if (distance(hero.pos, m_attackerPos) < 20) {
                        attackerIsReady = true;
                    } else {
                        hero.move(m_attackerPos);
                        continue;
                    }
                }

                auto monstersNearBy = hero.discover(m_monsters);
                for (const auto & m : monstersNearBy) {
                    if (m.hp >= 20 && m.shield == 0) {
                        hero.protect(m.id);
                        continue;
                    }
                }
                if (m_allies.size() != 0) {
                    const auto & m = m_allies[0];
                    hero.move(m.pos);
                    continue;
                } else {
                    hero.wait();
                    continue;
                }
                hero.wait();
                continue;
            }

            // defenders
            auto dist = distance(hero.pos, m_ourBase.pos);
            if (dist > 6000) {
                // do not go away
                hero.move(m_guardianPos);
            } else {
                auto enemiesNearBy = hero.discover(m_enemies);
                auto maxHp = find_max_hp(enemiesNearBy);

                if (maxHp > 8 && dist <= 800 && i == 0) {
                    // very urgent case
                    hero.wind(m_theirBase.pos);
                } else if (m_enemies.size() != 0) {
                    // fully focused
                    const auto & monster = m_enemies[0];
                    hero.move(monster.pos);
                } else {
                    hero.wait();
                }
            }
        }
    }

    Base m_ourBase;
    Base m_theirBase;

    Point m_guardianPos;
    Point m_attackerPos;

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
