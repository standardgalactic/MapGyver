/*
  ==============================================================================

	ScreenOutput.h
	Created: 9 Nov 2023 8:51:23pm
	Author:  rosta

  ==============================================================================
*/

#pragma once

class Screen;
class Media;

class ScreenOutput :
	public InspectableContentComponent,
	public OpenGLSharedRenderer,
	public KeyListener,
	public Parameter::AsyncListener
{
public:
	ScreenOutput(Screen* parent);
	~ScreenOutput();

	Screen* screen;

	bool isLive;
	double timeAtRender;


	void paint(Graphics& g) override {}
	void update();


	void newOpenGLContextCreated() override;
	void renderOpenGL() override;
	void openGLContextClosing() override;

	void userTriedToCloseWindow() override;

	void newMessage(const Parameter::ParameterEvent& e) override;

	bool keyPressed(const KeyPress& key, Component* originatingComponent);
};


class ScreenOutputWatcher :
	public ScreenManager::AsyncListener,
	public ContainerAsyncListener,
	public EngineListener
{
public:
	juce_DeclareSingleton(ScreenOutputWatcher, false);
	
	ScreenOutputWatcher();
	~ScreenOutputWatcher();

	OwnedArray<ScreenOutput> outputs;

	void updateOutput(Screen* s, bool forceRemove = false);

	ScreenOutput* getOutputForScreen(Screen* s);

	//void itemAdded(Screen* item) override;
	//void itemsAdded(Array<Screen*> items) override;
	//void itemRemoved(Screen* item) override;
	//void itemsRemoved(Array<Screen*> items) override;

	void newMessage(const ScreenManager::ManagerEvent& e) override;
	void newMessage(const ContainerAsyncEvent& e) override;

	void startLoadFile() override;
	void endLoadFile() override;
};