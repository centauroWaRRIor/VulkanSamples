//----------------------------------------------------------------------------------
// File:        LoadingAssets/AssetViewer.cpp
// SDK Version: v3.00 
// Email:       emmanuel.villagomez@gmail.com
// Site:        http://www.victoresite.net/
//
// Copyright (c) 2014-2015, NVIDIA CORPORATION. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------------
#include "AssetViewer.h"
#include "NvAppBase/NvInputTransformer.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvVkUtil/NvModelVK.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"

#define ARRAY_SIZE(a) ( sizeof(a) / sizeof( (a)[0] ))

enum {
	CYCLE_MODEL = 1024
};

AssetViewer::AssetViewer() 
{
	m_transformer->setTranslationVec(nv::vec3f(0.0f, 0.0f, -3.0f));
	m_transformer->setRotationVec(nv::vec3f(NV_PI*0.35f, 0.0f, 0.0f));

    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}

AssetViewer::~AssetViewer()
{
    LOGI("AssetViewer: destroyed\n");
}

void AssetViewer::configurationCallback(NvVKConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 0; 
}

void AssetViewer::initRendering(void) {
	NV_APP_BASE_SHARED_INIT();

	VkResult result = VK_ERROR_INITIALIZATION_FAILED;

	// Helps find the project root directory. Not needed when the working directory is set up correctly
	//NvAssetLoaderAddSearchPath("LoadingAssets");

	//////////////
	// Load Models

	mModelCount = 2;
	mModels = new NvModelVK*[mModelCount];
	mCurrentModel = 0;

	const char* nvmNames[2] = {
		"models/cow.nvm",
		"models/dragon.nvm"
	};

	for (uint32_t i = 0; i < mModelCount; i++) {
		int32_t length;
		char *modelData = NvAssetLoaderRead(nvmNames[i], length);
		NvModelVK* model = NvModelVK::CreateFromPreprocessed(vk(), (uint8_t *)modelData);
		mModels[i] = model;
		NvAssetLoaderFree(modelData);
	}

	// Create descriptor layout to match the shader resources
	VkDescriptorSetLayoutBinding binding[1];
	binding[0].binding = 0;
	binding[0].descriptorCount = 1;
	binding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	binding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	binding[0].pImmutableSamplers = NULL;

	VkDescriptorSetLayoutCreateInfo descriptorSetEntry = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	descriptorSetEntry.bindingCount = 1;
	descriptorSetEntry.pBindings = binding;
	result = vkCreateDescriptorSetLayout(device(), &descriptorSetEntry, 0, &mDescriptorSetLayout);
	CHECK_VK_RESULT();

	// Create descriptor region and set
	VkDescriptorPoolSize descriptorPoolInfo[1];
	descriptorPoolInfo[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	descriptorPoolInfo[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo descriptorRegionInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	descriptorRegionInfo.maxSets = 1;
	descriptorRegionInfo.poolSizeCount = 1;
	descriptorRegionInfo.pPoolSizes = descriptorPoolInfo;
	VkDescriptorPool descriptorRegion;
	result = vkCreateDescriptorPool(device(), &descriptorRegionInfo, NULL, &descriptorRegion);
	CHECK_VK_RESULT();

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	descriptorSetAllocateInfo.descriptorPool = descriptorRegion;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = &mDescriptorSetLayout;
	result = vkAllocateDescriptorSets(device(), &descriptorSetAllocateInfo, &mDescriptorSet);
	CHECK_VK_RESULT();

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &mDescriptorSetLayout;
	result = vkCreatePipelineLayout(device(), &pipelineLayoutCreateInfo, 0, &mPipelineLayout);
	CHECK_VK_RESULT();
	
	// Create static state info for the mPipeline.

	// set dynamically
	VkPipelineViewportStateCreateInfo vpStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	vpStateInfo.pNext = 0;
	vpStateInfo.viewportCount = 1;
	vpStateInfo.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rsStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rsStateInfo.depthClampEnable = VK_TRUE;
	rsStateInfo.rasterizerDiscardEnable = VK_FALSE;
	rsStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rsStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rsStateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rsStateInfo.lineWidth = 1.0f;

	VkPipelineColorBlendAttachmentState attachments[1] = {};
	attachments[0].blendEnable = VK_FALSE;
	attachments[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo cbStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	cbStateInfo.logicOpEnable = VK_FALSE;
	cbStateInfo.attachmentCount = ARRAY_SIZE(attachments);
	cbStateInfo.pAttachments = attachments;

	VkPipelineDepthStencilStateCreateInfo dsStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	dsStateInfo.depthTestEnable = VK_TRUE;
	dsStateInfo.depthWriteEnable = VK_TRUE;
	dsStateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	dsStateInfo.depthBoundsTestEnable = VK_FALSE;
	dsStateInfo.stencilTestEnable = VK_FALSE;
	dsStateInfo.minDepthBounds = 0.0f;
	dsStateInfo.maxDepthBounds = 1.0f;

	VkPipelineMultisampleStateCreateInfo msStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	msStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	msStateInfo.alphaToCoverageEnable = VK_FALSE;
	msStateInfo.sampleShadingEnable = VK_FALSE;
	msStateInfo.minSampleShading = 1.0f;
	uint32_t smplMask = 0x1;
	msStateInfo.pSampleMask = &smplMask;

	VkPipelineTessellationStateCreateInfo tessStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
	tessStateInfo.patchControlPoints = 0;

	VkPipelineDynamicStateCreateInfo dynStateInfo;
	VkDynamicState dynStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	memset(&dynStateInfo, 0, sizeof(dynStateInfo));
	dynStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynStateInfo.dynamicStateCount = 2;
	dynStateInfo.pDynamicStates = dynStates;

	// Shaders
	VkPipelineShaderStageCreateInfo shaderStages[2];
	uint32_t shaderCount = 0;
	{
		int32_t length;
		char* data = NvAssetLoaderRead("shaders/base_model.nvs", length);
		shaderCount = vk().createShadersFromBinaryBlob((uint32_t*)data,
			length, shaderStages, 2);
	}

	mModelPipelines = new VkPipeline[mModelCount];

	for (uint32_t i = 0; i < mModelCount; i++) {
		// Create mPipeline state VI-IA-VS-VP-RS-FS-CB
		VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		pipelineInfo.pVertexInputState = &mModels[i]->getVIInfo();
		pipelineInfo.pInputAssemblyState = &mModels[i]->getIAInfo();
		pipelineInfo.pViewportState = &vpStateInfo;
		pipelineInfo.pRasterizationState = &rsStateInfo;
		pipelineInfo.pColorBlendState = &cbStateInfo;
		pipelineInfo.pDepthStencilState = &dsStateInfo;
		pipelineInfo.pMultisampleState = &msStateInfo;
		pipelineInfo.pTessellationState = &tessStateInfo;
		pipelineInfo.pDynamicState = &dynStateInfo;

		pipelineInfo.stageCount = shaderCount;
		pipelineInfo.pStages = shaderStages;

		pipelineInfo.renderPass = vk().mainRenderTarget()->clearRenderPass();
		pipelineInfo.subpass = 0;

		pipelineInfo.layout = mPipelineLayout;

		result = vkCreateGraphicsPipelines(device(), VK_NULL_HANDLE, 1, &pipelineInfo, NULL,
			(mModelPipelines + i));
		CHECK_VK_RESULT();
	}

	mUBO.Initialize(vk());

	m_transformer->update(0.1f);

	mUBO->mModelViewMatrix = m_transformer->getModelViewMat();

	nv::perspectiveLH(mUBO->mProjectionMatrix, 45.0f * (NV_PI / 180.0f), 16.0f / 9.0f, 1.0f, 100.0f);

	mUBO->mInvProjectionMatrix = nv::inverse(mUBO->mProjectionMatrix);

	mUBO.Update();
	
	// Update the descriptor set
	VkDescriptorBufferInfo uboDescriptorInfo[1] = {};
	mUBO.GetDesc(uboDescriptorInfo[0]);

	VkWriteDescriptorSet writeDescriptorSets[3];
	writeDescriptorSets[0] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	writeDescriptorSets[0].dstSet = mDescriptorSet;
	writeDescriptorSets[0].dstBinding = 0;
	writeDescriptorSets[0].dstArrayElement = 0;
	writeDescriptorSets[0].descriptorCount = 1;
	writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	writeDescriptorSets[0].pBufferInfo = uboDescriptorInfo;
	writeDescriptorSets[1] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	vkUpdateDescriptorSets(device(), 1, writeDescriptorSets, 0, 0);
}



void AssetViewer::shutdownRendering(void) {

    vkDeviceWaitIdle(device());

    // destroy other resources here like the pipelines, vertex buffers and command pools
}

void AssetViewer::initUI(void) {
    if (mTweakBar) {

		NvTweakVarBase *var = mTweakBar->addButton("Change Model", CYCLE_MODEL);
		addTweakButtonBind(var, NvGamepad::BUTTON_Y);

        mTweakBar->syncValues();
    }
}

NvUIEventResponse AssetViewer::handleReaction(const NvUIReaction &react)
{
	return nvuiEventNotHandled;
}


/*void AssetViewer::updateRenderCommands() {
	VkResult result = VK_ERROR_INITIALIZATION_FAILED;

	result = vkDeviceWaitIdle(device());
	CHECK_VK_RESULT();

	VkCommandBufferInheritanceInfo inherit = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };
	inherit.framebuffer = vk().mainRenderTarget()->frameBuffer();
	inherit.renderPass = vk().mainRenderTarget()->clearRenderPass();

	// Record the commands (resets the buffer)
	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
	beginInfo.pInheritanceInfo = &inherit;

	result = vkBeginCommandBuffer(mCmd, &beginInfo);
	CHECK_VK_RESULT();

	// Bind the mPipeline state
	vkCmdBindPipeline(mCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

	{
		VkViewport vp;
		VkRect2D sc;
		vp.x = 0;
		vp.y = 0;
		vp.height = (float)(m_height);
		vp.width = (float)(m_width);
		vp.minDepth = 0.0f;
		vp.maxDepth = 1.0f;

		sc.offset.x = 0;
		sc.offset.y = 0;
		sc.extent.width = vp.width;
		sc.extent.height = vp.height;

		vkCmdSetViewport(mCmd, 0, 1, &vp);
		vkCmdSetScissor(mCmd, 0, 1, &sc);

		if (mDrawGeometry)
		{
			// Bind the vertex and index buffers
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(mCmd, 0, 1, &mVertexBuffer(), offsets);
			vkCmdBindIndexBuffer(mCmd, mIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

			// Draw the triangle
			vkCmdDrawIndexed(mCmd, 3, 1, 0, 0, 0);
		}
	}

	result = vkEndCommandBuffer(mCmd);
	CHECK_VK_RESULT();
}*/


void AssetViewer::reshape(int32_t width, int32_t height)
{
	//updateRenderCommands();
}

void AssetViewer::draw(void)
{
	// Single model case. Model->World transform is identity, so use transformer's matrix directly
	mUBO->mModelViewMatrix = m_transformer->getModelViewMat();
	mUBO->mInvModelViewMatrix = nv::inverse(mUBO->mModelViewMatrix);

	nv::vec4f cameraLight(0.57f, 0.57f, 0.57f, 0.0f);
	nv::vec4f eyeLight = normalize(cameraLight * m_transformer->getRotationMat());

	mUBO->mModelLight[0] = eyeLight.x;
	mUBO->mModelLight[1] = eyeLight.y;
	mUBO->mModelLight[2] = eyeLight.z;

	mUBO.Update();

	// Can't bake these clears into the system at init time, as the 
	// screen/targets can be resized.
	int32_t width = getAppContext()->width(); // TODO what does  this line do?
	int32_t height = getAppContext()->height(); // TODO what does this line do?

	VkResult result = VK_ERROR_INITIALIZATION_FAILED;

	VkCommandBuffer cmd = vk().getMainCommandBuffer();

	VkRenderPassBeginInfo renderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };

	renderPassBeginInfo.renderPass = vk().mainRenderTarget()->clearRenderPass();
	renderPassBeginInfo.framebuffer = vk().mainRenderTarget()->frameBuffer();
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = m_width;
	renderPassBeginInfo.renderArea.extent.height = m_height;

	VkClearValue clearValues[2];
	clearValues[0].color.float32[0] = 0.33f;
	clearValues[0].color.float32[1] = 0.44f;
	clearValues[0].color.float32[2] = 0.66f;
	clearValues[0].color.float32[3] = 1.0f;
	clearValues[1].depthStencil.depth = 1.0f;
	clearValues[1].depthStencil.stencil = 0;


	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.clearValueCount = 2;

	vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	{
		VkViewport vp;
		VkRect2D sc;
		vp.x = 0;
		vp.y = 0;
		vp.height = (float)(m_height);
		vp.width = (float)(m_width);
		vp.minDepth = 0.0f;
		vp.maxDepth = 1.0f;

		sc.offset.x = 0;
		sc.offset.y = 0;
		sc.extent.width = vp.width;
		sc.extent.height = vp.height;

		vkCmdSetViewport(cmd, 0, 1, &vp);
		vkCmdSetScissor(cmd, 0, 1, &sc);

		// Bind the mPipeline state
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mModelPipelines[mCurrentModel]);
		uint32_t offset[2];
		offset[0] = mUBO.getDynamicOffset();
		offset[1] = 0;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &mDescriptorSet, 1, offset);

		mModels[mCurrentModel]->Draw(cmd);
	}
	vkCmdEndRenderPass(cmd);

	vk().submitMainCommandBuffer();
}

NvAppBase* NvAppFactory() {
    return new AssetViewer();
}



