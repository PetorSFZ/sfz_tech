// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
//               For other contributors see Contributors.txt
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include "ph/renderer/NextGenRendererState.hpp"

namespace ph {

// NextGenRendererState: Helper methods
// ------------------------------------------------------------------------------------------------

uint32_t NextGenRendererState::findNextBarrierIdx() const noexcept
{
	uint32_t numStages = this->configurable.presentQueueStages.size();
	for (uint32_t i = this->currentStageSetIdx; i < numStages; i++) {
		const Stage& stage = this->configurable.presentQueueStages[i];
		if (stage.stageType == StageType::USER_STAGE_BARRIER) return i;
	}
	return ~0u;
}

uint32_t  NextGenRendererState::findActiveStageIdx(StringID stageName) const noexcept
{
	sfz_assert_debug(stageName != StringID::invalid())
	uint32_t numStages = this->configurable.presentQueueStages.size();
	for (uint32_t i = this->currentStageSetIdx; i < numStages; i++) {
		const Stage& stage = this->configurable.presentQueueStages[i];
		if (stage.stageName == stageName) return i;
		if (stage.stageType == StageType::USER_STAGE_BARRIER) break;
	}
	return ~0u;
}

uint32_t  NextGenRendererState::findPipelineRenderingIdx(StringID pipelineName) const noexcept
{
	sfz_assert_debug(pipelineName != StringID::invalid());
	uint32_t numPipelines = this->configurable.renderingPipelines.size();
	for (uint32_t i = 0; i < numPipelines; i++) {
		const PipelineRenderingItem& item = this->configurable.renderingPipelines[i];
		if (item.name == pipelineName) return i;
	}
	return ~0u;
}

PerFrame<ConstantBufferMemory>* NextGenRendererState::findConstantBufferInCurrentInputStage(
	uint32_t shaderRegister) noexcept
{
	// Find constant buffer
	Framed<ConstantBufferMemory>* framed = currentInputEnabledStage->constantBuffers.find(
		[&](Framed<ConstantBufferMemory>& item) {
		return item.states[0].state.shaderRegister == shaderRegister;
	});
	if (framed == nullptr) return nullptr;

	// Get this frame's data
	return &framed->getState(currentFrameIdx);
}

} // namespace ph
