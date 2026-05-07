# Rogue 
A 2D game written from scratch using only SDL2 and stb_image. 
The game design was inspired by Dragon Quest III and the traditional roguelike.  

  https://youtu.be/vBtpx2ELJWk

2D Game Engine:
- **Renderer**
  - Texture loading through stb_image
  - Basic draw ordering for 2D objects and UI
  
- **Physics / Collision**
  - AABB collision detection and resolution
  - Adjacent combat hitboxes
  - Line-of-sight checks and enemy chase radius logic

- **Audio**
  - Basic sound loading, playback support for music and sfx.

- **Input**
  - Keyboard-driven player movement and menu navigation


Game Design:
- **Game State / Data**
  - Class-based character data
  - Level-based stat growth tables
  - Personality quiz result mapping
  - Separate data tables for RPG progression and player setup

- **Personality Quiz**
  - Branching question flow used during character creation
  - Personality results that influence the player’s stat growth.

- **Class-Based System**
  - Multiple character classes with separate base stats
  - Level-based stat growth tables

- **Roguelike Gameplay**
  - Grid-based movement
  - Enemy detection through line-of-sight and chase radius checks
  - Simple adjacent combat interactions
