#include "PanelTexture.h"

#include <map>

namespace hs20::ui
{
    namespace
    {
        inline juce::uint32 hash (int x, int y, juce::uint32 seed) noexcept
        {
            auto h = (juce::uint32) x * 0x8da6b343u ^ (juce::uint32) y * 0xd8163841u ^ seed * 0xcb1ab31fu;
            h ^= h >> 13;
            h *= 0x5bd1e995u;
            h ^= h >> 15;
            return h;
        }

        inline float hashFloat (int x, int y, juce::uint32 seed) noexcept
        {
            return (float) (hash (x, y, seed) & 0xffffffu) / 16777216.0f;
        }

        inline float smooth (float t) noexcept { return t * t * (3.0f - 2.0f * t); }

        float valueNoise (float x, float y, juce::uint32 seed) noexcept
        {
            const int ix = (int) std::floor (x), iy = (int) std::floor (y);
            const float fx = smooth (x - (float) ix), fy = smooth (y - (float) iy);
            const float a = hashFloat (ix, iy, seed), b = hashFloat (ix + 1, iy, seed);
            const float c = hashFloat (ix, iy + 1, seed), d = hashFloat (ix + 1, iy + 1, seed);
            return juce::jmap (fy, juce::jmap (fx, a, b), juce::jmap (fx, c, d));
        }

        float fbm (float x, float y, int octaves, juce::uint32 seed) noexcept
        {
            float sum = 0.0f, amp = 0.5f, norm = 0.0f;
            for (int i = 0; i < octaves; ++i)
            {
                sum += amp * valueNoise (x, y, seed + (juce::uint32) i * 101u);
                norm += amp;
                x *= 2.03f;
                y *= 2.03f;
                amp *= 0.5f;
            }
            return sum / norm;
        }

        inline float smoothstep (float e0, float e1, float x) noexcept
        {
            return smooth (juce::jlimit (0.0f, 1.0f, (x - e0) / (e1 - e0)));
        }
    }

    juce::Image PanelTexture::get (int width, int height, const Style& style, float resolution)
    {
        static std::map<juce::String, juce::Image> cache;
        const auto key = juce::String (width) + "x" + juce::String (height) + "@" + juce::String (resolution)
                       + "#" + juce::String ((int) style.seed);

        auto it = cache.find (key);
        if (it != cache.end())
            return it->second;

        auto img = generate (width, height, style, resolution);
        cache[key] = img;
        return img;
    }

    juce::Image PanelTexture::generate (int width, int height, const Style& style, float res)
    {
        const int pw = juce::roundToInt ((float) width * res), ph = juce::roundToInt ((float) height * res);
        juce::Image img (juce::Image::RGB, pw, ph, false);
        const auto seed = style.seed;

        // Per-row offsets give the fine horizontal grain of brushed aluminium.
        std::vector<float> rowGrain ((size_t) ph);
        for (int y = 0; y < ph; ++y)
            rowGrain[(size_t) y] = hashFloat (y, 17, seed) * 2.0f - 1.0f;

        {
            juce::Image::BitmapData bd (img, juce::Image::BitmapData::writeOnly);
            const float W = (float) width, H = (float) height;

            for (int py = 0; py < ph; ++py)
            {
                const float v = (float) py / res;
                const float grainRow = 0.55f * rowGrain[(size_t) py]
                                     + 0.45f * rowGrain[(size_t) juce::jmax (0, py - 1)];

                for (int px = 0; px < pw; ++px)
                {
                    const float u = (float) px / res;

                    const float brushed = 0.55f * grainRow + 0.45f * (valueNoise (u * 0.004f, v * 1.6f, seed + 3u) * 2.0f - 1.0f);
                    const float grime = fbm (u / 150.0f, v / 150.0f, 3, seed + 11u);

                    // Wear: noise + edges + rings around the knobs.
                    const float d = juce::jmin (juce::jmin (u, v), juce::jmin (W - u, H - v));
                    const float edge = std::pow (juce::jlimit (0.0f, 1.0f, 1.0f - d / 30.0f), 2.0f);
                    float scuff = 0.0f;
                    for (const auto& s : style.scuffs)
                    {
                        const float dx = u - s.x, dy = v - s.y;
                        const float dist2 = dx * dx + dy * dy;
                        if (dist2 < 1600.0f)
                        {
                            const float r = std::sqrt (dist2);
                            scuff = juce::jmax (scuff, std::exp (-((r - 27.0f) * (r - 27.0f)) / 40.0f));
                        }
                    }

                    const float wearNoise = fbm (u / 42.0f + 50.0f, v / 42.0f, 4, seed + 29u);
                    const float wearVal = wearNoise * 0.76f + edge * 0.55f;
                    const float bare = smoothstep (0.71f, 0.80f, wearVal * style.wear);
                    const float thin = smoothstep (0.58f, 0.72f, wearVal * style.wear) * 0.35f;

                    float r = style.paint.getFloatRed(), gg = style.paint.getFloatGreen(), b = style.paint.getFloatBlue();
                    // fingers polish the paint in a soft ring around each knob
                    const float paintMul = 1.0f + brushed * 0.07f + (grime - 0.5f) * 0.32f + thin * 0.5f + scuff * 0.12f;
                    r *= paintMul; gg *= paintMul; b *= paintMul;

                    const float metalMul = 1.0f + brushed * 0.16f + (grime - 0.5f) * 0.25f;
                    const float mr = style.metal.getFloatRed() * metalMul;
                    const float mg = style.metal.getFloatGreen() * metalMul;
                    const float mb = style.metal.getFloatBlue() * metalMul;

                    r = juce::jmap (bare, r, mr);
                    gg = juce::jmap (bare, gg, mg);
                    b = juce::jmap (bare, b, mb);

                    // dust and pits
                    const float speck = hashFloat (px, py, seed + 77u);
                    if (speck > 0.9986f)      { r += 0.08f; gg += 0.08f; b += 0.075f; }
                    else if (speck < 0.0012f) { r *= 0.55f; gg *= 0.55f; b *= 0.55f; }

                    // a faint warm patina in the grime
                    r += (grime - 0.5f) * 0.015f;

                    // vignette
                    const float cx = (u / W - 0.5f) * 2.0f, cy = (v / H - 0.5f) * 2.0f;
                    const float vig = 1.0f - 0.16f * (cx * cx * 0.6f + cy * cy);
                    r *= vig; gg *= vig; b *= vig;

                    bd.setPixelColour (px, py, juce::Colour::fromFloatRGBA (juce::jlimit (0.0f, 1.0f, r),
                                                                            juce::jlimit (0.0f, 1.0f, gg),
                                                                            juce::jlimit (0.0f, 1.0f, b), 1.0f));
                }
            }
        }

        // Scratches, drawn as vectors on top of the grain.
        juce::Graphics g (img);
        g.addTransform (juce::AffineTransform::scale (res));
        juce::Random rng ((juce::int64) seed * 7919);
        const int numScratches = juce::roundToInt (((float) width * (float) height / 2600.0f) * style.scratches);

        for (int i = 0; i < numScratches; ++i)
        {
            const float x = rng.nextFloat() * (float) width, y = rng.nextFloat() * (float) height;
            const float len = 6.0f + std::pow (rng.nextFloat(), 2.0f) * 110.0f;
            const float angle = rng.nextFloat() < 0.6f ? (rng.nextFloat() - 0.5f) * 0.6f : rng.nextFloat() * juce::MathConstants<float>::pi;
            const float bend = (rng.nextFloat() - 0.5f) * len * 0.25f;

            const juce::Point<float> a (x, y), b (x + std::cos (angle) * len, y + std::sin (angle) * len);
            const auto mid = (a + b) * 0.5f + juce::Point<float> (-std::sin (angle), std::cos (angle)) * bend;
            juce::Path p;
            p.startNewSubPath (a);
            p.quadraticTo (mid, b);

            const bool deep = rng.nextFloat() < 0.12f;
            if (deep)
            {
                g.setColour (juce::Colours::black.withAlpha (0.35f));
                g.strokePath (p, juce::PathStrokeType (1.1f), juce::AffineTransform::translation (0.0f, 0.6f));
            }
            g.setColour (juce::Colour (0xffd8dadc).withAlpha (deep ? 0.22f : 0.035f + rng.nextFloat() * 0.07f));
            g.strokePath (p, juce::PathStrokeType (deep ? 0.7f : 0.35f + rng.nextFloat() * 0.5f));
        }

        return img;
    }
}
