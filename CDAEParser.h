#pragma once

#include <fstream> //ifstream

//forward declarations.
struct aiNode;
struct aiMesh;
struct aiString;
struct aiScene;
struct aiMaterial;

template<typename TReal>
class aiMatrix4x4t;

namespace my_collada_parser
{
	class CDAEParser
	{
	public:
		CDAEParser();
		virtual ~CDAEParser();
		bool startParsing(const char* inputFile, const char* outputFile);

	private:
		void writeMaterial(const aiMaterial* pMaterial, std::ofstream& outputStream);
		void writeChildComponent(const aiNode* pComponent, const aiScene* pScene, std::ofstream& outputStream);
		void writeColor3D(const float r, const float g, const float b, std::ofstream& outputStream);
		void writeColor4D(const float r, const float g, const float b, const float a, std::ofstream& outputStream);
		template<typename TReal>
		void writeMesh(const aiMesh* pMesh, const aiScene* pScene, const aiMatrix4x4t<TReal>* pTransformMatrix, std::ofstream& outputStream);
		void writeString(const aiString* pString, std::ofstream& outputStream);
		template<typename TReal>
		void ApplyTransform(const aiMesh* mesh, const aiMatrix4x4t<TReal>* pInputMatrix);
	};
}

