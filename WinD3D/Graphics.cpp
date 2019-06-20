#include "Graphics.h"
#include "dxerr.h"
#include <sstream>

#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "InputLayout.h"
#include "PixelShader.h"
#include "VertexShader.h"
#include "Topology.h"
#include "ConstantBuffer.h"

#include "GraphicsThrows.m"

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"D3DCompiler.lib")

// Exceptions
Graphics::HrException::HrException(int line, const char * file, HRESULT hr, std::vector<std::string> messages) noexcept
	:GException(line,file),hr(hr)
{
	// join messages in a single string
	for (const auto& m : messages)
	{
		info += m;
		info.push_back('\n');
	}
	if (!info.empty())
	{
		info.pop_back();
	}
}

HRESULT Graphics::HrException::GetErrorCode() const noexcept
{
	return hr;
}
std::string Graphics::HrException::GetErrorString() const noexcept
{
	return DXGetErrorString(hr);
}
std::string Graphics::HrException::GetErrorDescription() const noexcept
{
	char buf[512];
	DXGetErrorDescription(hr, buf, 512u);
	return buf;
}
std::string Graphics::HrException::GetErrorInfo() const noexcept
{
	return info;
}
const char* Graphics::HrException::what()const noexcept
{
	std::ostringstream oss;
	oss << GetType() << std::endl
		<< "[Error Code]: 0x" << std::hex << std::uppercase << GetErrorCode()
		<< std::dec << " (" << (unsigned long)GetErrorCode() << ")" << std::endl
		<< "[Error String]: " << GetErrorString() << std::endl
		<< "[Description]: " << GetErrorDescription() << std::endl;
	if (!info.empty())
	{
		oss << "\n [Error Info]:\n" << GetErrorInfo() << std::endl << std::endl;
	}

	oss << GetOriginString();
	whatBuffer = oss.str();
	return whatBuffer.c_str();
}
const char* Graphics::HrException::GetType()const noexcept
{
	return "Veritas Graphics Exception";
}

const char* Graphics::DeviceRemovedException::GetType()const noexcept
{
	return "Graphics Device Exception [Device Removed](DXGI_ERROR_DEVICE_REMOVED)";
}

// Graphics
Graphics::Graphics(HWND hWnd)
{
	DXGI_SWAP_CHAIN_DESC DSwapDesc = {};
	DSwapDesc.BufferDesc.Width = 0;
	DSwapDesc.BufferDesc.Height = 0;
	DSwapDesc.BufferDesc.Format = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM;
	DSwapDesc.BufferDesc.RefreshRate.Numerator = 0;
	DSwapDesc.BufferDesc.RefreshRate.Denominator = 0;
	DSwapDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	DSwapDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER::DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	
	DSwapDesc.SampleDesc.Count = 1;
	DSwapDesc.SampleDesc.Quality = 0;

	DSwapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	DSwapDesc.BufferCount = 1;
	DSwapDesc.OutputWindow = hWnd;
	DSwapDesc.Windowed = TRUE;
	DSwapDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	DSwapDesc.Flags = 0;

	UINT swapCreateFlags = 0u;
#ifndef NDEBUG
	swapCreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // !NDEBUG


	HRESULT hr;

	// create a device with pointers
	GFX_THROW_INFO(D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		swapCreateFlags,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		&DSwapDesc,
		&pSwap,
		&pDevice,
		nullptr,
		&pContext
	));

	// access to back buffer
	Microsoft::WRL::ComPtr<ID3D11Resource> pBackBuffer;
	GFX_THROW_INFO(pSwap->GetBuffer(0, __uuidof(ID3D11Texture2D), &pBackBuffer));
	GFX_THROW_INFO(pDevice->CreateRenderTargetView(
		pBackBuffer.Get(), nullptr, &pTarget
	));

	// Create a depth stencil
	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> pDSState;
	GFX_THROW_INFO(pDevice->CreateDepthStencilState(&dsDesc, &pDSState));

	//bind Depth Stencil
	pContext->OMSetDepthStencilState(pDSState.Get(), 1u);

	//create Depth Stencil texture
	Microsoft::WRL::ComPtr<ID3D11Texture2D> pDepthStencil;
	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = 800u;
	descDepth.Height = 600u;
	descDepth.MipLevels = 1u;
	descDepth.ArraySize = 1u;
	descDepth.Format = DXGI_FORMAT_D32_FLOAT;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	descDepth.SampleDesc.Count = 1u;
	descDepth.SampleDesc.Quality = 0u;

	//bind Depth Stencil texture
	GFX_THROW_INFO(pDevice->CreateTexture2D(&descDepth, nullptr, &pDepthStencil));

	//create view of depth stencil
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0u;
	GFX_THROW_INFO(pDevice->CreateDepthStencilView(
		pDepthStencil.Get(), &dsvDesc, &pDSV
	));

	//bind Depth Stencil view
	pContext->OMSetRenderTargets(1u, pTarget.GetAddressOf(), pDSV.Get());

	// configure viewport
	D3D11_VIEWPORT vp;
	vp.Width = 800.0f;
	vp.Height = 600.0f;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	pContext->RSSetViewports(1u, &vp);
}

void Graphics::EndFrame()
{
	HRESULT hr;
#ifndef NDEBUG
	infoManager.Set();
#endif // !NDEBUG

	if (FAILED(hr = pSwap->Present(1u, 0u)))
	{
		if (hr == DXGI_ERROR_DEVICE_REMOVED)
		{
			throw GFX_DEVICE_REMOVED_EXCEPT(pDevice->GetDeviceRemovedReason());
		}
		else
		{
			throw GFX_EXCEPT(hr);
		}
	}
}
void Graphics::ClearBuffer(float r, float g, float b) noexcept
{
	const float color[] = { r,g,b,1.0f };
	pContext->ClearRenderTargetView(pTarget.Get(), color);
	pContext->ClearDepthStencilView(pDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0u);
}
void Graphics::DrawTestTriangle(float angle, float x, float y)
{
	HRESULT hr;
	struct Vertex2D
	{
		struct
		{
			float x;
			float y;
			float z;
		}pos;
	};

	std::vector<struct Vertex2D> verticies =
	{
		{ -1.0f, -1.0f, -1.0f},
		{ 1.0f, -1.0f,-1.0f},
		{ -1.0f, 1.0f, -1.0f },
		{ 1.0f, 1.0f, -1.0f },
		{ -1.0f, -1.0f, 1.0f },
		{ 1.0f, -1.0f, 1.0f },
		{ -1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f }
	};

	// Create a vertex buffer
	VertexBuffer vb(*this, verticies);
	vb.Bind(*this);

	//create an index buffer
	const std::vector<WORD> indicies = 
	{
		0,2,1, 2,3,1,
		1,3,5, 3,7,5,
		2,6,3, 3,6,7,
		4,5,7, 4,7,6,
		0,4,2, 2,4,6,
		0,1,4, 1,5,4
	};

	IndexBuffer ib(*this, indicies);
	ib.Bind(*this);

	//Create constant buffer
	struct rConstantBuffer
	{
		DirectX::XMMATRIX transform;
	};

	const rConstantBuffer rcb =
	{
		{
			DirectX::XMMatrixTranspose(
			DirectX::XMMatrixRotationZ(angle) *
			DirectX::XMMatrixRotationX(angle) *
			DirectX::XMMatrixTranslation(x,0.0f,y+4.0f)*
			DirectX::XMMatrixPerspectiveLH(1.0f,3.0f/4.0f,0.5f,10.0f)
			)
		}
	};
	VertexConstantBuffer<rConstantBuffer> cb(*this, rcb);
	cb.Bind(*this);
	
	struct PCB
	{
		struct
		{
			float r, g, b, a;
		}face_colors[6];
	};
	const PCB pcb2 = 
	{
		{
			{ 1.0f, 0.0f, 1.0f },
			{ 1.0f, 0.0f, 0.0f },
			{ 1.0f, 1.0f, 0.0f },
			{ 0.0f, 0.0f, 1.0f },
			{ 1.0f, 1.0f, 1.0f },
			{ 0.0f, 1.0f, 1.0f },
		}
	};

	PixelConstantBuffer<PCB> pcbb(*this, pcb2);
	pcbb.Bind(*this);



	// Create a pixel shader 
	PixelShader ps(*this, L"PixelShader.cso");
	ps.Bind(*this);

	// Create vertex shader
	VertexShader vs(*this, L"VertexShader.cso");
	vs.Bind(*this);

	// input layout (connections with shaders)
	std::vector< D3D11_INPUT_ELEMENT_DESC> ied =
	{
		{
			"Position",
			0,
			DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			0,
			D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
			0
		}
	};

	InputLayout il(*this, ied, vs.GetBytecode());
	il.Bind(*this);

	// Setting primitive topology
	Topology tp(*this, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	tp.Bind(*this);

	// Configure a viewport
	D3D11_VIEWPORT vp = {};
	vp.Width = 800;
	vp.Height = 600;
	vp.MinDepth = 0;
	vp.MaxDepth = 1;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	pContext->RSSetViewports(1u, &vp);

	GFX_THROW_INFO_ONLY( pContext->DrawIndexed((UINT)std::size(indicies), 0u, 0u));
}
void Graphics::SetProjection(DirectX::FXMMATRIX proj) noexcept
{
	projection = proj;
}
DirectX::XMMATRIX Graphics::GetProjection() const noexcept
{
	return projection;
}
void Graphics::DrawIndexed(UINT count) noexcept(!IS_DEBUG)
{
	GFX_THROW_INFO_ONLY(pContext->DrawIndexed(count, 0u, 0u));
}


Graphics::ContextException::ContextException(int line, const char * file, std::vector<std::string> messages) noexcept
	:GException(line, file)
{
	// join messages in a single string
	for (const auto& m : messages)
	{
		info += m;
		info.push_back('\n');
	}
	if (!info.empty())
	{
		info.pop_back();
	}
}
std::string Graphics::ContextException::GetErrorInfo() const noexcept
{
	return info;
}
const char* Graphics::ContextException::what()const noexcept
{
	std::ostringstream oss;
	oss << GetType() << std::endl;
	if (!info.empty())
	{
		oss << "\n [Error Info]:\n" << GetErrorInfo() << std::endl << std::endl;
	}
	oss << GetOriginString();
	whatBuffer = oss.str();
	return whatBuffer.c_str();
}
const char* Graphics::ContextException::GetType()const noexcept
{
	return "Veritas Graphics Info Exception";
}
