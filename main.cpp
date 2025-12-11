#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <map>
#include <set>
#include <functional>
#include <iomanip>
#include <random>
#include <SFML/Audio/Music.hpp>

//I'm really sorry for the messy code, I would've loved to make it properly structured with more time!

//One known bug is interaction with a drawer, choice to place item inside is available before actually opening a drawer.

// Function to generate three unique random numbers, including one correct number, for one of the minigames
std::vector<int> generateUniqueRandomChoices(int& correctNumber, int rangeStart, int rangeEnd) {
    std::set<int> choices;

    // Generate a random correct number
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(rangeStart, rangeEnd);
    correctNumber = dist(gen);
    choices.insert(correctNumber);

    // Generate two more unique random numbers
    while (choices.size() < 3) {
        choices.insert(dist(gen));
    }

    return std::vector<int>(choices.begin(), choices.end());
}

// Non-passable tiles
std::set<int> nonPassableTiles = {1, 2, 3, 4, 5,6,7,8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34}; // Non-passable tiles

const int TILE_SIZE = 32;
int MAP_WIDTH = 0;
int MAP_HEIGHT = 0;

std::vector<std::vector<int>> mapData;
std::vector<std::string> mapFiles = {"maps/map1.txt"};
int currentMapIndex = 0;
int drawerItemCount = 0; // for the ending choice

void incrementDrawerItemCount() {
    drawerItemCount++;
}

bool tile11Completed;
bool tile13Completed;
bool tile14Completed;
bool drawerOpened;

struct SaveSlot {
    bool isUsed;
    std::string fileName;
    float playTime; // Total playtime in seconds
    int mapIndex; // Current map index
    sf::Vector2i playerPosition; // Player's grid position
    std::vector<std::pair<std::string, int>> inventory; // Inventory items
};



// Format time
std::string formatTime(float seconds) {
    int hours = static_cast<int>(seconds) / 3600;
    int minutes = (static_cast<int>(seconds) % 3600) / 60;
    int secs = static_cast<int>(seconds) % 60;

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << hours << ":"
        << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::setfill('0') << std::setw(2) << secs;
    return oss.str();
}

class Animation
{
public:
    Animation(const sf::Texture &texture, int frameWidth, int frameHeight)
        : m_texture(texture), m_frameWidth(frameWidth), m_frameHeight(frameHeight),
          m_currentFrame(1), m_direction(0), m_isMoving(false)
    {
        m_sprite.setTexture(texture);
        updateTextureRect();
    }

    void update(float deltaTime)
    {
        if (m_isMoving)
        {
            m_elapsedTime += deltaTime;

            if (m_elapsedTime >= 0.15f)
            {
                m_elapsedTime = 0.f;

                if (m_currentFrame == 0)
                    m_currentFrame = 1;
                else if (m_currentFrame == 1)
                    m_currentFrame = 2;
                else if (m_currentFrame == 2)
                    m_currentFrame = 1;

                updateTextureRect();
            }
        }
    }

    void setDirection(int direction)
    {
        m_direction = direction;
        updateTextureRect();
    }

    void setMoving(bool isMoving)
    {
        m_isMoving = isMoving;
        if (!m_isMoving)
            m_currentFrame = 1;
        updateTextureRect();
    }

    void draw(sf::RenderWindow &window, sf::Vector2f position) const {
        m_sprite.setPosition(position);
        window.draw(m_sprite);
    }

    sf::Vector2i getFacingDirection() const
    {
        switch (m_direction)
        {
        case 0:
            return {0, 1}; // Down
        case 1:
            return {-1, 0}; // Left
        case 2:
            return {1, 0}; // Right
        case 3:
            return {0, -1}; // Up
        default:
            return {0, 0};
        }
    }

private:
    void updateTextureRect()
    {
        int top = m_direction * m_frameHeight;
        int left = m_currentFrame * m_frameWidth;
        m_sprite.setTextureRect(sf::IntRect(left, top, m_frameWidth, m_frameHeight));
    }

    mutable sf::Sprite m_sprite;
    const sf::Texture &m_texture;
    int m_frameWidth, m_frameHeight;
    int m_currentFrame, m_direction;
    float m_elapsedTime = 0.f;
    bool m_isMoving;
};

struct Inventory {
    std::vector<std::pair<std::string, int>> items;

    void addItem(const std::string &item, int quantity = 1) {
        for (auto &existingItem : items) {
            if (existingItem.first == item) {
                existingItem.second += quantity;
                return;
            }
        }
        items.emplace_back(item, quantity);
    }
};

int displaySaveScreen(sf::RenderWindow &window, sf::Font &font, sf::Texture &player_Sprite,
                      std::vector<SaveSlot> &saveSlots, float currentPlayTime,
                      const sf::Vector2i &gridPosition, const Inventory &inventory){
    sf::RectangleShape background(sf::Vector2f(800, 600));
    background.setFillColor(sf::Color(240, 240, 240));

    sf::Text title("Save Your Soul's Data?", font, 28);
    title.setPosition(50, 20);
    title.setFillColor(sf::Color::Black);

    sf::Sprite playerSprite(player_Sprite);
    playerSprite.setScale(1.0f, 1.0f);

    int selectedSlot = 0;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Up) {
                    selectedSlot = (selectedSlot - 1 + saveSlots.size()) % saveSlots.size();
                } else if (event.key.code == sf::Keyboard::Down) {
                    selectedSlot = (selectedSlot + 1) % saveSlots.size();
                } if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Z) {
                    saveSlots[selectedSlot].isUsed = true;
                    saveSlots[selectedSlot].playTime = currentPlayTime;
                    saveSlots[selectedSlot].mapIndex = currentMapIndex;
                    saveSlots[selectedSlot].playerPosition = gridPosition;
                    saveSlots[selectedSlot].inventory = inventory.items;

                    // Open the save file
                    std::ofstream saveFile(saveSlots[selectedSlot].fileName);
                    if (saveFile.is_open()) {
                        // Save basic information
                        saveFile << currentPlayTime << " "
                                 << currentMapIndex << " "
                                 << gridPosition.x << " " << gridPosition.y << "\n";

                        // Save tile flags
                        saveFile << (tile11Completed ? 1 : 0) << " "
                                 << (tile13Completed ? 1 : 0) << " "
                                 << (tile14Completed ? 1 : 0) << "\n";

                        // Save drawerOpened state
                        saveFile << (drawerOpened ? 1 : 0) << "\n";

                        // Save inventory
                        saveFile << inventory.items.size() << "\n";
                        for (const auto &item : inventory.items) {
                            std::string itemName = item.first;
                            std::replace(itemName.begin(), itemName.end(), ' ', '_'); // Replace spaces with underscores for proper saving of items with 2 words
                            saveFile << itemName << " " << item.second << "\n";
                        }

                        saveFile.close();
                        std::cout << "Game saved to slot " << selectedSlot + 1 << "\n";
                    } else {
                        std::cerr << "Error: Could not open save file for writing.\n";
                    }
                    return selectedSlot;
                }


                else if (event.key.code == sf::Keyboard::Escape) {
                    return -1; // Exit save menu
                }
            }
        }

        // Draw save menu
        window.clear();
        window.draw(background);
        window.draw(title);

        for (size_t i = 0; i < saveSlots.size(); ++i) {
            float slotY = 100 + i * 100;
            sf::RectangleShape slotBackground(sf::Vector2f(700, 80));
            slotBackground.setPosition(50, slotY);
            slotBackground.setFillColor(i == selectedSlot ? sf::Color(200, 200, 200) : sf::Color(255, 255, 255));
            slotBackground.setOutlineThickness(2);
            slotBackground.setOutlineColor(sf::Color::Black);

            sf::Text slotText("", font, 20);
            slotText.setPosition(60, slotY + 20);
            slotText.setFillColor(sf::Color::Black);

            if (saveSlots[i].isUsed) {
                slotText.setString("File " + std::to_string(i + 1) + "\nTime: " + formatTime(saveSlots[i].playTime));
            } else {
                slotText.setString("File " + std::to_string(i + 1) + "\n[Empty]");
            }

            playerSprite.setPosition(700, slotY + 20);

            window.draw(slotBackground);
            window.draw(slotText);
            if (saveSlots[i].isUsed) {
                window.draw(playerSprite);
            }
        }

        window.display();
    }
    return -1;
}

int displayLoadScreen(sf::RenderWindow &window, sf::Font &font, std::vector<SaveSlot> &saveSlots) {
    sf::RectangleShape background(sf::Vector2f(800, 600));
    background.setFillColor(sf::Color(240, 240, 240));

    sf::Text title("Load Your Soul's Data", font, 28);
    title.setPosition(50, 20);
    title.setFillColor(sf::Color::Black);

    int selectedSlot = 0;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return -1; // Exit on close
            }

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Up) {
                    selectedSlot = (selectedSlot - 1 + saveSlots.size()) % saveSlots.size();
                } else if (event.key.code == sf::Keyboard::Down) {
                    selectedSlot = (selectedSlot + 1) % saveSlots.size();
                } else if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Z) {
                    if (saveSlots[selectedSlot].isUsed) {
                        return selectedSlot; // Return the selected slot index
                    }
                } else if (event.key.code == sf::Keyboard::Escape) {
                    return -1; // Exit load menu
                }
            }
        }

        window.clear();
        window.draw(background);
        window.draw(title);

        for (size_t i = 0; i < saveSlots.size(); ++i) {
            float slotY = 100 + i * 100;
            sf::RectangleShape slotBackground(sf::Vector2f(700, 80));
            slotBackground.setPosition(50, slotY);
            slotBackground.setFillColor(i == selectedSlot ? sf::Color(200, 200, 200) : sf::Color(255, 255, 255));
            slotBackground.setOutlineThickness(2);
            slotBackground.setOutlineColor(sf::Color::Black);

            sf::Text slotText("", font, 20);
            slotText.setPosition(60, slotY + 20);
            slotText.setFillColor(sf::Color::Black);

            if (saveSlots[i].isUsed) {
                slotText.setString("File " + std::to_string(i + 1) +
                                   "\nTime: " + formatTime(saveSlots[i].playTime));
            } else {
                slotText.setString("File " + std::to_string(i + 1) + "\n[Empty]");
            }

            window.draw(slotBackground);
            window.draw(slotText);
        }

        window.display();
    }

    return -1;
}

void displayInventory(sf::RenderWindow &window, sf::Font &font, const Inventory &inventory) {
    sf::RectangleShape background(sf::Vector2f(700, 500));
    background.setFillColor(sf::Color(191, 191, 191,200));
    background.setPosition(50, 50);

    sf::Text header("items", font, 30);
    header.setPosition(60, 60);
    header.setFillColor(sf::Color::Black);

    float startX = 70;
    float startY = 110;
    float xSpacing = 250;
    float ySpacing = 50;

    std::vector<sf::Text> inventoryTexts;

    for (size_t i = 0; i < inventory.items.size(); ++i) {
        const auto &item = inventory.items[i];

        // Item name
        sf::Text itemName(item.first, font, 24);
        itemName.setFillColor(sf::Color::Black);
        itemName.setPosition(startX + (i % 2) * xSpacing, startY + (i / 2) * ySpacing);

        // Item quantity
        sf::Text itemQuantity(":" + std::to_string(item.second), font, 24);
        itemQuantity.setFillColor(sf::Color::Black);
        itemQuantity.setPosition(startX + 180 + (i % 2) * xSpacing, startY + (i / 2) * ySpacing);

        inventoryTexts.push_back(itemName);
        inventoryTexts.push_back(itemQuantity);
    }

    window.draw(background);
    window.draw(header);
    for (const auto &text : inventoryTexts) {
        window.draw(text);
    }
}

void displayPreLoadDialogue(sf::RenderWindow &window, sf::Font &font, const std::vector<std::string> &dialogues) {
    sf::RectangleShape background(sf::Vector2f(window.getSize().x, window.getSize().y));
    background.setFillColor(sf::Color::Black);

    sf::Text dialogueText("", font, 24);
    dialogueText.setFillColor(sf::Color::White);
    dialogueText.setPosition(100, 250);
    dialogueText.setLineSpacing(1.5f);

    size_t lineIndex = 0;

    while (lineIndex < dialogues.size()) {
        // Ensure the current dialogue is not empty or too large
        const std::string &line = dialogues[lineIndex];
        if (line.empty()) {
            std::cerr << "Error: Empty dialogue line detected at index " << lineIndex << ". Skipping.\n";
            lineIndex++;
            continue;
        }

        if (line.size() > 500) {
            std::cerr << "Error: Dialogue line too long at index " << lineIndex << ". Skipping.\n";
            lineIndex++;
            continue;
        }

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Z || event.key.code == sf::Keyboard::Space) {
                    lineIndex++;
                    break;
                }
            }
        }

        dialogueText.setString(line);

        window.clear();
        window.draw(background);
        window.draw(dialogueText);
        window.display();
    }
}



struct DialogueChoice {
    std::string text;                        // Choice text
    std::vector<std::string> nextDialogue;   // Dialogue that follows this choice
    std::function<bool(const Inventory&)> condition; // Condition to enable this choice
    std::string itemToAdd;                   // Item to add if this choice is selected
    std::function<void()> action;            // Action to perform
};


struct TileDialogue {
    std::vector<std::string> simpleDialogue; // Standard dialogues
    std::vector<DialogueChoice> choices;    // Choices, if any
};

bool itemObtained = false;    // Indicates whether the player obtained the item

std::map<int, TileDialogue> tileDialogues = {
    {5, {
    {"The drawer is closed."}, // Simple dialogue
        {   // Choices available when a condition is met
            {"Open the drawer",
             {"You opened the drawer and found a slot\nto put something inside."},
             [](const Inventory& inv) {
                 return std::any_of(inv.items.begin(), inv.items.end(),
                                    [](const auto& item) { return item.first == "Key"; });
            }, ""},
{"Put Plushie Inside",
 {"You placed the plushie inside the drawer.\nIt clicked into place."},
 [](const Inventory& inv) {
     return drawerOpened && // Check if the drawer is opened
            std::any_of(inv.items.begin(), inv.items.end(),
                        [](const auto& item) { return item.first == "Plushie Toy"; });
}, "",
incrementDrawerItemCount // Call the helper function
},

{"Put Heart Piece Inside",
{"You placed the heart piece inside the drawer.\nIt clicked into place."},
[](const Inventory& inv) {
    return drawerOpened && // Check if the drawer is opened
           std::any_of(inv.items.begin(), inv.items.end(),
                       [](const auto& item) { return item.first == "Heart Piece"; });
}, "",
incrementDrawerItemCount // Call the helper function
},

{"Put Shiny Item Inside",
{"You placed the shiny item inside the drawer.\nIt clicked into place."},
[](const Inventory& inv) {
    return drawerOpened && // Check if the drawer is opened
           std::any_of(inv.items.begin(), inv.items.end(),
                       [](const auto& item) { return item.first == "Shiny Item"; });
}, "",
incrementDrawerItemCount // Call the helper function
},

{"Put Chibi Doll Inside",
{"You placed the chibi doll inside the drawer.\nIt clicked into place."},
[](const Inventory& inv) {
    return drawerOpened && // Check if the drawer is opened
           std::any_of(inv.items.begin(), inv.items.end(),
                       [](const auto& item) { return item.first == "Chibi Doll"; });
}, "",
incrementDrawerItemCount // Call the helper function
},

          {"Leave it",
           {"You decided to leave the drawer alone."},
           [](const Inventory&) {
               return true; // Always available
          }, ""} // No item added here
            }
    }},
    {1, {
                {"This drawer looks interesting."},
                {
                    {"Open the drawer", {"You found a Key inside!"},
                     [](const Inventory& inv) {
                         return std::none_of(inv.items.begin(), inv.items.end(),
                                            [](const auto& item) { return item.first == "Key"; });
                    }, "Key"},
                   {"Not touch", {"You decided to do other things."},
                    [](const Inventory&) {
                        return true; // Always available
                   }, ""}
                }
    }
    },
{17, {
            {"I don't want tea."} // Simple dialogue for tile 17
}},
{27, {
                            {"Water is too wet. I don't like getting wet."} // Simple dialogue for tile 17
}},
{29, {
                {"Just a normal chair. I don't need to sit."} // Simple dialogue for tile 17
}},
{33, {
                        {"I don't think touching it is a good idea."} // Simple dialogue for tile 17
}},
{34, {
                    {"Someone left apples on the table."} // Simple dialogue for tile 17
}}
};

// Function to display the code lock puzzle
bool displayCodeLockPuzzle(sf::RenderWindow &window, sf::Font &font) {

    std::vector<int> code(4, 0); // Player's input for the code
    std::vector<int> solution = {4, 5, 3, 6}; // solution based on the amount of sides of polygons

    // Generate clues corresponding to the solution digits
    std::vector<sf::CircleShape> polygons;
    for (int side : solution) {
        sf::CircleShape polygon(50, side); // Create polygon with 'side' sides
        polygon.setOutlineThickness(2);
        polygon.setOutlineColor(sf::Color::White);
        polygon.setFillColor(sf::Color::Transparent);
        polygons.push_back(polygon);
    }

    // Position the polygons on the screen
    for (size_t i = 0; i < polygons.size(); ++i) {
        polygons[i].setPosition(110 + i * 150, 100);
    }

    int selectedIndex = 0;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Left) {
                    selectedIndex = (selectedIndex - 1 + 4) % 4; // Navigate left
                } else if (event.key.code == sf::Keyboard::Right) {
                    selectedIndex = (selectedIndex + 1) % 4; // Navigate right
                } else if (event.key.code == sf::Keyboard::Up) {
                    code[selectedIndex] = (code[selectedIndex] + 1) % 10; // Increment digit
                } else if (event.key.code == sf::Keyboard::Down) {
                    code[selectedIndex] = (code[selectedIndex] - 1 + 10) % 10; // Decrement digit
                } else if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Z) {
                    if (code == solution) {
                        return true; // Puzzle solved
                    }
                } else if (event.key.code == sf::Keyboard::Escape) {
                    return false; // Exit puzzle
                }
            }
        }

        // Render the puzzle
        window.clear(sf::Color::Black);

        // Draw polygons as hints
        for (const auto &polygon : polygons) {
            window.draw(polygon);
        }

        // Draw lock code interface
        for (size_t i = 0; i < code.size(); ++i) {
            sf::Text digit(std::to_string(code[i]), font, 50);
            digit.setFillColor(i == selectedIndex ? sf::Color::Yellow : sf::Color::White);
            digit.setPosition(150 + i * 150, 300);
            window.draw(digit);

            sf::Text increment("+", font, 30);
            increment.setFillColor(sf::Color::Green);
            increment.setPosition(150 + i * 150, 250);
            window.draw(increment);

            sf::Text decrement("-", font, 30);
            decrement.setFillColor(sf::Color::Red);
            decrement.setPosition(150 + i * 150, 350);
            window.draw(decrement);
        }

        window.display();
    }

    return false; // Fallback
}

bool playSimonSays(sf::RenderWindow &window, sf::Font &font, int &reward) {
    std::vector<int> sequence; // Stores the sequence of patterns
    std::vector<sf::Color> colors = {sf::Color::Red, sf::Color::Green, sf::Color::Blue, sf::Color::Yellow};
    const std::vector<sf::Keyboard::Key> keys = {sf::Keyboard::A, sf::Keyboard::S, sf::Keyboard::D, sf::Keyboard::F};

    sf::RectangleShape colorDisplay(sf::Vector2f(100, 100));
    colorDisplay.setPosition(350, 250);

    sf::Text controlsText("A - Red   S - Green   D - Blue   F - Yellow", font, 20);
    controlsText.setFillColor(sf::Color::White);
    controlsText.setPosition(225, 450);

    sf::Text pressCountText("", font, 20);
    pressCountText.setFillColor(sf::Color::White);
    pressCountText.setPosition(350, 500);

    int round = 1;
    bool success = true;

    while (success) {
        // Add a new color to the sequence
        sequence.push_back(rand() % colors.size());

        // Display the sequence
        for (int index : sequence) {
            colorDisplay.setFillColor(colors[index]);
            window.clear();
            window.draw(colorDisplay);
            window.draw(controlsText);
            window.display();
            sf::sleep(sf::seconds(0.8));
            window.clear();
            window.draw(controlsText);
            window.display();
            sf::sleep(sf::seconds(0.4));
        }

        // Wait for player's input
        int keysPressed = 0; // Start keysPressed at 0 for each round
        for (int i = 0; i < sequence.size(); ++i) {
            bool inputReceived = false;

            while (window.isOpen() && !inputReceived) {
                sf::Event event;
                while (window.pollEvent(event)) {
                    if (event.type == sf::Event::Closed) {
                        window.close();
                        return false;
                    }

                    if (event.type == sf::Event::KeyPressed) {
                        int keyIndex = std::distance(keys.begin(), std::find(keys.begin(), keys.end(), event.key.code));
                        if (keyIndex >= 0 && keyIndex < colors.size()) {
                            if (keyIndex == sequence[i]) {
                                keysPressed++; // Increment keys pressed counter only on correct input
                                inputReceived = true; // Correct input
                            } else {
                                success = false; // Incorrect input
                                inputReceived = true;
                            }
                        }
                    }
                }

                // Update the press count text
                pressCountText.setString("Keys Pressed: " + std::to_string(keysPressed) + " / " + std::to_string(sequence.size()));

                // Draw the game screen while waiting for input
                window.clear();
                window.draw(controlsText);
                window.draw(pressCountText);
                window.display();
            }
        }

        // Stop the game after the 10th round
        if (round == 10) {
            success = false;
            reward = 1; // Assign a reward for successfully completing all rounds
        } else {
            round++;
        }

        if (success) {
            sf::Text roundText("Round " + std::to_string(round), font, 24);
            roundText.setFillColor(sf::Color::White);
            roundText.setPosition(350, 100);

            window.clear();
            window.draw(roundText);
            window.draw(controlsText);
            window.display();
            sf::sleep(sf::seconds(1.5));
        }
    }

    return reward > 0;
}

// Dynamic dialogue generation for tile with a random choice (not tile 10, has been changed)
bool generateDialogueForTile10(TileDialogue& tileDialogue) {
    int correctNumber = 0;
    auto randomChoices = generateUniqueRandomChoices(correctNumber, 1, 100);

    // Shuffle the choices to ensure random order
    std::shuffle(randomChoices.begin(), randomChoices.end(), std::mt19937(std::random_device()()));

    // Create choice texts
    std::vector<std::string> choiceTexts = {
        "Is it " + std::to_string(randomChoices[0]) + "?",
        "Is it " + std::to_string(randomChoices[1]) + "?",
        "Is it " + std::to_string(randomChoices[2]) + "?"
    };

    tileDialogue.simpleDialogue = {"You encounter a mysterious puzzle in the book!\nChoose the correct number."};
    tileDialogue.choices.clear();
    for (size_t i = 0; i < randomChoices.size(); ++i) {
        tileDialogue.choices.push_back({
            choiceTexts[i],
            randomChoices[i] == correctNumber
                ? std::vector<std::string>{"Correct! You solved the puzzle."}
                : std::vector<std::string>{"Wrong choice. Try again."},
            [](const Inventory&) { return true; }, // Always available
            randomChoices[i] == correctNumber ? "Plushie Toy" : "" // Reward item for correct choice
        });
    }

    return true;
}

struct Note {
    sf::RectangleShape shape;
    int laneIndex;
    bool isHit;
    float hitTime; // Time when the note was hit
};


bool playRhythmGame(sf::RenderWindow &window, sf::Font &font, const std::string &songName) {
    // Define lanes and keys
    const int numLanes = 4;
    const std::vector<sf::Keyboard::Key> laneKeys = {
        sf::Keyboard::Z, sf::Keyboard::X, sf::Keyboard::C, sf::Keyboard::V
    };
    const int laneWidth = 100;
    const int laneHeight = 600;
    const int noteWidth = 80;
    const int noteHeight = 20;
    const int hitZoneHeight = 20;
    const int hitZoneY = 550;

    // lanes
    std::vector<sf::RectangleShape> lanes;
    for (int i = 0; i < numLanes; ++i) {
        sf::RectangleShape lane(sf::Vector2f(laneWidth, laneHeight));
        lane.setFillColor(sf::Color(50, 50, 50));
        lane.setPosition(100 + i * laneWidth, 0);
        lanes.push_back(lane);
    }

    // hit zones
    std::vector<sf::RectangleShape> hitZones;
    for (int i = 0; i < numLanes; ++i) {
        sf::RectangleShape hitZone(sf::Vector2f(noteWidth, hitZoneHeight));
        hitZone.setFillColor(sf::Color::White);
        hitZone.setPosition(110 + i * laneWidth, hitZoneY);
        hitZones.push_back(hitZone);
    }

    // Display controls
    sf::Text controlsText("Controls: Z, X, C, V", font, 20);
    controlsText.setFillColor(sf::Color::White);
    controlsText.setPosition(550, 20); // Position at the top center

    // Falling notes
    std::vector<Note> notes;
    std::vector<float> noteTimes = {1.0f, 1.5f, 2.0f, 2.5f, 3.0f}; // Time for notes to spawn
    sf::Clock clock;

    // Load song
    sf::Music song;
    if (!song.openFromFile("assets/" + songName + ".ogg")) {
        std::cerr << "Error: Could not load song.\n";
        return false;
    }
    song.play();

    // Score
    int score = 0;
    sf::Text scoreText("Score: 0", font, 20);
    scoreText.setPosition(20, 20);

    // Rhythm Game loop
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return false;
            }
        }

        // Timing and note spawning
        float currentTime = clock.getElapsedTime().asSeconds();
        for (size_t i = 0; i < noteTimes.size(); ++i) {
            if (currentTime >= noteTimes[i] && currentTime < noteTimes[i] + 0.1f) {
                int laneIndex = rand() % numLanes;
                sf::RectangleShape noteShape(sf::Vector2f(noteWidth, noteHeight));
                noteShape.setFillColor(sf::Color::Red);
                noteShape.setPosition(110 + laneIndex * laneWidth, 0);
                notes.push_back({noteShape, laneIndex, false, 0.0f});
            }
        }

        // Update note positions and remove old notes
        for (auto &note : notes) {
            if (!note.isHit) note.shape.move(0, 300 * clock.restart().asSeconds());
        }
        notes.erase(std::remove_if(notes.begin(), notes.end(),
                                   [currentTime](const Note &note) {
                                       return note.shape.getPosition().y > 600 ||
                                              (note.isHit && currentTime - note.hitTime > 0.1f);
                                   }),
                    notes.end());

        // Check for key presses
        for (size_t i = 0; i < laneKeys.size(); ++i) {
            if (sf::Keyboard::isKeyPressed(laneKeys[i])) {
                for (auto &note : notes) {
                    if (!note.isHit && note.laneIndex == i) {
                        float noteY = note.shape.getPosition().y;
                        if (noteY >= hitZoneY - 10 && noteY <= hitZoneY + 10) {
                            score += 100;
                            note.shape.setFillColor(sf::Color::Green);
                            note.isHit = true;
                            note.hitTime = currentTime;
                            break;
                        } else if (noteY >= hitZoneY - 30 && noteY <= hitZoneY + 30) {
                            score += 50;
                            note.shape.setFillColor(sf::Color(0, 128, 0));
                            note.isHit = true;
                            note.hitTime = currentTime;
                            break;
                        }
                    }
                }
            }
        }

        // Render
        window.clear(sf::Color::Black);

        // Draw lanes, hit zones, and controls
        for (const auto &lane : lanes) window.draw(lane);
        for (const auto &hitZone : hitZones) window.draw(hitZone);
        window.draw(controlsText);

        // Draw notes
        for (const auto &note : notes) {
            window.draw(note.shape);
        }

        // Draw score
        scoreText.setString("Score: " + std::to_string(score));
        window.draw(scoreText);

        window.display();

        // End conditions
        if (notes.empty() && currentTime > *std::max_element(noteTimes.begin(), noteTimes.end()) + 2) break;
        if (song.getStatus() != sf::Music::Playing) break;
    }

    // Display results
    sf::Text resultText("Final Score: " + std::to_string(score), font, 30);
    resultText.setPosition(250, 300);
    window.clear();
    window.draw(resultText);
    window.display();
    sf::sleep(sf::seconds(3));

    return score > 9500;
}

// Check if a tile has dialogue
bool hasDialogue(int tileID)
{
    return tileDialogues.find(tileID) != tileDialogues.end();
}

const std::vector<std::string>& getDialogue(int tileID, size_t index = 0) {
    if (tileDialogues.find(tileID) != tileDialogues.end()) {
        const auto& dialogue = tileDialogues[tileID].simpleDialogue; // Access the simple dialogue
        if (index < dialogue.size()) {
            return dialogue; // Return the dialogue at the specified index
        }
    }
    static const std::vector<std::string> emptyDialogue; // Return an empty dialogue if no valid dialogue is found
    return emptyDialogue;
}

int displayDialogueWithChoices(sf::RenderWindow &window, sf::Font &font,
                               const std::string &dialogue,
                               const std::vector<std::string> &choices,
                               const std::map<int, sf::Texture> &tileTextures,
                               const Animation &playerAnimation,
                               const sf::Vector2f &pixelPosition,
                               const std::vector<std::vector<int>> &mapData) {
    int selectedChoice = 0;

    // Dialogue box
    sf::RectangleShape dialogueBox(sf::Vector2f(600, 150));
    dialogueBox.setFillColor(sf::Color(0, 0, 0, 200)); // Semi-transparent black
    dialogueBox.setOutlineThickness(2);
    dialogueBox.setOutlineColor(sf::Color::White);
    dialogueBox.setPosition(100, 400);

    sf::Text dialogueText(dialogue, font, 24);
    dialogueText.setFillColor(sf::Color::White);
    dialogueText.setPosition(dialogueBox.getPosition().x + 20, dialogueBox.getPosition().y + 20);

    // Choices box
    sf::RectangleShape choiceBox(sf::Vector2f(600, choices.size() * 40 + 10)); // Same width as dialogue box
    choiceBox.setFillColor(sf::Color(0, 0, 0, 200)); // Semi-transparent black
    choiceBox.setOutlineThickness(2);
    choiceBox.setOutlineColor(sf::Color::White);
    choiceBox.setPosition(dialogueBox.getPosition().x, dialogueBox.getPosition().y - choiceBox.getSize().y - 10); // Above dialogue box

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Up) {
                    selectedChoice = (selectedChoice - 1 + choices.size()) % choices.size(); // Navigate up
                } else if (event.key.code == sf::Keyboard::Down) {
                    selectedChoice = (selectedChoice + 1) % choices.size(); // Navigate down
                } else if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Z) {
                    return selectedChoice; // Player selected a choice
                } else if (event.key.code == sf::Keyboard::Escape) {
                    return -1; // Cancel/exit
                }
            }
        }

        window.clear(sf::Color(108, 158, 127)); // Background color for map in case tiles are not loaded correctly
        for (int y = 0; y < mapData.size(); ++y) {
            for (int x = 0; x < mapData[y].size(); ++x) {
                int tileID = mapData[y][x];
                if (tileTextures.find(tileID) != tileTextures.end()) {
                    sf::Sprite sprite(tileTextures.at(tileID));
                    sprite.setPosition(x * TILE_SIZE, y * TILE_SIZE);
                    window.draw(sprite);
                }
            }
        }
        playerAnimation.draw(window, pixelPosition); // Redraw player

        // Render the dialogue box
        window.draw(dialogueBox);
        window.draw(dialogueText);

        // Render the choices
        window.draw(choiceBox);
        for (size_t i = 0; i < choices.size(); ++i) {
            sf::Text choiceText(choices[i], font, 20);
            choiceText.setPosition(choiceBox.getPosition().x + 20, choiceBox.getPosition().y + 10 + i * 40);

            // Set color based on selection
            if (i == selectedChoice) {
                choiceText.setFillColor(sf::Color::Yellow); // Highlight the selected choice
            } else {
                choiceText.setFillColor(sf::Color::White); // Default color for unselected choices
            }

            window.draw(choiceText); // Draw each choice
        }

        window.display();
    }

    return -1; // Default fallback
}

void displayDialogueWithPortrait(
    sf::RenderWindow &window,
    sf::Font &font,
    const std::string &dialogue,
    const sf::Texture &portraitTexture,
    const std::map<int, sf::Texture> &tileTextures,
    const Animation &playerAnimation,
    const sf::Vector2f &pixelPosition,
    const std::vector<std::vector<int>> &mapData)
{
    // Background for the dialogue box
    sf::RectangleShape dialogueBox(sf::Vector2f(600, 150));
    dialogueBox.setFillColor(sf::Color(0, 0, 0, 200)); // Semi-transparent black
    dialogueBox.setPosition(100, 400); // Bottom of the screen

    sf::RectangleShape portraitFrame(sf::Vector2f(192, 192));
    portraitFrame.setPosition(dialogueBox.getPosition().x+20, dialogueBox.getPosition().y - 200);
    portraitFrame.setOutlineColor(sf::Color::Black);
    portraitFrame.setOutlineThickness(5);
    portraitFrame.setFillColor(sf::Color::Transparent);

    // Portrait of the character
    sf::Sprite portrait(portraitTexture);
    portrait.setPosition(dialogueBox.getPosition().x+20, dialogueBox.getPosition().y - 200);
    portrait.setScale(2.0f, 2.0f);

    // Dialogue text
    sf::Text dialogueText("", font, 24);
    dialogueText.setFillColor(sf::Color::White);
    dialogueText.setPosition(dialogueBox.getPosition().x + 20, dialogueBox.getPosition().y + 20);
    dialogueText.setLineSpacing(1.5f);

    // Typewriter effect
    sf::Clock clock;
    std::string displayedText;
    size_t currentChar = 0;
    bool dialogueComplete = false; // Track whether the dialogue has completed

    while (window.isOpen() && !dialogueComplete) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Z || event.key.code == sf::Keyboard::Space) {
                    if (currentChar < dialogue.size()) {
                        // Skip the typewriter effect
                        displayedText = dialogue;
                        currentChar = dialogue.size();
                    } else {
                        // End the dialogue if all text has been displayed
                        dialogueComplete = true;
                    }
                }
            }
        }

        // Typewriter effect logic
        if (currentChar < dialogue.size()) {
            if (clock.getElapsedTime().asMilliseconds() > 50) {
                displayedText += dialogue[currentChar++];
                clock.restart();
            }
        }

        // Update the dialogue text
        dialogueText.setString(displayedText);

        // Render the map and player
        window.clear(sf::Color(108, 158, 127)); // Background color for map in case tiles are not loaded correctly
        for (int y = 0; y < mapData.size(); ++y) {
            for (int x = 0; x < mapData[y].size(); ++x) {
                int tileID = mapData[y][x];
                if (tileTextures.find(tileID) != tileTextures.end()) {
                    sf::Sprite sprite(tileTextures.at(tileID));
                    sprite.setPosition(x * TILE_SIZE, y * TILE_SIZE);
                    window.draw(sprite);
                }
            }
        }

        // Draw the player on top of the map
        playerAnimation.draw(window, pixelPosition);

        // Draw the dialogue box and portrait
        window.draw(dialogueBox);
        window.draw(portrait);
        window.draw(portraitFrame);
        window.draw(dialogueText);

        // Display the updated screen
        window.display();
    }

    // cooldown with sleep to prevent instant re-trigger
    sf::sleep(sf::milliseconds(500));
}

// Load map data
bool loadMap(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
        return false;

    mapData.clear();
    std::string line;
    while (std::getline(file, line))
    {
        std::replace(line.begin(), line.end(), ',', ' ');
        std::istringstream lineStream(line);
        std::vector<int> row;
        int tile;

        while (lineStream >> tile)
            row.push_back(tile);
        if (!row.empty())
            mapData.push_back(row);
    }

    MAP_HEIGHT = mapData.size();
    MAP_WIDTH = mapData[0].size();
    return true;
}

int displayChoices(sf::RenderWindow &window, sf::Font &font, const std::vector<DialogueChoice> &choices) {
    int selectedChoice = 0;

    //choice box
    sf::Vector2f boxSize(150, choices.size() * 50 + 10);
    sf::RectangleShape choiceBox(boxSize);
    choiceBox.setFillColor(sf::Color(0, 0, 0, 200)); // Semi-transparent black
    choiceBox.setOutlineThickness(2);
    choiceBox.setOutlineColor(sf::Color::White);

    choiceBox.setPosition(window.getSize().x - 180, 120);

    // Prepare the text for each choice
    std::vector<sf::Text> choiceTexts;
    for (size_t i = 0; i < choices.size(); ++i) {
        sf::Text choiceText(choices[i].text, font, 20);
        choiceText.setPosition(choiceBox.getPosition().x + 20, choiceBox.getPosition().y + 10 + i * 50);
        choiceText.setFillColor(sf::Color::White); // Default color
        choiceTexts.push_back(choiceText);
    }

    // Main loop for rendering the choices
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Up) {
                    selectedChoice = (selectedChoice - 1 + choices.size()) % choices.size(); // Navigate up
                } else if (event.key.code == sf::Keyboard::Down) {
                    selectedChoice = (selectedChoice + 1) % choices.size(); // Navigate down
                } else if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Z) {
                    return selectedChoice; // Player selected a choice
                } else if (event.key.code == sf::Keyboard::Escape) {
                    return -1; // Cancel/exit
                }
            }
        }

        // Draw the choice box and text
        window.clear(sf::Color::Black); // Keep the game map visible
        window.draw(choiceBox);
        for (const auto &text : choiceTexts) {
            window.draw(text);
        }
        window.display();
    }

    return -1;
}


bool fetchDialogueForTile(int tileID, Inventory& inventory,
                          std::vector<std::string>& dialogueTexts,
                          std::string& currentItemToAdd,
                          sf::RenderWindow& window, sf::Font& font,
                          const std::map<int, sf::Texture>& tileTextures,
                          const Animation& playerAnimation,
                          const sf::Vector2f& pixelPosition,
                          const std::vector<std::vector<int>>& mapData){
    //tile numbers have changed, that's why some dialogues and flags don't match the number, I would've fixed it with more time :)
    //tiledialogues system has been a little too complicated, so for some tiles I wrote the code manually there

if (tileID == 28) {
    dialogueTexts = {"You feel a strange aura here.\nDo you want to finish the game?"};
    currentItemToAdd.clear();

    std::vector<std::string> choices = {"Yes", "No"};
    int choice = displayDialogueWithChoices(window, font, dialogueTexts[0], choices, tileTextures, playerAnimation, pixelPosition, mapData);

    if (choice == 0) { // Player chooses "Yes"
        if (drawerItemCount == 0) {
            // Bad Ending, Ending 1: Zero items placed in the drawer
            sf::Text endingText("You leave with nothing. The world remains unchanged.\nMaybe there's something you've missed.", font, 24);
            endingText.setFillColor(sf::Color::White);
            endingText.setPosition(100, 250);
            window.clear();
            window.draw(endingText);
            window.display();
            sf::sleep(sf::seconds(5)); // Pause to display ending
        } else if (drawerItemCount > 0 && drawerItemCount < 4) {
            // Normal Ending, Ending 2: Between 1 and 3 items placed in the drawer
            sf::Text endingText("You leave with a piece of the world.\nIt's not much, but somehow your heart feels at home.", font, 24);
            endingText.setFillColor(sf::Color::White);
            endingText.setPosition(100, 250);
            window.clear();
            window.draw(endingText);
            window.display();
            sf::sleep(sf::seconds(5)); // Pause to display ending
        } else if (drawerItemCount == 4) {
            // True Ending, Ending 3: All 4 items placed in the drawer
            sf::Text endingText("You have restored your soul.", font, 24);
            endingText.setFillColor(sf::Color::White);
            endingText.setPosition(100, 250);
            window.clear();
            window.draw(endingText);
            window.display();
            sf::sleep(sf::seconds(5)); // Pause to display ending
        }
        window.close(); // Close the game after displaying the ending
    } else {
        dialogueTexts = {"You decide to stay a little longer."};
    }
    return true;
}

    if (tileID == 3) {
        sf::Texture portraitTexture;
        if (portraitTexture.loadFromFile("assets/character_image.png")) {
            std::string dialogue = "Clock stopped.";
            displayDialogueWithPortrait(
                window, font, dialogue, portraitTexture, tileTextures,
                playerAnimation, pixelPosition, mapData);
        }
    }
    if (tileID == 25) {
        sf::Texture portraitTexture;
        if (portraitTexture.loadFromFile("assets/character_image.png")) {
            std::string dialogue = "Flowers make this place more lively.";
            displayDialogueWithPortrait(
                window, font, dialogue, portraitTexture, tileTextures,
                playerAnimation, pixelPosition, mapData);
        }
    }

    if (tileID == 22) {
        sf::Texture portraitTexture;
        if (portraitTexture.loadFromFile("assets/character_image.png")) {
            std::string dialogue = "Teddy seems friendly. Hi, friend.";
            displayDialogueWithPortrait(
                window, font, dialogue, portraitTexture, tileTextures,
                playerAnimation, pixelPosition, mapData);
        }
    }

    if (tileID == 23) {
        sf::Texture portraitTexture;
        if (portraitTexture.loadFromFile("assets/character_image.png")) {
            std::string dialogue = "This doll creeps me out.";
            displayDialogueWithPortrait(
                window, font, dialogue, portraitTexture, tileTextures,
                playerAnimation, pixelPosition, mapData);
        }
    }

    if (tileID == 10) {
        sf::Texture portraitTexture;
        if (portraitTexture.loadFromFile("assets/character_image.png")) {
            std::string dialogue = "I like nights. They are peaceful.";
            displayDialogueWithPortrait(
                window, font, dialogue, portraitTexture, tileTextures,
                playerAnimation, pixelPosition, mapData);
        }
    }

    if (tileID == 24) {
        sf::Texture portraitTexture;
        if (portraitTexture.loadFromFile("assets/character_image.png")) {
            std::string dialogue = "I'd love to try cake later.";
            displayDialogueWithPortrait(
                window, font, dialogue, portraitTexture, tileTextures,
                playerAnimation, pixelPosition, mapData);
        }
    }

    if (tileID == 18) {
        // Check if the player already has the "Plushie Toy"
        bool hasPlushieToy = std::any_of(inventory.items.begin(), inventory.items.end(),
                                         [](const auto& item) { return item.first == "Plushie Toy"; });
        if (hasPlushieToy) {
            return false; // Dialogue won't trigger
        }

        generateDialogueForTile10(tileDialogues[18]); // Regenerate dialogue for tile ID 10
    }
    if (tileID == 19) {
        if (tile11Completed) {
            if (itemObtained) {
                dialogueTexts = {"The box is closed shut."};
            } else {
                dialogueTexts = {"The box is closed shut. Maybe you can try something else."};
            }
            return true;
        }

        dialogueTexts = {"You approach the mysterious book.\nDo you want to try the rhythm game?"};
        currentItemToAdd.clear();

        std::vector<std::string> choices = {"Yes", "No"};
        int choice = displayDialogueWithChoices(window, font, dialogueTexts[0], choices, tileTextures, playerAnimation, pixelPosition, mapData);

        if (choice == 0) { // Player chooses "Yes"
            bool success = playRhythmGame(window, font, "rachie_addicted");
            tile11Completed = true; // Mark the game as completed

            if (success) {
                itemObtained = true;
                dialogueTexts = {"Something shining fell out of the book."};
                inventory.addItem("Shiny Item"); // Reward the player
            } else {
                dialogueTexts = {"The book is closed shut."};
            }
        } else {
            dialogueTexts = {"The book remains untouched."};
        }
        return true;
    }

    if (tileID == 20) {
        if (tile13Completed) {
            dialogueTexts = {"The book is already closed. Nothing else to find here."};
            return true;
        }

        dialogueTexts = {"You encounter a book with a strange code.\nWould you like to try solving it?"};
        currentItemToAdd.clear();

        std::vector<std::string> choices = {"Yes", "No"};
        int choice = displayDialogueWithChoices(window, font, dialogueTexts[0], choices, tileTextures, playerAnimation, pixelPosition, mapData);

        if (choice == 0) { // Player chooses "Yes"
            bool success = displayCodeLockPuzzle(window, font); // Solve the code lock puzzle
            if (success) {
                tile13Completed = true; // Mark the puzzle as completed
                dialogueTexts = {"The book flutters, revealing a Heart Piece!"};
                inventory.addItem("Heart Piece"); // Reward the player
            } else {
                dialogueTexts = {"You failed to unlock the book."};
            }
        } else {
            dialogueTexts = {"You decide to leave the book for now."};
        }

        return true;
    }

    if (tileID == 21) {
        if (tile14Completed) {
            dialogueTexts = {"The book is already closed. Nothing else to find here."};
            return true;
        }
        dialogueTexts = {"You see a strange glowing inside the book.\nDo you want to play Simon Says?"};
        currentItemToAdd.clear();

        std::vector<std::string> choices = {"Yes", "No"};
        int choice = displayDialogueWithChoices(window, font, dialogueTexts[0], choices, tileTextures, playerAnimation, pixelPosition, mapData);

        if (choice == 0) { // Player chooses "Yes"
            int reward = 0;
            bool success = playSimonSays(window, font, reward);
            if (success) {
                tile14Completed=true;
                dialogueTexts = {"You completed Simon Says! You receive a Chibi Doll!"};
                inventory.addItem("Chibi Doll");
            } else {
                dialogueTexts = {"You failed to achieve results."};
            }
        } else {
            dialogueTexts = {"You decide to leave the book alone."};
        }
        return true;
    }


    if (tileDialogues.find(tileID) != tileDialogues.end()) {
        const auto& tileDialogue = tileDialogues[tileID];

        std::vector<DialogueChoice> availableChoices;
        for (const auto& choice : tileDialogue.choices) {
            if (choice.condition(inventory)) {
                availableChoices.push_back(choice);
            }
        }

        if (!availableChoices.empty()) {
            std::string mainDialogue = !tileDialogue.simpleDialogue.empty() ? tileDialogue.simpleDialogue[0] : "";
            std::vector<std::string> choiceTexts;
            for (const auto& choice : availableChoices) {
                choiceTexts.push_back(choice.text);
            }

            int selectedChoice = displayDialogueWithChoices(
                window, font, mainDialogue, choiceTexts, tileTextures,
                playerAnimation, pixelPosition, mapData);

            if (selectedChoice >= 0 && selectedChoice < availableChoices.size()) {
                dialogueTexts = availableChoices[selectedChoice].nextDialogue;
                currentItemToAdd = availableChoices[selectedChoice].itemToAdd;

                // Handle the drawer state
                if (availableChoices[selectedChoice].text == "Open the drawer") {
                    drawerOpened = true; // Mark the drawer as opened
                }

                // Handle the removal of "Plushie"
                if (availableChoices[selectedChoice].text == "Open the drawer") {
                    auto rareIt = std::find_if(inventory.items.begin(), inventory.items.end(),
                                                  [](auto& item) { return item.first == "Key"; });
                    if (rareIt != inventory.items.end()) {
                        if (--rareIt->second == 0) {
                            inventory.items.erase(rareIt); // Remove Plushie if quantity becomes 0
                        }
                    }
                }

                if (availableChoices[selectedChoice].text == "Put Plushie Inside") {
                    incrementDrawerItemCount();
                    std::cout << "Plushie placed. drawerItemCount: " << drawerItemCount << std::endl;
                }
                if (availableChoices[selectedChoice].text == "Put Heart Piece Inside") {
                    incrementDrawerItemCount();
                    std::cout << "Heart Piece placed. drawerItemCount: " << drawerItemCount << std::endl;
                }
                if (availableChoices[selectedChoice].text == "Put Shiny Item Inside") {
                    incrementDrawerItemCount();
                    std::cout << "Shiny Item placed. drawerItemCount: " << drawerItemCount << std::endl;
                }
                if (availableChoices[selectedChoice].text == "Put Chibi Doll Inside") {
                    incrementDrawerItemCount();
                    std::cout << "Chibi Doll placed. drawerItemCount: " << drawerItemCount << std::endl;
                }


                // Handle the removal of "Plushie"
                if (availableChoices[selectedChoice].text == "Put Plushie Inside") {
                    auto plushieIt = std::find_if(inventory.items.begin(), inventory.items.end(),
                                                  [](auto& item) { return item.first == "Plushie Toy"; });
                    if (plushieIt != inventory.items.end()) {
                        if (--plushieIt->second == 0) {
                            inventory.items.erase(plushieIt); // Remove Plushie if quantity becomes 0
                        }
                    }
                }

                // Handle the removal of "heart"
                if (availableChoices[selectedChoice].text == "Put Heart Piece Inside") {
                    auto heartIt = std::find_if(inventory.items.begin(), inventory.items.end(),
                                                  [](auto& item) { return item.first == "Heart Piece"; });
                    if (heartIt != inventory.items.end()) {
                        if (--heartIt->second == 0) {
                            inventory.items.erase(heartIt);
                        }
                    }
                }

                // Handle the removal of "shiny"
                if (availableChoices[selectedChoice].text == "Put Shiny Item Inside") {
                    auto shinyIt = std::find_if(inventory.items.begin(), inventory.items.end(),
                                                  [](auto& item) { return item.first == "Shiny Item"; });
                    if (shinyIt != inventory.items.end()) {
                        if (--shinyIt->second == 0) {
                            inventory.items.erase(shinyIt);
                        }
                    }
                }

            // Handle the removal of "doll"
            if (availableChoices[selectedChoice].text == "Put Chibi Doll Inside") {
                auto dollIt = std::find_if(inventory.items.begin(), inventory.items.end(),
                                              [](auto& item) { return item.first == "Chibi Doll"; });
                if (dollIt != inventory.items.end()) {
                    if (--dollIt->second == 0) {
                        inventory.items.erase(dollIt); // Remove Plushie if quantity becomes 0
                    }
                }
            }
                return true;
            }

        } else if (!tileDialogue.simpleDialogue.empty()) {
            dialogueTexts = tileDialogue.simpleDialogue;
            currentItemToAdd.clear();
            return true;
        }
    }
    return false;
}


void displayDialogue(sf::RenderWindow &window, sf::Font &font, const std::string &text)
{
    sf::RectangleShape textBox(sf::Vector2f(600, 150));
    textBox.setFillColor(sf::Color(0, 0, 0, 200));
    textBox.setPosition(100, 400);

    sf::Text dialogueText(text, font, 24);
    dialogueText.setPosition(120, 420);
    dialogueText.setFillColor(sf::Color::White);

    window.draw(textBox);
    window.draw(dialogueText);
}

// Function to display the escape menu
void displayMenu(sf::RenderWindow &window, sf::Font &font, int currentMenuIndex, const std::vector<std::string> &menuOptions, const std::string &currentOptionDescription)
{
    sf::RectangleShape menuBackground(sf::Vector2f(200, menuOptions.size() * 40 + 20));
    menuBackground.setFillColor(sf::Color(0, 0, 0, 200));
    menuBackground.setPosition(50, 50);

    window.draw(menuBackground);

    for (size_t i = 0; i < menuOptions.size(); ++i)
    {
        sf::Text menuText(menuOptions[i], font, 24);
        menuText.setPosition(70, 60 + i * 40);
        if (i == currentMenuIndex)
        {
            menuText.setFillColor(sf::Color::Yellow);
        }
        else
        {
            menuText.setFillColor(sf::Color::White);
        }
        window.draw(menuText);
    }

    // Display context information on the right
    sf::RectangleShape infoBox(sf::Vector2f(400, 200));
    infoBox.setFillColor(sf::Color(0, 0, 0, 200));
    infoBox.setPosition(300, 50);

    sf::Text infoText(currentOptionDescription, font, 20);
    infoText.setFillColor(sf::Color::White);
    infoText.setPosition(320, 70);

    window.draw(infoBox);
    window.draw(infoText);
}


void game(sf::RenderWindow &window, std::vector<SaveSlot> &saveSlots, int saveSlotIndex = -1) {

    Inventory inventory;
    bool canTriggerDialogue = true;

    std::map<int, sf::Texture> tileTextures;
    sf::Texture playerTexture;
    if (!playerTexture.loadFromFile("assets/player_walk.png")) return;
    sf::Texture player_Sprite;
    if (!player_Sprite.loadFromFile("assets/player_sprite.png")) return;

    Animation playerAnimation(playerTexture, TILE_SIZE, TILE_SIZE);

    sf::Vector2i gridPosition(1, 1);

    bool isMoving = false, isDialogueActive = false;
    sf::Vector2i direction(0, 0);
    const float speed = 200.0f;
    sf::Clock clock;

    sf::Font font;
    if (!font.loadFromFile("assets/rainyhearts.ttf")) return;

    int dialogueIndex = 0;
    std::vector<std::string> dialogueTexts;

    bool isMenuActive = false;
    bool isInventoryActive = false;
    int currentMenuIndex = 0;
    std::vector<std::string> menuOptions = {"Inventory", "Save", "Game End"};

    std::string currentItemToAdd;
    bool itemAdded = false;

    float totalPlayTime = 0.0f; // Track total playtime

    if (saveSlotIndex >= 0 && saveSlots[saveSlotIndex].isUsed) {
        // Load game state from save
        currentMapIndex = saveSlots[saveSlotIndex].mapIndex;
        gridPosition = saveSlots[saveSlotIndex].playerPosition;
        totalPlayTime = saveSlots[saveSlotIndex].playTime;
        inventory.items = saveSlots[saveSlotIndex].inventory;
        // **Apply Loaded Flag: Restore tile11Completed from the save file**
        std::ifstream saveFile(saveSlots[saveSlotIndex].fileName);
        if (saveFile.is_open()) {
            float playTime;
            int mapIndex, posX, posY, tile11Flag, tile13Flag, tile14Flag, drawerOpenedFlag;
            if (saveFile >> playTime >> mapIndex >> posX >> posY >> tile11Flag >> tile13Flag >> tile14Flag >> drawerOpenedFlag) {
                tile11Completed = (tile11Flag == 1);
                tile13Completed = (tile13Flag == 1);
                tile14Completed = (tile14Flag == 1);
                drawerOpened = (drawerOpenedFlag == 1); // Restore drawerOpened state
            }
        }
    } else {
        tile11Completed = false; // Indicates whether the game has been completed
        tile13Completed = false;
        tile14Completed = false;
        // New game default state
        currentMapIndex = 0;
        gridPosition = {1, 1};
        totalPlayTime = 0.0f;

        // Show preload dialogues for new game
        std::vector<std::string> preLoadDialogues = {
            "For a proper experience, use Z or Space for interactions,",
            "and don't forget to navigate yourself with arrows.",
            "...",
            "Welcome, nameless soul...",
            "This journey will guide you through an unusual experience,",
            "and so it's up to you to face the outcome."
        };
        displayPreLoadDialogue(window, font, preLoadDialogues);
    }

    if (!loadMap(mapFiles[currentMapIndex])) return;

    sf::Vector2f pixelPosition(gridPosition.x * TILE_SIZE, gridPosition.y * TILE_SIZE);

    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        totalPlayTime += deltaTime; // Increment playtime

        sf::Event event;

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();

            if (event.type == sf::Event::KeyPressed) {
                if (isMenuActive) {
                    if (event.key.code == sf::Keyboard::Escape) {
                        isMenuActive = false;
                        isInventoryActive = false;
                    } else if (event.key.code == sf::Keyboard::Up) {
                        currentMenuIndex = (currentMenuIndex - 1 + menuOptions.size()) % menuOptions.size();
                    } else if (event.key.code == sf::Keyboard::Down) {
                        currentMenuIndex = (currentMenuIndex + 1) % menuOptions.size();
                    } else if (event.key.code == sf::Keyboard::Z || event.key.code == sf::Keyboard::Enter) {
                        if (menuOptions[currentMenuIndex] == "Inventory") {
                            isInventoryActive = true;
                        } else if (menuOptions[currentMenuIndex] == "Save") {
                            displaySaveScreen(window, font, player_Sprite, saveSlots, totalPlayTime, gridPosition, inventory);
                        }
                        else if (menuOptions[currentMenuIndex] == "Game End") {
                            window.close();
                        }
                    }
                } else if (isInventoryActive) {
                    if (event.key.code == sf::Keyboard::Escape) {
                        isInventoryActive = false;
                        isMenuActive = true;
                    }
                } else if (isDialogueActive) {
                    if (event.key.code == sf::Keyboard::Z || event.key.code == sf::Keyboard::Space) {
                        if (dialogueIndex < dialogueTexts.size() - 1) {
                            dialogueIndex++;
                        } else {
                            isDialogueActive = false;
                            dialogueIndex = 0;
                            if (!itemAdded && !currentItemToAdd.empty()) {
                                inventory.addItem(currentItemToAdd);
                                currentItemToAdd.clear();
                                itemAdded = true;
                            }
                        }
                    }
                } else {
                    if (event.key.code == sf::Keyboard::Escape) {
                        isMenuActive = true;
                    }
                }
                if (!isMenuActive && !isInventoryActive) {
                    currentMenuIndex = 0;
                }
            }
        }

        if (!isMenuActive) {
            if (!isDialogueActive) {
                direction = {0, 0};
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
                    direction = {-1, 0};
                    playerAnimation.setDirection(1);
                } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
                    direction = {1, 0};
                    playerAnimation.setDirection(2);
                } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
                    direction = {0, -1};
                    playerAnimation.setDirection(3);
                } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
                    direction = {0, 1};
                    playerAnimation.setDirection(0);
                }

                if (!isDialogueActive && canTriggerDialogue &&
                    (sf::Keyboard::isKeyPressed(sf::Keyboard::Z) || sf::Keyboard::isKeyPressed(sf::Keyboard::Space))) {
                    sf::Vector2i facingTile = gridPosition + playerAnimation.getFacingDirection();
                    if (facingTile.x >= 0 && facingTile.x < MAP_WIDTH &&
                        facingTile.y >= 0 && facingTile.y < MAP_HEIGHT) {
                        int tileID = mapData[facingTile.y][facingTile.x];
                        if (fetchDialogueForTile(tileID, inventory, dialogueTexts, currentItemToAdd,
                         window, font, tileTextures, playerAnimation, pixelPosition, mapData)) {
                            isDialogueActive = true;
                            dialogueIndex = 0;
                            itemAdded = false;
                            canTriggerDialogue = false;
                         }
                    }

    }

                if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Z) && !sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
                    canTriggerDialogue = true;
                }

                if (direction != sf::Vector2i(0, 0) && !isMoving) {
                    sf::Vector2i newGridPosition = gridPosition + direction;
                    if (newGridPosition.x >= 0 && newGridPosition.x < MAP_WIDTH &&
                        newGridPosition.y >= 0 && newGridPosition.y < MAP_HEIGHT &&
                        nonPassableTiles.find(mapData[newGridPosition.y][newGridPosition.x]) == nonPassableTiles.end()) {
                        gridPosition = newGridPosition;
                        isMoving = true;
                        playerAnimation.setMoving(true);
                    }
                }
            }

            if (isMoving) {
                sf::Vector2f targetPosition(gridPosition.x * TILE_SIZE, gridPosition.y * TILE_SIZE);
                sf::Vector2f moveDirection = targetPosition - pixelPosition;
                float distance = std::hypot(moveDirection.x, moveDirection.y);
                float step = speed * deltaTime;

                if (step >= distance) {
                    pixelPosition = targetPosition;
                    isMoving = false;
                    playerAnimation.setMoving(false);
                } else {
                    pixelPosition += moveDirection / distance * step;
                }
            }

            playerAnimation.update(deltaTime);
        }

        window.clear(sf::Color(108, 158, 127));
        for (int y = 0; y < MAP_HEIGHT; ++y)
            for (int x = 0; x < MAP_WIDTH; ++x) {
                int tileID = mapData[y][x];
                if (tileTextures.find(tileID) == tileTextures.end()) {
                    sf::Texture texture;
                    texture.loadFromFile("assets/tiles/tile_" + std::to_string(tileID) + ".png");
                    tileTextures[tileID] = texture;
                }
                sf::Sprite sprite(tileTextures[tileID]);
                sprite.setPosition(x * TILE_SIZE, y * TILE_SIZE);
                window.draw(sprite);
            }

        playerAnimation.draw(window, pixelPosition);

        if (isDialogueActive) {
            displayDialogue(window, font, dialogueTexts[dialogueIndex]);
        }

        if (isMenuActive) {
            if (isInventoryActive) {
                displayInventory(window, font, inventory);
            } else {
                displayMenu(window, font, currentMenuIndex, menuOptions, menuOptions[currentMenuIndex]);
            }
        }

        window.display();
    }
}


// Main menu function
void mainMenu(sf::RenderWindow &window) {
    sf::Font font;
    if (!font.loadFromFile("assets/rainyhearts.ttf")) {
        std::cerr << "Error loading font\n";
        return;
    }

    sf::Text title("Another World", font, 50);
    title.setPosition(250, 150);

    sf::Text start("Start Game", font, 40);
    start.setPosition(300, 300);

    sf::Text load("Load Game", font, 40);
    load.setPosition(300, 350);

    sf::Text quit("Quit", font, 40);
    quit.setPosition(300, 400);

    std::vector<SaveSlot> saveSlots = {
        {false, "save1.dat", 0},
        {false, "save2.dat", 0},
        {false, "save3.dat", 0},
        {false, "save4.dat", 0}
    };

    for (auto &slot : saveSlots) {
    std::ifstream saveFile(slot.fileName);
    if (saveFile.is_open()) {
        // Attempt to read basic save data
        float playTime;
        int mapIndex, posX, posY, tile11Flag, tile13Flag, tile14Flag, drawerOpenedFlag;
        if (saveFile >> playTime >> mapIndex >> posX >> posY >> tile11Flag >> tile13Flag >> tile14Flag >> drawerOpenedFlag) {
            slot.playTime = playTime;
            slot.mapIndex = mapIndex;
            slot.playerPosition = {posX, posY};
            tile11Completed = (tile11Flag == 1);
            tile13Completed = (tile13Flag == 1);
            tile14Completed = (tile14Flag == 1);
            drawerOpened = (drawerOpenedFlag == 1);

            // Attempt to read inventory size
            size_t inventorySize;
            if (saveFile >> inventorySize) {
                slot.inventory.clear();
                for (size_t i = 0; i < inventorySize; ++i) {
                    std::string itemName;
                    int quantity;
                    if (saveFile >> itemName >> quantity) {
                        std::replace(itemName.begin(), itemName.end(), '_', ' '); // Restore spaces
                        slot.inventory.emplace_back(itemName, quantity);
                    } else {
                        std::cerr << "Warning: Malformed inventory line at index " << i << " in " << slot.fileName << ".\n";
                        break;
                    }
                }
                slot.isUsed = true; // Mark as a valid save slot
            } else {
                std::cerr << "Warning: Inventory size not found in " << slot.fileName << ".\n";
                slot.isUsed = false;
            }
        } else {
            std::cerr << "Warning: Corrupted or empty save file " << slot.fileName << ".\n";
            slot.isUsed = false;
        }
    } else {
        std::cerr << "Info: Save file " << slot.fileName << " not found. Initializing new slot.\n";
        slot.isUsed = false;
    }
}




    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed) {
                auto mouse = sf::Mouse::getPosition(window);

                if (start.getGlobalBounds().contains(mouse.x, mouse.y)) {
                    game(window, saveSlots);
                }

                if (load.getGlobalBounds().contains(mouse.x, mouse.y)) {
                    int selectedSlot = displayLoadScreen(window, font, saveSlots);
                    if (selectedSlot >= 0) {
                        game(window, saveSlots, selectedSlot); // Load selected save slot
                    }
                }

                if (quit.getGlobalBounds().contains(mouse.x, mouse.y)) {
                    window.close();
                }
            }
        }

        window.clear();
        window.draw(title);
        window.draw(start);
        window.draw(load);
        window.draw(quit);
        window.display();
    }
}



int main()
{

    sf::RenderWindow window(sf::VideoMode(800, 600), "Another World");
    mainMenu(window);
    return 0;
}