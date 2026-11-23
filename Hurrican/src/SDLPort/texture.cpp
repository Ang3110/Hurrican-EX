/**
 *
 *  Copyright (C) 2011-2015 Scott R. Smith
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 *
 */

#include <cstdint>
#include <cstring>
#include <filesystem>
namespace fs = std::filesystem;

#include "DX8Graphics.hpp"
#include "DX8Texture.hpp"
#include "Main.hpp"
#include "Texts.hpp"  //For ReplaceAll() function
#include "texture.hpp"

#if defined(USE_ETC1)
constexpr int ETC1_HEADER_SIZE = 16;
#endif

#if defined(USE_PVRTC)
#  define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG 0x8C00
#  define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG 0x8C01
#  define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 0x8C02
#  define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG 0x8C03

constexpr int PVRTC_HEADER_SIZE = 52;

enum { PVRTC_RGB_2BPP = 0, PVRTC_RGBA_2BPP, PVRTC_RGB_4BPP, PVRTC_RGBA_4BPP };
#endif

// DKS - Textures are now managed in DX8Sprite.cpp in new TexturesystemClass.
//      This function now returns a TextureHandle object, and if it fails to load the file,
//      all of the returned TextureHandle's data members will be set to 0.
//      In th.npot_scalex and th.npot_scaley, it sets a correction
//      factor to apply to each dimension to compensate for any increases in size
//      from power-of-two expansion (each will remain 1.0 if none occurred).
static bool PadFontAtlas(image_t &image, SDL_Point glyphSize, bool whiteMask) {
    const int sourceWidth = static_cast<int>(std::lround(image.w * image.npot_scalex));
    const int sourceHeight = static_cast<int>(std::lround(image.h * image.npot_scaley));
    if (sourceWidth % glyphSize.x != 0 || sourceHeight % glyphSize.y != 0)
        return false;

    const int columns = sourceWidth / glyphSize.x;
    const int rows = sourceHeight / glyphSize.y;
    const int strideX = FontAtlasStride(glyphSize.x);
    const int strideY = FontAtlasStride(glyphSize.y);
    const uint32_t width = nextPowerOfTwo(columns * strideX);
    const uint32_t height = nextPowerOfTwo(rows * strideY);
    constexpr size_t pixelSize = 4;
    std::vector<char> pixels(static_cast<size_t>(width) * height * pixelSize);

    // The menu font uses a white mask tinted by the renderer. Other font sheets
    // retain their authored RGB shading, including the dark pixels in small text.
    if (whiteMask)
        for (size_t i = 0; i < pixels.size(); i += pixelSize)
            std::memset(pixels.data() + i, 0xff, 3);
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < columns; ++col) {
            for (int y = 0; y < glyphSize.y; ++y) {
                const size_t sourceY = row * glyphSize.y + y;
                const char *source = image.data.data() + (sourceY * image.w + col * glyphSize.x) * pixelSize;
                const size_t paddedY = row * strideY + FontAtlasPadding + y;
                const size_t paddedX = col * strideX + FontAtlasPadding;
                char *destination = pixels.data() + (paddedY * width + paddedX) * pixelSize;
                if (whiteMask) {
                    for (int x = 0; x < glyphSize.x; ++x)
                        destination[x * pixelSize + 3] = source[x * pixelSize + 3];
                } else {
                    std::memcpy(destination, source, glyphSize.x * pixelSize);
                }
            }
        }
    }

    image.data = std::move(pixels);
    image.w = width;
    image.h = height;
    image.npot_scalex = static_cast<double>(columns * strideX) / width;
    image.npot_scaley = static_cast<double>(rows * strideY) / height;
    return true;
}

static void BlendParallaxEdges(image_t &image, bool horizontal) {
    const size_t length = horizontal ? image.w : image.h;
    const size_t rows = horizontal ? image.h : image.w;
    const size_t span = std::min(length / 2, std::max<size_t>(1, length / 40));
    if (span == 0)
        return;

    constexpr size_t bytesPerPixel = 4;
    const size_t step = horizontal ? bytesPerPixel : bytesPerPixel * image.w;
    const size_t rowStep = horizontal ? bytesPerPixel * image.w : bytesPerPixel;
    auto *pixels = reinterpret_cast<uint8_t *>(image.data.data());

    // Match the two edge texels, easing back to the original art over the
    // neighboring texels. Weight RGB by alpha so foliage edges stay clear.
    for (size_t offset = 0; offset < span; ++offset) {
        const float t = static_cast<float>(offset) / span;
        const float mix = (1.0f - t * t * (3.0f - 2.0f * t)) * 0.5f;
        for (size_t row = 0; row < rows; ++row) {
            auto *first = pixels + row * rowStep + offset * step;
            auto *last = pixels + row * rowStep + (length - 1 - offset) * step;
            const float firstAlpha = first[3];
            const float lastAlpha = last[3];
            const float alphaAtFirst = firstAlpha * (1.0f - mix) + lastAlpha * mix;
            const float alphaAtLast = firstAlpha * mix + lastAlpha * (1.0f - mix);
            for (int channel = 0; channel < 3; ++channel) {
                const float a = first[channel] * firstAlpha;
                const float b = last[channel] * lastAlpha;
                first[channel] = alphaAtFirst > 0.0f ? std::lround((a * (1.0f - mix) + b * mix) / alphaAtFirst) : 0;
                last[channel] = alphaAtLast > 0.0f ? std::lround((a * mix + b * (1.0f - mix)) / alphaAtLast) : 0;
            }
            first[3] = std::lround(alphaAtFirst);
            last[3] = std::lround(alphaAtLast);
        }
    }
}

// GL_LINEAR mixes straight RGB before alpha blending. Give invisible edge texels
// the color of the visible neighbors so interpolation cannot introduce a black fringe.
static void ExtendTransparentColors(image_t &image, bool repeatX, bool repeatY) {
    auto *rgba = reinterpret_cast<uint8_t *>(image.data.data());
    const int width = static_cast<int>(image.w);
    const int height = static_cast<int>(image.h);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            auto *pixel = rgba + (static_cast<size_t>(y) * width + x) * 4;
            if (pixel[3] != 0)
                continue;
            unsigned sum[3]{};
            unsigned weight = 0;
            for (int dy = -1; dy <= 1; ++dy) {
                int sy = y + dy;
                if (sy < 0 || sy >= height) {
                    if (!repeatY) continue;
                    sy = sy < 0 ? height - 1 : 0;
                }
                for (int dx = -1; dx <= 1; ++dx) {
                    int sx = x + dx;
                    if (sx < 0 || sx >= width) {
                        if (!repeatX) continue;
                        sx = sx < 0 ? width - 1 : 0;
                    }
                    const auto *neighbor = rgba + (static_cast<size_t>(sy) * width + sx) * 4;
                    const unsigned alpha = neighbor[3];
                    weight += alpha;
                    for (int c = 0; c < 3; ++c)
                        sum[c] += alpha * neighbor[c];
                }
            }
            if (weight)
                for (int c = 0; c < 3; ++c)
                    pixel[c] = static_cast<uint8_t>((sum[c] + weight / 2) / weight);
        }
    }
}

bool SDL_LoadTexture(const std::string &path,
                     const std::string &filename,
                     void *buf,
                     unsigned int buf_size,
                     TextureHandle &th,
                     TextureLayout layout,
                     SDL_Point glyphSize) {
    th = TextureHandle{};
    image_t image;
    bool success = false;
    bool load_from_memory = buf_size > 0;
    std::string fullpath;
    std::string filename_sans_ext(filename);
    ReplaceAll(filename_sans_ext, ".png", "");
    const auto pngPath = path + "/" + filename;
    const bool horizontalParallax = layout == TextureLayout::Parallax;
    const bool verticalParallax = layout == TextureLayout::VerticalParallax;
    const bool repeating = layout == TextureLayout::Repeat || layout == TextureLayout::MirrorRepeat ||
                           horizontalParallax || verticalParallax;
    const bool padGlyphs = layout == TextureLayout::Font && glyphSize.x > 0 && glyphSize.y > 0 && fs::is_regular_file(pngPath)
#if defined(USE_ETC1)
        && !DirectGraphics.IsETC1Supported()
#endif
        ;

    if (load_from_memory && !buf) {
        Protokoll << "Error in SDL_LoadTexture() loading texture from memory!" << std::endl;
        GameRunning = false;
        return false;
    } else if (!load_from_memory && filename.empty()) {
        Protokoll << "Error in SDL_LoadTexture(): empty filename parameter" << std::endl;
        GameRunning = false;
        return false;
    }

#if defined(USE_ETC1)
    if (!repeating && DirectGraphics.IsETC1Supported()) {
        image_t alpha_image;

        // First, load the RGB texture:
        fullpath = path + "/etc1/" + filename_sans_ext + ".pkm";
        success = loadImageETC1(image, fullpath);

        if (success) {
            // Then, load the alpha texture:
            fullpath = path + "/etc1/" + filename_sans_ext + "_alpha.pkm";
            success = loadImageETC1(alpha_image, fullpath);
        }

        if (success) {
            success = load_texture(image, th.tex) && load_texture(alpha_image, th.alphatex);
        }

        image.data = std::vector<char>();
        alpha_image.data = std::vector<char>();
        ;

        if (success)
            goto loaded;
        else
            th.tex = th.alphatex = 0;
    }
#endif

#if defined(USE_PVRTC)
    if (!repeating && !padGlyphs && DirectGraphics.IsPVRTCSupported()) {
        fullpath = path + "/pvr/" + filename + ".pvr";

#ifndef NDEBUG
        Protokoll << "Using PVR looking for " << fullpath << std::endl;
#endif

        success = loadImagePVRTC(image, fullpath) && load_texture(image, th.tex);

        image.data = std::vector<char>();
        if (success)
            goto loaded;
        else
            th.tex = 0;
    }
#endif

    success = loadImageSDL(image, pngPath, buf, buf_size, repeating);
    if (success && padGlyphs)
        th.paddedGlyphs = PadFontAtlas(image, glyphSize, filename == "menufont.png");
    if (success && (horizontalParallax || verticalParallax))
        BlendParallaxEdges(image, horizontalParallax);
    if (success && layout != TextureLayout::Font)
        ExtendTransparentColors(image, horizontalParallax || layout == TextureLayout::Repeat,
                                verticalParallax || layout == TextureLayout::Repeat);
    success = success && load_texture(image, th.tex);
    if (success && (th.paddedGlyphs || verticalParallax))
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    if (success && (th.paddedGlyphs || horizontalParallax))
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (success && (layout == TextureLayout::MirrorRepeat || horizontalParallax)) {
#if defined(USE_GLES1)
        if (DirectGraphics.ExtensionSupported("GL_OES_texture_mirrored_repeat"))
#endif
        {
            if (layout == TextureLayout::MirrorRepeat)
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        }
    }
    image.data = std::vector<char>();

    if (success)
        goto loaded;
    else
        th.tex = 0;

loaded:
    if (success) {
        th.instances = 1;
        if (layout == TextureLayout::Sprite)
            th.halfTexel = {0.5f / image.w, 0.5f / image.h};
        th.npot_scalex = image.npot_scalex;
        th.npot_scaley = image.npot_scaley;
    } else {
        Protokoll << "Error loading texture " << filename << " in SDL_LoadTexture()" << std::endl;
        GameRunning = false;
    }

    return success;
}

void SDL_UnloadTexture(TextureHandle &th) {
    glDeleteTextures(1, &th.tex);
    th.tex = 0;
#if defined(USE_ETC1)
    glDeleteTextures(1, &th.alphatex);
    th.alphatex = 0;
#endif
    th.instances = 0;
}

bool load_texture(image_t &image, GLuint &new_texture) {
    GLuint texture;

    if (!image.data.empty()) {
        // Have OpenGL generate a texture object handle for us
        glGenTextures(1, &texture);

        // Bind the texture object
        glBindTexture(GL_TEXTURE_2D, texture);

        // Set the texture's stretching properties
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // DKS - this fixes the lightning-beam drawing in low,med-detail modes..
        //      and from looking at its code in Player.cpp, I am convinced this is
        //      correct edge setting for textures for this game. My play-testing
        //      shows no unwanted side effects from the change to GL_REPEAT.
        // glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        // glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        if (image.compressed) {
            glCompressedTexImage2D(GL_TEXTURE_2D, 0, image.format, image.w, image.h, 0, image.data.size(),
                                   image.data.data() + image.offset);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, image.format, image.w, image.h, 0, image.format, image.type,
                         image.data.data());
        }

#ifndef NDEBUG
        int error = glGetError();
        if (error != 0) {
            Protokoll << "GL load_texture Error " << error << std::endl;
            Protokoll << "Format " << std::hex << image.format << std::dec << " W " << image.w << " H " << image.h
                      << " S " << image.data.size() << " Data " << std::hex << reinterpret_cast<std::uintptr_t>(image.data.data())
                      << std::dec << " + " << image.offset << std::endl;
            return false;
        }
#endif

    } else {
        Protokoll << "ERROR Image data reference is NULL" << std::endl;
        return false;
    }

    new_texture = texture;
    return true;
}

#if defined(USE_ETC1)
bool loadImageETC1(image_t &image, const std::string &fullpath) {
    /*
        00-03 4 bytes header "PKM "
        04-05 2 bytes version "10"
        06-07 2 bytes data type (always zero)
        08-09 2 bytes extended width
        10-11 2 bytes extended height
        12-13 2 bytes original width
        14-15 2 bytes original height
        rest is data

        compressed size = (extended width / 4) * (extended height / 4) * 8
    */

    uint32_t etc1_filesize;

    if (fullpath.empty() || !fs::exists(fullpath) || !fs::is_regular_file(fullpath))
        return false;

    image.data = LoadFileToMemory(fullpath);

    if (!image.data.empty()) {
        if ((image.data[0] == 'P') && (image.data[1] == 'K') && (image.data[2] == 'M') && (image.data[3] == ' ')) {
            image.format = GL_ETC1_RGB8_OES;
            image.h = (image.data[14] << 8) + image.data[15];
            image.w = (image.data[12] << 8) + image.data[13];
            // image.size      = (((image.data[8]<<8)+image.data[9]) / 4) * (((image.data[10]<<8)+image.data[11]) / 4) *
            // 8;
            image.offset = ETC1_HEADER_SIZE;
            image.type = 0; /* dont care */
            image.compressed = true;

            Protokoll << "Loaded ETC type " << static_cast<char>(image.data[0]) << static_cast<char>(image.data[1])
                      << static_cast<char>(image.data[2]) << " for " << fullpath << std::endl;

#ifndef NDEBUG
            Protokoll << "Header " << static_cast<char>(image.data[0]) << static_cast<char>(image.data[1])
                      << static_cast<char>(image.data[2]) << static_cast<char>(image.data[3]) << "\nVersion "
                      << std::hex << (image.data[4] << 8) + image.data[5] << "\nType " << std::dec
                      << (image.data[6] << 8) + image.data[7] << "\nExt Width " << (image.data[8] << 8) + image.data[9]
                      << "\nExt Height " << (image.data[10] << 8) + image.data[11] << "\nWidth "
                      << (image.data[12] << 8) + image.data[13] << "\nHeight " << (image.data[14] << 8) + image.data[15]
                      << std::endl;
#endif
            return true;
        } else {
            Protokoll << "ERROR Unknown file type " << image.data[0] << image.data[1] << image.data[2] << image.data[3]
                      << std::endl;
            image.data = std::vector<char>();
        }
    }

    return false;
}
#endif

#if defined(USE_PVRTC)
bool loadImagePVRTC(image_t &image, const std::string &fullpath) {
    uint32_t *pvrtc_buffer32 = nullptr;
    uint32_t pvrtc_filesize, pvrtc_depth, pvrtc_bitperpixel;

    if (fullpath.empty() || !fs::exists(fullpath) || !fs::is_regular_file(fullpath))
        return false;

    image.data = LoadFileToMemory(fullpath);

    if (!image.data.empty()) {
        pvrtc_buffer32 = (uint32_t *)image.data.data();

        switch (pvrtc_buffer32[2]) {
            case PVRTC_RGB_2BPP:
                image.format = GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
                pvrtc_bitperpixel = 2;
                break;
            case PVRTC_RGBA_2BPP:
                image.format = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
                pvrtc_bitperpixel = 2;
                break;
            case PVRTC_RGB_4BPP:
                image.format = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
                pvrtc_bitperpixel = 4;
                break;
            case PVRTC_RGBA_4BPP:
                image.format = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
                pvrtc_bitperpixel = 4;
                break;
            default:
                Protokoll << "ERROR Unknown PVRTC format " << std::hex << pvrtc_buffer32[2] << std::endl;
                // delete [] image.data;
                // image.data = nullptr;
                image.data = std::vector<char>();
                return false;
        }

        image.h = pvrtc_buffer32[6];
        image.w = pvrtc_buffer32[7];
        pvrtc_depth = pvrtc_buffer32[8];
        // image.size      = (image.w * image.h * pvrtc_depth * pvrtc_bitperpixel) / 8;
        image.offset = PVRTC_HEADER_SIZE;
        image.type = 0; /* dont care */
        image.compressed = true;

        Protokoll << "Loaded PVRTC type " << std::dec << pvrtc_buffer32[2] << " for " << fullpath << std::endl;

#ifndef NDEBUG
        Protokoll << "Version " << std::hex << pvrtc_buffer32[0] << "\nFlags " << std::dec << pvrtc_buffer32[1]
                  << "\nPFormatA " << pvrtc_buffer32[2] << "\nPFormatB " << pvrtc_buffer32[3] << "\nColorS "
                  << pvrtc_buffer32[4] << "\nChanType " << pvrtc_buffer32[5] << "\nHeight " << pvrtc_buffer32[6]
                  << "\nWidth " << pvrtc_buffer32[7] << "\nDepth " << pvrtc_buffer32[8] << "\nNumSurf "
                  << pvrtc_buffer32[9] << "\nNumFaces " << pvrtc_buffer32[10] << "\nMipmap " << pvrtc_buffer32[11]
                  << "\nMeta " << pvrtc_buffer32[12] << std::endl;
#endif

        return true;
    }

    return false;
}
#endif

bool loadImageSDL(image_t &image, const std::string &fullpath, void *buf, unsigned int buf_size, bool nativeSize) {
    SDL_Surface *rawSurf = nullptr;  // This surface will tell us the details of the image
    SDL_Surface *finSurf = nullptr;

    // Init
    image.data = std::vector<char>();
    image.compressed = false;
    image.format = GL_RGBA;
    image.npot_scalex = 1.0;
    image.npot_scaley = 1.0;

    if (buf_size == 0)  // Load from file
    {
        if (fullpath.empty() || !fs::exists(fullpath) || !fs::is_regular_file(fullpath)) {
            Protokoll << "Error in loadImageSDL loading " << fullpath << std::endl;
            GameRunning = false;
            return false;
        }

        rawSurf = IMG_Load(fullpath.c_str());
    } else  // Load from memory
    {
        SDL_RWops *sdl_rw = SDL_RWFromConstMem(reinterpret_cast<const void *>(buf), buf_size);

        if (sdl_rw != nullptr) {
            rawSurf = IMG_Load_RW(sdl_rw, 1);
        } else {
            Protokoll << "ERROR Texture: Failed to load texture: " << SDL_GetError() << std::endl;
            GameRunning = false;
            return false;
        }
    }

    if (rawSurf != nullptr) {
        SDL_Rect rawDimensions;
        //  Store dimensions of original RAW surface
        rawDimensions.x = rawSurf->w;
        rawDimensions.y = rawSurf->h;

        // DKS - Allowing NPOT textures is now a command line switch:
        //      The safest thing seems to me to leave it off unless specifically requested,
        //      since some GPUs/drivers do not play nicely with it and old GL/GLES doesn't even pretend to.
        if (!nativeSize && !CommandLineParams.AllowNPotTextureSizes) {
            //  Check if surface is PoT
            if (!isPowerOfTwo(rawSurf->w)) {
                rawDimensions.x = nextPowerOfTwo(rawSurf->w);
                image.npot_scalex = static_cast<double>(rawSurf->w) / static_cast<double>(rawDimensions.x);
            }

            if (!isPowerOfTwo(rawSurf->h)) {
                rawDimensions.y = nextPowerOfTwo(rawSurf->h);
                image.npot_scaley = static_cast<double>(rawSurf->h) / static_cast<double>(rawDimensions.y);
            }
        }

        finSurf = SDL_CreateRGBSurface(SDL_SWSURFACE, rawDimensions.x, rawDimensions.y, 32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN  // OpenGL RGBA masks
                                       0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
#else
                                       0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF
#endif
        );

#if SDL_VERSION_ATLEAST(2, 0, 0)
        SDL_SetSurfaceAlphaMod(rawSurf, 255);
        SDL_SetSurfaceBlendMode(rawSurf, SDL_BLENDMODE_NONE);
#else /* SDL 1.2 */
        SDL_SetAlpha(rawSurf, 0, 0);
#endif

        SDL_FillRect(finSurf, nullptr, 0);
        SDL_BlitSurface(rawSurf, nullptr, finSurf, nullptr);
        SDL_FreeSurface(rawSurf);

#if defined(USE_GLES1) || defined(USE_GLES2)
        // Limited NPOT implementations only allow clamping. Fill the whole
        // texture on those devices so repeating still avoids transparent padding.
        if (nativeSize && (!isPowerOfTwo(finSurf->w) || !isPowerOfTwo(finSurf->h)) &&
            !DirectGraphics.ExtensionSupported("GL_OES_texture_npot")) {
            SDL_Surface *expanded = SDL_CreateRGBSurface(SDL_SWSURFACE, nextPowerOfTwo(finSurf->w),
                nextPowerOfTwo(finSurf->h), 32, finSurf->format->Rmask, finSurf->format->Gmask,
                finSurf->format->Bmask, finSurf->format->Amask);
            if (!expanded || SDL_SoftStretch(finSurf, nullptr, expanded, nullptr) != 0) {
                SDL_FreeSurface(expanded);
                SDL_FreeSurface(finSurf);
                return false;
            }
            SDL_FreeSurface(finSurf);
            finSurf = expanded;
        }
#endif

        uint8_t factor = 1;

        if (((finSurf->w >= CommandLineParams.TexSizeMin) || (finSurf->h >= CommandLineParams.TexSizeMin)) &&
            (CommandLineParams.TexFactor > 1)) {
            factor = CommandLineParams.TexFactor;
        }

        // Blacklist of image filenames (sub-strings) that shouldn't ever be resized, because of
        // resulting graphics glitches
        if (fullpath.find("font") != std::string::npos
            //|| fullpath.find("lightmap")     != std::string::npos           // Lightmaps were never actually used in
            //the game
            || fullpath.find("hurrican_rund") != std::string::npos  // Menu star/nebula background (ugly)
            || fullpath.find("roboraupe") != std::string::npos      // Flat spiky enemy worm-like thing (glitches)
            || fullpath.find("enemy-walker") != std::string::npos   // Frog-like robotic walker (glitches)
            || fullpath.find("stelzsack") != std::string::npos      // Stilt-walker enemy on elevator level
        ) {
            factor = 1;
        }

        if (factor > 1)
            image.data = LowerResolution(finSurf, factor);

        image.w = finSurf->w / factor;
        image.h = finSurf->h / factor;

        if (image.data.empty()) {
            image.data.resize(
                static_cast<size_t>(finSurf->w) * static_cast<size_t>(finSurf->h) * sizeof(uint32_t)
            );
            std::memcpy(image.data.data(), finSurf->pixels, image.data.size());
            image.type = GL_UNSIGNED_BYTE;
        }

        SDL_FreeSurface(finSurf);
    } else {
        Protokoll << "Error in loadImageSDL: Could not read image data into rawSurf" << std::endl;
        GameRunning = false;
        return false;
    }

    return true;
}

// DKS - Assumes rgba8888 input, rgba8888 output
std::vector<char> LowerResolution(SDL_Surface *surface, int factor) {
    if (factor != 2 && factor != 4) {
        Protokoll << "ERROR call to LowerResolution() with factor not equal to 2 or 4" << std::endl;
        return std::vector<char>();
    }

    if (surface->format->BytesPerPixel != 4) {
        Protokoll << "ERROR call to LowerResolution() with source surface bpp other than 4" << std::endl;
        return std::vector<char>();
    }

    const int width = surface->w / factor;
    const int height = surface->h / factor;
    std::vector<char> dataout(static_cast<size_t>(width) * height * 4);
    for (int y = 0; y < height; ++y) {
        const auto *row = static_cast<const char *>(surface->pixels) + y * factor * surface->pitch;
        for (int x = 0; x < width; ++x)
            std::memcpy(dataout.data() + (static_cast<size_t>(y) * width + x) * 4, row + x * factor * 4, 4);
    }
    return dataout;
}
