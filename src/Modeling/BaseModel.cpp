
/******************************************************************************\
                     This file is part of Folding Atomata,
          a program that displays 3D views of Folding@home proteins.

                      Copyright (c) 2013, Jesse Victors

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see http://www.gnu.org/licenses/

                For information regarding this software email:
                                Jesse Victors
                         jvictors@jessevictors.com
\******************************************************************************/

#include "BaseModel.hpp"
#include "Modeling/Shading/Program.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <iostream>


BaseModel::BaseModel(const std::shared_ptr<Mesh>& mesh) :
    mesh_(mesh), isVisible_(true)
{}



BaseModel::BaseModel(const std::shared_ptr<Mesh>& mesh,
                     const BufferList& optionalDBs) :
    BaseModel(mesh)
{
    optionalDBs_ = optionalDBs;

    /*
    std::cout << "Created a Model with { ";
    for (auto buffer : optionalDBs_)
        std::cout << typeid(*buffer).name() << " ";
    std::cout << "} OptionalDataBuffers." << std::endl;
    */
}



void BaseModel::saveAs(GLuint programHandle)
{
    std::cout << "Storing Model under Program " << programHandle << ": { ";

    mesh_->store(programHandle);
    /*std::cout << typeid(*mesh_).name() << " ";

    for (auto buffer : optionalDBs_)
    {
        buffer->store(programHandle);
        std::cout << typeid(*buffer).name() << " ";
    }
*/
    std::cout << "}" << std::endl;
    checkGlError();
}



// Objects that are not 'visible' will not be rendered
void BaseModel::setVisible(bool visible)
{
    isVisible_ = visible;
}



BufferList BaseModel::getOptionalDataBuffers()
{
    return optionalDBs_;
}



void BaseModel::render(GLuint programHandle)
{
    //static auto triangles = mesh_->getTriangles();

    if (isVisible_)
    {
        //enableDataBuffers();
        mesh_->enable();
        /*
        glUniformMatrix4fv(matrixUniform_, 1, GL_FALSE,
            glm::value_ptr(modelMatrix_)
        ); //necessary when multiple Models share a Program

        enableDataBuffers();
        mesh_->draw();
        //disableDataBuffers();*/

        //auto data = triangles.data();
        //std::cout << &data << std::endl;
        //std::cout << triangles.size() << std::endl;
        static auto ib = mesh_->indexBuffer_;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib->indexBuffer_);
        //if (triangles)

        static int size = (int)ib->indices_.size();
        //glDrawElements(GL_TRIANGLE_STRIP, size, GL_UNSIGNED_INT, 0);
        glDrawElementsInstancedEXT(GL_TRIANGLE_STRIP, size, GL_UNSIGNED_INT, 0, 100);
    }
}



void BaseModel::enableDataBuffers()
{
    mesh_->enable();

    for (auto buffer : optionalDBs_)
        buffer->enable();
}



void BaseModel::disableDataBuffers()
{
    mesh_->disable();

    for (auto buffer : optionalDBs_)
        buffer->disable();
}