#pragma once
#include "D3DUtil.h"
class PSOCommon {
private:
	ID3D12RootSignature* m_RootSignature{ nullptr };
	ID3D12PipelineState* m_PSO{ nullptr };
	void CreateRootSignature();
public:
	// args: shader path, with out file extent.
	PSOCommon();
	~PSOCommon();
	virtual void Bind(ID3D12GraphicsCommandList* cmdList);
};