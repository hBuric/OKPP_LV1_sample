/*
==============================================================================
Car Parking Sensor Simulation
==============================================================================
Educational example showing :
-SFML rendering and sound playback
- Clean code organization
- MISRA C++ 2008 / 2023 compliance guidelines
- Safe coding and maintainability practices
==============================================================================
*/

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

// ===============================
// Constants
// ===============================
std::vector<float> x_positions = { 400.0F, 1500.0F, 400.0F, 1500.0F };
std::vector<float> y_positions = { 200.0F, 200.0F, 800.0F, 800.0F };

struct ParkingSpot {
	std::string name;          // empty parking
	sf::RectangleShape shape;  //vizual
};

namespace constants {
	// Window dimensions should be constexpr and have explicit types
	constexpr unsigned int WINDOW_WIDTH = 1920U;    // MISRA: use unsigned for sizes
	constexpr unsigned int WINDOW_HEIGHT = 1080U;

	// Parking sensor rectangle dimensions
	constexpr float SENSOR_WIDTH = 30.0F;
	constexpr float SENSOR_HEIGHT = 100.0F;

	// Simulation and layout
	constexpr std::size_t SENSOR_COUNT = 4U;
	constexpr float DISTANCE_MAX = 100.0F;
	constexpr float DISTANCE_MIN = 10.0F;
	constexpr float DISTANCE_DECREASE = 0.5F;

	// Thresholds for colors
	constexpr float WARNING_THRESHOLD = 60.0F;
	constexpr float DANGER_THRESHOLD = 30.0F;

}

// ===============================
// Utility Functions
// ===============================

/**
 * @brief Centers a sprite in the given render window.
 *
 * MISRA C++: Always use const references for read-only parameters.
 *            Avoid raw pointers unless necessary.
 */
static void centerSprite(sf::Sprite& sprite, const sf::RenderWindow& window) {
	const sf::FloatRect bounds = sprite.getLocalBounds();

	// MISRA: Always use floating-point literals with suffix 'F'
	sprite.setOrigin({ bounds.size.x / 2.0F, bounds.size.y / 2.0F });
	sprite.setPosition({
		static_cast<float>(window.getSize().x) / 2.0F,
		static_cast<float>(window.getSize().y) / 2.0F
		});
}


/**
 * @brief Creates a vector of rectangular parking sensor indicators.
 *
 * MISRA: Functions should have single responsibility.
 *        Avoid global mutable data.
 */
static std::vector<sf::RectangleShape> createSensorIndicators() {
	
	std::vector<sf::RectangleShape> sensors;
	sensors.reserve(constants::SENSOR_COUNT); // Avoid dynamic reallocations

	sf::Angle buttonAngle = sf::degrees(135);
	sf::Angle buttonAngle2 = sf::degrees(90);

	sf::Angle topAngle = sf::degrees(45);
	sf::Angle topAngle2 = sf::degrees(90);

	constexpr float START_X = 400.0F;
	constexpr float START_Y = 200.0F;
	constexpr float SPACING = 1100.0F;
	constexpr float START_Y2 = 800.0F;


	for (std::size_t i = 0U; i < constants::SENSOR_COUNT; ++i) {
		sf::RectangleShape sensor({ constants::SENSOR_WIDTH, constants::SENSOR_HEIGHT });
		sensor.setFillColor(sf::Color::Green);

		if (i >= 2) {
			sensor.setPosition({ START_X + ((static_cast<float>(i) - 2) * SPACING), START_Y2 });
			buttonAngle = buttonAngle - buttonAngle2;
			sensor.setRotation(buttonAngle - buttonAngle2);
			sensors.push_back(sensor);
		}
		else {
			sensor.setPosition({ START_X + (static_cast<float>(i) * SPACING), START_Y });
			topAngle = topAngle - topAngle2;
			sensor.setRotation(topAngle - topAngle2);
			sensors.push_back(sensor);
		}

	}
	return sensors; // Return by value (NRVO applies)
}

std::array<sf::Vector2f, 4> getCorners(const sf::RectangleShape& r) {
	const sf::Transform& t = r.getTransform();
	sf::Vector2f size = r.getSize();

	return {
		t.transformPoint({0.f,      0.f}),
		t.transformPoint({size.x,   0.f}),
		t.transformPoint({size.x,   size.y}),
		t.transformPoint({0.f,      size.y})
	};
}
bool isCarParked(sf::FloatRect parkingBounds, const std::array<sf::Vector2f, 4>& corners) {

	for (const auto& corner : corners) {
		if (!parkingBounds.contains(corner)) {
			return false;
		}
	}
	return true;
}
void addParkingSpot(std::vector<ParkingSpot>& spots, const std::string& name, const sf::Vector2f& position, const sf::Vector2f& size = { 360.f, 550.f }, const sf::Color& color = sf::Color(255, 255, 0, 100)) {
	ParkingSpot spot;
	spot.name = name;

	spot.shape.setSize(size);
	spot.shape.setFillColor(color);
	spot.shape.setOrigin(sf::Vector2f(0.f, 0.f));   //top left corner
	spot.shape.setPosition(position);

	spots.push_back(spot);
}

/**
 * @brief Loads a texture safely and logs any error.
 *
 * MISRA: Error handling must be explicit and deterministic.
 */
[[nodiscard]] static sf::Texture loadTextureOrExit(const std::string& path) {
	sf::Texture texture;
	if (!texture.loadFromFile(path)) {
		std::cerr << "Error: Failed to load texture from "
			<< std::filesystem::absolute(path) << '\n';
		// MISRA: Avoid abrupt termination, but here itâ€™s educational
	}
	return texture;
}

/**
 * @brief Loads a sound buffer safely and logs any error.
 */
[[nodiscard]] static sf::SoundBuffer loadSoundOrExit(const std::string& path) {
	sf::SoundBuffer buffer;
	if (!buffer.loadFromFile(path)) {
		std::cerr << "Error: Failed to load sound from "
			<< std::filesystem::absolute(path) << '\n';
	}
	return buffer;
}

// ===============================
// Main Application
// ===============================

int main() {
	// ====================================
	// Window setup
	// ====================================

	sf::RenderWindow window(
		sf::VideoMode({ constants::WINDOW_WIDTH, constants::WINDOW_HEIGHT }),
		"Car Parking Sensor Simulation",
		sf::State::Windowed
	);
	window.setFramerateLimit(60U);

	// ====================================
	// Resource setup
	// ====================================

	const sf::Texture carTexture = loadTextureOrExit("assets/car_background.png");
	sf::Sprite carSprite(carTexture);
	centerSprite(carSprite, window);

	std::vector<sf::RectangleShape> sensors = createSensorIndicators();

	float distance = constants::DISTANCE_MAX;
	constexpr float edge = 50.0F;

	sf::Vector2f carCenter = carSprite.getPosition();
	sf::Vector2u windowSize = window.getSize();

	std::vector<sf::Vector2f> sensorOffsets;

	for (std::size_t i = 0; i < constants::SENSOR_COUNT; ++i) {
		sensorOffsets.push_back({ x_positions[i] - carCenter.x, y_positions[i] - carCenter.y });

	}

	sf::Vector2f scale(0.5f, 0.5f);
	carSprite.setScale(scale);
	// ====================================
	// Main loop
	// ====================================

	while (window.isOpen()) {
		// ---- Handle events ----
		while (auto event = window.pollEvent()) {
			if (event->is<sf::Event::Closed>()) {
				window.close();
			}
		}

		// ---- Update logic ----
		distance -= constants::DISTANCE_DECREASE;
		if (distance < constants::DISTANCE_MIN) {
			distance = constants::DISTANCE_MAX;
		}
		sf::Vector2f movement(0.f, 0.f);





		float rotationSpeed = 1.0f;

		float speed = 7.0f;

		float angleDeg = carSprite.getRotation().asDegrees();
		float angleRad = angleDeg * 3.14159265f / 180.f;


		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
			carSprite.rotate(sf::degrees(-rotationSpeed));
		}


		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
			carSprite.rotate(sf::degrees(rotationSpeed));
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
		{
			movement.x += std::cos(angleRad) * speed;
			movement.y += std::sin(angleRad) * speed;
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
		{
			movement.x -= std::cos(angleRad) * speed;
			movement.y -= std::sin(angleRad) * speed;
		}


		sf::Vector2f carPos = carSprite.getPosition();
		for (std::size_t i = 0; i < sensors.size(); ++i) {
			sensors[i].setPosition(carPos + sensorOffsets[i]);
		}


		carSprite.move(movement);
		
		std::vector<ParkingSpot> parkingSpots;
		addParkingSpot(parkingSpots, "Parking1", { 0.f, 0.f });
		addParkingSpot(parkingSpots, "Parking2", { 800.f, 530.f });
		addParkingSpot(parkingSpots, "Parking3", { 1560.f, 0.f });



		// ---- Rendering ----
		window.clear(sf::Color::Black);

		sf::RectangleShape carBounds({ carTexture.getSize().x * scale.x, carTexture.getSize().y * scale.y });
		carBounds.setOrigin(sf::Vector2(carBounds.getSize().x / 2.f, carBounds.getSize().y / 2.f));

		carBounds.setPosition(carSprite.getPosition());

		carBounds.setRotation(sf::degrees(carSprite.getRotation().asDegrees()));


		auto corners = getCorners(carBounds);
		bool parked = false;
		std::string parkedAt = "";

		for (const auto& spot : parkingSpots) {
			sf::FloatRect bounds = spot.shape.getGlobalBounds();

			if (isCarParked(bounds, corners)) {
				parked = true;
				parkedAt = spot.name;
				break;
			}
		}
		
		if (parked) {

			for (auto& spot : parkingSpots) {
				sf::FloatRect bounds = spot.shape.getGlobalBounds();

				if (isCarParked(bounds, corners)) {
					if (spot.name == parkedAt) {
						spot.shape.setFillColor(sf::Color(0, 255, 0, 100));
					}
				}
			}
		}
		else {
			for (auto& spot : parkingSpots) {
				sf::FloatRect bounds = spot.shape.getGlobalBounds();

				if (!isCarParked(bounds, corners)) {
					if (spot.name == parkedAt) {
						spot.shape.setFillColor(sf::Color(255, 255, 0, 100));
					}
				}
			}
		}

		for (const auto& corner : corners) {
			sf::CircleShape point(3.f);
			point.setFillColor(sf::Color::Green);
			point.setOrigin(sf::Vector2(5.f, 5.f));
			point.setPosition(corner);
			window.draw(point);
		}
		for (const auto& spot : parkingSpots) {
			window.draw(spot.shape);
		}
		window.draw(carSprite);
		window.display();
	}

	return 0;
}