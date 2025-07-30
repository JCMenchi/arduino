
#include <util/delay.h>

#include <millisec.h>
#include <CH1115Display.h>

#include <common.h>
#include <screens.h>
#include <highscore.h>
#include <bitmap_font.h>
#include <spaceship_engine.h>
#include <alien_engine.h>
#include <soundmanager.h>

#ifdef HAS_SERIAL
#include <usart_serial.h>
#endif

// Draws the "Game Over" screen with a breathing effect and then returns to the start screen.
void drawGameOver(CH1115Display *display) {
  // Clear the display and show "GAME OVER"
  display->drawScreen(0x00, true);
  display->drawString(40, 24, "GAME OVER");

  // Show the player's score
  display->drawString(30, 40, "SCORE:");
  display->drawInt(30 + 7 * FONT_CHAR_WIDTH, 40, UserScore::CurrentScore);

  // Set display to maximum contrast and enable breathing effect
  display->contrast(0xFF);
  display->breathingEffect(CH1115_ON);
  _delay_ms(5000); // Wait for 5 seconds

  // Disable breathing effect and set contrast to minimum
  display->breathingEffect(CH1115_OFF);
  display->contrast(0x01);

  // Return to the start screen
  drawStart(display);
}

// Draws the start screen with the game title and scrolling "insert coins..." message.
void drawStart(CH1115Display *display) {
  display->breathingEffect(CH1115_OFF); // Ensure breathing effect is off
  display->drawScreen(0x00, true);      // Clear the display
  display->drawString(40, 24, "INVADERS"); // Draw the game title
  display->drawString(3, 54, "insert coins..."); // Draw the prompt message

  // Set up a scrolling area for the prompt message
  display->scrollArea(6, 7, 2, 120, CH1115_SCROLL_RIGHT, CH1115_SCROLL_6FRAMES);
  display->scroll(CH1115_SCROLL_CONTINUOUS); // Start scrolling
}

// Draws the victory screen, applies a breathing effect, waits, then returns to the start screen.
void drawVictory(CH1115Display *display) {
  display->drawScreen(0x00, true); // Clear the display
  display->drawString(40, 24, "YOU WIN"); // Show victory message
  display->drawString(30, 40, "SCORE:");  // Show score label
  display->drawInt(30 + 7 * FONT_CHAR_WIDTH, 40, UserScore::CurrentScore); // Show score

  display->contrast(0xFF); // Max contrast
  display->breathingEffect(CH1115_ON); // Enable breathing effect
  _delay_ms(5000); // Wait for 5 seconds
  display->breathingEffect(CH1115_OFF); // Disable breathing effect
  display->contrast(0x01); // Min contrast
  drawStart(display); // Return to start screen
}

#define SHOW_PERFORMANCE 1

#ifdef SHOW_PERFORMANCE
unsigned long fps_start_time = 0; // Start time for FPS calculation
uint32_t fps_nb_frame = 0;        // Number of frames drawn
#endif

// Draws the main game scene, updates game objects, and optionally prints performance info.
void drawScene(CH1115Display *display, bool first) {
  unsigned long start = milliseconds(); // Record start time for performance

  if (first) {
    // initialize FPS counters
    #ifdef SHOW_PERFORMANCE
    fps_start_time = 0;
    fps_nb_frame = 0;
    #endif

    // Reset screen effects and initialize game objects
    display->scroll(CH1115_SCROLL_OFF);
    move_alien(MOVE_INIT);
    move_spaceship(MOVE_INIT);

    display->drawScreen(0x00); // Clear the display
    draw_shelter(display);     // Draw player shelters
  }

  // Update game objects
  update_spaceship(display); // Update spaceship position and state
  update_alien(display);     // Update alien position and state

  #ifdef SHOW_PERFORMANCE
  // Compute FPS and check drawing time
  unsigned long end = milliseconds();

  fps_nb_frame++;
  if (fps_start_time == 0) {
    fps_start_time = start;
  } else if ((end - fps_start_time) > 10000) { // Every 10 seconds
    #ifdef HAS_SERIAL
    USART_WriteString("FPS: ");
    USART_WriteInt(((fps_nb_frame * 1000) / (end - fps_start_time))); // Print FPS
    USART_WriteString("\n");
    #endif

    fps_start_time = 0;
    fps_nb_frame = 0;
  }

  // Warn if frame took too long to draw
  if ((end - start) > 190) {
    #ifdef HAS_SERIAL
    USART_WriteString("Frame refresh in (ms): ");
    USART_WriteInt(end - start);
    USART_WriteString("\n");
    #endif
  }
  #endif
}

