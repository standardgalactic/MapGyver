/*
  ==============================================================================

	Object.cpp
	Created: 26 Sep 2020 10:02:32am
	Author:  bkupe

  ==============================================================================
*/

#include "Media/MediaIncludes.h"
#include "Engine/RMPEngine.h"
#include "Media.h"

Media::Media(const String& name, var params, bool hasCustomSize) :
	BaseItem(name),
	width(nullptr),
	height(nullptr),
	customTime(-1),
	mediaParams("Media Parameters"),
	alwaysRedraw(false),
	shouldRedraw(false),
	autoClearFrameBufferOnRender(true),
	timeAtLastRender(0),
	lastFPSTick(0),
	lastFPSIndex(0),
	customFPSTick(false),
	isEditing(false),
	mediaNotifier(5)
{
	setHasCustomColor(true);

	addChildControllableContainer(&mediaParams);

	if (hasCustomSize)
	{
		width = addIntParameter("Width", "Width of the media", 1920, 1, 10000);
		height = addIntParameter("Height", "Height of the media", 1080, 1, 10000);
	}

	currentFPS = addFloatParameter("current FPS", "", 0, 0, 60);
	currentFPS->isSavable = false;
	currentFPS->enabled = false;

	GlContextHolder::getInstance()->registerOpenGlRenderer(this, 1);
	saveAndLoadRecursiveData = true;

	itemDataType = "Media";
}

Media::~Media()
{
	if (GlContextHolder::getInstanceWithoutCreating() != nullptr) GlContextHolder::getInstance()->unregisterOpenGlRenderer(this);
}


void Media::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
	if (cc == &mediaParams)
	{
		shouldRedraw = true;
	}
}

void Media::newOpenGLContextCreated()
{
	initGLInternal();
}

void Media::renderOpenGL()
{
	if (isClearing) return;

	Point<int> size = getMediaSize();
	if (size.isOrigin()) return;
	if (frameBuffer.getWidth() != size.x || frameBuffer.getHeight() != size.y) initFrameBuffer();


	if (!enabled->boolValue()) return;
	if (!isBeingUsed()) return;

	const double frameTime = 1000.0 / RMPSettings::getInstance()->fpsLimit->intValue();
	double t = GlContextHolder::getInstance()->timeAtRender;
	if (t < timeAtLastRender + frameTime) return;

	//log delta
	//LOG("Delta: " << t - timeAtLastRender);
	timeAtLastRender = t;

	if (!frameBuffer.isValid()) return;

	if (shouldRedraw || alwaysRedraw)
	{
		preRenderGLInternal(); //allow for pre-rendering operations even if not being used or disabled

		if (autoClearFrameBufferOnRender)
		{
			frameBuffer.makeCurrentAndClear();
			Init2DViewport(frameBuffer.getWidth(), frameBuffer.getHeight());
			glColor4f(1, 1, 1, 1);
		}
		else
		{
			frameBuffer.makeCurrentRenderingTarget();
		}

		renderGLInternal();

		frameBuffer.releaseAsRenderingTarget();
		shouldRedraw = false;

		if (!customFPSTick) FPSTick();
	}
}

OpenGLFrameBuffer* Media::getFrameBuffer()
{
	return &frameBuffer;
}

GLint Media::getTextureID()
{
	return getFrameBuffer()->getTextureID();
}

void Media::registerTarget(MediaTarget* target)
{
	usedTargets.addIfNotAlreadyThere(target);
}

void Media::unregisterTarget(MediaTarget* target)
{
	usedTargets.removeAllInstancesOf(target);
}

//for sequence or other meta-media systems
void Media::setCustomTime(double time, bool seekMode)
{
	customTime = time;
	shouldRedraw = true;
	if (customTime >= 0 && seekMode) handleSeek(customTime);
}


void Media::handleEnter(double time)
{
	setCustomTime(time, true);
}

void Media::handleExit()
{
	setCustomTime(-1);
}

bool Media::isBeingUsed()
{
	if (usedTargets.size() == 0) return false;
	for (auto& u : usedTargets) if (u->isUsingMedia(this)) return true;
	return false;
}

void Media::setIsEditing(bool editing)
{
	if (isEditing == editing) return;
	isEditing = editing;
	if(!isClearing)	mediaNotifier.addMessage(new MediaEvent(MediaEvent::EDITING_CHANGED, this));
}

void Media::openGLContextClosing()
{
	closeGLInternal();
	frameBuffer.release();
}

void Media::initFrameBuffer()
{
	Point<int> size = getMediaSize();
	if (size.isOrigin()) return;
	if (frameBuffer.isValid()) frameBuffer.release();
	frameBuffer.initialise(GlContextHolder::getInstance()->context, size.x, size.y);
	shouldRedraw = true;
}

Point<int> Media::getMediaSize()
{
	if (width != nullptr && height != nullptr) return Point<int>(width->intValue(), height->intValue());
	return Point<int>(0, 0);
}


void Media::FPSTick()
{
	double currentTime = juce::Time::getMillisecondCounterHiRes();
	double elapsedMillis = currentTime - lastFPSTick;
	lastFPSTick = currentTime;
	lastFPSIndex = (lastFPSIndex + 1) % 10;
	lastFPSHistory[lastFPSIndex] = elapsedMillis;

	double averageElapsed = 0;
	for (int i = 0; i < 10; i++) {
		averageElapsed += lastFPSHistory[i];
	}
	averageElapsed /= 10;

	float fps = 1000.0 / averageElapsed;

	int max = ceil(fps / 10.0) * 10;

	// Calcul des FPS
	MessageManager::callAsync([this, max, fps]()
		{
			if (isClearing) return;
			if (Engine::mainEngine->isClearing) return;

			if ((float)currentFPS->maximumValue < max) {
				currentFPS->maximumValue = max;
			}
			currentFPS->setValue(fps);
		});
}



// ImageMedia

ImageMedia::ImageMedia(const String& name, var params) :
	Media(name, params)
{
}

ImageMedia::~ImageMedia()
{
}

void ImageMedia::preRenderGLInternal()
{
	GenericScopedLock lock(imageLock);
	imageFBO.makeCurrentAndClear();
	glBindTexture(GL_TEXTURE_2D, imageFBO.getTextureID());
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image.getWidth(), image.getHeight(), GL_BGRA, GL_UNSIGNED_BYTE, bitmapData->data);
	imageFBO.releaseAsRenderingTarget();
	glBindTexture(GL_TEXTURE_2D, 0);
}

void ImageMedia::renderGLInternal()
{
	glBindTexture(GL_TEXTURE_2D, imageFBO.getTextureID());
	Draw2DTexRectFlipped(0, 0, imageFBO.getWidth(), imageFBO.getHeight());
	glBindTexture(GL_TEXTURE_2D, 0);
}

void ImageMedia::initFrameBuffer()
{
	GenericScopedLock lock(imageLock);
	Media::initFrameBuffer();
	if (imageFBO.isValid()) imageFBO.release();
	imageFBO.initialise(GlContextHolder::getInstance()->context, frameBuffer.getWidth(), frameBuffer.getHeight());

}

void ImageMedia::initImage(int width, int height)
{
	initImage(Image(Image::ARGB, width, height, true));
}

void ImageMedia::initImage(const Image& newImage)
{
	GenericScopedLock lock(imageLock);

	shouldRedraw = true;

	if (!newImage.isValid())
	{
		image = Image();
		return;
	}

	if (newImage.getWidth() != image.getWidth() || newImage.getHeight() != image.getHeight())
	{
		image = newImage.convertedToFormat(Image::ARGB);
		graphics = std::make_shared<Graphics>(image);
		bitmapData = std::make_shared<Image::BitmapData>(image, Image::BitmapData::readWrite);
	}

	if (graphics != nullptr && newImage.isValid())
		graphics->drawImage(newImage, Rectangle<float>(0, 0, image.getWidth(), image.getHeight()));
}

Point<int> ImageMedia::getMediaSize()
{
	return Point<int>(image.getWidth(), image.getHeight());
}
