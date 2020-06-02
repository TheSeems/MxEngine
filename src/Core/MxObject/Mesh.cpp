// Copyright(c) 2019 - 2020, #Momo
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
// 
// 1. Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and /or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "Mesh.h"
#include "Utilities/ObjectLoader/ObjectLoader.h"
#include "Utilities/Profiler/Profiler.h"
#include "Platform/GraphicAPI.h"
#include "Utilities/LODGenerator/LODGenerator.h"
#include "Utilities/Format/Format.h"
#include "Core/Resources/ResourceFactory.h"
#include "Core/Components/MeshRenderer.h"

#include <algorithm>

namespace MxEngine
{
	Resource<Material, ResourceFactory> ConvertMaterial(const MaterialInfo& mat, MxHashMap<StringId, GResource<Texture>>& textures)
	{
		auto materialResource = ResourceFactory::Create<Material>();
		auto& material = *materialResource;

		#define MAKE_TEX(tex) if(!mat.tex.empty()) {\
			auto id = MakeStringId(mat.tex);\
			if(textures.find(id) == textures.end())\
				textures[id] = GraphicFactory::Create<Texture>(mat.tex);\
			material.tex = textures[id];}

		MAKE_TEX(map_Ka);
		MAKE_TEX(map_Kd);
		MAKE_TEX(map_Ks);
		MAKE_TEX(map_Ke);
		MAKE_TEX(map_d);
		MAKE_TEX(map_height);
		MAKE_TEX(map_normal);

		material.Tf    = mat.Tf;
		material.Ka    = mat.Ka;
		material.Kd    = mat.Kd;
		material.Ke    = mat.Ke;
		material.Ks    = mat.Ks;
		material.illum = mat.illum;
		material.Ns    = mat.Ns;
		material.Ni    = mat.Ni;
		material.d     = mat.d;

		if (material.Ns == 0.0f) material.Ns = 128.0f; // bad as pow(0.0, 0.0) -> NaN //-V550

		return materialResource;
	}

	void Mesh::LoadFromFile(const MxString& filepath, MeshRenderer* meshRenderer)
	{
		ObjectInfo objectInfo = ObjectLoader::Load(filepath);
		this->boundingBox = objectInfo.boundingBox;

		MxVector<SubMesh::TransformHandle> submeshTransforms;
		MxVector<size_t> materialIds(objectInfo.meshes.size(), 0);

		for (const auto& group : objectInfo.meshes)
		{
			submeshTransforms.push_back(ComponentFactory::CreateComponent<Transform>());
		}

		if (meshRenderer != nullptr)
		{
			MxHashMap<StringId, GResource<Texture>> textures;
			materialIds.resize(0); // clear ids to fill them with meaningful values
			meshRenderer->Materials.resize(objectInfo.materials.size());

			for (size_t i = 0; i < objectInfo.materials.size(); i++)
			{
				meshRenderer->Materials[i] = ConvertMaterial(objectInfo.materials[i], textures);
			}

			for (const auto& group : objectInfo.meshes)
			{
				if(group.useTexture&& group.material != nullptr)
				{
					size_t offset = size_t(group.material - objectInfo.materials.data());
					materialIds.push_back(offset);
				}
			}
		}

		std::vector<ObjectInfo> LODdata;
		{
			MAKE_SCOPE_TIMER("MxEngine::LODGenerator", "GenerateLODs");
			MAKE_SCOPE_PROFILER("LODGenerator::GenerareLODs");

			LODGenerator lod(objectInfo);
			std::array LODfactors = { 0.001f, 0.01f, 0.05f, 0.15f, 0.3f };
			for (size_t factor = 0; factor < LODfactors.size(); factor++)
			{
				LODdata.push_back(lod.CreateObject(LODfactors[factor]));

				#if defined(MXENGINE_DEBUG)
				size_t vertecies = 0;
				for (const auto& mesh : LODdata.back().meshes)
				{
					vertecies += mesh.indicies.size();
				}
				Logger::Instance().Debug("MxEngine::LODGenerator", MxFormat("LOD[{0}]: vertecies = {1}", factor + 1, vertecies));
				#endif
			}
		}
		LODdata.insert(LODdata.begin(), std::move(objectInfo));

		MAKE_SCOPE_TIMER("MxEngine::Mesh", "GenerateBuffers");
		MAKE_SCOPE_PROFILER("Mesh::GenerateBuffers"); 
		this->LODs.clear();
		for(const auto& lod : LODdata)
		{
			auto& meshes = this->LODs.emplace_back();
			for (size_t i = 0; i < lod.meshes.size(); i++)
			{
				auto& meshData = lod.meshes[i];
				auto& materialId = materialIds[i];
				auto& transform = submeshTransforms[i];

				SubMesh submesh(materialId, transform);
				submesh.MeshData.GetVertecies() = std::move(meshData.vertecies);
				submesh.MeshData.GetIndicies() = std::move(meshData.indicies);
				submesh.MeshData.BufferVertecies();
				submesh.MeshData.BufferIndicies();
				submesh.Name = MakeStringId(meshData.name);

				meshes.push_back(std::move(submesh));
			}
		}
	}

    Mesh::Mesh(const FilePath& path, MeshRenderer* meshRenderer)
    {
		this->LoadFromFile(ToMxString(path), meshRenderer);
    }

	void Mesh::Load(const MxString& filepath, MeshRenderer* meshRenderer)
	{
		this->LoadFromFile(filepath, meshRenderer);
	}

	Mesh::LOD& Mesh::GetSubmeshes()
	{
		return this->LODs[this->currentLOD];
	}

	const Mesh::LOD& Mesh::GetSubmeshes() const
	{
		return this->LODs[this->currentLOD];
	}

    void Mesh::PushEmptyLOD()
    {
		this->LODs.emplace_back();
    }

	void Mesh::PopLastLOD()
	{
		if(!this->LODs.empty())
			this->LODs.pop_back();
	}

	void Mesh::SetLOD(size_t LOD)
	{
		this->currentLOD = Min(LOD, (int)this->LODs.size() - 1);
	}

    size_t Mesh::GetLOD() const
    {
		return this->currentLOD;
    }

	size_t Mesh::GetLODCount() const
	{
		return this->LODs.size();
	}

	const AABB& Mesh::GetAABB() const
	{
		return this->boundingBox;
	}

	void Mesh::SetAABB(const AABB& boundingBox)
	{
		this->boundingBox = boundingBox;
	}

	void Mesh::AddInstancedBuffer(UniqueRef<VertexBuffer> vbo, UniqueRef<VertexBufferLayout> vbl)
	{
		this->VBOs.push_back(std::move(vbo));
		this->VBLs.push_back(std::move(vbl));
		for (auto& meshes : this->LODs)
		{
			for (auto& mesh : meshes)
			{
				mesh.MeshData.GetVAO().AddInstancedBuffer(*this->VBOs.back(), *this->VBLs.back());
			}
		}
	}

	VertexBuffer& Mesh::GetBufferByIndex(size_t index)
	{
		assert(index < this->VBOs.size());
		return *this->VBOs[index];
	}

	VertexBufferLayout& Mesh::GetBufferLayoutByIndex(size_t index)
	{
		assert(index < this->VBLs.size());
		return *this->VBLs[index];
	}

    size_t Mesh::GetBufferCount() const
    {
		return this->VBOs.size();
    }
}