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

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/

const int kWidth = 17630;
const int kHeight  = 9000;

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

    // game loop
    while (1) {
        for (int i = 0; i < 2; i++) {
            int health; // Your base health
            int mana; // Ignore in the first league; Spend ten mana to cast a spell
            cin >> health >> mana; cin.ignore();

            if (i == 0) {
                myBase.hp = health;
                myBase.mp = mana;
                cerr << "my base: " << myBase << endl;
            } else {
                hisBase.hp = health;
                hisBase.mp = mana;
                cerr << "his base: " << hisBase << endl;
            }
        }
        int entity_count; // Amount of heros and monsters you can see
        cin >> entity_count; cin.ignore();
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
        }
        for (int i = 0; i < heroes_per_player; i++) {

            // Write an action using cout. DON'T FORGET THE "<< endl"
            // To debug: cerr << "Debug messages..." << endl;


            // In the first league: MOVE <x> <y> | WAIT; In later leagues: | SPELL <spellParams>;
            cout << "WAIT" << endl;
        }
    }
}
