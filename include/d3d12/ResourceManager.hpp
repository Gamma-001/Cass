//
// Author: github.com/Gamma-001
// Date created: 08-july-2025
//

#pragma once

#include <windows.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_4.h>

namespace Cass {
    using Microsoft::WRL::ComPtr;

    class D3d12ResourceManager {
    public:
        D3d12ResourceManager(UINT width, UINT height, HWND hWnd);
    
        void OnInit();
        void OnUpdate();
        void OnSize(UINT width, UINT height);
        void OnRender();
        void OnDestroy();

    private:
        static const UINT m_frameCount = 2;
        struct Vertex {
            DirectX::XMFLOAT3 position;
            DirectX::XMFLOAT4 color;
        };
        HWND m_hWnd;
        UINT m_width;
        UINT m_height;

        // Pipeline objects
        D3D12_VIEWPORT                      m_viewport;
        D3D12_RECT                          m_scissorRect;
        ComPtr<IDXGIFactory4>               m_factory;
        ComPtr<IDXGISwapChain3>             m_swapChain;
        ComPtr<ID3D12Device>                m_device;
        ComPtr<ID3D12Resource>              m_renderTargets[m_frameCount];
        ComPtr<ID3D12CommandQueue>          m_commandQueue;
        ComPtr<ID3D12CommandAllocator>      m_sceneCommandAllocator;
        ComPtr<ID3D12GraphicsCommandList>   m_sceneCommandList;
        ComPtr<ID3D12RootSignature>         m_rootSignature;
        ComPtr<ID3D12DescriptorHeap>        m_rtvHeap;
        ComPtr<ID3D12PipelineState>         m_pipelineState;
        UINT                                m_rtvDescSize;

        // App resources
        ComPtr<ID3D12Resource>      m_vertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW    m_vertexBufferView;
        ComPtr<ID3D12Resource>      m_indexBuffer;
        D3D12_INDEX_BUFFER_VIEW     m_indexBufferView;
    
        // Synchronization objects
        UINT m_frameIndex;
        HANDLE m_fenceEvent;
        ComPtr<ID3D12Fence> m_fence;
        UINT64 m_fenceValue;

        void LoadSizeIndependentResources();
        void LoadSizeDependentResources();
        void LoadAssets();
        void WaitForPreviousFrame();
        void PopulateCommandList();
        void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter, bool reqHighPerfAdapter = false);
    };
}