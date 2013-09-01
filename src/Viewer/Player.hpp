
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

#ifndef PLAYER
#define PLAYER

#include "World/Scene.hpp"
#include <memory>
#include <chrono>

/**
    The Player class handles functions relating to user interaction and movement.
    Its main goal is to represent the user interacting with the Scene.
    For example, one of its basic jobs is to accept and handle mouse and keyboard
    actions and use them to move the Scene's Camera accordingly.
    the Scene's Camera.
**/
class Player
{
    public:
        const float ACCELERATION = 0.018f;
        const float KEY_PRESSED_TIMEOUT = 100; //ms
        const float GEOMETRIC_SPEED_DECAY = 0.96f;
        const float MAX_SPEED = 1.5f;

        const float PITCH_COEFFICIENT = 0.05f;
        const float YAW_COEFFICIENT = 0.05f;
        const float ROLL_SPEED = 1.8f;

        const int CENTER_X, CENTER_Y;
/*
        enum Action
        {
            MOVE_LEFT, MOVE_RIGHT,
            MOVE_FORWARD, MOVE_BACKWARD,
            MOVE_DOWN, MOVE_UP
        };*/

    public:
        Player(std::shared_ptr<Scene> scene);
        void update(int deltaTime);
        void grabPointer();
        void releasePointer();
        void recenterCursor();

        void onKeyPress(unsigned char key);
        void onSpecialKeyPress(int key);
        void onMouseClick(int button, int state, int x, int y);
        void onMouseMotion(int x, int y);
        void onMouseDrag(int x, int y);

    private:
        std::shared_ptr<Scene> scene_;
        bool mouseControlsCamera_;

        glm::vec3 movementDelta_;
        std::chrono::time_point<std::chrono::steady_clock> lastKeyPressed_;
};

#endif
