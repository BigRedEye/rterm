#include "term.h"
#include "font.h"
#include "ttfont.h"
#include "tilefont.h"
#include "mouse.h"
#include "logger.h"

#include <iostream>
#include <functional>

#include <SDL2/SDL_image.h>

#define UNUSED(var) (void)(var);

namespace rterm {
using namespace events;

Term::Term()
    : Term(0, 0) {
}

Term::Term(size_t ncols, size_t nrows)
    : console_(ncols, nrows)
    , p_font_(new TTFont())
    , window_(400, 400)
    , quitRequested_(false)
    , fgCol_(0x00, 0xff, 0x00)
    , bgCol_(0x00, 0x00, 0x00)
     {
    p_tex_ = SdlPtr<SDL_Texture>(SDL_CreateTexture(window_.renderer().lock().get(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
                                  SDL_GetWindowSurface(window_.window().lock().get())->w, SDL_GetWindowSurface(window_.window().lock().get())->h));
    redraw(true);
    eventSystem_.registerCallback(EventType::Quit, [this](Event *ev){
        UNUSED(ev);
        this->close();
    });
    eventSystem_.registerCallback(EventType::WindowExposed, [this](Event *ev){
        UNUSED(ev);
        this->redraw(true);
    });
    eventSystem_.registerCallback(EventType::WindowResized, [this](Event *ev){
        auto event = static_cast<events::WindowResizedEvent*>(ev);
        this->setWindowSize(event->x(), event->y());
        this->redraw(true);
    });
    SDL_StartTextInput();
    eventSystem_.poll();
}

Term::~Term() {
    quitRequested_ = true;
    delete p_font_;
}

size_t Term::cols() const {
    return console_.cols();
}

size_t Term::rows() const {
    return console_.rows();
}

void Term::poll() {
    eventSystem_.poll();
}

bool Term::isRunning() const {
    return !quitRequested_;
}

void Term::delay(uint32_t msec) const {
    SDL_Delay(msec);
}

long double Term::fps() const {
    return frameRateCounter_.getFps();
}

size_t Term::getCursorX() const {
    return console_.cursorX();
}

size_t Term::getCursorY() const {
    return console_.cursorY();
}

void Term::setCursorPosition(size_t x, size_t y) {
    console_.setCursorPosition(x, y);
}

void Term::updateTexture() {
    SDL_Texture * tmp = p_tex_.get();

    int w, h;
    SDL_GetWindowSize(window_.window().lock().get(), &w, &h);
    p_tex_ = SdlPtr<SDL_Texture>(SDL_CreateTexture(window_.renderer().lock().get(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
                                  w - w % p_font_->w(),
                                  h - h % p_font_->h()));

    SDL_SetRenderTarget(window_.renderer().lock().get(), p_tex_.get());
    SDL_SetRenderDrawColor(window_.renderer().lock().get(), bgCol_.r(), bgCol_.g(), bgCol_.b(), bgCol_.a());
    SDL_RenderClear(window_.renderer().lock().get());
    SDL_Rect dstRect{0, 0, 0, 0};
    SDL_QueryTexture(p_tex_.get(), NULL, NULL, &dstRect.w, &dstRect.h);
    SDL_RenderCopy(window_.renderer().lock().get(), tmp, NULL, &dstRect);
    SDL_DestroyTexture(tmp);
    SDL_SetRenderTarget(window_.renderer().lock().get(), NULL);
}

void Term::setWindowSize(size_t width, size_t height) {
    size_t ncols = width / p_font_->w(),
           nrows = height / p_font_->h();
    console_.resize(ncols, nrows, bgCol_, fgCol_);

    /* resize window */
    int curw, curh;
    SDL_GetWindowSize(window_.window().lock().get(), &curw, &curh);
    if (static_cast<size_t>(curw) != width ||
        static_cast<size_t>(curh) != height)
        SDL_SetWindowSize(window_.window().lock().get(), width, height);
    updateTexture();
    SDL_RenderClear(window_.renderer().lock().get());
    redraw(true);
}

Term& Term::setTitle(const std::string &title) {
    SDL_SetWindowTitle(window_.window().lock().get(), title.c_str());
    return *this;
}

Term& Term::setIcon(const std::string &path) {
    SdlPtr<SDL_Surface> p_icon(IMG_Load(path.c_str()));
    if (!p_icon)
        Logger(Logger::CRITICAL) << IMG_GetError();
    else
        SDL_SetWindowIcon(window_.window().lock().get(), p_icon.get());
    return *this;
}

void Term::resize(size_t ncols, size_t nrows) {
    setWindowSize(ncols * p_font_->w(), nrows * p_font_->h());
}

void Term::setChar(size_t x, size_t y, char_t c) {
    console_.set(x, y, Char(c, bgCol_, fgCol_));
}

void Term::addChar(char_t c) {
    console_.addChar(c);
}

void Term::print(size_t x, size_t y, const std::string &fmt, ...) {
    va_list args;
    va_start(args, fmt);

    std::string formatted = vformat(fmt, args);
    size_t prevCursorX = getCursorX(),
           prevCursorY = getCursorY();
    setCursorPosition(x, y);
    for (auto c : formatted)
        addChar(c);
    setCursorPosition(prevCursorX, prevCursorY);
    va_end(args);
}

Key Term::getKey() {
    return eventSystem_.getKey();
}

char_t Term::getChar() {
    return eventSystem_.getChar();
}

void Term::getMousePosition(size_t &x, size_t &y) {
    int mx = 0, my = 0;
    SDL_GetMouseState(&mx, &my);
    x = mx / p_font_->w();
    y = my / p_font_->h();
    x = std::max(std::min(x, cols() - size_t(1)), size_t(0ul));
    y = std::max(std::min(y, rows() - size_t(1)), size_t(0ul));
}

int Term::getMouseButtons() {
    int result = SDL_GetMouseState(NULL, NULL);
    return result;
}

char_t Term::charAt(size_t x, size_t y) const {
    return console_.get(x, y).c();
}

Color Term::bgColorAt(size_t x, size_t y) const {
    return console_.get(x, y).bg();
}

Color Term::fgColorAt(size_t x, size_t y) const {
    return console_.get(x, y).fg();
}
void Term::setFullscreen(bool fullscr) {
    int ncols, nrows;
    static bool isFullscr = false;
    static SDL_DisplayMode *windowedMode = NULL;
    static int prevCols = 0, prevRows = 0;

    if (!isFullscr) {
        if (windowedMode == NULL) {
            windowedMode = new SDL_DisplayMode;
            SDL_GetWindowDisplayMode(window_.window().lock().get(), windowedMode);
        }
        prevCols = cols(), prevRows = rows();
    }

    if (isFullscr == fullscr)
        return;

    SDL_SetWindowFullscreen(window_.window().lock().get(), (fullscr ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));

    /* set correct resolution */
    ncols = prevCols, nrows = prevRows;
    SDL_DisplayMode mode;
    if (fullscr) {
        SDL_GetDesktopDisplayMode(0, &mode);
        ncols = mode.w / p_font_->w();
        nrows = mode.h / p_font_->h();
    }
    else
        mode = *windowedMode;
    SDL_SetWindowDisplayMode(window_.window().lock().get(), &mode);

    isFullscr = !isFullscr;
    resize(ncols, nrows);
    redraw(true);
}

void Term::setResizable(bool resizable) {
#if SDL_MAJOR_VERSION >= 2 && SDL_PATCHLEVEL >= 5
    SDL_SetWindowResizable(window_.window().lock().get(), (resizable ? SDL_TRUE : SDL_FALSE));  
#else
    UNUSED(resizable);
    Logger(Logger::ERROR).printf("SDL version %d.%d.%d doesn't support setWindowResizable, update it to 2.0.5", 
                                  SDL_MAJOR_VERSION,
                                  SDL_MINOR_VERSION,
                                  SDL_PATCHLEVEL);
#endif
}

void Term::setMinWindowSize(size_t width, size_t height) {
    SDL_SetWindowMinimumSize(window_.window().lock().get(), width, height);
}

void Term::setMaxWindowSize(size_t width, size_t height) {
    SDL_SetWindowMaximumSize(window_.window().lock().get(), width, height);
}

void Term::close() {
    quitRequested_ = true;
}

void Term::setFont(const std::string &path, size_t sz) {
    if (p_font_)
        delete p_font_;
    p_font_ = new TTFont(path, sz);
    setWindowSize(p_font_->w() * cols(), p_font_->h() * rows());
    redraw(true);
}

void Term::setFont(const std::string &path, size_t w, size_t h) {
    if (p_font_)
        delete p_font_;

    p_font_ = new TileFont(path, w, h);
    setWindowSize(p_font_->w() * cols(), p_font_->h() * rows());
    redraw(true);
}

void Term::setBgColor(const Color &bg) {
    bgCol_ = bg;
    for (size_t i = 0; i < rows(); ++i)
        for (size_t j = 0; j < cols(); ++j)
            console_.set(j, i, Char(console_.get(j, i).c(),
                                    bg,
                                    console_.get(j, i).fg()));
    redraw();
}

void Term::setBgColor(const Color &bg, size_t x, size_t y) {
    console_.set(x, y, Char(console_.get(x, y).c(),
                            bg,
                            console_.get(x, y).fg()));
}

void Term::setFgColor(const Color &fg) {
    fgCol_ = fg;
    for (size_t i = 0; i < rows(); ++i)
        for (size_t j = 0; j < cols(); ++j)
            console_.set(j, i, Char(
                console_.get(j, i).c(),
                console_.get(j, i).bg(),
                fg
            ));
    redraw();
}

void Term::setFgColor(const Color &fg, size_t x, size_t y) {
    console_.set(x, y, Char(
        console_.get(x, y).c(),
        console_.get(x, y).bg(),
        fg
    ));
}

void Term::redraw(bool force) {
    auto update = console_.getUpdatedChars(force);
    for (auto p : update)
        redraw(p.first, p.second);
    if (wasShift_ || !update.empty() || force)
        renderToScreen();
}

void Term::shift(int dx, int dy) {
    console_.shift(dx, dy);

    SdlPtr<SDL_Texture> prevTexture(p_tex_.release());
    updateTexture();
    SDL_SetRenderTarget(window_.renderer().lock().get(), p_tex_.get());
    SDL_RenderClear(window_.renderer().lock().get());
    int w = static_cast<int>(cols() * p_font_->w()),
        h = static_cast<int>(rows() * p_font_->h());
    SDL_Rect dstRect{0, 0, 0, 0};
    /*
     * 0, 0, w, h -> dx, dy, w - dx, h - dy
     *            -> 0,  0,  w + dx, h + dy */
    dstRect.w = w - abs(dx * static_cast<int>(p_font_->w()));
    dstRect.x = dx > 0 ? dx * static_cast<int>(p_font_->w()) : 0;
    dstRect.h = h - abs(dy * static_cast<int>(p_font_->h()));
    dstRect.y = dy > 0 ? static_cast<int>(dy * p_font_->h()) : 0;
    SDL_Rect srcRect{dstRect.x - dx * static_cast<int>(p_font_->w()),
                     dstRect.y - dy * static_cast<int>(p_font_->h()),
                     dstRect.w,
                     dstRect.h};
    SDL_RenderCopy(window_.renderer().lock().get(), prevTexture.get(), &srcRect, &dstRect);
    wasShift_ = true;
    SDL_SetRenderTarget(window_.renderer().lock().get(), NULL);
}

void Term::renderToScreen() {
    SDL_Rect textureRect{0, 0, 0, 0};
    SDL_Rect windowRect{0, 0, 0, 0};
    SDL_QueryTexture(p_tex_.get(), NULL, NULL, &textureRect.w, &textureRect.h);
    SDL_GetWindowSize(window_.window().lock().get(), &windowRect.w, &windowRect.h);
    SDL_Rect dstRect{0, 0, std::min(textureRect.w, windowRect.w), 
                           std::min(textureRect.h, windowRect.h)};
    SDL_RenderClear(window_.renderer().lock().get());
    SDL_RenderCopy(window_.renderer().lock().get(), p_tex_.get(), &dstRect, &dstRect);

    SDL_RenderPresent(window_.renderer().lock().get());
    
    /* count fps */
    frameRateCounter_.nextFrame(highResClock::now());
}

void Term::redraw(size_t x, size_t y) {
    SDL_Rect dst {static_cast<int>(x * p_font_->w()),
                  static_cast<int>(y * p_font_->h()),
                  static_cast<int>(p_font_->w()),
                  static_cast<int>(p_font_->h())};
    Char ch = console_.get(x, y);
    SDL_SetRenderTarget(window_.renderer().lock().get(), p_tex_.get());
    p_font_->render(window_.renderer().lock().get(), dst, ch.ch_, ch.fg_, ch.bg_);
    SDL_SetRenderTarget(window_.renderer().lock().get(), NULL);
}
}
