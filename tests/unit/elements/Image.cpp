#include <gtest/gtest.h>

#include <element/image/Image.hpp>
#include <hyprgraphics/resource/resources/ImageResource.hpp>
#include <hyprtoolkit/system/Icons.hpp>

#include <limits>

using namespace Hyprtoolkit;

class CDummyIcon final : public ISystemIconDescription {
  public:
    bool exists() override {
        return true;
    }

    bool scalable() override {
        return true;
    }
};

// the cache key has to encode the fit mode, otherwise two elements sharing a path but rendering
// with different fit modes collide on one cached texture (the texture bakes in the fit mode). this
// is the hyprpaper "same image, two monitors, different fit_mode" bug.
TEST(Element, imageCacheStringEncodesFitMode) {
    SImageImpl a, b;
    a.data.path = b.data.path = "wallpaper.png";

    a.data.fitMode = IMAGE_FIT_MODE_COVER;
    b.data.fitMode = IMAGE_FIT_MODE_STRETCH;
    EXPECT_NE(a.getCacheString(), b.getCacheString());

    b.data.fitMode = IMAGE_FIT_MODE_COVER;
    EXPECT_EQ(a.getCacheString(), b.getCacheString());
}

TEST(Element, imageSourcesAreExclusive) {
    SImageData data;
    const auto icon = makeShared<CDummyIcon>();

    data.setData({1, 2, 3});
    EXPECT_FALSE(data.data.empty());
    EXPECT_TRUE(data.path.empty());
    EXPECT_FALSE(data.icon);

    data.setPath("image.png");
    EXPECT_TRUE(data.data.empty());
    EXPECT_EQ(data.path, "image.png");
    EXPECT_FALSE(data.icon);

    data.setIcon(icon);
    EXPECT_TRUE(data.data.empty());
    EXPECT_TRUE(data.path.empty());
    EXPECT_EQ(data.icon, icon);
}

TEST(Element, imageCacheEntryReportsFailure) {
    const auto entry = makeShared<Asset::CAssetCacheEntry>("broken-image");
    int        done  = 0;
    entry->m_events.done.listenStatic([&done] { ++done; });

    entry->fail();

    EXPECT_EQ(entry->status(), Asset::CACHE_ENTRY_FAILED);
    EXPECT_EQ(done, 1);
}

TEST(Element, imageRecognizesScalablePaths) {
    SImageImpl image;

    image.data.path = "image.png";
    EXPECT_FALSE(image.scalable());

    image.data.path = "image.svg";
    EXPECT_TRUE(image.scalable());
}

TEST(Element, imagePixelBufferValidation) {
    std::vector<uint8_t> pixels(4 * 3 * 4);

    EXPECT_TRUE(SImageImpl::pixelBufferValid(pixels, {4, 3}));
    EXPECT_TRUE(SImageImpl::pixelBufferValid(pixels, {2, 2}));
    EXPECT_FALSE(SImageImpl::pixelBufferValid(pixels, {4, 4}));
    EXPECT_FALSE(SImageImpl::pixelBufferValid(pixels, {0, 3}));
    EXPECT_FALSE(SImageImpl::pixelBufferValid(pixels, {-4, 3}));
    EXPECT_FALSE(SImageImpl::pixelBufferValid(pixels, {2.5, 2.0}));
    EXPECT_FALSE(SImageImpl::pixelBufferValid({}, {1, 1}));

    // sizes a broken decoder could produce must not overflow the byte count
    constexpr auto INF  = std::numeric_limits<double>::infinity();
    constexpr auto QNAN = std::numeric_limits<double>::quiet_NaN();
    EXPECT_FALSE(SImageImpl::pixelBufferValid(pixels, {INF, 1.0}));
    EXPECT_FALSE(SImageImpl::pixelBufferValid(pixels, {1.0, INF}));
    EXPECT_FALSE(SImageImpl::pixelBufferValid(pixels, {QNAN, 1.0}));
    EXPECT_FALSE(SImageImpl::pixelBufferValid(pixels, {1.0, QNAN}));
    EXPECT_FALSE(SImageImpl::pixelBufferValid(pixels, {1e300, 1e300}));
    EXPECT_FALSE(SImageImpl::pixelBufferValid(pixels, {4294967296.0, 4294967296.0}));
}
