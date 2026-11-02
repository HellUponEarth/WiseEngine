// Code Modification - SDK Integration: Verify the selected libraries' headers, linkage, and runtime ABIs together.
// Scope: standalone test executable. Performance: bounded CPU/memory work without device startup. Toggle: WISEENGINE_BUILD_TESTS.
#include <WiseEngine/LuauBindings.h>
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <SDL3/SDL.h>
#include <TaskScheduler.h>
#include <asio.hpp>
#include <boost/predef.h>
#include <boost/container/small_vector.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <physfs.h>
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>
#include <zstd.h>
#include <xxhash.h>
#include <meshoptimizer.h>
#include <DirectXTex.h>
#include <miniaudio.h>
#include <stb_image.h>
#include <tracy/Tracy.hpp>
#include <DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h>
#include <DiligentCore/Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h>
#if VULKAN_SUPPORTED
#include <DiligentCore/Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h>
#endif
#include <DiligentTools/ThirdParty/imgui/imgui.h>
#include <atomic>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

static void require(bool condition, const char* message)
{
    if (!condition) { throw std::runtime_error(message); }
}

static int add(int a, int b) { return a + b; }

int main(int argc, char* argv[])
{
    (void)argc;
    try {
        require(BOOST_OS_WINDOWS != 0, "Boost platform detection");
        boost::container::small_vector<int, 4> smallValues{1, 2, 3};
        boost::counting_iterator<int> counter(4);
        require(smallValues.size() == 3 && *counter == 4, "Boost collection/iterator");
        const glm::vec3 vector(1, 2, 3);
        require(glm::dot(vector, vector) == 14, "GLM");
        entt::registry registry;
        const auto entity = registry.create();
        registry.emplace<int>(entity, 42);
        require(registry.get<int>(entity) == 42, "EnTT");
        require(SDL_Init(0), "SDL init");
        require(SDL_GetPerformanceFrequency() != 0, "SDL clock");
        SDL_Quit();

        enki::TaskScheduler scheduler;
        scheduler.Initialize(2);
        std::atomic<unsigned int> total{0};
        enki::TaskSet work(100, [&total](enki::TaskSetPartition range, uint32_t) {
            for (auto i = range.start; i < range.end; ++i) { total.fetch_add(i, std::memory_order_relaxed); }
        });
        scheduler.AddTaskSetToPipe(&work);
        scheduler.WaitforTask(&work);
        require(total.load() == 4950, "enkiTS tasks");
        scheduler.WaitforAllAndShutdown();

        require(PHYSFS_init(argv[0]) != 0, "PhysicsFS init");
        require(PHYSFS_mount(PHYSFS_getBaseDir(), "/", 1) != 0, "PhysicsFS mount");
        require(PHYSFS_deinit() != 0, "PhysicsFS shutdown");
        asio::io_context io;
        asio::ip::udp::socket socket(io, asio::ip::udp::v4());
        require(socket.is_open(), "Asio socket linkage");
        socket.close();

        lua_State* state = luaL_newstate();
        require(state != nullptr, "Luau state");
        luaL_openlibs(state);
        luabridge::getGlobalNamespace(state).addFunction("add", &add);
        luaL_sandbox(state);
        const auto bytecode = Luau::compile("return add(20, 22)");
        require(luau_load(state, "sdk-smoke", bytecode.data(), bytecode.size(), 0) == 0, "Luau compiler/VM ABI");
        require(lua_pcall(state, 0, 1, 0) == 0, "LuaBridge native callback");
        require(lua_tonumber(state, -1) == 42, "Luau result");
        lua_close(state);

        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();
        {
            JPH::SphereShapeSettings settings(1.0f);
            const auto shape = settings.Create();
            require(!shape.HasError(), "Jolt shape / allocator ABI");
        }
        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;

        const std::string original = "WiseEngine SDK compression roundtrip";
        std::vector<char> compressed(ZSTD_compressBound(original.size()));
        const auto size = ZSTD_compress(compressed.data(), compressed.size(), original.data(), original.size(), 1);
        require(!ZSTD_isError(size), "Zstd compress");
        std::string restored(original.size(), '\0');
        require(ZSTD_decompress(restored.data(), restored.size(), compressed.data(), size) == original.size(), "Zstd decompress");
        require(restored == original && XXH3_64bits(restored.data(), restored.size()) == XXH3_64bits(original.data(), original.size()), "Zstd/xxHash roundtrip");
        const nlohmann::json json{{"value", 42}};
        require(json.at("value").get<int>() == 42 && YAML::Load("value: 42")["value"].as<int>() == 42, "JSON/YAML");
        const unsigned int indices[]{0, 1, 2};
        unsigned int optimized[3]{};
        meshopt_optimizeVertexCache(optimized, indices, 3, 3);
        require(optimized[0] < 3, "meshoptimizer");
        DirectX::ScratchImage image;
        require(SUCCEEDED(image.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4, 1, 1)), "DirectXTex");
        const auto audio = ma_engine_config_init();
        (void)audio;
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::DestroyContext();
        require(Diligent::GetEngineFactoryD3D12() != nullptr, "Diligent DX12 factory linkage");
#if VULKAN_SUPPORTED
        require(Diligent::GetEngineFactoryVk() != nullptr, "Diligent Vulkan factory linkage");
#endif
        spdlog::info("{}", fmt::format("SDK integration checks passed: {}", 42));
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "SDK integration failed: " << error.what() << '\n';
        return 1;
    }
}
// End of Code Modification - SDK Integration
