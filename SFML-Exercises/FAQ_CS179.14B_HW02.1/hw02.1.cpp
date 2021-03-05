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
    constexpr unsigned int window_w{800};
    constexpr unsigned int window_h{600};
    constexpr unsigned int boxes_count{5};
    constexpr float speed{10.f};
    const sf::Vector2f rectSizes[boxes_count] = {
        sf::Vector2f(200,200),
        sf::Vector2f(150,100),
        sf::Vector2f(50,150),
        sf::Vector2f(150,50),
        sf::Vector2f(50,300)
    };
    constexpr float rotation_speed[boxes_count] = {
        5.f / 3.f,
        4.f / 3.f,
        3.f / 3.f,
        2.f / 3.f,
        1.f / 3.f
    };
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
unsigned int boxes_count{default_vals::boxes_count};
float speed{default_vals::speed};

bool directionFlags[4] = {false, false, false, false};
bool leftMouseButtonFlag = false;

std::vector<sf::RectangleShape> rects;
std::vector<sf::RectangleShape> boundingBoxEntity;
std::vector<sf::FloatRect> boundingBoxValues;
std::vector<sf::Vector2f> rectSizes;
std::vector<float> rotation_speed;

void resizeVectors(unsigned int size) {
    rects.resize(size);
    boundingBoxEntity.resize(size);
    boundingBoxValues.resize(size);
    rectSizes.resize(size);
    rotation_speed.resize(size);
}

bool readFromAvailableText() {
    std::string input;
    std::ifstream settings("hw02.1.txt");
    if (settings.is_open()) {
        settings >> window_w >> window_h;
        settings >> speed;
        settings >> boxes_count;
        resizeVectors(boxes_count);
        for (unsigned int i = 0; i < boxes_count; ++i) {
            settings >> rectSizes[i].x >> rectSizes[i].y >> rotation_speed[i];
        }
        settings.close();
        return true;
    } else {
        return false;
    }
}

void initializeSettings() {
    if (readFromAvailableText()) {
        std::cout << "hw02.1.txt successfully loaded.\n";
    } else {
        std::cout << "hw02.1.txt not loaded. Using default values.\n";
        resizeVectors(boxes_count);
        for (unsigned int i = 0; i < boxes_count; ++i) {
            rectSizes[i] = default_vals::rectSizes[i];
            rotation_speed[i] = default_vals::rotation_speed[i];
        }
    }

    for (unsigned int i = 0; i < boxes_count; ++i) {
        rects[i].setSize(rectSizes[i]);
        rects[i].setOrigin(rectSizes[i].x / 2, rectSizes[i].y / 2);
    }

    if (boxes_count == 5) {
        // compliance with the exercise
        rects[0].setPosition(window_w/4, window_h/4);
        rects[1].setPosition((3*window_w)/4, window_h/4);
        rects[2].setPosition(window_w/4, (3*window_h)/4);
        rects[3].setPosition((3*window_w)/4, (3*window_h)/4);
        rects[4].setPosition(window_w/2, window_h/2);    
    } else {
        unsigned int sqr = 1;
        // find the square that fits all boxes
        while (sqr*sqr < boxes_count) {
            sqr++;
        }

        for (unsigned int i = 0; i < sqr; ++i) {
            for (unsigned int j = 0; j < sqr; ++j) {
                if (i*sqr + j >= boxes_count) {
                    break;
                }
                rects[i*sqr + j].setPosition(
                    (j+0.5f) * window_w / sqr, 
                    (i+0.5f) * window_h / sqr
                );
            }
        }
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

    sf::Vector2f dir;
    if (directionFlags[static_cast<unsigned int>(Direction::up)]) dir.y -= 69.f;
    if (directionFlags[static_cast<unsigned int>(Direction::left)]) dir.x -= 69.f;
    if (directionFlags[static_cast<unsigned int>(Direction::down)]) dir.y += 69.f;
    if (directionFlags[static_cast<unsigned int>(Direction::right)]) dir.x += 69.f;
    float dir_mag = std::hypot(dir.x, dir.y);
    if (dir_mag > epsilon) {
        sf::Vector2f v = (dir / dir_mag) * speed * delta;
        rects[0].move(v);
    }

    // just prevent the center of mass from getting out
    if(rects[0].getPosition().x < 0) {
        rects[0].setPosition(0, rects[0].getPosition().y);
    }
    if(rects[0].getPosition().x > window_w) {
        rects[0].setPosition(window_w, rects[0].getPosition().y);
    }
    if(rects[0].getPosition().y < 0) {
        rects[0].setPosition(rects[0].getPosition().x, 0);
    }
    if(rects[0].getPosition().y > window_h) {
        rects[0].setPosition(rects[0].getPosition().x, window_h);
    }

    for(unsigned int i = 0; i < boxes_count; ++i) {
        // rectangles
        rects[i].rotate(rotation_speed[i]);
        rects[i].setFillColor(sf::Color::White);

        // bounding boxes
        boundingBoxValues[i] = rects[i].getGlobalBounds();
        boundingBoxEntity[i].setSize(sf::Vector2f(boundingBoxValues[i].width, boundingBoxValues[i].height));
        boundingBoxEntity[i].setPosition(boundingBoxValues[i].left, boundingBoxValues[i].top); 
        boundingBoxEntity[i].setFillColor(sf::Color::Transparent);
        boundingBoxEntity[i].setOutlineThickness(1.0f);
        boundingBoxEntity[i].setOutlineColor(sf::Color::White);
    }

    for(unsigned int i = 0; i < boxes_count; ++i) {
        for(unsigned int j = i+1; j < boxes_count; ++j) {
            if(boundingBoxValues[i].intersects(boundingBoxValues[j])) {  
                rects[i].setFillColor(sf::Color::Green);
                rects[j].setFillColor(sf::Color::Green);
                boundingBoxEntity[i].setOutlineColor(sf::Color::Green);
                boundingBoxEntity[j].setOutlineColor(sf::Color::Green);           
            }
        }
    }  
}

void render(sf::RenderWindow& window) {
    window.clear(sf::Color::Black);
    for (unsigned int i = 0; i < boxes_count; ++i) {
        window.draw(rects[i]);
        window.draw(boundingBoxEntity[i]);
    }
    window.display();
}

int main () {
    srand(time(NULL));
    sf::RenderWindow window(sf::VideoMode(window_w, window_h), "HW02.1");
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