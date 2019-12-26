#pragma once
#include <Engine/Architecture/TransformCBuf.h>

class TransformUnified : public TransformCbuf
{
public:
	TransformUnified(Graphics& gfx, const Drawable& parent, UINT slotV = 0u, UINT slotP = 0u);
public:
	void Bind(Graphics& gfx)noexcept override;
protected:
	void UpdateBindImpl(Graphics& gfx, const Transforms& tf)noexcept;
private:
	static std::unique_ptr<PixelConstantBuffer<Transforms>>pPCBuf;
};