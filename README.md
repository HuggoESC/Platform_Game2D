# Platform Game 2D – Assignment 3  
*C++ / SDL3 / Box2D*

This project is a **2D horizontal platformer** developed in **C++** as part of a **university Game Development assignment**.  
It is built on a **custom engine architecture** using **SDL3**, **Box2D**, and an **Entity–Component–Manager approach**, following all the requirements specified in the assignment rubric.

---

## 🎯 Assignment Requirements Coverage

This project fulfills the required features defined in the assignment rubric:

### ✔ Entities and Map Integration
- All gameplay elements (**player, enemies, boss, collectibles, checkpoints, triggers**) are:
  - Defined as **Entities**
  - Loaded dynamically from **Tiled (.tmx) maps**
  - Managed through a centralized **EntityManager**

### ✔ Collectibles
- Implemented **Gem collectibles**
- Loaded from the map (Entities layer in Tiled)
- Collected via collision sensors
- Play a sound effect on pickup
- Displayed in the HUD with a **gem counter**
- Correctly handled in **Save / Load**

### ✔ HUD and UI Feedback
- Player health displayed using **heart-based HUD**
- Gem counter displayed under health
- HUD is **camera-independent**
- Visual feedback for:
  - Health
  - Collected items
  - Attack availability
  - Cooldowns

### ✔ Save & Load System
- **F5** → Save game  
- **F6** → Load game  
- Implemented using **XML (PugiXML)**

The system stores:
- Player position and state
- Camera position
- Active entities (enemies, boss, collectibles not yet collected)
- Boss state (boss logic preserved on load)

### ✔ Enemies and Boss
- Regular enemies implemented as entities
- **Boss enemy** implemented with:
  - Distinct logic
  - Different hit detection
  - Persistent boss state through Save / Load

### ✔ Audio
- Sound effects for:
  - Collectibles
  - Life pickups
  - Player actions
- Audio feedback integrated with gameplay events

### ✔ Physics
- All collisions handled using **Box2D**
- Sensors used for collectibles and triggers
- Proper collision filtering using custom collider types

---

## 🎮 Gameplay Features

- Horizontal 2D platformer gameplay
- Player abilities:
  - Movement (left / right)
  - Double jump
  - Dash
  - Directional attack
- Checkpoints and respawn logic
- Level transitions
- Boss encounter

---

## 🛠️ Technologies Used

- **Language:** C++
- **IDE:** Visual Studio
- **Libraries:**
  - SDL3
  - Box2D
  - PugiXML
- **Tools:**
  - Tiled Map Editor
- **Platform:** Windows (PC)

---

## 🧱 Engine Architecture

The project uses a modular custom engine architecture:

- **Engine Core**
- **Modules**
  - Render
  - Input
  - Audio
  - Physics
  - Scene
  - Map
- **Entity System**
  - Base `Entity` class
  - `EntityManager`
  - Specialized entities:
    - Player
    - Enemy
    - Boss
    - Gem
    - LifeUp
    - Item
    - Triggers

This structure ensures scalability, clarity, and separation of responsibilities.

---

## 🗺️ Level Design

- Levels created using **Tiled**
- Gameplay elements placed in the **Entities layer**
- Dynamic loading of entities at runtime
- Supports multiple levels and level transitions

---

## ⌨️ Controls

### Player Controls
- **A / D** → Move left / right
- **SPACE** → Jump (double jump)
- **SHIFT** → Dash
- **E** → Attack (after picking up the dagger)

### Debug / System
- **F5** → Save game
- **F6** → Load game
- **F10** → God Mode
- **F9** → Collisions
- **T** → Debug teleport

---

## 📂 Project Structure
/Assets
/Audio
/Maps
/Textures
/src
/Entities
/Modules
Engine.cpp
main.cpp


---

## 🚧 Project Status

This project is **actively developed** for academic purposes.

Future improvements may include:
- GUI-based menus (Title, Pause, Settings)
- Further optimization and profiling
- Additional visual polish
- Expanded boss mechanics

---

## 👨‍🎓 Author

- **Name:** Hugo Escobar and Vladimir Solovev  
- **Course:** Game Development  
- **Context:** University Assignment  

---

## 📄 License

This project is developed **for educational purposes only**.  
Non-commercial use.


