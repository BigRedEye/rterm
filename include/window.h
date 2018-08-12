#pragma once


#include "api.h"
#include "sdl_loader.h"
#include "renderer.h"

#include <SDL2/SDL.h>

#include <memory>


namespace rterm {

class Window {
public:
    using AbstractRenderer = Renderer<Api::api>;

    Window();
    Window(int w, int h);

    inline SDL_Window* get() const {
        return window_.get();
    }

    AbstractRenderer& renderer() {
        return renderer_;
    }

    const AbstractRenderer& renderer() const {
        return renderer_;
    }

    void resize(int width, int height);
    void resize(Vector<int, 2> newSize);
    void setTitle(const std::string& title);
    void setIcon(const SoftwareTexture& icon);
    void setMinimumSize(int width, int height);
    void setMaximumSize(int width, int height);

    Vector<int, 2> size() const;

    inline int w() {
        return size()[0];
    }

    inline int h() {
        return size()[1];
    }

private:
    void initOpenGL();
    void initSDL();

private:
    SdlHolder<SDL_Window> window_;
    AbstractRenderer renderer_;
    SdlLoader loader_;
    int w_;
    int h_;
};

} // namespace rterm
