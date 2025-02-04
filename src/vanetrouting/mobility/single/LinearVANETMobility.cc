//
// Author: Emin Ilker Cetinbas (niw3_at_yahoo_d0t_com)
// Copyright (C) 2005 Emin Ilker Cetinbas
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//


#include "LinearVANETMobility.h"
#include "FWMath.h"


Define_Module(LinearVANETMobility);


LinearVANETMobility::LinearVANETMobility()
{
    speed = 0;
    angle = 0;
    acceleration = 0;
    angularPosition = Coord::ZERO;
}

void LinearVANETMobility::initialize(int stage)
{
    MovingVANETMobilityBase::initialize(stage);

    EV_TRACE << "initializing LinearVANETMobility stage " << stage << endl;
    if (stage == 0)
    {
        speed = par("speed");
        angle = fmod((double)par("angle"), 360);
        acceleration = par("acceleration");
        stationary = (speed == 0) && (acceleration == 0.0);
    }
}

void LinearVANETMobility::move()
{
    double rad = PI * angle / 180;
    Coord direction(cos(rad), sin(rad));
    lastSpeed = direction * speed;
    double elapsedTime = (simTime() - lastUpdate).dbl();
    lastPosition += lastSpeed * elapsedTime;
    lastAngularPosition = direction;

    // do something if we reach the wall
    Coord dummy;
    handleIfOutside(REFLECT, dummy, dummy, angle);

    // accelerate
    speed += acceleration * elapsedTime;
    if (speed <= 0)
    {
        speed = 0;
        stationary = true;
    }
}
