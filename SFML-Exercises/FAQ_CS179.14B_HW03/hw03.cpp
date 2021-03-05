#include <iostream>
#include <math.h>
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
    constexpr float c_radius{10.f};
    constexpr float smoothness{10.f};
    constexpr unsigned int control_points{3};
    constexpr int curves = (control_points-1)/2;
    constexpr int points = curves * smoothness + 1;
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
float c_radius{default_vals::c_radius};
float smoothness{default_vals::smoothness};
unsigned int control_points{default_vals::control_points};
int curves{default_vals::curves};
int points{default_vals::points};
float inv_smoothness{1.f/smoothness};

bool directionFlags[4] = {false, false, false, false};
bool leftMouseButtonFlag = false;


std::vector<sf::CircleShape> circles;
sf::VertexArray ctrlPoints{sf::LineStrip};
sf::VertexArray allPoints{sf::LineStrip};
std::vector<bool> circlesFlags;

void updateVertexPoint(int idx) {
	for (unsigned int i = 0; i <= smoothness; ++i) {
		allPoints[idx * smoothness + i].position = 
		make_curve(ctrlPoints[idx * 2].position,
			ctrlPoints[idx * 2 + 1].position,
			ctrlPoints[idx * 2 + 2].position,
			inv_smoothness * i);
	}
}

bool readFromAvailableText() {
    std::string input;
    std::ifstream settings("hw03.txt");
    if (settings.is_open()) {
    	settings >> smoothness;
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
        std::cout << "hw03.txt successfully loaded.\n";
    } else {
        std::cout << "hw03.txt not loaded. Using default values.\n";
        circles.resize(control_points);
    	for (unsigned int i = 0; i < control_points; ++i) {
    		circles[i].setRadius(c_radius);
    		circles[i].setOrigin(c_radius, c_radius);
    		circles[i].setPosition(1.f * window_w / control_points * i + c_radius, window_h / 2.f); 
    	}
    }

    inv_smoothness = 1.f/smoothness;
    curves = (control_points-1)/2;
    points = curves * smoothness + 1;
    ctrlPoints.resize(control_points);
    allPoints.resize(points);
    circlesFlags.resize(control_points);

    for (unsigned int i = 0; i < control_points; ++i) {
    	ctrlPoints[i].position = circles[i].getPosition();
    	circles[i].setFillColor(sf::Color::Transparent);
    	circles[i].setOutlineColor(sf::Color::Green);
    	circles[i].setOutlineThickness(2.f);
    	circlesFlags[i] = false;
    }

    for (unsigned int i = 0; i < curves; ++i) {
    	updateVertexPoint(i);
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
    			if (i%2 == 0) {
    				updateVertexPoint(std::max(i/2 - 1, 0));
    			}
    			updateVertexPoint(std::min(i/2, curves-1));
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
    window.display();
}

int main () {
    srand(time(NULL));
    sf::RenderWindow window(sf::VideoMode(window_w, window_h), "HW03");
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