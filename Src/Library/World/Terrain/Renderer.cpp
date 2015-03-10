//==================================================================================================================|
// Created 2014.10.19 by Daniel L. Watkins
//
// Copyright (C) 2014-2015 Daniel L. Watkins
// This file is licensed under the MIT License.
//==================================================================================================================|

#include <Core/Image.h>
#include <World/Terrain/Renderer.h>
#include "Utility.h"

namespace t3d { namespace world { namespace terrain
{
	void Renderer::init(Data *terrainData)
	{
		initializeOpenGLFunctions();
		mTerrainData = terrainData;
		loadShaders();

		mProgram->bind();
		{
			glGenVertexArrays(1, &mVao);
			glBindVertexArray(mVao);
			{
				uploadTerrainData();

				glPatchParameteri(GL_PATCH_VERTICES, 4);
				loadTextures();
			}
			glBindVertexArray(0);
		}
		mProgram->release();

		mWaterRenderer.init(mTerrainData, mTerrainData->heightScale()*0.30f);
	}


	void Renderer::cleanup()
	{
		mTerrainData->cleanup();
		mWaterRenderer.cleanup();

		mProgram->removeAllShaders();
		glDeleteBuffers(2, mVbo);
		glDeleteTextures(1, &mTextures.heightMap);
		glDeleteTextures(1, &mTextures.indicies);
		glDeleteTextures(1, &mTextures.terrain);
		glDeleteVertexArrays(1, &mVao);
	}


	void Renderer::render(Vec3f cameraPos, const Mat4 &modelViewMatrix, const Mat4 &perspectiveMatrix)
	{
		mProgram->bind();
		{
			glUniformMatrix4fv(mUniforms.mvMatrix, 1, GL_FALSE, glm::value_ptr(modelViewMatrix));
			glUniformMatrix4fv(mUniforms.projMatrix, 1, GL_FALSE, glm::value_ptr(perspectiveMatrix));

			glBindVertexArray(mVao);
			{
				switch (mMode)
				{
					case Mode::Normal:
					{
						glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
						break;
					}

					case Mode::WireFrame:
					{
						glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
						break;
					}
				}

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, mTextures.heightMap);
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, mTextures.lightMap);
				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_BUFFER, mTextures.indicies);
				glActiveTexture(GL_TEXTURE3);
				glBindTexture(GL_TEXTURE_2D_ARRAY, mTextures.terrain);

				const int terrainSize = mTerrainData->heightMap().size();
				const int chunkSize = mTerrainData->chunkSize();
				const int chunksPerSide = terrainSize / chunkSize;

				glUniform3fv(mUniforms.cameraPos, 1, glm::value_ptr(cameraPos));
				mProgram->setUniformValue(mUniforms.lod, mLodFactor);
				mProgram->setUniformValue(mUniforms.ivd, mIvdFactor);

				glDrawArraysInstanced(GL_PATCHES, 0, 4, chunksPerSide*chunksPerSide);
			}

			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glBindVertexArray(0);
		}
		mProgram->release();

		mWaterRenderer.render(cameraPos, modelViewMatrix, perspectiveMatrix);
	}


	void Renderer::reloadShaders()
	{
		loadShaders();
		mWaterRenderer.reloadShaders();
	}


///// PRIVATE
	void Renderer::loadShader(const QString &filename, QOpenGLShader::ShaderType shaderType)
	{
		qDebug() << "Loading shader " << filename << "...";

		QOpenGLShader *shader = new QOpenGLShader(shaderType, mProgram.get());
		if (!shader->compileSourceFile(gDefaultPathShaders + "/terrain/" + filename))
			qDebug() << "Error compiling shader " << filename << " of type " << static_cast<int>(shaderType);
		
		if (!mProgram->addShader(shader))
			qDebug() << "Error adding shader " << filename << " of type " << static_cast<int>(shaderType);
	}


	void Renderer::loadShaders()
	{
		mProgram = std::make_unique<QOpenGLShaderProgram>();

		loadShader("terrain.vs.glsl", QOpenGLShader::Vertex);
		loadShader("terrain.tcs.glsl", QOpenGLShader::TessellationControl);
		loadShader("terrain.tes.glsl", QOpenGLShader::TessellationEvaluation);
		loadShader("terrain.fs.glsl", QOpenGLShader::Fragment);

		if (mProgram->link() == false)
			qFatal("Problem linking shaders");
		else
			qDebug() << "Initialized shaders";

		qDebug() << "We have " << mProgram->shaders().count() << " shaders attached";

		mProgram->bind();
		{
			#define ULOC(id) mUniforms.id = mProgram->uniformLocation(#id)
			ULOC(mvMatrix);
			ULOC(projMatrix);

			ULOC(terrainSize);
			ULOC(chunkSize);
			ULOC(lod);
			ULOC(ivd);
			ULOC(cameraPos);
			ULOC(heightScale);
			ULOC(spanSize);

			ULOC(spacing);
			ULOC(textureMapResolution);
			ULOC(heightMapSize);
			#undef ULOC

			mProgram->setUniformValue(mUniforms.terrainSize, mTerrainData->heightMap().size());
			mProgram->setUniformValue(mUniforms.heightScale, mTerrainData->heightScale());
			mProgram->setUniformValue(mUniforms.spanSize, mTerrainData->spanSize());
			mProgram->setUniformValue(mUniforms.chunkSize, mTerrainData->chunkSize());
			mProgram->setUniformValue(mUniforms.spacing, mTerrainData->spacing());
			mProgram->setUniformValue(mUniforms.textureMapResolution, mTerrainData->textureMapResolution());
			mProgram->setUniformValue(mUniforms.heightMapSize, mTerrainData->heightMap().size());
		}
		mProgram->release();
	}


	void Renderer::loadTextures()
	{
		glGenTextures(1, &mTextures.indicies);
		glBindTexture(GL_TEXTURE_BUFFER, mTextures.indicies);
		{
			GLuint buffer;
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_TEXTURE_BUFFER, buffer);
			{
				glBufferData(GL_TEXTURE_BUFFER, sizeof(GLubyte)*mTerrainData->textureIndicies().size(), &mTerrainData->textureIndicies()[0], GL_STATIC_DRAW);
				glTexBuffer(GL_TEXTURE_BUFFER, GL_R8UI, buffer);
			}
		}

		glGenTextures(1, &mTextures.terrain);
		{
			Image imageWater;
			imageWater.loadFromFile_PNG(gDefaultPathTextures + "dirt.png");

			Image imageSand;
			imageSand.loadFromFile_PNG(gDefaultPathTextures + "sand.png");

			Image imageGrass;
			imageGrass.loadFromFile_PNG(gDefaultPathTextures + "grass.png");

			Image imageMountain;
			imageMountain.loadFromFile_PNG(gDefaultPathTextures + "mountain.png");

			int imageSize = imageWater.getWidth();	//for now, assume all images are the same width and height

			glBindTexture(GL_TEXTURE_2D_ARRAY, mTextures.terrain);

			int mipLevels = 8;
			glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipLevels, GL_RGBA8, imageSize, imageSize, 4);

			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
							0, 0, 0,
							imageSize, imageSize, 1,
							GL_RGBA, GL_UNSIGNED_BYTE, &imageWater.getImageData()[0]);

			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
							0, 0, 1,
							imageSize, imageSize, 1,
							GL_RGBA, GL_UNSIGNED_BYTE, &imageSand.getImageData()[0]);

			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
							0, 0, 2,
							imageSize, imageSize, 1,
							GL_RGBA, GL_UNSIGNED_BYTE, &imageGrass.getImageData()[0]);

			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
							0, 0, 3,
							imageSize, imageSize, 1,
							GL_RGBA, GL_UNSIGNED_BYTE, &imageMountain.getImageData()[0]);

			glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
			glSamplerParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glSamplerParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		}
	}


	void Renderer::uploadTerrainData()
	{
		//height map
		{
			glGenTextures(1, &mTextures.heightMap);

			glBindTexture(GL_TEXTURE_2D, mTextures.heightMap);

			HeightMap &hm = mTerrainData->heightMap();
			glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, hm.size(), hm.size());
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, hm.size(), hm.size(), GL_RED, GL_FLOAT, hm.raw());

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}


		//light map
		{
			glGenTextures(1, &mTextures.lightMap);
			glBindTexture(GL_TEXTURE_2D, mTextures.lightMap);

			LightMap &lm = mTerrainData->lightMap();
			glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, lm.size(), lm.size());
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, lm.size(), lm.size(), GL_RED, GL_UNSIGNED_SHORT, lm.raw());

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
	}
}}}