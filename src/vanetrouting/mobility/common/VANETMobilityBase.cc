/* -*- mode:c++ -*- ********************************************************
 * file:        VANETMobilityBase.cc
 *
 * author:      Daniel Willkomm, Andras Varga, Zoltan Bojthe
 *
 * copyright:   (C) 2004 Telecommunication Networks Group (TKN) at
 *              Technische Universitaet Berlin, Germany.
 *
 *              (C) 2005 Andras Varga
 *              (C) 2011 Zoltan Bojthe
 *
 *              This program is free software; you can redistribute it
 *              and/or modify it under the terms of the GNU General Public
 *              License as published by the Free Software Foundation; either
 *              version 2 of the License, or (at your option) any later
 *              version.
 *              For further information see file COPYING
 *              in the top level directory
 ***************************************************************************
 * part of:     framework implementation developed by tkn
 **************************************************************************/


#include "VANETMobilityBase.h"
#include "FWMath.h"

Coord VANETMobilityBase::mininumArea;
Coord VANETMobilityBase::maximumArea;
bool  VANETMobilityBase::areaInitalized=false;

Register_Abstract_Class(VANETMobilityBase);

static bool parseIntTo(const char *s, double& destValue)
{
    if (!s || !*s)
        return false;

    char *endptr;
    int value = strtol(s, &endptr, 10);

    if (*endptr)
        return false;

    destValue = value;
    return true;
}

static bool isFiniteNumber(double value)
{
    return value <= DBL_MAX && value >= -DBL_MAX;
}

VANETMobilityBase::VANETMobilityBase()
{
    visualRepresentation = NULL;
    constraintAreaMin = Coord::ZERO;
    constraintAreaMax = Coord::ZERO;
    lastPosition = Coord::ZERO;
}

/**
* Sets up background size by adding the following tags:
* "p=0,0;b=$playgroundSizeX,$playgroundSizeY"
*/
void VANETMobilityBase::updateDisplayString()
{
	cModule *playgroundMod = this->getParentModule();
	while (playgroundMod->getParentModule())
	    playgroundMod = playgroundMod->getParentModule();
     cDisplayString& d = playgroundMod->getDisplayString();

     if (isFiniteNumber(mininumArea.x)) d.setTagArg("bgp", 0, (long)mininumArea.x);
     if (isFiniteNumber(mininumArea.y)) d.setTagArg("bgp", 1, (long)mininumArea.y);
     if (isFiniteNumber(maximumArea.x)) d.setTagArg("bgb", 0, (long) maximumArea.x);
     if (isFiniteNumber(maximumArea.y)) d.setTagArg("bgb", 1, (long) maximumArea.y);
}

void VANETMobilityBase::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    EV_TRACE << "initializing VANETMobilityBase stage " << stage << endl;
    if (stage == 0)
    {
        constraintAreaMin.x = par("constraintAreaMinX");
        constraintAreaMin.y = par("constraintAreaMinY");
        constraintAreaMin.z = par("constraintAreaMinZ");
        constraintAreaMax.x = par("constraintAreaMaxX");
        constraintAreaMax.y = par("constraintAreaMaxY");
        constraintAreaMax.z = par("constraintAreaMaxZ");
        visualRepresentation = findVisualRepresentation();
        if (visualRepresentation) {
            const char *s = visualRepresentation->getDisplayString().getTagArg("p", 2);
            if (s && *s)
                error("The coordinates of '%s' are invalid. Please remove automatic arrangement"
                      " (3rd argument of 'p' tag) from '@display' attribute.", visualRepresentation->getFullPath().c_str());
        }
    }
    else if (stage == 1)
    {
        initializePosition();
        if (!isFiniteNumber(lastPosition.x) || !isFiniteNumber(lastPosition.y) || !isFiniteNumber(lastPosition.z))
            throw cRuntimeError("Mobility position is not a finite number after initialize (x=%g,y=%g,z=%g)", lastPosition.x, lastPosition.y, lastPosition.z);
        if (isOutside())
            throw cRuntimeError("Mobility position (x=%g,y=%g,z=%g) is outside the constraint area (%g,%g,%g - %g,%g,%g)",
                  lastPosition.x, lastPosition.y, lastPosition.z,
                  constraintAreaMin.x, constraintAreaMin.y, constraintAreaMin.z,
                  constraintAreaMax.x, constraintAreaMax.y, constraintAreaMax.z);
        EV << "initial position. x = " << lastPosition.x << " y = " << lastPosition.y << " z = " << lastPosition.z << endl;
        emitMobilityStateChangedSignal();
        updateVisualRepresentation();

        if (!areaInitalized)
        {
        	mininumArea=constraintAreaMin;
        	maximumArea = constraintAreaMax;
            updateDisplayString();
            areaInitalized=true;
        }
        else
        {

            bool change=false;
            if (isFiniteNumber(mininumArea.x) && mininumArea.x>constraintAreaMin.x)
            {
                mininumArea.x=constraintAreaMin.x;
                change=true;
            }
            if (isFiniteNumber(mininumArea.y) && mininumArea.y>constraintAreaMin.y)
            {
            	mininumArea.y=constraintAreaMin.y;
            	change=true;
            }
            if (isFiniteNumber(mininumArea.z) && mininumArea.z>constraintAreaMin.z)
            {
            	mininumArea.z=constraintAreaMin.z;
                change=true;
            }
            if (isFiniteNumber(maximumArea.x) && maximumArea.x<constraintAreaMax.x)
            {
            	maximumArea.x=constraintAreaMax.x;
                change=true;
            }
            if (isFiniteNumber(maximumArea.y) && maximumArea.y<constraintAreaMax.y)
            {
            	maximumArea.y=constraintAreaMax.y;
                change=true;
            }
            if (isFiniteNumber(maximumArea.z) && maximumArea.z<constraintAreaMax.z)
            {
            	maximumArea.z=constraintAreaMax.z;
                change=true;
            }
            if (change)
                updateDisplayString();
         //   vanetMobilityStateChangedSignal = registerSignal("vanetMobilityStateChanged");
        }

    }
}

void VANETMobilityBase::initializePosition()
{
    setInitialPosition();
    checkPosition();
    emitMobilityStateChangedSignal();
    updateVisualRepresentation();
}

void VANETMobilityBase::setInitialPosition()
{
    // reading the coordinates from omnetpp.ini makes predefined scenarios a lot easier
    bool filled = false;
    if (hasPar("initFromDisplayString") && par("initFromDisplayString").boolValue() && visualRepresentation)
    {
        filled = parseIntTo(visualRepresentation->getDisplayString().getTagArg("p", 0), lastPosition.x)
              && parseIntTo(visualRepresentation->getDisplayString().getTagArg("p", 1), lastPosition.y);
        if (filled)
            lastPosition.z = 0;

    }
    // not all mobility models have "initialX", "initialY" and "initialZ" parameters
    else if (hasPar("initialX") && hasPar("initialY") && hasPar("initialZ"))
    {
        lastPosition.x = par("initialX");
        lastPosition.y = par("initialY");
        lastPosition.z = par("initialZ");
        filled = true;
    }
    if (!filled)
        lastPosition = getRandomPosition();
}

void VANETMobilityBase::checkPosition()
{
    if (!isFiniteNumber(lastPosition.x) || !isFiniteNumber(lastPosition.y) || !isFiniteNumber(lastPosition.z))
        throw cRuntimeError("Mobility position is not a finite number after initialize (x=%g,y=%g,z=%g)", lastPosition.x, lastPosition.y, lastPosition.z);
    if (isOutside())
        throw cRuntimeError("Mobility position (x=%g,y=%g,z=%g) is outside the constraint area (%g,%g,%g - %g,%g,%g)",
              lastPosition.x, lastPosition.y, lastPosition.z,
              constraintAreaMin.x, constraintAreaMin.y, constraintAreaMin.z,
              constraintAreaMax.x, constraintAreaMax.y, constraintAreaMax.z);
}

void VANETMobilityBase::handleMessage(cMessage * message)
{
    if (message->isSelfMessage())
        handleSelfMessage(message);
    else
        throw cRuntimeError("Mobility modules can only receive self messages");
}

void VANETMobilityBase::updateVisualRepresentation()
{
    EV_INFO << "current position = " << lastPosition << endl;
    if (ev.isGUI() && visualRepresentation)
    {
        visualRepresentation->getDisplayString().setTagArg("p", 0, (long)lastPosition.x);
        visualRepresentation->getDisplayString().setTagArg("p", 1, (long)lastPosition.y);
    }
}

void VANETMobilityBase::emitMobilityStateChangedSignal()
{
    ev<<"##@##"<<getSignalName(vanetMobilityStateChangedSignal);
    emit(vanetMobilityStateChangedSignal, this);
}

Coord VANETMobilityBase::getRandomPosition()
{
    Coord p;
    p.x = uniform(constraintAreaMin.x, constraintAreaMax.x);
    p.y = uniform(constraintAreaMin.y, constraintAreaMax.y);
    p.z = uniform(constraintAreaMin.z, constraintAreaMax.z);
    return p;
}

bool VANETMobilityBase::isOutside()
{
    return lastPosition.x < constraintAreaMin.x || lastPosition.x > constraintAreaMax.x
        || lastPosition.y < constraintAreaMin.y || lastPosition.y > constraintAreaMax.y
        || lastPosition.z < constraintAreaMin.z || lastPosition.z > constraintAreaMax.z;
}

static int reflect(double min, double max, double &coordinate, double &speed)
{
    double size = max - min;
    double value = coordinate - min;
    int sign = 1 - FWMath::modulo(floor(value / size), 2) * 2;
    ASSERT(sign == 1 || sign == -1);
    coordinate = FWMath::modulo(sign * value, size) + min;
    speed = sign * speed;
    return sign;
}

void VANETMobilityBase::reflectIfOutside(Coord& targetPosition, Coord& speed, double& angle)
{
    int sign;
    double dummy;
    if (lastPosition.x < constraintAreaMin.x || constraintAreaMax.x < lastPosition.x) {
        sign = reflect(constraintAreaMin.x, constraintAreaMax.x, lastPosition.x, speed.x);
        reflect(constraintAreaMin.x, constraintAreaMax.x, targetPosition.x, dummy);
        angle = 90 + sign * (angle - 90);
    }
    if (lastPosition.y < constraintAreaMin.y || constraintAreaMax.y < lastPosition.y) {
        sign = reflect(constraintAreaMin.y, constraintAreaMax.y, lastPosition.y, speed.y);
        reflect(constraintAreaMin.y, constraintAreaMax.y, targetPosition.y, dummy);
        angle = sign * angle;
    }
    if (lastPosition.z < constraintAreaMin.z || constraintAreaMax.z < lastPosition.z) {
        sign = reflect(constraintAreaMin.z, constraintAreaMax.z, lastPosition.z, speed.z);
        reflect(constraintAreaMin.z, constraintAreaMax.z, targetPosition.z, dummy);
        // NOTE: angle is not affected
    }
}

static void wrap(double min, double max, double &coordinate)
{
    coordinate = FWMath::modulo(coordinate - min, max - min) + min;
}

void VANETMobilityBase::wrapIfOutside(Coord& targetPosition)
{
    if (lastPosition.x < constraintAreaMin.x || constraintAreaMax.x < lastPosition.x) {
        wrap(constraintAreaMin.x, constraintAreaMax.x, lastPosition.x);
        wrap(constraintAreaMin.x, constraintAreaMax.x, targetPosition.x);
    }
    if (lastPosition.y < constraintAreaMin.y || constraintAreaMax.y < lastPosition.y) {
        wrap(constraintAreaMin.y, constraintAreaMax.y, lastPosition.y);
        wrap(constraintAreaMin.y, constraintAreaMax.y, targetPosition.y);
    }
    if (lastPosition.z < constraintAreaMin.z || constraintAreaMax.z < lastPosition.z) {
        wrap(constraintAreaMin.z, constraintAreaMax.z, lastPosition.z);
        wrap(constraintAreaMin.z, constraintAreaMax.z, targetPosition.z);
    }
}

void VANETMobilityBase::placeRandomlyIfOutside(Coord& targetPosition)
{
    if (isOutside())
    {
        Coord newPosition = getRandomPosition();
        targetPosition += newPosition - lastPosition;
        lastPosition = newPosition;
    }
}

void VANETMobilityBase::raiseErrorIfOutside()
{
    if (isOutside())
    {
        throw cRuntimeError("Mobility moved outside the area %g,%g,%g - %g,%g,%g (x=%g,y=%g,z=%g)",
              constraintAreaMin.x, constraintAreaMin.y, constraintAreaMin.z,
              constraintAreaMax.x, constraintAreaMax.y, constraintAreaMax.z,
              lastPosition.x, lastPosition.y, lastPosition.z);
    }
}

void VANETMobilityBase::handleIfOutside(BorderPolicy policy, Coord& targetPosition, Coord& speed, double& angle)
{
    switch (policy)
    {
        case REFLECT:       reflectIfOutside(targetPosition, speed, angle); break;
        case WRAP:          wrapIfOutside(targetPosition); break;
        case PLACERANDOMLY: placeRandomlyIfOutside(targetPosition); break;
        case RAISEERROR:    raiseErrorIfOutside(); break;
        default:            throw cRuntimeError("Invalid outside policy=%d in module", policy, getFullPath().c_str());
    }
}
