#include "Dx12Scene.h"

#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <wrl/client.h>

#include "DiligentCore/Common/interface/RefCntAutoPtr.hpp"
#include "DiligentCore/Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h"
#include "DiligentCore/Graphics/GraphicsEngineD3D12/interface/RenderDeviceD3D12.h"
#include "BringUpPS.h"
#include "BringUpVS.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <array>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <utility>
#include <vector>

namespace CE::Renderer {
namespace {
// Code Modification - Rendering: Use deterministic geometry and colors to check the real shader and texture path.
// Scope: generated cube and checkerboard. Performance: immutable geometry and texture uploaded once. Toggle: --no-render.
using namespace Diligent;
constexpr auto Transition = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
constexpr auto ColorFormat = TEX_FORMAT_RGBA8_UNORM;
constexpr auto DepthFormat = TEX_FORMAT_D32_FLOAT;
constexpr std::array<float, 4> ClearColor{0.035f, 0.055f, 0.095f, 1.0f};

struct Vertex {
    float position[3];
    float uv[2];
};

constexpr std::array<Vertex, 24> Vertices{{
    {{-1,-1,-1},{0,1}}, {{-1,1,-1},{0,0}}, {{1,1,-1},{1,0}}, {{1,-1,-1},{1,1}},
    {{1,-1,1},{0,1}}, {{1,1,1},{0,0}}, {{-1,1,1},{1,0}}, {{-1,-1,1},{1,1}},
    {{-1,-1,1},{0,1}}, {{-1,1,1},{0,0}}, {{-1,1,-1},{1,0}}, {{-1,-1,-1},{1,1}},
    {{1,-1,-1},{0,1}}, {{1,1,-1},{0,0}}, {{1,1,1},{1,0}}, {{1,-1,1},{1,1}},
    {{-1,1,-1},{0,1}}, {{-1,1,1},{0,0}}, {{1,1,1},{1,0}}, {{1,1,-1},{1,1}},
    {{-1,-1,1},{0,1}}, {{-1,-1,-1},{0,0}}, {{1,-1,-1},{1,0}}, {{1,-1,1},{1,1}},
}};

constexpr auto makeIndices()
{
    std::array<Uint16, 36> indices{};
    for (Uint16 face = 0; face < 6; ++face) {
        const auto base = static_cast<Uint16>(face * 4);
        const auto offset = static_cast<std::size_t>(face) * 6;
        indices[offset] = base;
        indices[offset + 1] = static_cast<Uint16>(base + 1);
        indices[offset + 2] = static_cast<Uint16>(base + 2);
        indices[offset + 3] = base;
        indices[offset + 4] = static_cast<Uint16>(base + 2);
        indices[offset + 5] = static_cast<Uint16>(base + 3);
    }
    return indices;
}
constexpr auto Indices = makeIndices();

bool fail(const char* message)
{
    std::cerr << "DX12: " << message << '\n';
    return false;
}
// End of Code Modification - Rendering
}

// Code Modification - Rendering: Isolate Diligent and native validation behind the renderer lifetime.
// Scope: device, swap chain, depth, resources, and single graphics pipeline.
// Performance: swap-chain resize can synchronize. Shutdown and smoke readback wait for GPU idle. Toggle: DX12 validation and smoke pixel check.
struct Dx12Scene::State {
    bool initialize(void* window, Uint32 width, Uint32 height, bool validation)
    {
        auto* factory = GetEngineFactoryD3D12();
        if (!factory || !window || !width || !height) {
            return fail("Missing DX12 factory or valid Win32 drawable");
        }
        EngineD3D12CreateInfo createInfo;
        createInfo.SetValidationLevel(validation ? VALIDATION_LEVEL_1 : VALIDATION_LEVEL_DISABLED);
        factory->CreateDeviceAndContextsD3D12(createInfo, &device, &context);
        if (!device || !context) {
            return fail("Could not create a device and immediate context");
        }
        const RefCntAutoPtr<IRenderDeviceD3D12> nativeDevice(device, IID_RenderDeviceD3D12);
        if (!nativeDevice) {
            return fail("Diligent did not expose its DX12 device");
        }
        native = nativeDevice->GetD3D12Device();
        if (validation && FAILED(native->QueryInterface(IID_PPV_ARGS(&messages)))) {
            return fail("The D3D12 debug layer is unavailable. Install Windows Graphics Tools or use --no-gpu-validation.");
        }

        SwapChainDesc swapDesc;
        swapDesc.Width = width;
        swapDesc.Height = height;
        swapDesc.ColorBufferFormat = ColorFormat;
        swapDesc.DepthBufferFormat = DepthFormat;
        factory->CreateSwapChainD3D12(device, context, swapDesc, {}, Win32NativeWindow(window), &swapChain);
        if (!swapChain || !targetsMatch(width, height)) {
            return fail("Could not create matching color and depth swap-chain views");
        }
        if (!createResources() || !createPipeline()) {
            return false;
        }
        std::cout << "DX12 adapter: " << device->GetAdapterInfo().Description
                  << " | validation=" << (validation ? "on" : "off") << " | " << width << 'x' << height << std::endl;
        return true;
    }

    bool targetsMatch(Uint32 width, Uint32 height) const
    {
        auto* color = swapChain->GetCurrentBackBufferRTV();
        auto* depth = swapChain->GetDepthBufferDSV();
        return color && depth && color->GetTexture()->GetDesc().Width == width &&
            color->GetTexture()->GetDesc().Height == height && depth->GetTexture()->GetDesc().Width == width &&
            depth->GetTexture()->GetDesc().Height == height;
    }

    bool createResources()
    {
        BufferDesc buffer;
        buffer.Name = "Bring-up cube vertices";
        buffer.Size = sizeof(Vertices);
        buffer.Usage = USAGE_IMMUTABLE;
        buffer.BindFlags = BIND_VERTEX_BUFFER;
        BufferData data(Vertices.data(), sizeof(Vertices));
        device->CreateBuffer(buffer, &data, &vertices);
        buffer.Name = "Bring-up cube indices";
        buffer.Size = sizeof(Indices);
        buffer.BindFlags = BIND_INDEX_BUFFER;
        data = BufferData(Indices.data(), sizeof(Indices));
        device->CreateBuffer(buffer, &data, &indices);

        static_assert(sizeof(glm::mat4) == 64);
        buffer.Name = "Bring-up camera matrix";
        buffer.Size = sizeof(glm::mat4);
        buffer.Usage = USAGE_DYNAMIC;
        buffer.BindFlags = BIND_UNIFORM_BUFFER;
        buffer.CPUAccessFlags = CPU_ACCESS_WRITE;
        device->CreateBuffer(buffer, nullptr, &camera);

        constexpr Uint32 side = 8;
        using Pixel = std::array<Uint8, 4>;
        std::array<Pixel, side * side> pixels{};
        for (Uint32 y = 0; y < side; ++y) {
            for (Uint32 x = 0; x < side; ++x) {
                pixels[y * side + x] = (x + y) % 2 == 0 ? Pixel{242, 143, 35, 255} : Pixel{26, 195, 205, 255};
            }
        }
        TextureDesc desc;
        desc.Name = "Bring-up checkerboard";
        desc.Type = RESOURCE_DIM_TEX_2D;
        desc.Width = side;
        desc.Height = side;
        desc.Format = ColorFormat;
        desc.Usage = USAGE_IMMUTABLE;
        desc.BindFlags = BIND_SHADER_RESOURCE;
        TextureSubResData subresource(pixels.data(), side * sizeof(Pixel));
        TextureData textureData(&subresource, 1);
        device->CreateTexture(desc, &textureData, &texture);
        return (vertices && indices && camera && texture) || fail("Could not create the cube resources");
    }

    bool createPipeline()
    {
        RefCntAutoPtr<IShader> vs;
        RefCntAutoPtr<IShader> ps;
        ShaderCreateInfo shader;
        shader.Desc = ShaderDesc{"Bring-up vertex shader", SHADER_TYPE_VERTEX};
        shader.ByteCode = BringUpVS;
        shader.ByteCodeSize = sizeof(BringUpVS);
        device->CreateShader(shader, &vs);
        shader.Desc = ShaderDesc{"Bring-up pixel shader", SHADER_TYPE_PIXEL};
        shader.ByteCode = BringUpPS;
        shader.ByteCodeSize = sizeof(BringUpPS);
        device->CreateShader(shader, &ps);
        if (!vs || !ps) {
            return fail("Could not load the embedded DXIL shaders");
        }

        GraphicsPipelineStateCreateInfo info;
        info.PSODesc.Name = "Bring-up textured cube";
        info.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;
        info.pVS = vs;
        info.pPS = ps;
        auto& graphics = info.GraphicsPipeline;
        graphics.NumRenderTargets = 1;
        graphics.RTVFormats[0] = ColorFormat;
        graphics.DSVFormat = DepthFormat;
        graphics.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        graphics.RasterizerDesc.CullMode = CULL_MODE_NONE;
        graphics.DepthStencilDesc.DepthEnable = True;
        graphics.DepthStencilDesc.DepthWriteEnable = True;
        const LayoutElement layout[]{{0, 0, 3, VT_FLOAT32, False}, {1, 0, 2, VT_FLOAT32, False}};
        graphics.InputLayout.LayoutElements = layout;
        graphics.InputLayout.NumElements = 2;

        SamplerDesc sampler;
        sampler.MinFilter = sampler.MagFilter = sampler.MipFilter = FILTER_TYPE_POINT;
        const ImmutableSamplerDesc immutable{SHADER_TYPE_PIXEL, "PointSampler", sampler};
        info.PSODesc.ResourceLayout.ImmutableSamplers = &immutable;
        info.PSODesc.ResourceLayout.NumImmutableSamplers = 1;
        device->CreateGraphicsPipelineState(info, &pipeline);
        if (!pipeline) {
            return fail("Could not create the graphics pipeline");
        }
        auto* matrixBinding = pipeline->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Camera");
        auto* textureBinding = pipeline->GetStaticVariableByName(SHADER_TYPE_PIXEL, "Checker");
        auto* view = texture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        if (!matrixBinding || !textureBinding || !view) {
            return fail("DXIL bindings do not match the scene resources");
        }
        matrixBinding->Set(camera);
        textureBinding->Set(view);
        pipeline->CreateShaderResourceBinding(&resources, true);
        return resources || fail("Could not initialize shader bindings");
    }

    bool render(Uint32 width, Uint32 height, float angle, bool verifyPixels)
    {
        const auto& desc = swapChain->GetDesc();
        if (width != desc.Width || height != desc.Height) {
            context->SetRenderTargets(0, nullptr, nullptr, Transition);
            swapChain->Resize(width, height);
            if (!targetsMatch(width, height)) {
                return fail("Swap-chain resize did not recreate the color and depth views");
            }
            std::cout << "DX12 resize: " << width << 'x' << height << std::endl;
        }

        const auto aspect = static_cast<float>(width) / static_cast<float>(height);
        const auto view = glm::lookAtLH(glm::vec3(0.0f, 1.35f, -5.2f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        const auto projection = glm::perspectiveLH_ZO(glm::radians(50.0f), aspect, 0.1f, 100.0f);
        const auto model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.35f, 1.0f, 0.0f));
        const auto matrix = projection * view * model;
        void* mapped = nullptr;
        context->MapBuffer(camera, MAP_WRITE, MAP_FLAG_DISCARD, mapped);
        if (!mapped) {
            return fail("Could not map the camera buffer");
        }
        std::memcpy(mapped, &matrix, sizeof(matrix));
        context->UnmapBuffer(camera, MAP_WRITE);

        auto* color = swapChain->GetCurrentBackBufferRTV();
        auto* depth = swapChain->GetDepthBufferDSV();
        context->SetRenderTargets(1, &color, depth, Transition);
        const Viewport viewport{static_cast<float>(width), static_cast<float>(height)};
        context->SetViewports(1, &viewport, width, height);
        context->ClearRenderTarget(color, ClearColor.data(), Transition);
        context->ClearDepthStencil(depth, CLEAR_DEPTH_FLAG, 1.0f, 0, Transition);
        IBuffer* vertexBuffer = vertices;
        constexpr Uint64 offset = 0;
        context->SetVertexBuffers(0, 1, &vertexBuffer, &offset, Transition, SET_VERTEX_BUFFERS_FLAG_RESET);
        context->SetIndexBuffer(indices, 0, Transition);
        context->SetPipelineState(pipeline);
        context->CommitShaderResources(resources, Transition);
        context->DrawIndexed({static_cast<Uint32>(Indices.size()), VT_UINT16, messages ? DRAW_FLAG_VERIFY_ALL : DRAW_FLAG_NONE});
        if (verifyPixels && !verify(color->GetTexture())) {
            return false;
        }
        swapChain->Present(1);
        return deviceHealthy();
    }

    bool deviceHealthy() const
    {
        return SUCCEEDED(native->GetDeviceRemovedReason()) || fail("Graphics device was removed");
    }

    bool verify(ITexture* color)
    {
        auto desc = color->GetDesc();
        desc.Name = "Bring-up smoke readback";
        desc.Usage = USAGE_STAGING;
        desc.BindFlags = BIND_NONE;
        desc.CPUAccessFlags = CPU_ACCESS_READ;
        RefCntAutoPtr<ITexture> staging;
        device->CreateTexture(desc, nullptr, &staging);
        if (!staging) {
            return fail("Could not create smoke readback texture");
        }
        context->SetRenderTargets(0, nullptr, nullptr, Transition);
        context->CopyTexture({color, Transition, staging, Transition});
        context->WaitForIdle();
        MappedTextureSubresource mapped;
        context->MapTextureSubresource(staging, 0, 0, MAP_READ, MAP_FLAG_DO_NOT_WAIT, nullptr, mapped);
        if (!mapped.pData) {
            return fail("Could not map smoke readback");
        }
        std::size_t orange = 0;
        std::size_t teal = 0;
        std::size_t background = 0;
        const auto* data = static_cast<const Uint8*>(mapped.pData);
        for (Uint32 y = 0; y < desc.Height; ++y) {
            const auto* row = data + y * mapped.Stride;
            for (Uint32 x = 0; x < desc.Width; ++x) {
                const auto* pixel = row + 4 * x;
                orange += pixel[0] > 200 && pixel[1] > 110 && pixel[2] < 70;
                teal += pixel[0] < 65 && pixel[1] > 155 && pixel[2] > 155;
                background += pixel[0] < 20 && pixel[1] < 30 && pixel[2] < 40;
            }
        }
        context->UnmapTextureSubresource(staging, 0, 0);
        std::cout << "DX12 readback: orange=" << orange << " teal=" << teal << " background=" << background << std::endl;
        return (orange > 100 && teal > 100 && background > 100) || fail("Readback did not contain both checker colors and the clear color");
    }

    bool validate() const
    {
        if (!messages) {
            return true;
        }
        bool clean = true;
        for (UINT64 index = 0; index < messages->GetNumStoredMessages(); ++index) {
            SIZE_T bytes = 0;
            if (FAILED(messages->GetMessage(index, nullptr, &bytes))) {
                return fail("Could not inspect the DX12 debug message queue");
            }
            std::vector<std::byte> storage(bytes);
            auto* message = reinterpret_cast<D3D12_MESSAGE*>(storage.data());
            if (FAILED(messages->GetMessage(index, message, &bytes))) {
                return fail("Could not read a DX12 debug message");
            }
            if (message->Severity <= D3D12_MESSAGE_SEVERITY_ERROR) {
                std::cerr << "DX12 validation error: " << message->pDescription << '\n';
                clean = false;
            }
        }
        return clean;
    }

    RefCntAutoPtr<IRenderDevice> device;
    ID3D12Device* native{};
    RefCntAutoPtr<IDeviceContext> context;
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> messages;
    RefCntAutoPtr<ISwapChain> swapChain;
    RefCntAutoPtr<IBuffer> vertices;
    RefCntAutoPtr<IBuffer> indices;
    RefCntAutoPtr<IBuffer> camera;
    RefCntAutoPtr<ITexture> texture;
    RefCntAutoPtr<IPipelineState> pipeline;
    RefCntAutoPtr<IShaderResourceBinding> resources;
};

Dx12Scene::Dx12Scene() = default;
Dx12Scene::~Dx12Scene()
{
    if (state_ && state_->context) {
        state_->context->WaitForIdle();
    }
}

bool Dx12Scene::open(void* window, std::uint32_t width, std::uint32_t height, bool validation)
{
    if (state_) {
        return fail("Scene is already open");
    }
    state_ = std::make_unique<State>();
    return state_->initialize(window, width, height, validation);
}

bool Dx12Scene::draw(std::uint32_t width, std::uint32_t height, float angle, bool verifyPixels)
{
    return state_ && state_->swapChain && width && height && state_->render(width, height, angle, verifyPixels);
}

bool Dx12Scene::close()
{
    if (!state_) {
        return true;
    }
    if (state_->context) {
        state_->context->SetRenderTargets(0, nullptr, nullptr, Transition);
        state_->context->WaitForIdle();
    }
    state_->swapChain.Release();
    const auto clean = state_->validate();
    state_.reset();
    return clean;
}
// End of Code Modification - Rendering
}
