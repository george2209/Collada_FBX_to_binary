#include "CDAEParser.h"
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include<iostream> //cout
#include "my_utils.h"
#include <cassert>

namespace my_collada_parser
{
	using namespace std;

	CDAEParser::CDAEParser()
	{

	}


	CDAEParser::~CDAEParser()
	{

	}

	/// <summary>
	/// the parsing is made on the caller thread, meaning there is no async operation done as at this moment I consider this an overdesign.
	/// If later this shall run in a backgroudn thread better create that thread externally and call this function from there.
	/// </summary>
	/// <param name="inputFile">inputFile</param>
	/// <param name="outputFile">outputFile</param>
	/// <returns>false in case of failure (a console message will be sent to std::cout"</returns>
	bool CDAEParser::startParsing(const char* inputFile, const char* outputFile)
	{
		Assimp::Importer importer;

		//to be check/considered: aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices | aiProcess_GenNormals | aiProcess_RemoveRedundantMaterials 
		//| aiProcess_FixInfacingNormals | aiProcess_FindDegenerates | 
		// aiProcess_TransformUVCoords --> only for DirectX
		//DO NOT USE:
		// * aiProcess_PreTransformVertices --> is destroying the internal structure so you cannot find the names and ids of the subcomponents.
		const aiScene* pScene = importer.ReadFile(inputFile,
			//aiProcess_ValidateDataStructure | 
			//aiProcess_FlipUVs |
			aiProcess_Triangulate //probably not needed..kept here in case the export from Blender is not following my suggestions from documentation
			/////////////////////////////////| aiProcess_PreTransformVertices //added to bring all objects into the world space from the 0,0,0 origin
			| aiProcess_FindInvalidData);

		if (!pScene || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !pScene->mRootNode)
		{
			cout << "Parsing input file...FAILED! Please make sure that the file is a Collada (.dae) file." << endl;
			cout << "ERROR::ASSIMP::" << importer.GetErrorString() << endl;
			return false;
		}
		else 
		{
			ofstream outputStream;
			outputStream.open(outputFile, ofstream::binary);
			try {
				//no of materials
				outputStream.write((char*)&(pScene->mNumMaterials), sizeof(signed int));
				assert(pScene->mNumMaterials > 0);
				assert(pScene->mNumMaterials <= INT_MAX);
				for (signed int m = 0; m < static_cast<signed int>(pScene->mNumMaterials); m++)
				{
					//each material
					const aiMaterial* pMaterial = pScene->mMaterials[m];
					writeMaterial(pMaterial, outputStream);
				}

				//write the components tree.
				const aiNode* rootNode = pScene->mRootNode;
				cout << "starting with ROOT: " << rootNode->mName.C_Str() << endl;
				assert(rootNode->mNumMeshes == 0);
				assert(rootNode->mNumChildren <= INT_MAX);

				const signed int componentsNo = static_cast<signed int>(rootNode->mNumChildren);
				outputStream.write((char*)&(componentsNo), sizeof(signed int));

				for (signed int c = 0; c < componentsNo; c++)
				{
					const aiNode* pComponent = rootNode->mChildren[c];
					aiMatrix4x4 tmpMatrix = importer.GetPropertyMatrix(AI_CONFIG_PP_PTV_ROOT_TRANSFORMATION, aiMatrix4x4());
					if(!tmpMatrix.IsIdentity())
						pScene->mRootNode->mTransformation = tmpMatrix * pScene->mRootNode->mTransformation;
					writeChildComponent(pComponent, pScene, outputStream);
				}
								

				//cleanup
				outputStream.flush();
			}
			catch (exception& e)
			{
				cout << "Export failed!" << endl << e.what() << endl;
				
			}

			outputStream.close();
			return true;
		}
	}

	/// <summary>
	/// write one material unit into the output stream
	/// </summary>
	/// <param name="pMaterial"></param>
	/// <param name="outputStream"></param>
	void CDAEParser::writeMaterial(const aiMaterial* pMaterial, std::ofstream& outputStream)
	{
		aiString materialName = pMaterial->GetName();
		//material name + length
		this->writeString(&materialName, outputStream);
		//ambient color KA factor
		aiColor4D ambientKA;
		//see all default values inside "........\assimp\code\AssetLib\Collada\ColladaHelper.h" (constructor of 
		assert(aiReturn_SUCCESS == pMaterial->Get(AI_MATKEY_COLOR_AMBIENT, ambientKA));
		bool isAmbientDefault = false;
		//KA ambient material color (0.1f,0.1f,0.1f) means it doesn`t exist. I don`t like this... :(
		IS_DEFAULT_4D_COLOR((&ambientKA), "AI_MATKEY_COLOR_AMBIENT", (&isAmbientDefault));
		char val = isAmbientDefault ? 0 : 1;
		outputStream.write(&val, 1);
		if (!isAmbientDefault)
		{
			writeColor4D(ambientKA.r, ambientKA.g, ambientKA.b, ambientKA.a, outputStream);
		}
		//diffuse color KE factor
		aiColor4D diffuseKE;
		assert(aiReturn_SUCCESS == pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseKE));
		bool isDiffuseDefault = false;
		IS_DEFAULT_4D_COLOR((&diffuseKE), "AI_MATKEY_COLOR_DIFFUSE", (&isDiffuseDefault));
		val = isDiffuseDefault ? 0 : 1;
		outputStream.write(&val, 1);
		if (!isDiffuseDefault)
		{
			writeColor4D(diffuseKE.r, diffuseKE.g, diffuseKE.b, diffuseKE.a, outputStream);
		}
		//ambient texture *******************TODO**********************************************
		if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
			aiString myTexture;
			pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &myTexture);
		}
		signed short fileSize = 0;
		outputStream.write((char*)(&fileSize), sizeof(unsigned short));
		//diffuse texture *******************TODO**********************************************
		outputStream.write((char*)(&fileSize), sizeof(unsigned short));
	}

	/// <summary>
	/// write a mesh of this object`s root into the output stream
	/// </summary>
	/// <param name="pMesh"></param>
	/// <param name="pScene"></param>
	/// <param name="pTransformMatrix"></param>
	/// <param name="outputStream"></param>
	template<typename TReal>
	void CDAEParser::writeMesh(const aiMesh* pMesh, const aiScene* pScene, const aiMatrix4x4t<TReal>* pTransformMatrix, std::ofstream& outputStream)
	{
		cout << "\t mesh: " << pMesh->mName.C_Str() << " having " << pMesh->mNumVertices << " vertices" << endl;
		//material id
		assert(pMesh->mMaterialIndex < SHRT_MAX);
		const signed short materialID = static_cast<signed short>(pMesh->mMaterialIndex);
		outputStream.write((char*)(&materialID), sizeof(unsigned short));
		const aiMaterial* pMaterial = pScene->mMaterials[materialID];
		//no of vertices
		assert(pMesh->mNumVertices < INT_MAX);
		const signed int numVertices = static_cast<signed int>(pMesh->mNumVertices);
		outputStream.write((char*)(&numVertices), sizeof(unsigned int));
		
		//before writing them we must convert them into the world space. Otherwise all objects will be draw relative to the 0,0,0 axis of the world against the 0,0,0 axis of the model
		//meaning that a crone of a tree for example will be placed on the floor instead of beeing on top of its trunk
		ApplyTransform<ai_real>(pMesh, pTransformMatrix);

		//write vertices properties as follows:
		//bool::isUV, bool::isNormal, bool::isColorPerVertex
		const bool isAmbientTexture = (pMaterial->GetTextureCount(aiTextureType_AMBIENT) > 0);
		const bool isDiffuseTexture = (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0);
		const bool isNormalsTexture = (pMaterial->GetTextureCount(aiTextureType_NORMALS) > 0);
		const bool isRoughnessTexture = (pMaterial->GetTextureCount(aiTextureType_NORMALS) > 0);
		const bool isUV = (pMesh->mTextureCoords[0] != NULL) &&
			(isAmbientTexture || isDiffuseTexture || isNormalsTexture || isRoughnessTexture);
		outputStream.write((char*)&(isUV), sizeof(bool)); // uv
		const bool isNormal = pMesh->HasNormals();
		outputStream.write((char*)&(isNormal), sizeof(bool)); // norm
		const bool hasColors = pMesh->HasVertexColors(0);
		outputStream.write((char*)&(hasColors), sizeof(bool));//color per vertex

		for (signed int v = 0; v < numVertices; v++)
		{
			//X,Y,Z coordinates as signed float
			const aiVector3D& transformedVector = pMesh->mVertices[v];
			assert(sizeof(transformedVector.x) == sizeof(float));
			outputStream.write((char *) &(transformedVector.x), sizeof(float)); //x
			outputStream.write((char*)&(transformedVector.y), sizeof(float)); //y
			outputStream.write((char*)&(transformedVector.z), sizeof(float)); //z
			cout << "\tx=" << transformedVector.x << ", y=" << transformedVector.y << ", z=" << transformedVector.z << endl;
			//write U,V
			//I will not write the U,V if there is no texture that I am interested in deffined.
			if (isUV)
			{
				assert(pMesh->mTextureCoords[0] != nullptr);
				const aiVector3D& uvPosition = pMesh->mTextureCoords[0][v];
				//u, v
				outputStream.write((char*)&(uvPosition.x), sizeof(float)); //u
				outputStream.write((char*)&(uvPosition.y), sizeof(float)); //v
			}
			//write normals
			if (isNormal)
			{
				const aiVector3D& normal = pMesh->mNormals[v];
				outputStream.write((char*)&(normal.x), sizeof(float)); //x
				outputStream.write((char*)&(normal.y), sizeof(float)); //y
				outputStream.write((char*)&(normal.z), sizeof(float)); //z
			}

			// color per vertex
			if (hasColors)
			{
				const aiColor4D * pVrtexColor = pMesh->mColors[v];
				writeColor4D((pVrtexColor->r), (pVrtexColor->g), (pVrtexColor->b), (pVrtexColor->a), outputStream);
			}

			//TODO: tangents, bitangents		
		}

		//index array
		assert(pMesh->mNumFaces < INT_MAX);
		///////////////////////////////assert(pMesh->mNumFaces * 3 == numVertices); //only valid for Collada. But not valid for FBX.
		const signed int vaoSIZE = pMesh->mNumFaces * 3;
		outputStream.write((char*)&(vaoSIZE), sizeof(signed int)); //size if the VAO
		for (signed int vao = 0; vao < static_cast<signed int>(pMesh->mNumFaces); vao++)
		{
			assert(pMesh->mFaces[vao].mNumIndices < INT_MAX);
			for (signed int faceID = 0; faceID < static_cast<signed int>(pMesh->mFaces[vao].mNumIndices); faceID++)
			{
				assert(pMesh->mFaces[vao].mIndices[faceID] < INT_MAX);
				const signed int faceVertexIndex = static_cast<signed int>(pMesh->mFaces[vao].mIndices[faceID]);
				outputStream.write((char*)(&faceVertexIndex), sizeof(signed int)); //VAO element
			}
		}
	}


	template<typename TReal>
	void CDAEParser::ApplyTransform(const aiMesh* mesh, const aiMatrix4x4t<TReal>* pInputMatrix)
	{
		// Check whether we need to transform the coordinates at all
		if (!pInputMatrix->IsIdentity()) {
			assert(mesh->HasPositions());
			
				for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
					mesh->mVertices[i] = (*pInputMatrix) * mesh->mVertices[i];
				}
			
			if (mesh->HasNormals() || mesh->HasTangentsAndBitangents()) {
				aiMatrix4x4 mWorldIT = *pInputMatrix;
				mWorldIT.Inverse().Transpose();

				// TODO: implement Inverse() for aiMatrix3x3
				aiMatrix3x3 m = aiMatrix3x3(mWorldIT);

				if (mesh->HasNormals()) {
					for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
						mesh->mNormals[i] = (m * mesh->mNormals[i]).Normalize();
					}
				}
				if (mesh->HasTangentsAndBitangents()) {
					for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
						mesh->mTangents[i] = (m * mesh->mTangents[i]).Normalize();
						mesh->mBitangents[i] = (m * mesh->mBitangents[i]).Normalize();
					}
				}
			}
		}
	}

	/// <summary>
	/// write a component of this object`s root into the output stream
	/// </summary>
	/// <param name="pComponent"></param>
	/// <param name="outputStream"></param>
	void CDAEParser::writeChildComponent(const aiNode* pComponent, const aiScene* pScene, std::ofstream& outputStream)
	{
		cout << "processing component: " << pComponent->mName.C_Str() << " with " << pComponent->mNumMeshes << " meshes" << endl;
		assert(pComponent->mNumChildren == 0);

		//component name
		this->writeString(&(pComponent->mName), outputStream);
		//number of meshes
		assert(pComponent->mNumMeshes < INT_MAX);
		signed int numMeshes = static_cast<signed int>(pComponent->mNumMeshes);
		outputStream.write((char*)(&numMeshes), sizeof(signed int));
		for (int m = 0; m < numMeshes; m++)
		{
			const aiMesh * pMesh = pScene->mMeshes[ pComponent->mMeshes[m] ];
			//
			//You need to use the instance of aiNode where the mesh is referencedand perform the whole transformation chain from your node up to the root node.The you can extract the position out of it.So in short:
			//Check if your node has any parent node.
			//	If yes, get the transformation node from the parent, get its transformationand multiply this with your child transformation
			//	Repeat this until you have reached the root node(parent pointer will be nullptr to find it)
			//	With this global transformation you have the information where your mesh is located in the World - Space.
			// see: https://stackoverflow.com/questions/68751664/how-to-get-the-position-of-the-imported-mesh-from-assimp

			writeMesh<ai_real>(pMesh, pScene, &(pComponent->mTransformation), outputStream);
		}

	}

	/// <summary>
	/// write a color into the output stream
	/// </summary>
	/// <param name="r"></param>
	/// <param name="g"></param>
	/// <param name="b"></param>
	/// <param name="outputStream"></param>
	void CDAEParser::writeColor3D(const float r, const float g, const float b, ofstream& outputStream)
	{
		const char* pTmp = (char*)(&r);
		assert(sizeof(r) == sizeof(float));
		outputStream.write(pTmp, sizeof(float));

		pTmp = (char*)(&g);
		outputStream.write(pTmp, sizeof(float));
		pTmp = (char*)(&b);
		outputStream.write(pTmp, sizeof(float));
	}

	/// <summary>
	/// write a color into the output stream
	/// </summary>
	/// <param name="r"></param>
	/// <param name="g"></param>
	/// <param name="b"></param>
	/// <param name="a"></param>
	/// <param name="outputStream"></param>
	void CDAEParser::writeColor4D(const float r, const float g, const float b, const float a, ofstream& outputStream)
	{
		const char* pTmp = (char*)(&r);
		assert(sizeof(r) == sizeof(float));
		outputStream.write(pTmp, sizeof(float));

		pTmp = (char*)(&g);
		outputStream.write(pTmp, sizeof(float));
		pTmp = (char*)(&b);
		outputStream.write(pTmp, sizeof(float));

		pTmp = (char*)(&a);
		outputStream.write(pTmp, sizeof(float));
	}

	/// <summary>
	/// write a string as follows:
	/// * string length: signed short (without the termination character, only the pure string length)
	/// * string itself: char array
	/// </summary>
	/// <param name="pString"></param>
	/// <param name="outputStream"></param>
	void CDAEParser::writeString(const aiString* pString, ofstream& outputStream)
	{
		//name length
		assert(pString->length < SHRT_MAX);
		const short strLength = (signed short)pString->length;
		outputStream.write((char*)&(strLength), sizeof(signed short));
		//material name
		const char* pStringChar = pString->C_Str();
		assert(strlen(pStringChar) == pString->length);
		outputStream.write(pStringChar, pString->length);
	}
}
