#include <iostream>
#include <cstdint>
#include <hammock/hammock.h>

#include "medium/ParticipatingMediumScene.h"
#include "renderer/Renderer.h"

using namespace hammock;


int main(int argc, char * argv[]) {
    ArgParser parser;
    parser.addArgument<int32_t>("width", "Window width in pixels", true);
    parser.addArgument<int32_t>("height", "Window height in pixels", true);
    parser.addArgument<std::string>("scene", "Scene option: [medium, renderer]", false);
    parser.addArgument<std::string>("weather", "Weather map option: [stratus, stratocumulus, cumulus, nubis]", false);
    parser.addArgument<std::string>("terrain", "Terrain type: [default, mountain]", false);

    try {
        parser.parse(argc, argv);
    } catch (const std::exception & e) {
        parser.printHelp();
        Logger::log(LOG_LEVEL_ERROR, e.what());
    }

    const auto width = parser.get<int32_t>("width");
    const auto height = parser.get<int32_t>("height");
    auto selectedScene = parser.get<std::string>("scene");
    auto weatherMap = parser.get<std::string>("weather");
    auto terrain = parser.get<std::string>("terrain");

    std::string sceneName = "renderer";
    if(!selectedScene.empty()){
        if(selectedScene == "medium")
            sceneName = "medium";
        else if (selectedScene == "renderer")
            sceneName = "renderer";
        else{
            Logger::log(LOG_LEVEL_ERROR, "Invalid scene!");
                exit(EXIT_FAILURE);
        }
    }

    if (sceneName == "medium") {
        ParticipatingMediumScene mediumScene{"Participating medium playground", static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        mediumScene.render();
    }
    else if (sceneName == "renderer") {
#ifdef CLOUD_RENDER_SUBSAMPLE
        std::cout << "Using subsampling when rendering clouds, note this is na experimental feature" << std::endl;
#endif
#ifdef HIGH_QUALITY_CLOUDS
        std::cout << "Using high fidelity clouds. This is recommended only for powerfull GPUs" << std::endl;
#endif

        WeatherMap weatherMapEnum = WeatherMap::Stratocumulus;
        TerrainType terrainEnum = TerrainType::Default;

        if(!weatherMap.empty()){
            if(weatherMap == "stratus")
                weatherMapEnum = WeatherMap::Stratus;
            else if(weatherMap == "stratocumulus")
                weatherMapEnum = WeatherMap::Stratocumulus;
            else if(weatherMap == "cumulus")
                weatherMapEnum = WeatherMap::Cumulus;
            else if(weatherMap == "nubis")
                weatherMapEnum = WeatherMap::Nubis;
            else{
                Logger::log(LOG_LEVEL_ERROR, "Invalid weather map!");
                exit(EXIT_FAILURE);
            }
        }

        if(!terrain.empty()){
            if(terrain == "default")
                terrainEnum = TerrainType::Default;
            else if (terrain == "mountain")
                terrainEnum = TerrainType::Mountain;
            else {
                Logger::log(LOG_LEVEL_ERROR, "Invalid terrain!");
                exit(EXIT_FAILURE);
            }
        }

        Renderer renderer{width, height, weatherMapEnum, terrainEnum};
        renderer.render();
        auto benchmarkResult = renderer.getBenchmarkResult();
        std::cout << "-- PROFILER RESULTS --" << std::endl;
        std::cout << "Depth pre-pass avg: " << BenchmarkResult::average(benchmarkResult.depthPrePass.getSnapshot()) << " ms" << std::endl;
        std::cout << "Terrain draw avg: " << BenchmarkResult::average(benchmarkResult.terrainDraw.getSnapshot()) << " ms" << std::endl;
        std::cout << "Cloud compute avg: " << BenchmarkResult::average(benchmarkResult.cloudComputePass.getSnapshot()) << " ms"  << std::endl;
        std::cout << "Transmittance LUT compute avg: " << BenchmarkResult::average(benchmarkResult.transmittanceLUT.getSnapshot()) << " ms"  << std::endl;
        std::cout << "Multiple scattering LUT compute avg: " << BenchmarkResult::average(benchmarkResult.multipleScatteringLUT.getSnapshot()) << " ms"  << std::endl;
        std::cout << "Sky view LUT compute avg: " << BenchmarkResult::average(benchmarkResult.skyViewLUT.getSnapshot()) << " ms"  << std::endl;
        std::cout << "Aerial perspective LUT compute avg: " << BenchmarkResult::average(benchmarkResult.aerialPerspectiveLUT.getSnapshot()) << " ms"  << std::endl;
        std::cout << "God rays mask gen avg: " << BenchmarkResult::average(benchmarkResult.godRaysMask.getSnapshot()) << " ms"  << std::endl;
        std::cout << "God rays blur gen avg: " << BenchmarkResult::average(benchmarkResult.godRaysBlur.getSnapshot()) << " ms"  << std::endl;
        std::cout << "Sky view upsample avg: " << BenchmarkResult::average(benchmarkResult.skyUpsample.getSnapshot()) << " ms"  << std::endl;
        std::cout << "Composition avg: " << BenchmarkResult::average(benchmarkResult.composition.getSnapshot()) << " ms"  << std::endl;
        std::cout << "Post processing avg: " << BenchmarkResult::average(benchmarkResult.post.getSnapshot()) << " ms"  << std::endl;
        std::cout << "TOTAL FRAME avg: " << BenchmarkResult::average(benchmarkResult.total.getSnapshot()) << " ms"  << std::endl;
        std::cout << "-- ---------------- --" << std::endl;
    }
    else {
        Logger::log(LOG_LEVEL_ERROR, "Invalid scene option");
    }
}