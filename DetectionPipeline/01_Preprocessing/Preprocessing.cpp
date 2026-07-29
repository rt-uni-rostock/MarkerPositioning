#include "Preprocessing.h"

#include "ImageSource/ImageFrame.h"

// Constructor: 
Preprocessing::Preprocessing(const DetectionPipelineConfig& config) : config_(config)
{

}

Preprocessing::~Preprocessing()
{
	// Destructor implementation
}

ImageFrame Preprocessing::process(const ImageFrame& frame)
{
	// Preprocessing implementation
	return frame;
}