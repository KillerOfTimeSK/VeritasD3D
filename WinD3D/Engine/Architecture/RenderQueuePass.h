#pragma once
#include <Engine\Architecture\BindingPass.h>
#include "Job.h"
#include <vector>

class Graphics;

namespace RG
{
	class RenderQueuePass : public BindingPass
	{
	public:
		using BindingPass::BindingPass;
		void Accept(Job job) noexcept;
		void Execute(Graphics& gfx) const noxnd override;
		void Reset() noxnd override;
	private:
		std::vector<Job> jobs;
	};
}