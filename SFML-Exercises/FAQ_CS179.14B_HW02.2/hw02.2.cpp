#include <iostream>
#include <math.h>
#include <cmath> // pow
#include <fstream>
#include <vector>
#include <limits>
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
const sf::Vector2f limit_vector{std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
constexpr float pi{std::acos(-1)};
constexpr float deg_to_rad{pi/180.f};
constexpr float rad_to_deg{180.f/pi};

// default values
namespace default_vals {
    constexpr unsigned int window_w{800};
    constexpr unsigned int window_h{600};
    constexpr float speed{100.f};
    constexpr unsigned int polysCount{0};
}

template <typename T>
T dot (const sf::Vector2<T>& a, const sf::Vector2<T>& b) {
    return a.x*b.x + a.y*b.y;
}

template <typename T>
T cross (const sf::Vector2<T>& a, const sf::Vector2<T>& b) {
    return a.x*b.y - b.x*a.y;
}

template <typename T>
sf::Vector2<T> perp (const sf::Vector2<T>& a) {
    return sf::Vector2<T>(-a.y, a.x);
}

template <typename T>
T norm (const sf::Vector2<T>& a) {
    return std::hypot(a.x, a.y);
}

// +------------+
// | cos | -sin |
// +-----+------+
// | sin |  cos |
// +------------+

template <typename T>
sf::Vector2<T> vectorRotate(const sf::Vector2<T>& a, float cos, float sin) {
    return sf::Vector2<T>{a.x*cos - a.y*sin, a.x*sin + a.y*cos};
}

// enumerations
enum Direction {up, down, left, right};

// globals
unsigned int window_w{default_vals::window_w};
unsigned int window_h{default_vals::window_h};
unsigned int polysCount{default_vals::polysCount};
float speed{default_vals::speed};

bool directionFlags[4] = {false, false, false, false};
bool leftMouseButtonFlag = false;
bool spaceButtonFlag = false;

std::vector<sf::ConvexShape> polys;
std::vector<sf::RectangleShape> boundingBoxEntity;
std::vector<sf::FloatRect> boundingBoxValues;
std::vector<sf::Vector2f> rectSizes;
std::vector<float> rotation_speed;

bool SAT (sf::ConvexShape a, sf::ConvexShape b) 
{
    float rota = a.getRotation() * deg_to_rad;
    float cosa = cos(rota);
    float sina = sin(rota);
    float rotb = b.getRotation() * deg_to_rad;
    float cosb = cos(rotb);
    float sinb = sin(rotb);

    int an = a.getPointCount();
    int bn = b.getPointCount();

    // get all axes
    sf::Vector2f axes[an + bn], p0, p1;
    for (int i = 0; i < an; i++) 
    {
        p0 = vectorRotate(a.getPoint(i), cosa, sina);
        p1 = vectorRotate(a.getPoint((i+1)%an), cosa, sina);
        axes[i] = perp(p1 - p0);
    }

    for (int i = 0; i < bn; i++) 
    {
        p0 = vectorRotate(b.getPoint(i), cosb, sinb);
        p1 = vectorRotate(b.getPoint((i+1)%bn), cosb, sinb);
        axes[i + an] = perp(p1 - p0);
    }

    // project the polygon onto the axis
    float amin, amax, bmin, bmax, ca, cb, ra, rb, t;
    sf::Vector2f naxis;
    for (int i = 0; i < an + bn; ++i) 
    {
        naxis = axes[i];
        amin = amax = dot(naxis, vectorRotate(a.getPoint(0), cosa, sina) + a.getPosition());
        bmin = bmax = dot(naxis, vectorRotate(b.getPoint(0), cosb, sinb) + b.getPosition());

        for (int j = 1; j < an; ++j) 
        {
            t = dot(naxis, vectorRotate(a.getPoint(j), cosa, sina) + a.getPosition());
            amin = std::min(amin, t); amax = std::max(amax, t);
        }   
        for (int j = 1; j < bn; ++j) 
        {
            t = dot(naxis, vectorRotate(b.getPoint(j), cosb, sinb) + b.getPosition());
            bmin = std::min(bmin, t); bmax = std::max(bmax, t);
        }

        ca = (amin + amax) / 2.f;
        cb = (bmin + bmax) / 2.f;
        ra = (amax - amin) / 2.f;
        rb = (bmax - bmin) / 2.f;

        // if the distance of the centers are farther than the sum of their radius, they are separated
        if (std::abs(cb-ca) > ra+rb) 
        {
            return false;
        }
    }
    return true;
}

void resizeVectors(unsigned int size) {
    polys.resize(size);
    boundingBoxEntity.resize(size);
    boundingBoxValues.resize(size);
    rectSizes.resize(size);
    rotation_speed.resize(size);
}

bool readFromAvailableText() {
    std::string input;
    std::ifstream settings("hw02.2.txt");
    if (settings.is_open()) {
        settings >> window_w >> window_h;
        settings >> speed;
        settings >> polysCount;
        resizeVectors(polysCount);
        int polySize;
        sf::Vector2f pos, p;
        for (unsigned int i = 0; i < polysCount; ++i) {
            settings >> polySize;
            polys[i].setPointCount(polySize);
            for (unsigned int j = 0; j < polySize; ++j) {
                settings >> p.x >> p.y;
                polys[i].setPoint(j, sf::Vector2f{p});
            }
            settings >> pos.x >> pos.y;
            polys[i].setPosition(pos);
            polys[i].setFillColor(sf::Color::White);
            polys[i].setOutlineThickness(3.f);
            polys[i].setOutlineColor(sf::Color::Red);
            // rotation speed
        }
        settings.close();
        return true;
    } else {
        return false;
    }
}

void initializeSettings() {
    if (readFromAvailableText()) {
        std::cout << "hw02.2.txt successfully loaded.\n";
    } else {
        std::cout << "hw02.2.txt not loaded. Using default values.\n";
        resizeVectors(polysCount);
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
        case sf::Keyboard::Space:
            spaceButtonFlag = !spaceButtonFlag;
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
        polys[0].move(v);
    }

    // just prevent the center of mass from getting out
    if(polys[0].getPosition().x < 0) {
        polys[0].setPosition(0, polys[0].getPosition().y);
    }
    if(polys[0].getPosition().x > window_w) {
        polys[0].setPosition(window_w, polys[0].getPosition().y);
    }
    if(polys[0].getPosition().y < 0) {
        polys[0].setPosition(polys[0].getPosition().x, 0);
    }
    if(polys[0].getPosition().y > window_h) {
        polys[0].setPosition(polys[0].getPosition().x, window_h);
    }

    float minX, minY, maxX, maxY;
    sf::Vector2f p;
    for(unsigned int i = 0; i < polysCount; ++i) {
        // rectangles
        if (spaceButtonFlag) {
            polys[i].rotate(i * 0.25f + 0.25f);
        }
        polys[i].setFillColor(sf::Color::White);
        polys[i].setOutlineThickness(3.f);
        polys[i].setOutlineColor(sf::Color::Red);

        p = polys[i].getTransform().transformPoint(polys[i].getPoint(0));
        minX = maxX = p.x;
        minY = maxY = p.y;

        for (unsigned int j = 1; j < polys[i].getPointCount(); ++j) {
            p = polys[i].getTransform().transformPoint(polys[i].getPoint(j));
            minX = std::min(minX, p.x);
            maxX = std::max(maxX, p.x);
            minY = std::min(minY, p.y);
            maxY = std::max(maxY, p.y);
        }

        // bounding boxes
        boundingBoxValues[i] = sf::FloatRect(minX, minY, maxX - minX, maxY - minY);
        boundingBoxEntity[i].setSize(sf::Vector2f(boundingBoxValues[i].width, boundingBoxValues[i].height));
        boundingBoxEntity[i].setPosition(boundingBoxValues[i].left, boundingBoxValues[i].top); 
        boundingBoxEntity[i].setFillColor(sf::Color::Transparent);
        boundingBoxEntity[i].setOutlineThickness(1.0f);
        boundingBoxEntity[i].setOutlineColor(sf::Color::White);
    }

    for(unsigned int i = 0; i < polysCount; ++i) {
        for(unsigned int j = i+1; j < polysCount; ++j) {
            if(boundingBoxValues[i].intersects(boundingBoxValues[j])) {  
                polys[i].setFillColor(sf::Color::Green);
                polys[j].setFillColor(sf::Color::Green);
            }
            if (SAT(polys[i], polys[j])) {
                polys[i].setFillColor(sf::Color::Blue);
                polys[j].setFillColor(sf::Color::Blue);
            }
        }
    }  
}

void render(sf::RenderWindow& window) {
    window.clear(sf::Color::Black);
    for (unsigned int i = 0; i < polysCount; ++i) {
        window.draw(polys[i]);
        window.draw(boundingBoxEntity[i]);
    }
    window.display();
}

int main () {
    srand(time(NULL));
    sf::RenderWindow window(sf::VideoMode(window_w, window_h), "HW02.2");
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

/* unused; reference code from CS179.10 on my HW9 with wil
sf::Vector2f SAT(const std::vector<sf::Vector2f>& a, const std::vector<sf::Vector2f>& b) {
    int aL = a.size();
    int bL = b.size();

    // centroid
    sf::Vector2f cA = zero_vector;
    for (const auto& p : a) {
        cA += p;
    }
    cA /= aL;

    sf::Vector2f cB = zero_vector;
    for (const auto& p : b) {
        cB += p;
    }
    cB /= bL;

    sf::Vector2f AB = cB - cA;

    // axes
    std::vector<sf::Vector2f> axes = {AB};
    for (int i = 1; i < aL; ++i) {
        axes.push(perp(a[i] - a[i-1]));
    }
    axes.push(perp(a[0] - a[aL - 1]))
    for (int i = 1; i < bL; ++i) {
        axes.push(perp(b[i] - b[i-1]));
    }
    axes.push(perp(b[0] - b[bL - 1]))

    // normalization
    for (auto& x : axes) {
        x /= norm(x);
    }

    // overlap
    double overlap = std::numeric_limits<double>::max(); // some big number
    sf::Vector2f axis = limit_vector;
    for (const auto& x : axes) {
        sf::Vector2f intervalA = limit_vector; // some big number
        sf::Vector2f intervalB = limit_vector; // some big number
        for (const auto& p : a) {
            intervalA.x = std::min(intervalA.x, dot(p, x));
            intervalA.y = std::min(intervalA.y, dot(p, x));
        }
        for (const auto& p : b) {
            intervalB.x = std::min(intervalB.x, dot(p, x));
            intervalB.y = std::min(intervalB.y, dot(p, x));
        }

        double cIA = (intervalA.x + intervalA.y) / 2.f;
        double cIB = (intervalB.x + intervalB.y) / 2.f;
        double wIA = std::abs(intervalA.x - intervalA.y) / 2.f;
        double wIB = std::abs(intervalB.x - intervalB.y) / 2.f;
        double result = wIA + wIB - std::abs(cIA - cIB);
        if (result < 0) {
            return limit_vector;
        } else if (result >= 0 && result < overlap) {
            overlap = result;
            axis = x;
        }
    }

    if (axis != limit_vector) {
        if (dot(axis, AB) <= 0) {
            overlap = -overlap;
        }
        axis.x *= overlap;
        axis.y *= overlap;
    }

    return axis;
}

bool checkSATIntersection(const std::vector<sf::Vector2f>& a, const std::vector<sf::Vector2f>& b) {
    sf::Vector2f intersection = SAT(a, b);
    if (intersection != limit_vector) {
        return true;
    } else {
        return false;
    }
}
*/