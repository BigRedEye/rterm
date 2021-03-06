/**
  @file
  @brief basic rterm functionality
*/

#pragma once

#include "rterm/rasterizer/glyph_cache.h"
#include "rterm/renderer/software_texture.h"
#include "rterm/term/term_format.h"
#include "rterm/term/virtualconsole.h"

#include "rterm/window/event_system.h"
#include "rterm/window/key.h"
#include "rterm/window/window.h"

#include "rterm/util/char.h"
#include "rterm/util/color.h"
#include "rterm/util/framerate_counter.h"
#include "rterm/util/sdl/sdl_ptr.h"

#include <SDL.h>

#include <chrono>
#include <cstddef>

namespace rterm {

/**
 * @brief Terminal class
 *
 * This class controls event handling, terminal window and rendering
 */
class Term {
public:
    /**
     * @brief Constructs empty terminal
     */
    Term();

    /**
     * @brief Constructs terminal with fixed size
     * @param cols columns
     * @param rows rows
     */
    Term(size_t cols, size_t rows)
        : Term(TermFormat(cols, rows)) {
    }

    /*
     * @brief Constructs terminal with given fomat
     * @param format
     */
    Term(const TermFormat& format);

    ~Term();

    /**
     * @brief Get if terminal is running
     * @return true if quit wasn't requested by user or system
     * @return false otherwise
     */
    bool isRunning() const;

    /**
     * @brief Get number of columns
     * @return number of columns in terminal
     */
    size_t cols() const;

    /**
     * @brief Get number of rows
     * @return number of rows in terminal
     */
    size_t rows() const;

    /**
     * @brief Poll events
     */
    void poll();

    /**
     * @brief Wait for some time
     * @param msec number of milliseconds to wait
     */
    void delay(uint32_t msec);

    /**
     * @brief Get fps
     * @return frames per second
     */
    long double fps() const;

    /**
     * @brief Get cursor X
     * @return cursor X position
     */
    size_t getCursorX() const;

    /**
     * @brief Get cursor Y
     * @return cursor y position
     */
    size_t getCursorY() const;

    /**
     * @brief Set cursor position
     * @param x new cursor x
     * @param y new cursor y
     */
    void setCursorPosition(size_t x, size_t y);

    /**
     * @brief Set terminal size in characters, not pixels
     * @param cols_ new number of columns
     * @param rows_ new number of rows
     */
    void resize(size_t cols_, size_t rows_);

    /**
     * @brief Set terminal window size
     * @param width in pixels
     * @param height in pixels
     * @note This function can adjust logical size of terminal in characters
     */
    void setWindowSize(size_t width, size_t height);

    /**
     * @brief Set window title
     * @param title
     * @return reference to this Term object
     */
    Term& setTitle(const std::string& title);

    /**
     * @brief Set window icon
     * @param path path to icon file
     * @return reference to this Term object
     */
    Term& setIcon(const std::string& path);

    /**
     * @brief Set unicode character on certain position
     * @param x position of char
     * @param y position of char
     * @param c UTF8 char
     * @note This function won't redraw terminal window
     * @see redraw
     */
    void setChar(size_t x, size_t y, ch32 c);

    /**
     * @brief Set true type terminal font
     * @param path to a .ttf file
     * @param sz size of font
     */
    void setFont(const std::string& path, size_t sz);

    /**
     * @brief Set tiled terminal font
     * @param path to a tilemap
     * @param w with of a tile in pixels
     * @param h height of a tile in pixels
     */
    void setFont(const std::string& path, size_t w, size_t h);

    /**
     * @brief Set default background color
     * @param bg background color
     */
    void setBgColor(const Color& bg);

    /**
     * @brief Set background color at certain position
     * @param bg background color
     * @param x position
     * @param y position
     */
    void setBgColor(const Color& bg, size_t x, size_t y);

    /**
     * @brief Set default foreground color
     * @param fg foreground color
     */
    void setFgColor(const Color& fg);

    /**
     * @brief Set foreground color at certain position
     * @param fg foreground color
     * @param x position
     * @param y position
     */
    void setFgColor(const Color& fg, size_t x, size_t y);

    /**
     * @brief Set terminal window fullscreen
     * @param fullscreen if true, set fullscreen, otherwise set terminal in a
     * windowed mode
     */
    void setFullscreen(bool fullscreen);

    /**
     * @brief Set terminal window resizable
     * @param resizable if true, set resizable, otherwise false
     * @note this function requires SDL version not lower than 2.0.5
     */
    void setResizable(bool resizable);

    /**
     * @brief Set minimum window size
     * @param width minimum width in pixels
     * @param height minimum width in pixels
     */
    void setMinWindowSize(size_t width, size_t height);

    /**
     * @brief Set maximum window size
     * @param width maximum width in pixels
     * @param height maximum width in pixels
     */
    void setMaxWindowSize(size_t width, size_t height);

    /**
     * @brief Close terminal
     */
    void close();

    /**
     * @brief Wait for the next pressed key
     * @return Key object represents next pressed key
     * @note if during this function call the terminal will be closed it will
     * return Key(rterm::UNKNOWN)
     * @see getChar if you want only printable characters
     */
    Key getKey();

    /**
     * @brief Wait for the next printable character
     * @return Key object represents next pressed printable key
     * @note if during this function call the terminal will be closed it will
     * return Key(rterm::UNKNOWN)
     * @see getKey if you doesn't want only printable characters
     */
    ch32 getChar();

    /**
     * @brief Get mouse position in tile
     * @param[out] x mouse position
     * @param[out] y mouse position
     */
    void getMousePosition(size_t& x, size_t& y);

    /**
     * @brief Get pressed mouse buttons
     * @return OR-ed combination of rterm::MouseButton constants
     */
    int getMouseButtons();

    /**
     * @brief Write next character and move cursor forward
     * @param c unicode char to write
     */
    void addChar(ch32 c);

    /**
     * @brief Print printf-style fomated data
     * @param x position of the first character
     * @param y position of the first character
     * @param fmt format
     */
    template<typename... Args>
    void print(size_t x, size_t y, const std::string& fmt, Args&&... args) {
        print(x, y, format(fmt, std::forward<Args>(args)...));
    }

    /**
     * @brief Print utf8 string
     * @param x position of the first character
     * @param y position of the first character
     * @param str string
     */
    void print(size_t x, size_t y, const std::string& str);

    /**
     * @brief Draw all changes to screen
     * @param force if true the whole screen will be redrawn otherwise only
     * updated part
     *
     * This function applies all changes since last redraw() call and renders
     * them on the screen
     */
    void redraw(bool force = false);

    /**
     * @brief Get char at specific position
     * @param x position
     * @param y position
     * @return unicode char at (x, y)
     */
    ch32 charAt(size_t x, size_t y) const;

    /**
     * @brief Get background color at specific position
     * @param x position
     * @param y position
     * @return background color at (x, y)
     */
    Color bgColorAt(size_t x, size_t y) const;

    /**
     * @brief Get foreground color at specific position
     * @param x position
     * @param y position
     * @return foreground color at (x, y)
     */
    Color fgColorAt(size_t x, size_t y) const;

    /**
     * @brief Get screen image
     * @return SoftwareTexture contains screen image
     * @note This function is extremely slow and should only be used for debug / testing purposes
     */
    SoftwareTexture dumpScreen() const;

    std::string getError() const;

    template<typename F>
    void onKeyDown(F&& callback);

    template<typename F>
    void onKeyUp(F&& callback);

    template<typename F>
    void onMouseMove(F&& callback);

    template<typename F>
    void onMouseDown(F&& callback);

    template<typename F>
    void onMouseUp(F&& callback);

    template<typename F>
    void onMouseWheel(F&& callback);

    template<typename F>
    void onWindowResized(F&& callback);

    template<typename F>
    void onWindowMoved(F&& callback);

    template<typename F>
    void onWindowShown(F&& callback);

    template<typename F>
    void onWindowHidden(F&& callback);

private:
    /// @cond INTERNAL

    void setupCallbacks();

    /**
     * @brief Redraw specific character
     * @param x position
     * @param y position
     */
    void redrawGlyph(size_t x, size_t y);

    void redrawBackground(const std::vector<std::pair<ui32, ui32>>& updates);

    void redrawForeground(const std::vector<std::pair<ui32, ui32>>& updates);

    /**
     * @brief Get reference to Char at (x, y)
     * @param x position
     * @param y position
     * @return reference to Char at specific position
     */
    Char& get(size_t x, size_t y);

    /**
     * @brief Get Char at (x, y)
     * @param x position
     * @param y position
     * @return Char at (x, y)
     */
    Char get(size_t x, size_t y) const;

    /**
     * @brief Render texture to screen
     */
    void renderToScreen();

private:
    SdlLoader loader_;       ///< keeps SDL loaded
    VirtualConsole console_; ///< logical console

    Window window_;         ///< wrapper around SDL_Window
    GlyphCache glyphCache_; ///< glyph renderer

    bool quitRequested_; ///< was quit requested by user or system
    bool wasShift_;      ///< was screen shifted

    Color fgCol_; ///< default foreground color
    Color bgCol_; ///< default background color

    FrameRateCounter frameRateCounter_; ///< framerate counter

    EventSystem eventSystem_; ///< event system
    /// @endcond
};

/// @cond INTERNAL
/**
 * @brief SDL event filter
 * @param data pointer to terminal
 * @param ev next event
 * @return 1
 */
int unusedEventFilter(void* data, SDL_Event* ev);

/// @endcond

} // namespace rterm

/* template members implementation */
#include "term.tpp"
