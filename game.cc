#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

struct Point {
    int x;
    int y;

    Point() : x(0), y(0) {}
    Point(int m, int n) : x(m), y(n) {}

    void display(std::ostream & os) {
        os << "(" << x << "," << y << ")";
    }
};

float distance(const Point & a, const Point & b) {
    return std::hypot(a.x - b.x, a.y - b.y);
}

std::ostream & operator<<(std::ostream & os, Point p) {
    p.display(os);
    return os;
}

struct Base {
    int hp;
    int mp;
    Point pos;

    Base() : hp(0), mp(0), pos(0, 0) {}

    void display(std::ostream & os) {
        os << "hp=" << hp;
        os << "; mp=" << mp;
        os << "; pos=" << pos;
    }
};

std::ostream & operator<<(std::ostream & os, Base b) {
    b.display(os);
    return os;
}

struct Entity {
    int id; // Unique identifier
    int type; // 0=monster, 1=your hero, 2=opponent hero
    Point pos;
    int shield; // Ignore for this league; Count down until shield spell fades
    int controlled; // Ignore for this league; Equals 1 when this entity is under a control spell
    int hp; // Remaining health of this monster
    int vx; // Trajectory of this monster
    int vy;
    int target; // 0=monster with no target yet, 1=monster targeting a base
    int threat; // Given this monster's trajectory, is it a threat to 1=your base, 2=your opponent's base, 0=neither

    // risk of this entity based on the distance
    int risk(const Point & p) const {
        if (type != 0)
            return 0;

        if (target != 0) {
            if (threat == 1) {
                return 100;
            } else {
                return 0;
            }
        } else {
            float k = 1 / (distance(pos, p) + 1);
            return 70 * k;
        }
    }
};

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/

const int kWidth = 17630;
const int kHeight  = 9000;

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
        vector<Entity> monsters;
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
                case 0:
                    monsters.push_back(e);
                    break;

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
            return a.risk(myBase.pos) > b.risk(myBase.pos);
        });
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
