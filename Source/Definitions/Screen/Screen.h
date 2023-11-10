/*
  ==============================================================================

    Object.h
    Created: 26 Sep 2020 10:02:32am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"
#include "ScreenOutput.h"
#include "Surface/SurfaceManager.h"

// #include "../Command/CommandSelectionManager.h"
class CommandSelectionManager;

class Screen :
    public BaseItem
{
public:
    Screen(var params = var());
    virtual ~Screen();

    String objectType;
    var objectData;

    IntParameter* screenNumber;
    ScreenOutput output;

    SurfaceManager surfaces;

    void onContainerParameterChangedInternal(Parameter* p);

    String getTypeString() const override { return objectType; }
    static Screen* create(var params) { return new Screen(params); }
};


