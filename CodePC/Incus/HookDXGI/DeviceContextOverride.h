#pragma once

#include "GlobalData.h"

typedef HRESULT(__stdcall *D3D11_DeviceContext_RSSetStateType)(ID3D11DeviceContext*, ID3D11RasterizerState *);
typedef HRESULT(__stdcall *D3D11_DeviceContext_VSSetShaderType)(ID3D11DeviceContext*, ID3D11VertexShader*, ID3D11ClassInstance*const*, UINT);



class DeviceContextOverride
{
	static DeviceContextOverride* instance;

	ID3D11DeviceContext* deviceContext;
	ID3D11DeviceContext1* deviceContext1;
	ID3D11DeviceContext2* deviceContext2;
	ID3D11DeviceContext3* deviceContext3;
	ID3D11DeviceContext4* deviceContext4;

	D3DResolve* d3d_resolve_;

	HardHook hook_RSSetState;
	HardHook hook_VSSetShader;

	DeviceContextOverride()
	{
		deviceContext = nullptr;
		deviceContext1 = nullptr;
		deviceContext2 = nullptr;
		deviceContext3 = nullptr;
		deviceContext4 = nullptr;
	}

public:

	ID3D11DeviceContext* GetDeviceContext()
	{
		return deviceContext;
	}
	
	static DeviceContextOverride* GetInstance();

	HRESULT Init(ID3D11DeviceContext* deviceContext)
	{
		this->deviceContext = deviceContext;

		// Setup Hooks
		hook_RSSetState.SetupInterface(deviceContext, 43, reinterpret_cast<voidFunc>(RSSetState)); // d3d_4.h 43
		hook_VSSetShader.SetupInterface(deviceContext, 11, reinterpret_cast<voidFunc>(VSSetShader)); // 11
		//MessageBoxA(0, "Done setting up devicecontext interface!", "", 0); 11

		return S_OK;
	}

	void CopySharedResource(ID3D11Resource* &original, ID3D11Resource* &copy)
	{
		ID3D11Texture2D* texture = (ID3D11Texture2D*)copy;
		D3D11_TEXTURE2D_DESC desc;

		texture->GetDesc(&desc);

		for (UINT item = 0; item < desc.ArraySize; ++item)
		{
			for (UINT level = 0; level < desc.MipLevels; ++level)
			{
				UINT index = D3D11CalcSubresource(level, item, desc.MipLevels);
				GlobalDeviceContext->ResolveSubresource(copy, index, original, index, desc.Format);
			}
		}

		//deviceContext->CopyResource(copy, original);
	}

	static HRESULT __stdcall RSSetState(ID3D11DeviceContext* devicecontext, ID3D11RasterizerState *pRasterizerState)
	{
		return DeviceContextOverride::GetInstance()->RSSetStateInternal(devicecontext, pRasterizerState);
	}

	HRESULT RSSetStateInternal(ID3D11DeviceContext* devicecontext, ID3D11RasterizerState *pRasterizerState)
	{
		HRESULT hr = S_OK;
		//MessageBoxA(0, "My set rasterizer state function was called!", "", 0);


		//MessageBoxA(0, ("Looking for :" + std::to_string((unsigned int)pRasterizerState)).c_str(), "", 0);


		for (auto& stateData : rData)
		{
			//MessageBoxA(0, ("In vector :" + std::to_string((unsigned int)stateData.original)).c_str(), "", 0);
			if (stateData.original == pRasterizerState)
			{
				//MessageBoxA(0, "Found it!", "", 0);
				if (stateData.useWireframe)
					pRasterizerState = stateData.wireframe;

				break;
			}
		}

		D3D11_DeviceContext_RSSetStateType oRSSetState = (D3D11_DeviceContext_RSSetStateType)hook_RSSetState.call;
		hook_RSSetState.Restore();
		hr = oRSSetState(devicecontext, pRasterizerState);
		hook_RSSetState.Inject();

		return hr;
	}

	static HRESULT __stdcall VSSetShader(ID3D11DeviceContext* devicecontext, ID3D11VertexShader* pVertexShader, ID3D11ClassInstance*const* ppClassInstances, UINT NumClassInstances)
	{
		return DeviceContextOverride::GetInstance()->VSSetShaderInternal(devicecontext, pVertexShader, ppClassInstances, NumClassInstances);
	}

	HRESULT VSSetShaderInternal(ID3D11DeviceContext* devicecontext, ID3D11VertexShader* pVertexShader, ID3D11ClassInstance*const* ppClassInstances, UINT NumClassInstances)
	{
		HRESULT hr = S_OK;
		//MessageBoxA(0, "My set vs shader function was called!", "", 0);
		D3D11_DeviceContext_VSSetShaderType oVSSetShader = (D3D11_DeviceContext_VSSetShaderType)hook_VSSetShader.call;
		hook_VSSetShader.Restore();
		hr = oVSSetShader(devicecontext, pVertexShader, ppClassInstances, NumClassInstances);
		hook_VSSetShader.Inject();

		return hr;
	}

};

DeviceContextOverride* DeviceContextOverride::instance = nullptr;
DeviceContextOverride* DeviceContextOverride::GetInstance()
{
	if (!instance)
		instance = new DeviceContextOverride();

	return instance;
}