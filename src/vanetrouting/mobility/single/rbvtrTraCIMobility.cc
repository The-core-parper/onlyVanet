//
// Copyright (C) 2006-2012 Christoph Sommer <christoph.sommer@uibk.ac.at>
//
// Documentation for these modules is at http://veins.car2x.org/
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//


#include <limits>
#include <iostream>
#include <sstream>

#include "FWMath.h"  // for M_PI
#include "mobility/single/rbvtrTraCIMobility.h"

Define_Module(rbvtrTraCIMobility);

namespace {
    const double MY_INFINITY = (std::numeric_limits<double>::has_infinity ? std::numeric_limits<double>::infinity() : std::numeric_limits<double>::max());

    /*double roadIdAsDouble(std::string road_id) {
        std::istringstream iss(road_id);
        double d;
        if (!(iss >> d)) return MY_INFINITY;
        return d;
    }*/
}

void rbvtrTraCIMobility::Statistics::initialize()
{
    firstRoadNumber = MY_INFINITY;
    startTime = simTime();
    totalTime = 0;
    stopTime = 0;
    minSpeed = MY_INFINITY;
    maxSpeed = -MY_INFINITY;
    totalDistance = 0;
}

void rbvtrTraCIMobility::Statistics::watch(cSimpleModule& )
{
    WATCH(totalTime);
    WATCH(minSpeed);
    WATCH(maxSpeed);
    WATCH(totalDistance);
}

void rbvtrTraCIMobility::Statistics::recordScalars(cSimpleModule& module)
{
    if (firstRoadNumber != MY_INFINITY) module.recordScalar("firstRoadNumber", firstRoadNumber);
    module.recordScalar("startTime", startTime);
    module.recordScalar("totalTime", totalTime);
    module.recordScalar("stopTime", stopTime);
    if (minSpeed != MY_INFINITY) module.recordScalar("minSpeed", minSpeed);
    if (maxSpeed != -MY_INFINITY) module.recordScalar("maxSpeed", maxSpeed);
    module.recordScalar("totalDistance", totalDistance);
}

void rbvtrTraCIMobility::initialize(int stage)
{
    //TODO why call the base::initialize() at the end?

    if (stage == 0)
    {
        accidentCount = par("accidentCount");

        currentPosXVec.setName("posx");
        currentPosYVec.setName("posy");
        currentSpeedVec.setName("speed");
        currentAccelerationVec.setName("acceleration");

        statistics.initialize();
        statistics.watch(*this);

        WATCH(road_id);
        WATCH(speed);
        WATCH(acceleration);
        WATCH(angle);
        WATCH(lastPosition.x);
        WATCH(lastPosition.y);

        startAccidentMsg = 0;
        stopAccidentMsg = 0;
        manager = 0;
        last_speed = -1;

        if (accidentCount > 0) {
            simtime_t accidentStart = par("accidentStart");
            startAccidentMsg = new cMessage("scheduledAccident");
            stopAccidentMsg = new cMessage("scheduledAccidentResolved");
            scheduleAt(simTime() + accidentStart, startAccidentMsg);
        }

        if (ev.isGUI()) updateDisplayString();
    }

    VANETMobilityBase::initialize(stage);
}

void rbvtrTraCIMobility::setInitialPosition() {
    ASSERT(isPreInitialized);
    isPreInitialized = false;
}

void rbvtrTraCIMobility::finish()
{
    statistics.stopTime = simTime();

    statistics.recordScalars(*this);

    cancelAndDelete(startAccidentMsg);
    cancelAndDelete(stopAccidentMsg);

    isPreInitialized = false;
}

void rbvtrTraCIMobility::handleSelfMessage(cMessage *msg)
{
    if (msg == startAccidentMsg) {
        commandSetSpeed(0);
        simtime_t accidentDuration = par("accidentDuration");
        scheduleAt(simTime() + accidentDuration, stopAccidentMsg);
        accidentCount--;
    }
    else if (msg == stopAccidentMsg) {
        commandSetSpeed(-1);
        if (accidentCount > 0) {
            simtime_t accidentInterval = par("accidentInterval");
            scheduleAt(simTime() + accidentInterval, startAccidentMsg);
        }
    }
}

void rbvtrTraCIMobility::preInitialize(std::string external_id, const Coord& position, std::string road_id, double speed, double acceleration, double angle)
{
    this->external_id = external_id;
    this->lastUpdate = 0;
    nextPos = position;
    lastPosition = position;
    this->road_id = road_id;
    this->speed = speed;
    this->acceleration = acceleration;
    this->angle = angle;

    isPreInitialized = true;
}

void rbvtrTraCIMobility::nextPosition(const Coord& position, std::string road_id, double speed, double acceleration, double angle, rbvtrTraCIScenarioManager::VehicleSignal signals)
{
    EV_DEBUG << "next position = " << position << " " << road_id << " " << speed << " "  << acceleration << " " << angle << std::endl;
    isPreInitialized = false;
    nextPos = position;
    this->road_id = road_id;
    this->speed = speed;
    this->angle = angle;
    this->acceleration = acceleration;
    this->signals = signals;
    move();
}

void rbvtrTraCIMobility::move()
{
    // ensure we're not called twice in one time step
    ASSERT(lastUpdate != simTime());

    // keep statistics (for current step)
    currentPosXVec.record(lastPosition.x);
    currentPosYVec.record(lastPosition.y);

    // keep statistics (relative to last step)
    if (statistics.startTime != simTime()) {
        simtime_t updateInterval = simTime() - this->lastUpdate;
        this->lastUpdate = simTime();

        double distance = lastPosition.distance(nextPos);
        statistics.totalDistance += distance;
        statistics.totalTime += updateInterval;
        if (speed != -1) {
            statistics.minSpeed = std::min(statistics.minSpeed, speed);
            statistics.maxSpeed = std::max(statistics.maxSpeed, speed);
            currentSpeedVec.record(speed);
            if (last_speed != -1) {
                acceleration = (speed - last_speed) / updateInterval;
                currentAccelerationVec.record(acceleration);
            }
            last_speed = speed;
        } else {
            last_speed = -1;
            speed = -1;
        }
    }

    lastPosition = nextPos;
    if (ev.isGUI()) updateDisplayString();
    fixIfHostGetsOutside();
    emitMobilityStateChangedSignal();
    updateVisualRepresentation();
}

void rbvtrTraCIMobility::updateDisplayString() {
    ASSERT(-M_PI <= angle);
    ASSERT(angle < M_PI);

    getParentModule()->getDisplayString().setTagArg("b", 2, "rect");
    getParentModule()->getDisplayString().setTagArg("b", 3, "red");
    getParentModule()->getDisplayString().setTagArg("b", 4, "red");
    getParentModule()->getDisplayString().setTagArg("b", 5, "0");

    if (angle < -M_PI + 0.5 * M_PI_4 * 1) {
        getParentModule()->getDisplayString().setTagArg("t", 0, "\u2190");
        getParentModule()->getDisplayString().setTagArg("b", 0, "4");
        getParentModule()->getDisplayString().setTagArg("b", 1, "2");
    }
    else if (angle < -M_PI + 0.5 * M_PI_4 * 3) {
        getParentModule()->getDisplayString().setTagArg("t", 0, "\u2199");
        getParentModule()->getDisplayString().setTagArg("b", 0, "3");
        getParentModule()->getDisplayString().setTagArg("b", 1, "3");
    }
    else if (angle < -M_PI + 0.5 * M_PI_4 * 5) {
        getParentModule()->getDisplayString().setTagArg("t", 0, "\u2193");
        getParentModule()->getDisplayString().setTagArg("b", 0, "2");
        getParentModule()->getDisplayString().setTagArg("b", 1, "4");
    }
    else if (angle < -M_PI + 0.5 * M_PI_4 * 7) {
        getParentModule()->getDisplayString().setTagArg("t", 0, "\u2198");
        getParentModule()->getDisplayString().setTagArg("b", 0, "3");
        getParentModule()->getDisplayString().setTagArg("b", 1, "3");
    }
    else if (angle < -M_PI + 0.5 * M_PI_4 * 9) {
        getParentModule()->getDisplayString().setTagArg("t", 0, "\u2192");
        getParentModule()->getDisplayString().setTagArg("b", 0, "4");
        getParentModule()->getDisplayString().setTagArg("b", 1, "2");
    }
    else if (angle < -M_PI + 0.5 * M_PI_4 * 11) {
        getParentModule()->getDisplayString().setTagArg("t", 0, "\u2197");
        getParentModule()->getDisplayString().setTagArg("b", 0, "3");
        getParentModule()->getDisplayString().setTagArg("b", 1, "3");
    }
    else if (angle < -M_PI + 0.5 * M_PI_4 * 13) {
        getParentModule()->getDisplayString().setTagArg("t", 0, "\u2191");
        getParentModule()->getDisplayString().setTagArg("b", 0, "2");
        getParentModule()->getDisplayString().setTagArg("b", 1, "4");
    }
    else if (angle < -M_PI + 0.5 * M_PI_4 * 15) {
        getParentModule()->getDisplayString().setTagArg("t", 0, "\u2196");
        getParentModule()->getDisplayString().setTagArg("b", 0, "3");
        getParentModule()->getDisplayString().setTagArg("b", 1, "3");
    }
    else {
        getParentModule()->getDisplayString().setTagArg("t", 0, "\u2190");
        getParentModule()->getDisplayString().setTagArg("b", 0, "4");
        getParentModule()->getDisplayString().setTagArg("b", 1, "2");
    }
}

void rbvtrTraCIMobility::fixIfHostGetsOutside()
{
    raiseErrorIfOutside();
}
