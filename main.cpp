/*
==============================================================================
Car Parking Sensor Simulation
==============================================================================
Educational example showing:
 - SFML rendering and sound playback
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
	constexpr float CAR_SPEED = 300.0F;
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
		// MISRA: Avoid abrupt termination, but here it’s educational
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

static void sensorPositionUpdate(std::vector<sf::RectangleShape>& sensors,
	const sf::Sprite& car) {
	const sf::FloatRect carBounds = car.getGlobalBounds();
	const sf::Vector2f carCenter = car.getPosition();

	// Left rear sensor
	sensors[0].setRotation(sf::degrees(45.0F));
	sensors[0].setPosition({
		carCenter.x - (constants::SENSOR_WIDTH * 16),
		carBounds.position.y - (constants::SENSOR_HEIGHT)
		});

	//Left front sensor
	sensors[1].setRotation(sf::degrees(-45.0F));
	sensors[1].setPosition({ 
		carCenter.x + (constants::SENSOR_WIDTH * 16),
		carBounds.position.y - (constants::SENSOR_HEIGHT)
		});

	//Rear right sensor
	sensors[2].setRotation(sf::degrees(135.0F));
	sensors[2].setPosition({
		carCenter.x - (constants::SENSOR_WIDTH * 16),
		carBounds.position.y + (constants::SENSOR_HEIGHT * 5)
		});

	// Right side sensor
	sensors[3].setRotation(sf::degrees(-135.0F));
	sensors[3].setPosition({
			carCenter.x + (constants::SENSOR_WIDTH * 16),
		carBounds.position.y + (constants::SENSOR_HEIGHT * 5)
		});
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

	const sf::SoundBuffer beepBuffer = loadSoundOrExit("assets/beep.mp3");
	sf::Sound beep(beepBuffer);
	float distance = constants::DISTANCE_MAX;

	// ====================================
	// Main loop
	// ====================================

	sf::Clock clock;

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
		
		
		//Movement logic

		const float deltaTime = clock.restart().asSeconds();
		 
		sf::Vector2f movement{ 0.0F, 0.0F };
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Up)) {
			movement.y -= constants::CAR_SPEED * deltaTime;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Down)) {
			movement.y += constants::CAR_SPEED * deltaTime;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Left)) {
			movement.x -= constants::CAR_SPEED * deltaTime;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Right)) {
			movement.x += constants::CAR_SPEED * deltaTime;
		}
		
		carSprite.move(movement);
		sensorPositionUpdate(sensors, carSprite);


		// Determine sensor color based on distance
		sf::Color sensorColor = sf::Color::Green;
		if (distance < constants::WARNING_THRESHOLD) {
			sensorColor = sf::Color::Yellow;
		}
		if (distance < constants::DANGER_THRESHOLD) {
			sensorColor = sf::Color::Red;
		}

		for (auto& sensor : sensors) {
			sensor.setFillColor(sensorColor);
		}
			
		
		// Sound logic (play warning sound when too close)
		if ((distance < constants::DANGER_THRESHOLD) &&
			(beep.getStatus() != sf::Sound::Status::Playing)) {
			beep.play();
		}

		// ---- Rendering ----
		window.clear(sf::Color::Black);
		window.draw(carSprite);

		for (const auto& sensor : sensors) {
			window.draw(sensor);
		}

		window.display();

		

	}

	
	return 0;
}

