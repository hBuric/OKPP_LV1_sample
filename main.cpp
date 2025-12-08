/*
==============================================================================
Car Parking Sensor Simulation - Task 2
==============================================================================
Now includes:
 - Car movement using keyboard
 - Sensors move relative to car position
 - Delta-time–based smooth motion
==============================================================================
*/

#include <SFML/Graphics.hpp>
#include <iostream>
#include <SFML/Audio.hpp>



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
	constexpr float CAR_SPEED = 500.0F; // pixels per second
	constexpr float CAR_ROTATION_SPEED = 2.5F;

	// Thresholds for colors
	constexpr float WARNING_THRESHOLD = 60.0F;
	constexpr float DANGER_THRESHOLD = 30.0F;

	float PI = 3.14;

	//THE COLORS OF THE PARKING INDICATOR; TRANSPARENT GREEN AND TRANSPARENT RED
	const sf::Color transGreen = sf::Color(0, 255, 0, 100);
	const sf::Color transRed = sf::Color(255, 0, 0, 100);


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

static float distance(const sf::Vector2f& a, const sf::Vector2f& b) {
	const float dx = b.x - a.x;
	const float dy = b.y - a.y;
	return std::sqrt(dx * dx + dy * dy);
}

static void playBeepIfNear(const std::vector<sf::RectangleShape>& sensors,
	const std::vector<sf::CircleShape>& stupovi,
	sf::Sound& beepSound,
	sf::Clock& beepClock)
{
	const float timeSinceLastBeep = beepClock.getElapsedTime().asSeconds();
	float closestDist = std::numeric_limits<float>::max();

	for (const auto& sensor : sensors) {
		const sf::Vector2f sensorPos = sensor.getPosition();
		for (const auto& stup : stupovi) {
			float dist = distance(sensorPos, stup.getPosition());
			if (dist < closestDist) {
				closestDist = dist;
			}
		}
	}

	float interval = 0.0f;

	if (closestDist <= 80.0f) {
		interval = 0.1f;
	}
	else if (closestDist <= 180.0f) {
		interval = 0.25f;
	}
	else if (closestDist <= 300.0f) {
		interval = 0.5f;
	}

	if (timeSinceLastBeep >= interval) {
		beepSound.play();
		beepClock.restart();
	}
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

	constexpr float START_X = 800.0F;
	constexpr float START_Y = 200.0F;
	constexpr float SPACING = 100.0F;

	for (std::size_t i = 0U; i < constants::SENSOR_COUNT; ++i) {
		sf::RectangleShape sensor({ constants::SENSOR_WIDTH, constants::SENSOR_HEIGHT });
		sensor.setFillColor(sf::Color::Green);
		sensor.setPosition({ START_X + (static_cast<float>(i) * SPACING), START_Y });
		sensors.push_back(sensor);
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



static void updateSensorPositions(std::vector<sf::RectangleShape>& sensors,
	const sf::Sprite& car)
{
	const sf::FloatRect carBounds = car.getGlobalBounds();
	const sf::Vector2f carCenter = car.getPosition();


	constexpr float SENSOR_HALF_WIDTH = constants::SENSOR_WIDTH / 2.0F;
	constexpr float SENSOR_LENGTH = constants::SENSOR_HEIGHT;


	constexpr float DIAGONAL_OFFSET = 10.0F;


	sensors[0].setRotation(sf::degrees(45.0F));

	sensors[0].setPosition({
		carBounds.position.x - SENSOR_HALF_WIDTH - DIAGONAL_OFFSET,
		carBounds.position.y - SENSOR_LENGTH + SENSOR_HALF_WIDTH - DIAGONAL_OFFSET
		});


	sensors[1].setRotation(sf::degrees(315.0F));

	sensors[1].setPosition({
		carBounds.position.x + carBounds.size.x + SENSOR_HALF_WIDTH + DIAGONAL_OFFSET,
		carBounds.position.y - SENSOR_HALF_WIDTH - DIAGONAL_OFFSET
		});


	sensors[2].setRotation(sf::degrees(135.0F));

	sensors[2].setPosition({
		carBounds.position.x - SENSOR_HALF_WIDTH - DIAGONAL_OFFSET,
		carBounds.position.y + carBounds.size.y + SENSOR_HALF_WIDTH + DIAGONAL_OFFSET
		});


	sensors[3].setRotation(sf::degrees(225.0F));

	sensors[3].setPosition({
		carBounds.position.x + carBounds.size.x + SENSOR_HALF_WIDTH + DIAGONAL_OFFSET,
		carBounds.position.y + carBounds.size.y + SENSOR_HALF_WIDTH + DIAGONAL_OFFSET
		});
}

// FUNC TO HANDLE THE PARK INDICATOR
static bool parkOccupied(const sf::FloatRect& carBounds, const sf::FloatRect& parkBounds) {
	const bool isLeftInside = carBounds.position.x >= parkBounds.position.x;

	const bool isRightInside = (carBounds.position.x + carBounds.size.x)
		<= (parkBounds.position.x + parkBounds.size.x);

	const bool isTopInside = carBounds.position.y >= parkBounds.position.y;

	const bool isBottomInside = (carBounds.position.y + carBounds.size.y)
		<= (parkBounds.position.y + parkBounds.size.y);

	return isLeftInside && isRightInside && isTopInside && isBottomInside;
}



// ===============================
// Main Application
// ===============================

int main() {
	// ====================================
	// Window setup
	// ====================================
	sf::SoundBuffer beepBuffer;
	beepBuffer.loadFromFile("assets/beep.mp3");
	sf::Sound beepSound(beepBuffer);
	sf::Music beepMusic;

	if (!beepMusic.openFromFile("assets/beep.mp3")) {
		std::cerr << "NEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE\n";
	}

	sf::RenderWindow window(
		sf::VideoMode({ constants::WINDOW_WIDTH, constants::WINDOW_HEIGHT }),
		"Car Parking Sensor Simulation - Task 2",
		sf::State::Windowed
	);
	window.setFramerateLimit(60U);

	sf::Clock beepClock;



	std::vector<sf::CircleShape> stupovi;
	std::vector<sf::Vector2f> pozicije = {
		{800.f, 500.f},
		{1550.f, 800.f},
		{1810.f, 800.f}
	};

	for (const auto& poz : pozicije) {
		sf::CircleShape stup;
		stup.setRadius(25.f);
		stup.setFillColor(sf::Color::White);
		stup.setPosition(poz);
		stupovi.push_back(stup);
	}

	// ====================================
	// Resource setup
	// ====================================

	// --- SCALING DOWN THE SPRITE ---
	float SCALE_DOWN_FACTOR = 0.30F; // Scale to 15% of original size


	const sf::Texture carTexture = loadTextureOrExit("assets/car_background.png");
	sf::Sprite carSprite(carTexture);




	// Apply the scaling using setScale()

	carSprite.scale({ SCALE_DOWN_FACTOR, SCALE_DOWN_FACTOR });


	centerSprite(carSprite, window);
	carSprite.setPosition(sf::Vector2f(250, 250));
	std::vector<sf::RectangleShape> sensors = createSensorIndicators();
	updateSensorPositions(sensors, carSprite);


	//PARK SENSOR - consts
	constexpr float parkWidth = 200.0F;
	constexpr float parkHeight = 350.0F;
	constexpr float margin = 10.0F;

	//Set height/witdh, color and border thickness of the parking indicator
	sf::RectangleShape parkIndicator({ parkWidth, parkHeight });
	parkIndicator.setFillColor(constants::transGreen);
	parkIndicator.setOutlineColor(sf::Color::White);
	parkIndicator.setOutlineThickness(2.0F);

	//Draw the park indicator
	parkIndicator.setPosition({ static_cast<float>(constants::WINDOW_WIDTH) - parkWidth - margin, margin });



	// ====================================
	// Main loop
	// ====================================
	sf::Clock clock;

	while (window.isOpen()) {
		const float deltaTime = clock.restart().asSeconds();

		// Movement logic
		sf::Vector2f movement{ 0.0F, 0.0F };

		float rotationChange = 0.0F;

		// ---- Handle events ----
		while (auto event = window.pollEvent()) {
			if (event->is<sf::Event::Closed>()) {
				window.close();
			}

			// Window closed or escape key pressed: exit
			if ((event->is<sf::Event::KeyPressed>() &&
				event->getIf<sf::Event::KeyPressed>()->code == sf::Keyboard::Key::Space))
				std::cout << "KeyPressed event has occured, key pressed is: Space\n";
		}

		// ---- Update logic ---

		sf::Angle rotation;


		float currentRotationDeg = carSprite.getRotation().asDegrees();
		float currentRotationRad = currentRotationDeg * (constants::PI / 180.0F);

		const float forwardX = cos(currentRotationRad);
		const float forwardY = sin(currentRotationRad);


		const float distancePerFrame = constants::CAR_SPEED * deltaTime;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Up)) {

			movement.x += forwardX * distancePerFrame;
			movement.y += forwardY * distancePerFrame;
		}


		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Down)) {
			movement.x -= forwardX * distancePerFrame;
			movement.y -= forwardY * distancePerFrame;

			//movement -= forward * (constants::CAR_SPEED * deltaTime);
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Left)) {
			rotation = sf::degrees(-constants::CAR_ROTATION_SPEED);

		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Right)) {
			rotation = sf::degrees(constants::CAR_ROTATION_SPEED);
		}

		carSprite.rotate(rotation);
		carSprite.move(movement);

		updateSensorPositions(sensors, carSprite);
		playBeepIfNear(sensors, stupovi, beepSound, beepClock);



		// ---- Optional logic ----
		if (sensors[0].getPosition().y <= 0) {
			sensors[0].setFillColor(sf::Color::Red);
		}
		else {
			sensors[0].setFillColor(sf::Color::Green);
		}
		if (sensors[2].getPosition().x <= (constants::SENSOR_HEIGHT)) {
			sensors[2].setFillColor(sf::Color::Red);
		}
		else {
			sensors[2].setFillColor(sf::Color::Green);
		}

		// ---- Rendering ----
		window.clear(sf::Color(30, 30, 30));
		window.draw(carSprite);

		//DRAW THE PARK INDICATOR
		window.draw(parkIndicator);

		//PARKING INDICATION - GET LOCATION OF THE CAR AND THE INDICATOR
		const sf::FloatRect carBounds = carSprite.getGlobalBounds();
		const sf::FloatRect parkBounds = parkIndicator.getGlobalBounds();

		//CHANGE COLOR OF PARK INDICATOR; ON OCCUPATION
		if (parkOccupied(carBounds, parkBounds)) {
			parkIndicator.setFillColor(constants::transRed);
		}
		else {
			parkIndicator.setFillColor(constants::transGreen);
		}


		//UNCOMMENT IF YOU NEED TO HAVE PARK SENSORS AROUND THE CAR
		//for (const auto& sensor : sensors) {
			//window.draw(sensor);
		//}
		for (auto& s : stupovi) {
			window.draw(s);
		}
		window.display();
	}

	return 0;
}