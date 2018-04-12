#include <SDL2/SDL.h>

#ifndef RTERM_WITHOUT_IMG
#include <SDL2/SDL_image.h>
#else
int IMG_Init(int) { return 0; }
void IMG_Quit() {}
#endif // RTERM_WITHOUT_IMG

#ifndef RTERM_WITHOUT_TTF
#include <SDL2/SDL_ttf.h>
#else
int TTF_Init(int) { return 0; }
void TTF_Quit() {}
#endif // RTERM_WITHOUT_TTF

#include "sdl_loader.h"
#include "sdl_lock.h"

namespace rterm {

int SdlLoader::countOfLoads_ = 0;

SdlLoader::SdlLoader() {
    if (countOfLoads_ == 0) {
        SDL_Init(SDL_INIT_EVERYTHING);
        TTF_Init();
        IMG_Init(IMG_INIT_PNG);
    }
    ++countOfLoads_;
}

SdlLoader::~SdlLoader() {
    --countOfLoads_;
    if (countOfLoads_ == 0) {
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
    }
}

}