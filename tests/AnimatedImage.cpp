#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/core/Timer.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Image.hpp>

#include <hyprutils/memory/SharedPtr.hpp>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

using namespace Hyprutils::Memory;
using namespace Hyprutils::Math;
using namespace Hyprtoolkit;

#define SP CSharedPointer

constexpr int               WIDTH = 256, HEIGHT = 256;

static SP<IBackend>         backend;
static SP<CImageElement>    image;
static std::vector<uint8_t> pixels(WIDTH * HEIGHT * 4);
static size_t               frame = 0;

// plasma-ish pattern, regenerated on the cpu every frame and pushed with setPixels()
static void renderFrame() {
    const float T = frame++ / 60.F;

    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            const float    V     = std::sin(x / 16.F + T) + std::sin(y / 12.F - T * 1.3F) + std::sin((x + y) / 20.F + T * 0.7F);
            const uint32_t R     = static_cast<uint32_t>(127.5F + 127.5F * std::sin(V));
            const uint32_t G     = static_cast<uint32_t>(127.5F + 127.5F * std::sin(V + 2.094F));
            const uint32_t B     = static_cast<uint32_t>(127.5F + 127.5F * std::sin(V + 4.188F));
            const uint32_t PIXEL = 0xFF000000 | (R << 16) | (G << 8) | B;

            std::memcpy(&pixels[(y * WIDTH + x) * 4], &PIXEL, 4);
        }
    }

    image->setPixels(pixels, {WIDTH, HEIGHT});
}

static void scheduleFrame() {
    backend->addTimer(
        std::chrono::milliseconds(16),
        [](CAtomicSharedPointer<CTimer>, void*) {
            renderFrame();
            scheduleFrame();
        },
        nullptr);
}

int main(int argc, char** argv, char** envp) {
    backend = IBackend::create();

    auto window = CWindowBuilder::begin()->preferredSize({512, 512})->appTitle("Animated image")->appClass("hyprtoolkit")->commence();

    image = CImageBuilder::begin() //
                ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})
                ->fitMode(IMAGE_FIT_MODE_COVER)
                ->commence();

    window->m_rootElement->addChild(image);

    renderFrame();
    scheduleFrame();

    window->open();

    backend->enterLoop();

    return 0;
}
