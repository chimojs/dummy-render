#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#define MYD3DWINDOWCLASS L"MyD3DWindowClass"
#define MYD3DWINDOWNAME L"MyD3DWindowName"

using namespace DirectX;

LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, Msg, wParam, lParam);
    }

    return 0;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    WNDCLASSEX wndClass = { 0 };
    wndClass.cbSize = sizeof(wndClass);
    wndClass.lpfnWndProc = WndProc;
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.hInstance = hInstance;
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = MYD3DWINDOWCLASS;

    if (!RegisterClassEx(&wndClass))
    {
        return 1;
    }

    RECT rect = { 0, 0, 600, 600 };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowEx(0, MYD3DWINDOWCLASS, MYD3DWINDOWNAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, hInstance, NULL);
    if (!hwnd)
    {
        return 1;
    }
    ShowWindow(hwnd, nShowCmd);
    UpdateWindow(hwnd);

    //d3d code here
    RECT dimensions;
    GetClientRect(hwnd, &dimensions);
    unsigned int width = dimensions.right - dimensions.left;
    unsigned int height = dimensions.bottom - dimensions.top;

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP, D3D_DRIVER_TYPE_SOFTWARE
    };
    unsigned int totalDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    unsigned int totalFeatureLevels = ARRAYSIZE(featureLevels);

    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Width = width;
    swapChainDesc.BufferDesc.Height = height;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.Windowed = true;
    swapChainDesc.SampleDesc.Count = 4;
    swapChainDesc.SampleDesc.Quality = 0;

    ID3D11Device* d3dDevice_;
    ID3D11DeviceContext* d3dContext_;
    IDXGISwapChain* swapChain_;

    unsigned int creationFlags = 0;
#ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    HRESULT result;
    unsigned int driver = 0;
    D3D_DRIVER_TYPE driverType_;
    D3D_FEATURE_LEVEL featureLevel_;

    for (driver = 0; driver < totalDriverTypes; ++driver)
    {
        result = D3D11CreateDeviceAndSwapChain(0, driverTypes[driver], 0,
            creationFlags, featureLevels, totalFeatureLevels,
            D3D11_SDK_VERSION, &swapChainDesc, &swapChain_,
            &d3dDevice_, &featureLevel_, &d3dContext_);
        if (SUCCEEDED(result))
        {
            driverType_ = driverTypes[driver];
            break;
        }
    }

    if (FAILED(result))
    {
        //DXTRACE_MSG(L"Failed to create the Direct3D device!");
        return false;
    }

    ID3D11RenderTargetView* backBufferTarget_;
    ID3D11Texture2D* backBufferTexture;
    result = swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D),
        (LPVOID*)&backBufferTexture);
    if (FAILED(result))
    {
        //DXTRACE_MSG(L"Failed to get the swap chain back buffer!");
        return false;
    }
    result = d3dDevice_->CreateRenderTargetView(backBufferTexture, 0,
        &backBufferTarget_);
    if (backBufferTexture)
        backBufferTexture->Release();
    if (FAILED(result))
    {
        //DXTRACE_MSG(L"Failed to create the render target view!");
        return false;
    }
    d3dContext_->OMSetRenderTargets(1, &backBufferTarget_, 0);

    D3D11_VIEWPORT viewport;
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    d3dContext_->RSSetViewports(1, &viewport);

    ID3DBlob* vsBuffer = 0;
    ID3DBlob* vsErrMsg = 0;
    result = D3DCompileFromFile(L"SolidGreenColor.vs", NULL, NULL,
        "VS_Main", "vs_4_0", 0, 0, &vsBuffer, &vsErrMsg);
    if (FAILED(result))
    {
        if (vsErrMsg != nullptr)
            OutputDebugStringA((char*)vsErrMsg->GetBufferPointer());

        if (vsErrMsg) vsErrMsg->Release();

        return 1;
    }

    if (vsErrMsg) vsErrMsg->Release();

    ID3D11VertexShader *pVertexShader;
    result = d3dDevice_->CreateVertexShader(vsBuffer->GetBufferPointer(),
        vsBuffer->GetBufferSize(), 0, &pVertexShader);
    if (FAILED(result))
    {
        if (vsBuffer)
            vsBuffer->Release();
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC solidColorLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
        0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,
        0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    unsigned int totalLayoutElements = ARRAYSIZE(solidColorLayout);
    ID3D11InputLayout *pInputLayout;
    result = d3dDevice_->CreateInputLayout(solidColorLayout,
        totalLayoutElements, vsBuffer->GetBufferPointer(),
        vsBuffer->GetBufferSize(), &pInputLayout);
    vsBuffer->Release();
    if (FAILED(result))
    {
        return false;
    }

    ID3DBlob* psBuffer = 0;
    result = D3DCompileFromFile(L"SolidGreenColor.ps", NULL, NULL,
        "PS_Main", "ps_4_0", 0, 0, &psBuffer, &vsErrMsg);
    if (FAILED(result))
    {
        if (vsErrMsg != nullptr)
            OutputDebugStringA((char*)vsErrMsg->GetBufferPointer());

        if (vsErrMsg) vsErrMsg->Release();

        return false;
    }

    if (vsErrMsg) vsErrMsg->Release();

    ID3D11PixelShader *pPixelShader;
    result = d3dDevice_->CreatePixelShader(psBuffer->GetBufferPointer(),
        psBuffer->GetBufferSize(), 0, &pPixelShader);
    psBuffer->Release();
    if (FAILED(result))
    {
        return false;
    }

    struct VertexPos
    {
        XMFLOAT3 pos;
        XMFLOAT2 tex0;
    };
    VertexPos vertices[] =
    {
        { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },
    };

    D3D11_BUFFER_DESC vertexDesc;
    ZeroMemory(&vertexDesc, sizeof(vertexDesc));
    vertexDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexDesc.ByteWidth = sizeof(VertexPos) * 6;
    D3D11_SUBRESOURCE_DATA resourceData;
    ZeroMemory(&resourceData, sizeof(resourceData));
    resourceData.pSysMem = vertices;

    ID3D11Buffer *pBuffer;
    result = d3dDevice_->CreateBuffer(&vertexDesc,
        &resourceData, &pBuffer);
    if (FAILED(result))
    {
        return false;
    }

    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
    desc.Width = 600;
    desc.Height = 600;
    desc.MipLevels = desc.ArraySize = 1;
    desc.SampleDesc.Count = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    ID3D11Texture2D *pTexture;

    DWORD numberOfBytesRead = 0;

    int nNumberOfBytesToRead = 600 * 600 * 4;
    void* pSrcData = malloc(nNumberOfBytesToRead);
    HANDLE  hVideo = CreateFile(L"raw_v", GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    ReadFile(hVideo, pSrcData, nNumberOfBytesToRead, &numberOfBytesRead, NULL);
    D3D11_SUBRESOURCE_DATA data = {0};
    data.pSysMem = pSrcData;
    data.SysMemPitch = 600 * 4;

    result = d3dDevice_->CreateTexture2D(&desc, NULL, &pTexture);
    if (FAILED(result))
    {
        return false;
    }

    ID3D11ShaderResourceView *pSRView;
    result = d3dDevice_->CreateShaderResourceView(pTexture, 0, &pSRView);

    D3D11_SAMPLER_DESC colorMapDesc;
    ZeroMemory(&colorMapDesc, sizeof(colorMapDesc));
    colorMapDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    colorMapDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    colorMapDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    colorMapDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    colorMapDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    colorMapDesc.MaxLOD = D3D11_FLOAT32_MAX;
    ID3D11SamplerState *pSamplerState;
    result = d3dDevice_->CreateSamplerState(&colorMapDesc,
        &pSamplerState);
    if (FAILED(result))
    {
        return false;
    }


    MSG msg = { 0 };
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            //Normal render and update
            float clearColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
            d3dContext_->ClearRenderTargetView(backBufferTarget_, clearColor);

			ReadFile(hVideo, pSrcData, nNumberOfBytesToRead, &numberOfBytesRead, NULL);
			Sleep(40);
            D3D11_MAPPED_SUBRESOURCE mappedResource;
            ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
            result = d3dContext_->Map(pTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			LPBYTE pRow = (LPBYTE)mappedResource.pData;
			LPBYTE pData = (LPBYTE)pSrcData;
			for (int i = 0; i < 600; ++i)
			{
				memcpy(pRow, pData, 600 * 4);
				pRow += mappedResource.RowPitch;
				pData += 600 * 4;
			}
            d3dContext_->Unmap(pTexture, 0);

            unsigned int stride = sizeof(VertexPos);
            unsigned int offset = 0;
            d3dContext_->IASetInputLayout(pInputLayout);
            d3dContext_->IASetVertexBuffers(0, 1, &pBuffer, &stride, &offset);
            d3dContext_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            d3dContext_->VSSetShader(pVertexShader, 0, 0);
            d3dContext_->PSSetShader(pPixelShader, 0, 0);

            d3dContext_->PSSetShaderResources(0, 1, &pSRView);
            d3dContext_->PSSetSamplers(0, 1, &pSamplerState);

            d3dContext_->Draw(6, 0);

            swapChain_->Present(0, 0);
        }
    }

    if (backBufferTarget_) backBufferTarget_->Release();
    if (swapChain_) swapChain_->Release();
    if (d3dContext_) d3dContext_->Release();
    if (d3dDevice_) d3dDevice_->Release();
	if (pPixelShader) pPixelShader->Release();
	if (pInputLayout) pInputLayout->Release();
	if (pVertexShader) pVertexShader->Release();
	if (pBuffer) pBuffer->Release();
	if (pTexture) pTexture->Release();
	if (pSRView) pSRView->Release();
	if (pSamplerState) pSamplerState->Release();

    free(pSrcData);
    CloseHandle(hVideo);
    return msg.wParam;
}