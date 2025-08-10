//
// Author: github.com/Gamma-001
// Date created: 08-july-2025
//

#include <d3d12/ResourceManager.hpp>
#include <utils.hpp>

#include <synchapi.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <directx/d3dx12.h>

using namespace Cass;

// ---------- Public Methods

D3d12ResourceManager::D3d12ResourceManager(UINT width, UINT height, HWND hWnd) {
    m_width = width;
    m_height = height;
    m_hWnd = hWnd;
    m_viewport = D3D12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    m_scissorRect = D3D12_RECT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    m_rtvDescSize = 0;
}

void D3d12ResourceManager::OnInit() {
    LoadSizeIndependentResources();
    LoadSizeDependentResources();
    LoadAssets();

    // Disable Alt + Enter to switch full screen
    ThrowIfFailed(m_factory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER));
}

void D3d12ResourceManager::OnUpdate() {

}

void D3d12ResourceManager::OnSize(UINT width, UINT height) {
    // TOOD
}

void D3d12ResourceManager::OnRender() {
    PopulateCommandList();

    // Execute the command list
    ID3D12CommandList* ppCommandList[] = { m_sceneCommandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);

    // Present the frame
    ThrowIfFailed(m_swapChain->Present(1, 0));

    WaitForPreviousFrame();
}

void D3d12ResourceManager::OnDestroy() {
    WaitForPreviousFrame();

    CloseHandle(m_fenceEvent);
}

// ---------- Private Methods

void D3d12ResourceManager::LoadSizeIndependentResources() {
#ifdef _DEBUG
    // Enable D3D12 Debug Layer
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();
        }
    }
#endif

    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_factory)));

    ComPtr<IDXGIAdapter1> hardwareAdapter;
    GetHardwareAdapter(m_factory.Get(), &hardwareAdapter);

    ThrowIfFailed(D3D12CreateDevice(
        hardwareAdapter.Get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&m_device)
    ));

    // Describe and create the command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
}

void D3d12ResourceManager::LoadSizeDependentResources() {
    // Describe and create the swap chain
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = m_frameCount;
    swapChainDesc.BufferDesc.Width = m_width;
    swapChainDesc.BufferDesc.Height = m_height;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.OutputWindow = m_hWnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;

    ComPtr<IDXGISwapChain> swapChain;
    ThrowIfFailed(m_factory->CreateSwapChain(
        m_commandQueue.Get(),
        &swapChainDesc,
        &swapChain
    ));
    ThrowIfFailed(swapChain.As(&m_swapChain));

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Create descriptor heaps 
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = m_frameCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

        m_rtvDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // Create frame resources
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create an RTV for each frame
        for (UINT i = 0; i < m_frameCount; i++) {
            ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
            m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescSize);
        }
    }

    ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_sceneCommandAllocator)));
}

void D3d12ResourceManager::LoadAssets() {
    // Create an empty root signature
    {
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
        ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
    }

    // Create a the pipeline state which includes compiling and loading the shaders
    {
        ComPtr<ID3DBlob> vertShader;
        ComPtr<ID3DBlob> pixelShader;
        UINT compileFlags = NULL;

#ifdef _DEBUG
        compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
       
        ThrowIfFailed(D3DCompileFromFile(L"./src/shader/default_vert.hlsl", nullptr, nullptr, "main", "vs_5_0", compileFlags, NULL, &vertShader, nullptr));
        ThrowIfFailed(D3DCompileFromFile(L"./src/shader/default_pixel.hlsl", nullptr, nullptr, "main", "ps_5_0", compileFlags, NULL, &pixelShader, nullptr));
    
        D3D12_INPUT_ELEMENT_DESC ieDesc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { ieDesc, _countof(ieDesc) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertShader.Get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;

        ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
    }

    // Create the command list
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_sceneCommandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_sceneCommandList)));

    // Command Lists are created in recording state, but there is nothing
    // to record sate. Yet the main loop expects it to be closed, so close it now
    ThrowIfFailed(m_sceneCommandList->Close());

    // Single use command allocator and command list for loading resources
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
    ThrowIfFailed(m_device->CreateCommandList(NULL, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

    // Create the vertex buffer
    ComPtr<ID3D12Resource> vertexBufferUpload;
    {
        Vertex vertices[] = {
            { { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { -0.5f, 0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { 0.5f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        };
        const UINT vertexBufferSize = sizeof(vertices);

        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        auto desc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
        ThrowIfFailed(m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_vertexBuffer)
        ));

        heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        ThrowIfFailed(m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&vertexBufferUpload)
        ));
        
        // Copy data to the intermediate upload heap
        UINT8* pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0); // Resourced is not intended to be read from the CPU
        ThrowIfFailed(vertexBufferUpload->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, vertices, vertexBufferSize);
        vertexBufferUpload->Unmap(0, nullptr);

        // Schedule a copy from the upload heap to the vertex buffer 
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        commandList->CopyBufferRegion(m_vertexBuffer.Get(), 0, vertexBufferUpload.Get(), 0, vertexBufferSize);
        commandList->ResourceBarrier(1, &barrier);

        // Initialize the vertex buffer view
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        m_vertexBufferView.SizeInBytes = vertexBufferSize;
    }

    // Execute the command for copying the buffers
    ThrowIfFailed(commandList->Close());
    ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Create a synchronization object and wait until the data has been uploaded to the GPU
    {
        ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceValue = 1;

        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr) {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        WaitForPreviousFrame();
    }
}

void D3d12ResourceManager::WaitForPreviousFrame() {
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.

    // Signal and increment the fence value.
    const UINT64 fence = m_fenceValue;
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
    m_fenceValue += 1;

    // Wait until the previous frame is finished
    if (m_fence->GetCompletedValue() < fence) {
        ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void D3d12ResourceManager::PopulateCommandList() {
    // Reclaim the memory back that was associated with the command allocator
    // Command list allocators can only be reset when the associated command lists have finished execution on the GPU, otherwise the call will fail
    // apps should use fences to determine GPU execution progress.
    ThrowIfFailed(m_sceneCommandAllocator->Reset());

    // Command list can be reset at any time, after which it goes into recording state
    // A command list can also be reset immediately after it is submitted without waiting for its completion
    ThrowIfFailed(m_sceneCommandList->Reset(m_sceneCommandAllocator.Get(), m_pipelineState.Get()));

    // Set neccessary states
    m_sceneCommandList->SetGraphicsRootSignature(m_rootSignature.Get());
    m_sceneCommandList->RSSetViewports(1, &m_viewport);
    m_sceneCommandList->RSSetScissorRects(1, &m_scissorRect);

    // Indicate that the back buffer will be used as the render target
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_sceneCommandList->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescSize);
    m_sceneCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Record commands
    const float clearColor[] = { 0.05f, 0.05f, 0.05f, 1.0f };
    m_sceneCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_sceneCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_sceneCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    m_sceneCommandList->DrawInstanced(3, 1, 0, 0);

    // Indicate that the back buffer will now be used to present
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_sceneCommandList->ResourceBarrier(1, &barrier);

    // Transition out of the recording state
    ThrowIfFailed(m_sceneCommandList->Close());
}

void D3d12ResourceManager::GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter, bool reqHighPerfAdapter) {
    *ppAdapter = nullptr;

    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<IDXGIFactory6> factory6;

    // Supporting option for selecting high performance adapters
    if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6)))) {
        for (
            UINT adapterIndex = 0;
            SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                adapterIndex,
                reqHighPerfAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
                IID_PPV_ARGS(&adapter)
            ));
            adapterIndex++
            ) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) {
                // Don't select the basic render driver adapter
                // warp should be used for software adapter
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, don't create the actual device
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr))) {
                break;
            }
        }
    }

    // Without any support for selecting high performance adapters
    if (adapter.Get() == nullptr) {
        for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); adapterIndex++) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }

            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr))) {
                break;
            }
        }
    }

    *ppAdapter = adapter.Detach();
}