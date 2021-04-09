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
    constexpr unsigned int curve_order{2};
    constexpr float c_radius{10.f};
    constexpr float smoothness{10.f};
    constexpr unsigned int control_points{3};
    constexpr int curves = (control_points-1)/2;
    constexpr int points = curves * smoothness + 1;
    constexpr int tanNorm{3};
    constexpr int tan_points = curves * tanNorm + 1;
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
sf::Vector2<T> lerp(const sf::Vector2<T>& v0, const sf::Vector2<T>& v1, T t) {
	return v0 + ((v1 - v0) * t);
}

template <typename T>
sf::Vector2<T> make_curve(const sf::Vector2<T>& v0, const sf::Vector2<T>& v1, const sf::Vector2<T>& v2, T t) {
	return lerp(lerp(v0, v1, t), lerp(v1, v2, t), t);
}

// enumerations
enum Direction {up, down, left, right};

// globals
unsigned int window_w{default_vals::window_w};
unsigned int window_h{default_vals::window_h};
int curve_order{default_vals::curve_order};
float c_radius{default_vals::c_radius};
float smoothness{default_vals::smoothness};
int control_points{default_vals::control_points};
int curves{default_vals::curves};
int points{default_vals::points};
float inv_smoothness{1.f/smoothness};
int tanNorm{default_vals::tanNorm};
int tan_points{default_vals::tan_points};
float inv_tanNorm{1.f/tanNorm};

bool directionFlags[4] = {false, false, false, false};
bool leftMouseButtonFlag = false;

std::vector<sf::CircleShape> circles;
sf::VertexArray ctrlPoints{sf::LineStrip};
sf::VertexArray allPoints{sf::LineStrip};
sf::VertexArray tanPoints{sf::Lines};
sf::VertexArray normalPoints{sf::Lines};
std::vector<bool> circlesFlags;
std::vector<std::vector<long long>> pascal;
std::vector<std::vector<float>> poly_coefs;
std::vector<std::vector<float>> tsrc_coefs;
std::vector<std::vector<float>> tang_coefs;

void updatePascalTriangle(unsigned int order) {
    pascal.resize(order+1);
    for (unsigned int i = 0; i <= order; ++i) {
        pascal[i].resize(i+1);
        pascal[i][0] = pascal[i][i] = 1;
        for (unsigned int j = 1; j < i; ++j) {
            pascal[i][j] = pascal[i-1][j-1] + pascal[i-1][j];
        }
    }
}

void updatePolyCoefs(unsigned int level, unsigned int order) {
    updatePascalTriangle(order);
    poly_coefs.resize(level+1);
    for (unsigned int i = 0; i <= level; ++i) {
        poly_coefs[i].resize(order+1);
        float g = inv_smoothness * i;
        float f = 1.f - g;
        for (unsigned int j = 0; j <= order; ++j) {
            poly_coefs[i][j] = pascal[order][j] * std::pow(f, order - j) * std::pow(g, j);
        }
    }
}

void updateTangCoefs(unsigned int level, unsigned int order) {
    tang_coefs.resize(level);
    tsrc_coefs.resize(level);
    for (unsigned int i = 0; i < level; ++i) {
        tang_coefs[i].resize(order);
        tsrc_coefs[i].resize(order + 1);
        float g = inv_tanNorm * i;
        float f = 1.f - g;
        for (unsigned int j = 0; j <= order - 1; ++j) {
            tang_coefs[i][j] = pascal[order - 1][j] * std::pow(f, order - 1 - j) * std::pow(g, j);
            tang_coefs[i][j] *= (order);
        }

        for (unsigned int j = 0; j <= order; ++j) {
            tsrc_coefs[i][j] = pascal[order][j] * std::pow(f, order - j) * std::pow(g, j);
        }
    }
}

void updateVertexPoint(int idx) {
    for (unsigned int i = 0; i <= smoothness; ++i) {
        allPoints[idx * smoothness + i] = zero_vector;
        for (unsigned int j = 0; j <= curve_order; ++j) {
            // sums the polynomial
            allPoints[idx * smoothness + i].position +=
                ctrlPoints[idx * curve_order + j].position * poly_coefs[i][j];    
        }
    }
}

void updateTangentPoint(int idx) {
    sf::Vector2f v, vn;
    int tanPtLoc, ptLoc;
    for (unsigned int i = 0; i < tanNorm; ++i) {
        tanPtLoc = (idx * tanNorm + i) * 2;
        ptLoc = idx * curve_order;

        tanPoints[tanPtLoc] = zero_vector;
        for (unsigned int j = 0; j <= curve_order; ++j) {
            tanPoints[tanPtLoc].position += (ctrlPoints[ptLoc + j].position) * tsrc_coefs[i][j];
        }
        normalPoints[tanPtLoc].position = tanPoints[tanPtLoc].position;

        // TODO: computation bug somewhere
        tanPoints[tanPtLoc+1] = zero_vector;
        for (unsigned int j = 0; j <= curve_order - 1; ++j) {
            tanPoints[tanPtLoc+1].position += (ctrlPoints[ptLoc + j + 1].position) * tang_coefs[i][j];
            tanPoints[tanPtLoc+1].position -= (ctrlPoints[ptLoc + j].position) * tang_coefs[i][j];
        }

        // normalizing the line
        v = tanPoints[tanPtLoc+1].position;
        v /= std::hypot(v.x, v.y);
        v *= 20.f;
        vn.x = -v.y;
        vn.y = v.x;
        tanPoints[tanPtLoc+1].position = tanPoints[tanPtLoc].position + v;
        normalPoints[tanPtLoc+1].position = normalPoints[tanPtLoc].position + vn;

        // setting the color of the lines
        tanPoints[tanPtLoc].color = sf::Color::Red;
        tanPoints[tanPtLoc+1].color = sf::Color::Red;
        normalPoints[tanPtLoc].color = sf::Color::Blue;
        normalPoints[tanPtLoc+1].color = sf::Color::Blue;
    }
}

bool readFromAvailableText() {
    std::string input;
    std::ifstream settings("hw05.txt");
    if (settings.is_open()) {
        settings >> curve_order;
    	settings >> smoothness;
        settings >> tanNorm;
    	settings >> control_points;
    	circles.resize(control_points);
    	float x, y;
    	for (unsigned int i = 0; i < control_points; ++i) {
    		settings >> x >> y;
    		circles[i].setRadius(c_radius);
    		circles[i].setOrigin(c_radius, c_radius);
    		circles[i].setPosition(x, y);
    	}
        settings.close();
        return true;
    } else {
        return false;
    }
}

void initializeSettings() {
    if (readFromAvailableText()) {
        std::cout << "hw05.txt successfully loaded.\n";
    } else {
        std::cout << "hw05.txt not loaded. Using default values.\n";
        circles.resize(control_points);
    	for (unsigned int i = 0; i < control_points; ++i) {
    		circles[i].setRadius(c_radius);
    		circles[i].setOrigin(c_radius, c_radius);
    		circles[i].setPosition(1.f * window_w / control_points * i + c_radius, window_h / 2.f); 
    	}
    }

    inv_smoothness = 1.f/smoothness;
    curves = (control_points-1)/curve_order;
    points = curves * smoothness + 1;
    tan_points = (curves * tanNorm) * 2;
    inv_tanNorm = 1.f / (tanNorm - 1);
    ctrlPoints.resize(control_points);
    allPoints.resize(points);
    tanPoints.resize(tan_points);
    normalPoints.resize(tan_points);
    circlesFlags.resize(control_points);

    updatePolyCoefs(smoothness, curve_order);
    updateTangCoefs(tanNorm, curve_order);

    for (unsigned int i = 0; i < control_points; ++i) {
    	ctrlPoints[i].position = circles[i].getPosition();
    	circles[i].setFillColor(sf::Color::Transparent);
    	circles[i].setOutlineColor(sf::Color::Green);
    	circles[i].setOutlineThickness(2.f);
    	circlesFlags[i] = false;
    }

    for (unsigned int i = 0; i < curves; ++i) {
    	updateVertexPoint(i);
        updateTangentPoint(i);
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

    sf::Vector2f mousePosition = sf::Vector2f(sf::Mouse::getPosition(window));
    sf::Vector2f placeholder;

    if (leftMouseButtonFlag) {
    	for (int i = 0; i < control_points; ++i) {
    		placeholder = circles[i].getPosition() - mousePosition;
    		float dist = std::hypot(placeholder.x, placeholder.y);
    		if (dist < c_radius) {
    			ctrlPoints[i].position = mousePosition;
    			circles[i].setPosition(mousePosition);
    			circlesFlags[i] = true;
                // TODO: updateTangentPoint lazily
    			if (i%2 == 0) {
    				updateVertexPoint(std::max(i/int(curve_order) - 1, 0));
                    updateTangentPoint(std::max(i/int(curve_order) - 1, 0));
    			}
    			updateVertexPoint(std::min(i/int(curve_order), int(curves)-1));
                updateTangentPoint(std::min(i/int(curve_order), int(curves)-1));
    			break;
    		}
    	}
    }
}

void render(sf::RenderWindow& window) {
    window.clear(sf::Color::Black);
    for (unsigned int i = 0; i < control_points; ++i) {
    	window.draw(circles[i]);
    }
    window.draw(allPoints);
    window.draw(tanPoints);
    window.draw(normalPoints);
    window.display();
}

int main () {
    srand(time(NULL));
    sf::RenderWindow window(sf::VideoMode(window_w, window_h), "HW05");
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