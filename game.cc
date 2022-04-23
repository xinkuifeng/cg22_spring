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

class Hero : public Entity {
public:

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

// risk of this entity based on the distance
int eval_risk(const Base & ref, const Monster & m) {
    int ans = 0;
    float l = distance(ref.pos, m.pos);

    if (m.target != 0) {
        if (m.threat == 1) {
            ans = 100 - l / 400;
        } else {
            // do nothing
        }
    } else {
        float k = 1 / (l + 1);
        // if this unit can eventually reach to the base
        if (m.eta(ref) > 0)
            ans = 70 * k;
    }
    return ans > 0 ? ans : 0;
}

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/

Base myBase, hisBase;

void show_base() {
    cerr << "my base: " << myBase << endl;
    cerr << "his base: " << hisBase << endl;
}

int main()
{
    int base_x; // The corner of the map representing your base
    int base_y;
    cin >> base_x >> base_y; cin.ignore();
    int heroes_per_player; // Always 3
    cin >> heroes_per_player; cin.ignore();

    myBase.pos = Point(base_x, base_y);
    hisBase.pos = Point(kWidth - base_x, kHeight - base_y);

    // game loop
    while (1) {
        for (int i = 0; i < 2; i++) {
            int health; // Your base health
            int mana; // Ignore in the first league; Spend ten mana to cast a spell
            cin >> health >> mana; cin.ignore();

            if (i == 0) {
                myBase.hp = health;
                myBase.mp = mana;
            } else {
                hisBase.hp = health;
                hisBase.mp = mana;
            }
        }
        show_base();
        int entity_count; // Amount of heros and monsters you can see
        cin >> entity_count; cin.ignore();
        vector<Entity> myHeros;
        vector<Monster> monsters;
        vector<Entity> hisHeros;
        for (int i = 0; i < entity_count; i++) {
            int id; // Unique identifier
            int type; // 0=monster, 1=your hero, 2=opponent hero
            int x; // Position of this entity
            int y;
            int shield_life; // Ignore for this league; Count down until shield spell fades
            int is_controlled; // Ignore for this league; Equals 1 when this entity is under a control spell
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

            switch (type) {
                case 0: {
                    Monster m(e);
                    if (m.eta(myBase) > 0) monsters.push_back(e);
                    break;
                }

                case 1:
                    myHeros.push_back(e);
                    break;

                case 2:
                    hisHeros.push_back(e);
                    break;

                default:
                    throw("unknown type");
                    return -1;
            }
        }
        // sort by risk level
        sort(monsters.begin(), monsters.end(), [](const auto & a, const auto & b) {
            return eval_risk(myBase, a) > eval_risk(myBase, b);
        });
        // display the monsters with the highest risk
        for (int i = 0; i < 3 && i < monsters.size(); ++i) {
            const auto & m = monsters[i];
            cerr << m << "; ETA=" << m.eta(myBase) << endl;
        }
        for (int i = 0; i < heroes_per_player; i++) {
            const auto & hero = myHeros[i];
            auto dist = distance(hero.pos, myBase.pos);

            if (dist > 6000) {
                // do not go away
                cout << "MOVE " << myBase.pos.x << " " << myBase.pos.y << endl;
            } else {
                if (monsters.size() != 0) {
                    // fully focused
                    const auto & monster = monsters[0];
                    cout << "MOVE " << monster.pos.x << " " << monster.pos.y << endl;
                } else {
                    cout << "WAIT On Hold" << endl;
                }
            }
        }
    }
}
