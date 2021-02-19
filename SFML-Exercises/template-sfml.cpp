#include <iostream>
#include <math.h>
#include <cmath> // pow
#include <fstream>
#include <vector>
#include <SFML/Graphics.hpp>

namespace utility {
    // in case the person compiling this does not have C++17 installed
    // https://en.cppreference.com/w/cpp/algorithm/clamp
    // follows the first version of possible implementations
    template <class T>
    constexpr const T& clamp (const T& v, const T& lo, const T& hi) {
        assert(!(hi < lo));
        return (v < lo) ? lo : (hi < v) ? hi : v;
    }
}

// constants
constexpr unsigned int fps_limit{255};
constexpr float epsilon{1e-6f};
const sf::Time fixed_update_time = sf::seconds(1.f/fps_limit);
const sf::Vector2f zero_vector{0.f,0.f};
constexpr float pi{std::acos(-1)};
constexpr float deg_to_rad{pi/180.f};
constexpr float rad_to_deg{180.f/pi};

// default values
namespace default_vals {
    constexpr unsigned int window_w{1500};
    constexpr unsigned int window_h{900};
}

template <typename T>
T dot (const sf::Vector2<T>& a, const sf::Vector2<T>& b) {
    return a.x*b.x + a.y*b.y;
}

template <typename T>
T cross (const sf::Vector2<T>& a, const sf::Vector2<T>& b) {
    return a.x*b.y - b.x*a.y;
}

// enumerations
enum Direction {up, down, left, right};

// globals
unsigned int window_w{default_vals::window_w};
unsigned int window_h{default_vals::window_h};

bool directionFlags[4] = {false, false, false, false};
bool leftMouseButtonFlag = false;

bool readFromAvailableText() {
    std::string input;
    std::ifstream settings("collision.txt");
    if (settings.is_open()) {
        // read stuff here
        settings.close();
        return true;
    } else {
        return false;
    }
}

void initializeSettings() {
    if (readFromAvailableText()) {
        std::cout << "collision.txt successfully loaded.\n";
    } else {
        std::cout << "collision.txt not loaded. Using default values.\n";
    }
}

void pressEvents(sf::RenderWindow& window, const sf::Event& event) {
    switch (event.key.code) {
        case sf::Keyboard::Escape:
            window.close();
            break;
        case sf::Keyboard::W:
            directionFlags[static_cast<unsigned int>(Direction::up)] = true;
            break;
        case sf::Keyboard::A:
            directionFlags[static_cast<unsigned int>(Direction::left)] = true;
            break;
        case sf::Keyboard::S:
            directionFlags[static_cast<unsigned int>(Direction::down)] = true;
            break;
        case sf::Keyboard::D:
            directionFlags[static_cast<unsigned int>(Direction::right)] = true;
            break;
        default:
            // nothing
            break;
    }
}

void releaseEvents(sf::RenderWindow& window, const sf::Event& event) {
    switch (event.key.code) {
        case sf::Keyboard::W:
            directionFlags[static_cast<unsigned int>(Direction::up)] = false;
            break;
        case sf::Keyboard::A:
            directionFlags[static_cast<unsigned int>(Direction::left)] = false;
            break;
        case sf::Keyboard::S:
            directionFlags[static_cast<unsigned int>(Direction::down)] = false;
            break;
        case sf::Keyboard::D:
            directionFlags[static_cast<unsigned int>(Direction::right)] = false;
            break;
        default:
            // nothing
            break;
    }
}

void handleInput(sf::RenderWindow& window) {
    sf::Event event;
    while (window.pollEvent(event)) {
        switch (event.type) {
            case sf::Event::Closed:
                window.close();
                break;
            case sf::Event::KeyPressed:
                pressEvents(window, event);
                break;
            case sf::Event::MouseButtonPressed:
                if (event.mouseButton.button == sf::Mouse::Left) {leftMouseButtonFlag = true;}
                break;
            case sf::Event::KeyReleased:
                releaseEvents(window, event);
                break;
            case sf::Event::MouseButtonReleased:
                if (event.mouseButton.button == sf::Mouse::Left) {leftMouseButtonFlag = false;}
                break;
            default:
                // nothing
                break;
        }
    }
}

void update(const sf::Time& elapsed, sf::RenderWindow& window) {
    float delta = elapsed.asSeconds();
    // update stuff here
}

void render(sf::RenderWindow& window) {
    window.clear(sf::Color::Black);
    // draw stuff here
    window.display();
}

int main () {
    srand(time(NULL));
    sf::RenderWindow window(sf::VideoMode(window_w, window_h), "Collision");
	window.setFramerateLimit(fps_limit);

    initializeSettings();
    
    sf::Clock clock;
    sf::Time timeSinceLastUpdate;
    while(window.isOpen()) {
        sf::Time elapsed = clock.restart();
        timeSinceLastUpdate += elapsed;

        handleInput(window);
        while (timeSinceLastUpdate >= fixed_update_time) {
            update(fixed_update_time, window);
            timeSinceLastUpdate -= fixed_update_time;
        }
        render(window);
    }
    return 0;
}