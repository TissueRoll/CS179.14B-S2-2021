#include <iostream>
#include <math.h>
#include <fstream>
#include <vector>
#include <algorithm>
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
constexpr unsigned int fps_limit{60};
constexpr float epsilon{1e-6f};
const sf::Time fixed_update_time = sf::seconds(1.f/144.f);
const sf::Vector2f zero_vector{0.f,0.f};
constexpr float pi{std::acos(-1)};
constexpr float deg_to_rad{pi/180.f};
constexpr float rad_to_deg{180.f/pi};

// default values
namespace default_vals {
    constexpr unsigned int window_w{1500};
    constexpr unsigned int window_h{900};
    constexpr float force{10000.f};
    namespace user {
        constexpr float radius{30.f};
        constexpr float mass{1000.f};
        constexpr float elasticity{0.f};
        constexpr float friction{0.05f};
    }
    constexpr unsigned int num_circles{8};
    namespace enemy {
        constexpr float radius{30.f};
        constexpr float mass{500.f};
        constexpr float elasticity{0.5f};
        constexpr float friction{0.05f};
    }
}

struct Material {
    float mass{100.f};
    float elasticity{0.f};
    float friction{0.01f};
};

template <typename T>
T dot (const sf::Vector2<T>& a, const sf::Vector2<T>& b) {
    return a.x*b.x + a.y*b.y;
}

template <typename T>
T cross (const sf::Vector2<T>& a, const sf::Vector2<T>& b) {
    return a.x*b.y - b.x*a.y;
}

struct BallEntity {
    sf::CircleShape ball;
    Material material;
    float radius;
    sf::Vector2f velocity;
    sf::Color colorNoFriction{sf::Color::Green};
    sf::Color colorFriction{sf::Color::Red};

    BallEntity() = default;

    void setFrictionColors(const sf::Color& cNF, const sf::Color& cF) {
        colorNoFriction = cNF;
        colorFriction = cF;
    }

    void initializeEntity(float x, float y, bool frictionEnabled = false) {
        ball.setOrigin(radius,radius);
        ball.setRadius(radius);
        ball.setPosition(x, y);
        ball.setFillColor(frictionEnabled ? colorFriction : colorNoFriction);
    }

    void moveEntity(const sf::Vector2f& acceleration, float delta, bool frictionEnabled = false) {
        sf::Vector2f nVelocity = velocity;
        sf::Vector2f pos = ball.getPosition();
        pos += acceleration * 0.5f * delta * delta + nVelocity * delta;
        nVelocity += acceleration * delta;
        ball.setPosition(pos);

        float nVMag = std::hypot(nVelocity.x, nVelocity.y);
        if (frictionEnabled) {
            ball.setFillColor(colorFriction);
            if (std::fabs(nVMag) > epsilon) {
                sf::Vector2f nVNorm = nVelocity / nVMag;
                nVMag = std::max(0.f, nVMag - material.friction * delta);
                nVelocity = nVNorm * nVMag;
            }
        } else {
            ball.setFillColor(colorNoFriction);
        }

        if (std::fabs(nVMag) > epsilon) {
            velocity = nVelocity;
        } else {
            velocity = zero_vector;
        }
    }

    // this WILL change the other entity
    // if you don't like this, do another kind of collision resolution
    bool collisionWith(BallEntity& other) {
        sf::Vector2f this_entity = ball.getPosition();
        sf::Vector2f other_entity = other.ball.getPosition();

        sf::Vector2f difference_vector = other_entity - this_entity; // negate if other way
        float dist = std::hypot(difference_vector.x, difference_vector.y);
        float interpenetration_dist = (radius + other.radius) - dist;

        sf::Vector2f collision_normal;
        if (std::fabs(dist) > epsilon) {
            collision_normal = difference_vector / dist;
        }

        if (interpenetration_dist > epsilon) { // touching
            // resolve interpenetration
            ball.move(-collision_normal * interpenetration_dist);

            sf::Vector2f vAB = velocity - other.velocity;
            sf::Vector2f vBA = -vAB;

            float sum_massriprocals = 1.f/material.mass + 1.f/other.material.mass;

            // note: the "elasticity" is also known as the coefficient of restitution
            // different physics engines may choose to modify this depending on the situation
            float this_impulse = -(((1 + material.elasticity) * dot(vAB, collision_normal)) / sum_massriprocals);
            float other_impulse = -(((1 + other.material.elasticity) * dot(vBA, collision_normal)) / sum_massriprocals);

            sf::Vector2f this_new_velocity;
            if (std::fabs(material.mass) > epsilon) {
                this_new_velocity = velocity + collision_normal * (this_impulse / material.mass);
            }

            sf::Vector2f other_new_velocity;
            if (std::fabs(other.material.mass) > epsilon) {
                other_new_velocity = other.velocity + collision_normal * (other_impulse / other.material.mass);
            }

            velocity = this_new_velocity;
            other.velocity = other_new_velocity;
            return true;
        } else {
            return false;
        }
    }

    // snapping; can't think of a better way
    void wallBounce(float x_bound, float y_bound) {
        sf::Vector2f tempPosition = ball.getPosition();
        if (tempPosition.x - radius < 0) {
            ball.setPosition(radius, tempPosition.y);
            tempPosition.x = radius;
            velocity.x *= -material.elasticity;
        }

        if (tempPosition.y - radius < 0) {
            ball.setPosition(tempPosition.x, radius);
            tempPosition.y = radius;
            velocity.y *= -material.elasticity;
        }

        if (tempPosition.x + radius > x_bound) {
            ball.setPosition(x_bound - radius, tempPosition.y);
            tempPosition.x = x_bound - radius;
            velocity.x *= -material.elasticity;
        }

        if (tempPosition.y + radius > y_bound) {
            ball.setPosition(tempPosition.x, y_bound - radius);
            tempPosition.y = y_bound - radius;
            velocity.y *= -material.elasticity;
        }
    }
};

// enumerations
enum Direction {up, down, left, right};

// globals
unsigned int window_w{default_vals::window_w};
unsigned int window_h{default_vals::window_h};
float force{default_vals::force};
unsigned int num_circles{default_vals::num_circles};

bool directionFlags[4] = {false, false, false, false};
bool leftMouseButtonFlag = false;
bool gfrictionEnabled = false;

BallEntity userBallEntity;
Material enemy_material{default_vals::enemy::mass, default_vals::enemy::elasticity, default_vals::enemy::friction};
float enemy_radius{default_vals::enemy::radius};
std::vector<BallEntity> otherBallEntities;
bool userBallEntityFlag;
std::vector<bool> otherBallEntitiesFlag;

bool readFromAvailableText() {
    std::string input;
    std::ifstream settings("hw01_settings.txt");
    if (settings.is_open()) {
        settings >> window_w >> window_h;
        settings >> force;
        settings >> userBallEntity.material.mass >> userBallEntity.material.elasticity >> userBallEntity.material.friction;
        settings >> userBallEntity.radius;
        settings >> num_circles;
        settings >> enemy_material.mass >> enemy_material.elasticity >> enemy_material.friction;
        settings >> enemy_radius;
        settings.close();
        return true;
    } else {
        return false;
    }
}

void initializeSettings() {
    if (readFromAvailableText()) {
        std::cout << "hw01_settings.txt successfully loaded.\n";
    } else {
        std::cout << "hw01_settings.txt not loaded. Using default values.\n";
        userBallEntity.material = {default_vals::user::mass, default_vals::user::elasticity, default_vals::user::friction};
        userBallEntity.radius = default_vals::user::radius;
        userBallEntity.setFrictionColors(sf::Color::Green, sf::Color::Red);
    }

    otherBallEntities.resize(num_circles);
    float borderX = window_w - 4 * enemy_radius;
    float borderY = window_h - 2 * userBallEntity.radius - 4 * enemy_radius;
    for (int i = 0; i < num_circles; ++i) {
        int row = i / 7;
        int column = i % 7;
        otherBallEntities[i].material = enemy_material;
        otherBallEntities[i].radius = enemy_radius;
        otherBallEntities[i].setFrictionColors(sf::Color::Blue, sf::Color::Yellow);
        otherBallEntities[i].initializeEntity(borderX / 7.f * column + 4 * enemy_radius, borderY / 5.f * row + 2 * enemy_radius, gfrictionEnabled);
    }

    userBallEntityFlag = true;
    otherBallEntitiesFlag.resize(num_circles);
    std::fill(otherBallEntitiesFlag.begin(), otherBallEntitiesFlag.end(), true);

    userBallEntity.initializeEntity(window_w / 2.f, window_h - userBallEntity.radius, gfrictionEnabled);
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
        case sf::Keyboard::F:
            gfrictionEnabled = !gfrictionEnabled;
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

// note: if it's instantaneous acceleration, use a local variable instead
void update(const sf::Time& elapsed) {
    float delta = elapsed.asSeconds();

    sf::Vector2f dir;
    sf::Vector2f acceleration;
    if (directionFlags[static_cast<unsigned int>(Direction::up)]) dir.y -= 69.f;
    if (directionFlags[static_cast<unsigned int>(Direction::left)]) dir.x -= 69.f;
    if (directionFlags[static_cast<unsigned int>(Direction::down)]) dir.y += 69.f;
    if (directionFlags[static_cast<unsigned int>(Direction::right)]) dir.x += 69.f;
    float dir_mag = std::hypot(dir.x, dir.y);
    if (dir_mag > epsilon) {
        acceleration = (dir / dir_mag) * force / userBallEntity.material.mass;
    }

    userBallEntityFlag = false;
    std::fill(otherBallEntitiesFlag.begin(), otherBallEntitiesFlag.end(), false);

    // move first
    userBallEntity.moveEntity(acceleration, delta, gfrictionEnabled);
    for (int i = 0; i < num_circles; ++i) {
        otherBallEntities[i].moveEntity(zero_vector, delta, gfrictionEnabled);
    }

    // resolve interpenetrations
    userBallEntity.wallBounce(window_w, window_h);
    for (int i = 0; i < num_circles; ++i) {
        otherBallEntities[i].wallBounce(window_w, window_h);
    }

    for (int i = 0; i < num_circles; ++i) {
        for (int j = i+1; j < num_circles; ++j) {
            if (i == j) continue;
            otherBallEntities[i].collisionWith(otherBallEntities[j]);
        }
        userBallEntity.collisionWith(otherBallEntities[i]);
    }
}

void render(sf::RenderWindow& window) {
    window.clear(sf::Color::Black);
    window.draw(userBallEntity.ball);
    for (int i = 0; i < num_circles; ++i) {
        window.draw(otherBallEntities[i].ball);
    }
    window.display();
}

int main () {
    srand(time(NULL));
    sf::RenderWindow window(sf::VideoMode(window_w, window_h), "HW 1");
	window.setFramerateLimit(fps_limit);

    initializeSettings();
    
    sf::Clock clock;
    sf::Time timeSinceLastUpdate;
    while(window.isOpen()) {
        sf::Time elapsed = clock.restart();
        timeSinceLastUpdate += elapsed;

        handleInput(window);
        while (timeSinceLastUpdate >= fixed_update_time) {
            update(fixed_update_time);
            timeSinceLastUpdate -= fixed_update_time;
        }
        render(window);
    }
    return 0;
}