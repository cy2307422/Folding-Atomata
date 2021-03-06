
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

#define _GLIBCXX_USE_NANOSLEEP

#include "Viewer.hpp"
#include "FAHClientIO.hpp"
#include "Sockets/SocketException.hpp"
#include "PyON/TrajectoryParser.hpp"
#include "Modeling/DataBuffers/SampledBuffers/Image.hpp"
#include "Modeling/DataBuffers/SampledBuffers/TexturedCube.hpp"
#include "Options.hpp"
#include <thread>
#include <algorithm>
#include <iostream>

/*
    1) return a vector of all trajectories for all slots
    2) display only slot 0
    3) display only snapshot 0
        draw atoms
        draw bonds
    4) then draw multiple snapshots (0, 1, 2, 3, ..., 0, 1, 2, 3, ...)

*/


Viewer::Viewer() :
    scene_(std::make_shared<Scene>(createCamera())),
    user_(std::make_shared<User>(scene_)),
    timeSpentRendering_(0), frameCount_(0), needsRerendering_(true)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    addModels();
    user_->grabPointer();
    reportFPS();
}



void Viewer::reportFPS()
{
    std::thread fpsReporter([&]()
    {
        std::cout << "Note: FPS will be low when the camera is still." << std::endl;
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(2));

            glm::vec3 cameraPos = scene_->getCamera()->getPosition();
            std::cout << frameCount_ / 2 << " FPS, spent " <<
                timeSpentRendering_ / 2 << " ms rendering. <" << cameraPos.x <<
                ", " << cameraPos.y << ", " << cameraPos.z << ">" << std::endl;

            frameCount_ = 0;
            timeSpentRendering_ = 0;
        }
    });

    fpsReporter.detach();
}



void Viewer::addModels()
{
    auto boundingBoxes = addSlotViewers();
    addBoundingBoxOutlines(boundingBoxes);

    if (!Options::getInstance().skyboxDisabled())
        addSkybox();
}



void Viewer::addSkybox()
{
    std::cout << "Creating skybox..." << std::endl;

    auto image = std::make_shared<Image>(
        Options::getInstance().getSkyboxPath());

    BufferList list = { std::make_shared<TexturedCube>(image, image,
        image, image, image, image) };
    auto matrix = glm::scale(glm::mat4(), glm::vec3(4096));
    auto model = std::make_shared<InstancedModel>(getSkyboxMesh(), matrix, list);
    scene_->addModel(model); //add to Scene and save

    std::cout << "... done creating skybox." << std::endl;
}



std::vector<BoundingBoxPtr> Viewer::addSlotViewers()
{
    typedef glm::vec3 v3;
    const std::vector<std::vector<v3>> OFFSET_UNIT_VECTORS{
        { v3( 0, 0, 0) },
        { v3( 0, 1, 0), v3( 0, -1, 0) },
        { v3( 1, 0, 0), v3( 0,  1, 0), v3(0,  0,  1) },
        { v3(-1, 1, 0), v3(-1, -1, 0), v3(1,  0, -1), v3( 1,  0, 1) },
        { v3(-1, 1, 0), v3( 1,  1, 0), v3(1, -1,  0), v3(-1, -1, 0), v3(0, 0, 1) }
    };

    std::vector<TrajectoryPtr> trajectories = getTrajectories();
    if (Options::getInstance().showOneSlot())
    {
        slotViewers_.push_back(std::make_shared<SlotViewer>(trajectories[0],
            OFFSET_UNIT_VECTORS[0][0], scene_));
        return { trajectories[0]->calculateBoundingBox() };
    }

    //fill this vector with the bounding boxes of each trajectory
    std::vector<BoundingBoxPtr> boundingBoxes;
    boundingBoxes.resize(trajectories.size());
    std::transform(trajectories.begin(), trajectories.end(),
        boundingBoxes.begin(), [&](const TrajectoryPtr& trajectory)
        {
            return trajectory->calculateBoundingBox();
        }
    );

    //these vectors will be expanded so that no two bounding boxes overlap
    std::cout << "Separating bounding boxes... ";
    std::vector<BoundingBoxPtr> resizedBoxes(boundingBoxes);
    auto offsetVectors = OFFSET_UNIT_VECTORS[trajectories.size() - 1];
    bool boundingBoxesOverlap;
    do
    {
        //determines whether or not any of the bounding boxes overlap
        boundingBoxesOverlap = false;
        for (std::size_t j = 0; j < resizedBoxes.size(); j++)
            for (std::size_t k = j + 1; k < resizedBoxes.size(); k++)
                if (resizedBoxes[j]->intersectsWith(resizedBoxes[k]))
                    boundingBoxesOverlap = true;

        //expand vectors if needed
        if (boundingBoxesOverlap)
        {
            for (std::size_t j = 0; j < trajectories.size(); j++)
            {
                *resizedBoxes[j] = *boundingBoxes[j] + offsetVectors[j];
                offsetVectors[j] *= 2;
            }
        }
    } while (boundingBoxesOverlap);
    std::cout << "done." << std::endl;

    for (std::size_t j = 0; j < 5 && j < trajectories.size(); j++)
        slotViewers_.push_back(std::make_shared<SlotViewer>(trajectories[j],
            offsetVectors[j], scene_));

    return resizedBoxes;
}



void Viewer::addBoundingBoxOutlines(const std::vector<BoundingBoxPtr>& boxes)
{
    std::cout << "Adding bounding box outlines..." << std::endl;

    std::vector<glm::mat4> matrices;
    matrices.resize(boxes.size());
    std::transform(boxes.begin(), boxes.end(),
        matrices.begin(), [&](const BoundingBoxPtr& boundingBox)
        {
            auto matrix = glm::translate(glm::mat4(), boundingBox->getMinimum());
            return glm::scale(matrix, boundingBox->getSizes());
        }
    );

    auto mesh = getBoundingBoxMesh();
    BufferList list = {std::make_shared<ColorBuffer>(glm::vec3(0, 0.1f, 0), 8)};
    scene_->addModel(std::make_shared<InstancedModel>(mesh, matrices, list));
    std::cout << "... done adding bounding box outlines." << std::endl;
}



std::vector<TrajectoryPtr> Viewer::getTrajectories()
{
    std::vector<TrajectoryPtr> trajectories;

    try
    {
        auto socket = std::make_shared<ClientSocket>(
            Options::getInstance().getHost(),
            Options::getInstance().getPort()
        );

        FAHClientIO io(socket);
        trajectories = io.getTrajectories();

        if (trajectories.empty())
            std::cerr << "Not enough slots to work with. " <<
                "Using demo protein." << std::endl;
    }
    catch (SocketException& se)
    {
        std::cerr << "Error connection to FAHClient (" << se.description() <<
            "). Using demo protein." << std::endl;
    }

    if (trajectories.empty())
    {
        const std::string FILENAME = "/usr/share/FoldingAtomata/demoProtein";
        std::ifstream fin(FILENAME, std::ios::in | std::ios::binary);
        if (!fin.is_open())
            throw std::runtime_error("Unable to demo protein!");

        std::string proteinStr;
        fin.seekg(0, std::ios::end);
        proteinStr.resize((unsigned long)fin.tellg()); //allocate enough space
        fin.seekg(0, std::ios::beg);
        fin.read(&proteinStr[0], (long)proteinStr.size()); //read entire file
        fin.close();

        TrajectoryParser parser(proteinStr, false);
        trajectories.push_back(parser.parse());
    }

    return trajectories;
}



std::shared_ptr<Mesh> Viewer::getSkyboxMesh()
{
    static std::shared_ptr<Mesh> mesh = nullptr;

    if (mesh)
        return mesh;

    const std::vector<glm::vec3> VERTICES = {
        glm::vec3(-1, -1, -1),
        glm::vec3(-1, -1,  1),
        glm::vec3(-1,  1, -1),
        glm::vec3(-1,  1,  1),
        glm::vec3( 1, -1, -1),
        glm::vec3( 1, -1,  1),
        glm::vec3( 1,  1, -1),
        glm::vec3( 1,  1,  1)
    };

    //visible from the inside only, so faces in
    const std::vector<GLuint> INDICES = {
        0, 1, 5, 4, //front
        6, 7, 3, 2, //back
        2, 0, 4, 6, //top
        7, 5, 1, 3, //bottom
        2, 3, 1, 0, //left
        4, 5, 7, 6  //right
    };

    auto vBuffer = std::make_shared<VertexBuffer>(VERTICES);
    auto iBuffer = std::make_shared<IndexBuffer>(INDICES, GL_QUADS);
    mesh = std::make_shared<Mesh>(vBuffer, iBuffer, GL_QUADS);
    return mesh;
}



std::shared_ptr<Mesh> Viewer::getBoundingBoxMesh()
{
    static std::shared_ptr<Mesh> mesh = nullptr;

    if (mesh)
        return mesh;

    const std::vector<glm::vec3> VERTICES = {
        glm::vec3(0, 0, 0),
        glm::vec3(0, 0, 1),
        glm::vec3(0, 1, 1),
        glm::vec3(0, 1, 0),
        glm::vec3(1, 0, 0),
        glm::vec3(1, 0, 1),
        glm::vec3(1, 1, 1),
        glm::vec3(1, 1, 0)
    };

    //visible from the inside only, so faces in
    const std::vector<GLuint> INDICES = {
        0, 1,       1, 2,       2, 3,       3, 0,   //bottom
        4, 5,       5, 6,       6, 7,       7, 4,   //top
        0, 4,       1, 5,       2, 6,       3, 7    //connecting the faces
    };

    auto vBuffer = std::make_shared<VertexBuffer>(VERTICES);
    auto iBuffer = std::make_shared<IndexBuffer>(INDICES, GL_LINES);
    mesh = std::make_shared<Mesh>(vBuffer, iBuffer, GL_LINES);
    return mesh;
}



std::shared_ptr<Camera> Viewer::createCamera()
{
    auto camera = std::make_shared<Camera>();
    camera->setPosition(glm::vec3(0, -50, 0));

    camera->lookAt(
        glm::vec3(0, 0, 0),
        glm::vec3(0, 0, 1)
    );

    return camera;
}



void Viewer::update(int deltaTime)
{
    user_->update(deltaTime);
    //note: tests of whether or not the player is moving is done in render()
}



void Viewer::animate(int deltaTime)
{
    bool animationHappened = false;
    for (auto viewer : slotViewers_)
        if (viewer->animate(deltaTime)) //test if animation happened
            animationHappened = true;

    if (animationHappened)
        needsRerendering_ = true; //the atoms moved, so redraw the scene
}



void Viewer::render()
{
    if (!needsRerendering_ && !user_->isMoving())
        return;
    needsRerendering_ = false; //it was true, so reset it and then render

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    timeSpentRendering_ += scene_->render();
    frameCount_++;

    glutSwapBuffers();
}



void Viewer::handleWindowReshape(int newWidth, int newHeight)
{
    scene_->getCamera()->setAspectRatio(newWidth / (float)newHeight);
    user_->setWindowOffset(glutGet(GLUT_WINDOW_X), glutGet(GLUT_WINDOW_Y));
    needsRerendering_ = true; //need to redraw after window update

    std::cout << "Windows updated to " << newWidth << " by " << newHeight <<
        ", a ratio of " << (newWidth / (float)newHeight) << std::endl;
}



std::shared_ptr<User> Viewer::getUser()
{
    return user_;
}



Viewer* Viewer::singleton_ = 0;

Viewer& Viewer::getInstance()
{
    try
    {
        if (singleton_)
            return *singleton_;

        std::cout << "Creating Viewer..." << std::endl;
        singleton_ = new Viewer();
        std::cout << "... done creating Viewer." << std::endl;
    }
    catch (std::exception& e)
    {
        std::cerr << std::endl;
        std::cerr << "Caught " << typeid(e).name() <<
            " during Viewer initiation: " << e.what() << std::endl;
        glutDestroyWindow(glutGetWindow());
    }

    return *singleton_;
}
