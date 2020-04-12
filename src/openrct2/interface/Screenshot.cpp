/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Screenshot.h"

#include "../Context.h"
#include "../Game.h"
#include "../Intro.h"
#include "../OpenRCT2.h"
#include "../actions/SetCheatAction.hpp"
#include "../audio/audio.h"
#include "../core/Console.hpp"
#include "../core/Imaging.h"
#include "../drawing/Drawing.h"
#include "../drawing/X8DrawingEngine.h"
#include "../localisation/Localisation.h"
#include "../platform/platform.h"
#include "../util/Util.h"
#include "../world/Climate.h"
#include "../world/Map.h"
#include "../world/Park.h"
#include "../world/Surface.h"
#include "Viewport.h"

#include <cctype>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>

using namespace std::literals::string_literals;
using namespace OpenRCT2;
using namespace OpenRCT2::Drawing;

uint8_t gScreenshotCountdown = 0;

static bool WriteDpiToFile(const std::string_view& path, const rct_drawpixelinfo* dpi, const rct_palette& palette)
{
    auto const pixels8 = dpi->bits;
    auto const pixelsLen = (dpi->width + dpi->pitch) * dpi->height;
    try
    {
        Image image;
        image.Width = dpi->width;
        image.Height = dpi->height;
        image.Depth = 8;
        image.Stride = dpi->width + dpi->pitch;
        image.Palette = std::make_unique<rct_palette>(palette);
        image.Pixels = std::vector<uint8_t>(pixels8, pixels8 + pixelsLen);
        Imaging::WriteToFile(path, image, IMAGE_FORMAT::PNG);
        return true;
    }
    catch (const std::exception& e)
    {
        log_error("Unable to write png: %s", e.what());
        return false;
    }
}

/**
 *
 *  rct2: 0x006E3AEC
 */
void screenshot_check()
{
    if (gScreenshotCountdown != 0)
    {
        gScreenshotCountdown--;
        if (gScreenshotCountdown == 0)
        {
            // update_rain_animation();
            std::string screenshotPath = screenshot_dump();

            if (!screenshotPath.empty())
            {
                audio_play_sound(SoundId::WindowOpen, 100, context_get_width() / 2);
            }
            else
            {
                context_show_error(STR_SCREENSHOT_FAILED, STR_NONE);
            }

            // redraw_rain();
        }
    }
}

static rct_palette screenshot_get_rendered_palette()
{
    rct_palette palette;
    for (int32_t i = 0; i < 256; i++)
    {
        palette.entries[i] = gPalette[i];
    }
    return palette;
}

static std::string screenshot_get_park_name()
{
    return GetContext()->GetGameState()->GetPark().Name;
}

static std::string screenshot_get_directory()
{
    char screenshotPath[MAX_PATH];
    platform_get_user_directory(screenshotPath, "screenshot", sizeof(screenshotPath));
    return screenshotPath;
}

static std::pair<rct2_date, rct2_time> screenshot_get_date_time()
{
    rct2_date date;
    platform_get_date_local(&date);

    rct2_time time;
    platform_get_time_local(&time);

    return { date, time };
}

static std::string screenshot_get_formatted_date_time()
{
    auto [date, time] = screenshot_get_date_time();
    char formatted[64];
    snprintf(
        formatted, sizeof(formatted), "%4d-%02d-%02d %02d-%02d-%02d", date.year, date.month, date.day, time.hour, time.minute,
        time.second);
    return formatted;
}

static std::optional<std::string> screenshot_get_next_path()
{
    auto screenshotDirectory = screenshot_get_directory();
    if (!platform_ensure_directory_exists(screenshotDirectory.c_str()))
    {
        log_error("Unable to save screenshots in OpenRCT2 screenshot directory.");
        return {};
    }

    auto parkName = screenshot_get_park_name();
    auto dateTime = screenshot_get_formatted_date_time();
    auto name = parkName + " " + dateTime;

    // Generate a path with a `tries` number
    auto pathComposer = [&screenshotDirectory, &name](int tries) {
        auto composedFilename = platform_sanitise_filename(
            name + ((tries > 0) ? " ("s + std::to_string(tries) + ")" : ""s) + ".png");
        return screenshotDirectory + PATH_SEPARATOR + composedFilename;
    };

    for (int tries = 0; tries < 100; tries++)
    {
        auto path = pathComposer(tries);
        if (!platform_file_exists(path.c_str()))
        {
            return path;
        }
    }

    log_error("You have too many saved screenshots saved at exactly the same date and time.");
    return {};
};

std::string screenshot_dump_png(rct_drawpixelinfo* dpi)
{
    // Get a free screenshot path
    auto path = screenshot_get_next_path();

    if (path == std::nullopt)
    {
        return "";
    }

    auto renderedPalette = screenshot_get_rendered_palette();
    if (WriteDpiToFile(path->c_str(), dpi, renderedPalette))
    {
        return *path;
    }
    else
    {
        return "";
    }
}

std::string screenshot_dump_png_32bpp(int32_t width, int32_t height, const void* pixels)
{
    auto path = screenshot_get_next_path();

    if (path == std::nullopt)
    {
        return "";
    }

    const auto pixels8 = (const uint8_t*)pixels;
    const auto pixelsLen = width * 4 * height;

    try
    {
        Image image;
        image.Width = width;
        image.Height = height;
        image.Depth = 32;
        image.Stride = width * 4;
        image.Pixels = std::vector<uint8_t>(pixels8, pixels8 + pixelsLen);
        Imaging::WriteToFile(path->c_str(), image, IMAGE_FORMAT::PNG_32);
        return *path;
    }
    catch (const std::exception& e)
    {
        log_error("Unable to save screenshot: %s", e.what());
        return "";
    }
}

enum class EdgeType
{
    LEFT,
    TOP,
    RIGHT,
    BOTTOM
};

static CoordsXY GetEdgeTile(int32_t mapSize, int32_t rotation, EdgeType edgeType, bool visible)
{
    int32_t lower = (visible ? 1 : 0) * 32;
    int32_t upper = (visible ? mapSize - 2 : mapSize - 1) * 32;
    switch (edgeType)
    {
        default:
        case EdgeType::LEFT:
            switch (rotation)
            {
                default:
                case 0:
                    return { upper, lower };
                case 1:
                    return { upper, upper };
                case 2:
                    return { lower, upper };
                case 3:
                    return { lower, lower };
            }
        case EdgeType::TOP:
            switch (rotation)
            {
                default:
                case 0:
                    return { lower, lower };
                case 1:
                    return { upper, lower };
                case 2:
                    return { upper, upper };
                case 3:
                    return { lower, upper };
            }
        case EdgeType::RIGHT:
            switch (rotation)
            {
                default:
                case 0:
                    return { lower, upper };
                case 1:
                    return { lower, lower };
                case 2:
                    return { upper, lower };
                case 3:
                    return { upper, upper };
            }
        case EdgeType::BOTTOM:
            switch (rotation)
            {
                default:
                case 0:
                    return { upper, upper };
                case 1:
                    return { lower, upper };
                case 2:
                    return { lower, lower };
                case 3:
                    return { upper, lower };
            }
    }
}

static int32_t GetHighestBaseClearanceZ(const CoordsXY& location)
{
    int32_t z = 0;
    auto element = map_get_first_element_at(location);
    if (element != nullptr)
    {
        do
        {
            z = std::max<int32_t>(z, element->GetBaseZ());
            z = std::max<int32_t>(z, element->GetClearanceZ());
        } while (!(element++)->IsLastForTile());
    }
    return z;
}

static int32_t GetTallestVisibleTileTop(int32_t mapSize, int32_t rotation)
{
    int32_t minViewY = 0;
    for (int32_t y = 1; y < mapSize - 1; y++)
    {
        for (int32_t x = 1; x < mapSize - 1; x++)
        {
            auto location = TileCoordsXY(x, y).ToCoordsXY();
            int32_t z = GetHighestBaseClearanceZ(location);
            int32_t viewY = translate_3d_to_2d_with_z(rotation, CoordsXYZ(location, z)).y;
            minViewY = std::min(minViewY, viewY);
        }
    }
    return minViewY - 256;
}

static rct_drawpixelinfo CreateDPI(const rct_viewport& viewport)
{
    rct_drawpixelinfo dpi;
    dpi.width = viewport.width;
    dpi.height = viewport.height;
    dpi.bits = new (std::nothrow) uint8_t[dpi.width * dpi.height];
    if (dpi.bits == nullptr)
    {
        throw std::runtime_error("Giant screenshot failed, unable to allocate memory for image.");
    }

    if (viewport.flags & VIEWPORT_FLAG_TRANSPARENT_BACKGROUND)
    {
        std::memset(dpi.bits, PALETTE_INDEX_0, (size_t)dpi.width * dpi.height);
    }

    return dpi;
}

static void ReleaseDPI(rct_drawpixelinfo& dpi)
{
    if (dpi.bits != nullptr)
        delete[] dpi.bits;
    dpi.bits = nullptr;
    dpi.width = 0;
    dpi.height = 0;
}

static rct_viewport GetGiantViewport(int32_t mapSize, int32_t rotation, ZoomLevel zoom)
{
    // Get the tile coordinates of each corner
    auto leftTileCoords = GetEdgeTile(mapSize, rotation, EdgeType::LEFT, false);
    auto rightTileCoords = GetEdgeTile(mapSize, rotation, EdgeType::RIGHT, false);
    auto bottomTileCoords = GetEdgeTile(mapSize, rotation, EdgeType::BOTTOM, false);

    // Centre the coordinates so we don't have a hard crop at the edge of the visible tile
    leftTileCoords += CoordsXY(16, 16);
    rightTileCoords += CoordsXY(16, 16);
    bottomTileCoords += CoordsXY(16, 16);

    // Calculate the viewport bounds
    int32_t left = translate_3d_to_2d_with_z(rotation, CoordsXYZ(leftTileCoords, 0)).x;
    int32_t top = GetTallestVisibleTileTop(mapSize, rotation);
    int32_t right = translate_3d_to_2d_with_z(rotation, CoordsXYZ(rightTileCoords, 0)).x;
    int32_t bottom = translate_3d_to_2d_with_z(rotation, CoordsXYZ(bottomTileCoords, 0)).y;

    rct_viewport viewport{};
    viewport.viewPos = { left, top };
    viewport.view_width = right - left;
    viewport.view_height = bottom - top;
    viewport.width = viewport.view_width / zoom;
    viewport.height = viewport.view_height / zoom;
    viewport.zoom = zoom;
    return viewport;
}

static void RenderViewport(IDrawingEngine* drawingEngine, const rct_viewport& viewport, rct_drawpixelinfo& dpi)
{
    // Ensure sprites appear regardless of rotation
    reset_all_sprite_quadrant_placements();

    std::unique_ptr<X8DrawingEngine> tempDrawingEngine;
    if (drawingEngine == nullptr)
    {
        tempDrawingEngine = std::make_unique<X8DrawingEngine>(GetContext()->GetUiContext());
        drawingEngine = tempDrawingEngine.get();
    }
    dpi.DrawingEngine = drawingEngine;
    viewport_render(&dpi, &viewport, 0, 0, viewport.width, viewport.height);
}

void screenshot_giant()
{
    rct_drawpixelinfo dpi{};
    try
    {
        auto path = screenshot_get_next_path();
        if (path == std::nullopt)
        {
            throw std::runtime_error("Giant screenshot failed, unable to find a suitable destination path.");
        }

        int32_t rotation = get_current_rotation();
        ZoomLevel zoom = 0;

        auto mainWindow = window_get_main();
        auto vp = window_get_viewport(mainWindow);
        if (mainWindow != nullptr && vp != nullptr)
        {
            zoom = vp->zoom;
        }

        auto viewport = GetGiantViewport(gMapSize, rotation, zoom);
        if (vp != nullptr)
        {
            viewport.flags = vp->flags;
        }
        if (gConfigGeneral.transparent_screenshot)
        {
            viewport.flags |= VIEWPORT_FLAG_TRANSPARENT_BACKGROUND;
        }

        dpi = CreateDPI(viewport);

        RenderViewport(nullptr, viewport, dpi);
        auto renderedPalette = screenshot_get_rendered_palette();
        WriteDpiToFile(path->c_str(), &dpi, renderedPalette);

        // Show user that screenshot saved successfully
        set_format_arg(0, rct_string_id, STR_STRING);
        set_format_arg(2, char*, path_get_filename(path->c_str()));
        context_show_error(STR_SCREENSHOT_SAVED_AS, STR_NONE);
    }
    catch (const std::exception& e)
    {
        log_error("%s", e.what());
        context_show_error(STR_SCREENSHOT_FAILED, STR_NONE);
    }

    ReleaseDPI(dpi);
}

// TODO: Move this at some point into a more appropriate place.
template<typename FN> static inline double MeasureFunctionTime(const FN& fn)
{
    const auto startTime = std::chrono::high_resolution_clock::now();
    fn();
    const auto endTime = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double>(endTime - startTime).count();
}

static void benchgfx_render_screenshots(const char* inputPath, std::unique_ptr<IContext>& context, uint32_t iterationCount)
{
    if (!context->LoadParkFromFile(inputPath))
    {
        return;
    }

    gIntroState = INTRO_STATE_NONE;
    gScreenFlags = SCREEN_FLAGS_PLAYING;

    // Create Viewport and DPI for every rotation and zoom.
    constexpr int32_t MAX_ROTATIONS = 4;
    constexpr int32_t MAX_ZOOM_LEVEL = 3;
    std::array<rct_drawpixelinfo, MAX_ROTATIONS * MAX_ZOOM_LEVEL> dpis;
    std::array<rct_viewport, MAX_ROTATIONS * MAX_ZOOM_LEVEL> viewports;

    for (int32_t zoom = 0; zoom < MAX_ZOOM_LEVEL; zoom++)
    {
        for (int32_t rotation = 0; rotation < MAX_ROTATIONS; rotation++)
        {
            auto& viewport = viewports[zoom * MAX_ZOOM_LEVEL + rotation];
            auto& dpi = dpis[zoom * MAX_ZOOM_LEVEL + rotation];
            viewport = GetGiantViewport(gMapSize, rotation, zoom);
            dpi = CreateDPI(viewport);
        }
    }

    const uint32_t totalRenderCount = iterationCount * MAX_ROTATIONS * MAX_ZOOM_LEVEL;

    try
    {
        double totalTime = 0.0;

        std::array<double, MAX_ZOOM_LEVEL> zoomAverages;

        // Render at every zoom.
        for (int32_t zoom = 0; zoom < MAX_ZOOM_LEVEL; zoom++)
        {
            double zoomLevelTime = 0.0;

            // Render at every rotation.
            for (int32_t rotation = 0; rotation < MAX_ROTATIONS; rotation++)
            {
                // N iterations.
                for (uint32_t i = 0; i < iterationCount; i++)
                {
                    auto& dpi = dpis[zoom * MAX_ZOOM_LEVEL + rotation];
                    auto& viewport = viewports[zoom * MAX_ZOOM_LEVEL + rotation];
                    double elapsed = MeasureFunctionTime([&viewport, &dpi]() { RenderViewport(nullptr, viewport, dpi); });
                    totalTime += elapsed;
                    zoomLevelTime += elapsed;
                }
            }

            zoomAverages[zoom] = zoomLevelTime / static_cast<double>(MAX_ROTATIONS * iterationCount);
        }

        const double average = totalTime / static_cast<double>(totalRenderCount);
        const auto engineStringId = DrawingEngineStringIds[DRAWING_ENGINE_SOFTWARE];
        const auto engineName = format_string(engineStringId, nullptr);
        std::printf("Engine: %s\n", engineName.c_str());
        std::printf("Render Count: %u\n", totalRenderCount);
        for (int32_t zoom = 0; zoom < MAX_ZOOM_LEVEL; zoom++)
        {
            const auto zoomAverage = zoomAverages[zoom];
            std::printf("Zoom[%d] average: %.06fs, %.f FPS\n", zoom, zoomAverage, 1.0 / zoomAverage);
        }
        std::printf("Total average: %.06fs, %.f FPS\n", average, 1.0 / average);
        std::printf("Time: %.05fs\n", totalTime);
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "%s", e.what());
    }

    for (auto& dpi : dpis)
        ReleaseDPI(dpi);
}

int32_t cmdline_for_gfxbench(const char** argv, int32_t argc)
{
    if (argc != 1 && argc != 2)
    {
        printf("Usage: openrct2 benchgfx <file> [<iteration_count>]\n");
        return -1;
    }

    core_init();
    int32_t iterationCount = 5;
    if (argc == 2)
    {
        iterationCount = atoi(argv[1]);
    }

    const char* inputPath = argv[0];

    gOpenRCT2Headless = true;

    std::unique_ptr<IContext> context(CreateContext());
    if (context->Initialise())
    {
        drawing_engine_init();

        benchgfx_render_screenshots(inputPath, context, iterationCount);

        drawing_engine_dispose();
    }

    return 1;
}

static void ApplyOptions(const ScreenshotOptions* options, rct_viewport& viewport)
{
    if (options->weather != 0)
    {
        if (options->weather < 1 || options->weather > 6)
        {
            throw std::runtime_error("Weather can only be set to an integer value from 1 till 6.");
        }

        uint8_t customWeather = options->weather - 1;
        climate_force_weather(customWeather);
    }

    if (options->hide_guests)
    {
        viewport.flags |= VIEWPORT_FLAG_INVISIBLE_PEEPS;
    }

    if (options->hide_sprites)
    {
        viewport.flags |= VIEWPORT_FLAG_INVISIBLE_SPRITES;
    }

    if (options->mowed_grass)
    {
        CheatsSet(CheatType::SetGrassLength, GRASS_LENGTH_MOWED);
    }

    if (options->clear_grass || options->tidy_up_park)
    {
        CheatsSet(CheatType::SetGrassLength, GRASS_LENGTH_CLEAR_0);
    }

    if (options->water_plants || options->tidy_up_park)
    {
        CheatsSet(CheatType::WaterPlants);
    }

    if (options->fix_vandalism || options->tidy_up_park)
    {
        CheatsSet(CheatType::FixVandalism);
    }

    if (options->remove_litter || options->tidy_up_park)
    {
        CheatsSet(CheatType::RemoveLitter);
    }

    if (options->transparent || gConfigGeneral.transparent_screenshot)
    {
        viewport.flags |= VIEWPORT_FLAG_TRANSPARENT_BACKGROUND;
    }
}

int32_t cmdline_for_screenshot(const char** argv, int32_t argc, ScreenshotOptions* options)
{
    // Don't include options in the count (they have been handled by CommandLine::ParseOptions already)
    for (int32_t i = 0; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            // Setting argc to i works, because options can only be at the end of the command
            argc = i;
            break;
        }
    }

    bool giantScreenshot = (argc == 5) && _stricmp(argv[2], "giant") == 0;
    if (argc != 4 && argc != 8 && !giantScreenshot)
    {
        std::printf("Usage: openrct2 screenshot <file> <output_image> <width> <height> [<x> <y> <zoom> <rotation>]\n");
        std::printf("Usage: openrct2 screenshot <file> <output_image> giant <zoom> <rotation>\n");
        return -1;
    }

    int32_t exitCode = 1;
    rct_drawpixelinfo dpi;
    try
    {
        core_init();
        bool customLocation = false;
        bool centreMapX = false;
        bool centreMapY = false;

        const char* inputPath = argv[0];
        const char* outputPath = argv[1];

        gOpenRCT2Headless = true;
        auto context = CreateContext();
        if (!context->Initialise())
        {
            throw std::runtime_error("Failed to initialize context.");
        }

        drawing_engine_init();

        if (!context->LoadParkFromFile(inputPath))
        {
            throw std::runtime_error("Failed to load park.");
        }

        gIntroState = INTRO_STATE_NONE;
        gScreenFlags = SCREEN_FLAGS_PLAYING;

        rct_viewport viewport{};
        if (giantScreenshot)
        {
            auto zoom = std::atoi(argv[3]);
            auto rotation = std::atoi(argv[4]) & 3;
            viewport = GetGiantViewport(gMapSize, rotation, zoom);
            gCurrentRotation = rotation;
        }
        else
        {
            int32_t resolutionWidth = std::atoi(argv[2]);
            int32_t resolutionHeight = std::atoi(argv[3]);
            int32_t customX = 0;
            int32_t customY = 0;
            int32_t customZoom = 0;
            int32_t customRotation = 0;
            if (argc == 8)
            {
                customLocation = true;
                if (argv[4][0] == 'c')
                    centreMapX = true;
                else
                    customX = std::atoi(argv[4]);

                if (argv[5][0] == 'c')
                    centreMapY = true;
                else
                    customY = std::atoi(argv[5]);

                customZoom = std::atoi(argv[6]);
                customRotation = std::atoi(argv[7]) & 3;
            }

            int32_t mapSize = gMapSize;
            if (resolutionWidth == 0 || resolutionHeight == 0)
            {
                resolutionWidth = (mapSize * 32 * 2) >> customZoom;
                resolutionHeight = (mapSize * 32 * 1) >> customZoom;

                resolutionWidth += 8;
                resolutionHeight += 128;
            }

            viewport.width = resolutionWidth;
            viewport.height = resolutionHeight;
            viewport.view_width = viewport.width;
            viewport.view_height = viewport.height;
            if (customLocation)
            {
                if (centreMapX)
                    customX = (mapSize / 2) * 32 + 16;
                if (centreMapY)
                    customY = (mapSize / 2) * 32 + 16;

                int32_t z = tile_element_height({ customX, customY });
                CoordsXYZ coords3d = { customX, customY, z };

                auto coords2d = translate_3d_to_2d_with_z(customRotation, coords3d);

                viewport.viewPos = { coords2d.x - ((viewport.view_width << customZoom) / 2),
                                     coords2d.y - ((viewport.view_height << customZoom) / 2) };
                viewport.zoom = customZoom;
                gCurrentRotation = customRotation;
            }
            else
            {
                viewport.viewPos = { gSavedView - ScreenCoordsXY{ (viewport.view_width / 2), (viewport.view_height / 2) } };
                viewport.zoom = gSavedViewZoom;
                gCurrentRotation = gSavedViewRotation;
            }
        }

        ApplyOptions(options, viewport);

        dpi = CreateDPI(viewport);

        RenderViewport(nullptr, viewport, dpi);
        auto renderedPalette = screenshot_get_rendered_palette();
        WriteDpiToFile(outputPath, &dpi, renderedPalette);
    }
    catch (const std::exception& e)
    {
        std::printf("%s\n", e.what());
        exitCode = -1;
    }
    ReleaseDPI(dpi);

    drawing_engine_dispose();

    return exitCode;
}
