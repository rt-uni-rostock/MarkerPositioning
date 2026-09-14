#include "SupervisorFactory.h"

#include <stdexcept>

#include "ISupervisorMode.h"
#include "LiveSupervisorMode.h"
#include "LiveImagePassthroughSupervisorMode.h"
#include "DetectionPipeline.h"
#include "DetectionPipelineConfig.h"
#include "ImageSource/ImageSourceFactory.h"
#include "ImageSource/ImageSourceConfig.h"
#include "Sink.h"
#include "SinkConfig.h"
#include "ImageUdpPublisher.h"
#include "Logger.h"

namespace {
DetectionPipelineConfig buildPipelineConfig(const GeneralSettings& settings, const CameraSettings& camera) {
	DetectionPipelineConfig pipelineConfig;
	pipelineConfig.cameraId = camera.id;
	pipelineConfig.cx = camera.cx;
	pipelineConfig.cy = camera.cy;
	pipelineConfig.fx = camera.fx;
	pipelineConfig.fy = camera.fy;
	pipelineConfig.d1 = camera.d1;
	pipelineConfig.d2 = camera.d2;
	pipelineConfig.d3 = camera.d3;
	pipelineConfig.d4 = camera.d4;
	pipelineConfig.d5 = camera.d5;

	if (camera.id == 2 || camera.id == 3) {
		pipelineConfig.tagSize = 0.12;
		pipelineConfig.tagID = 17;
	}
	else if (camera.id == 4) {
		pipelineConfig.tagSize = 0.75;
		pipelineConfig.tagID = 11;
	}
	else {
		pipelineConfig.tagSize = settings.tagSize;
		pipelineConfig.tagID = settings.tagID;
	}

	pipelineConfig.quadDecimate = settings.quadDecimate;
	pipelineConfig.detectionType = (settings.tagType == TagType::AprilTag) ? DetectionType::AprilTag : DetectionType::ArUco;

	pipelineConfig.enableImageLogging = settings.enableImageLogging;
	pipelineConfig.imageOutputPath = settings.imageOutputPath;
	pipelineConfig.saveRawFrames = settings.imageLogOptions.saveRawFrames;
	pipelineConfig.saveGrayFrames = settings.imageLogOptions.saveGrayFrames;
	pipelineConfig.saveDetectionResults = settings.imageLogOptions.saveDetectionResults;
	pipelineConfig.visualizeAllDetections = settings.imageLogOptions.visualizeAllDetections;

	return pipelineConfig;
}

SinkConfig buildSinkConfig(const GeneralSettings& settings) {
	SinkConfig sinkConfig;
	sinkConfig.udpEnabled = true;
	sinkConfig.loggingEnabled = true;
	sinkConfig.udpAddress = settings.udpIp;
	sinkConfig.udpPort = settings.udpPort;
	sinkConfig.sqliteFilePath = "results.db";
	sinkConfig.batchSize = 10;
	sinkConfig.batchTimeoutMs = 500;
	return sinkConfig;
}
}

SupervisorFactory::SupervisorFactory(const GeneralSettings& settings)
	: settings_(settings) {
}

SupervisorFactory::~SupervisorFactory() = default;

std::unique_ptr<ISupervisorMode> SupervisorFactory::create() {
	switch (settings_.supervisorMode) {
	case SupervisorMode::LiveDetection:
		initializeLiveDetectionDependencies();
		return std::make_unique<LiveSupervisorMode>(imgSourcePtrs_, pipelinePtrs_, *sink_, settings_);
	case SupervisorMode::StaticDetection:
		throw std::runtime_error("StaticSupervisorMode is not implemented yet.");
	case SupervisorMode::LiveImagePassthrough:
		initializeLiveImagePassthroughDependencies();
		return std::make_unique<LiveImagePassthroughSupervisorMode>(imgSourcePtrs_, activeCameraIds_, *imageUdpPublisher_, settings_);
	default:
		throw std::runtime_error("Unsupported supervisor mode.");
	}
}

void SupervisorFactory::initializeLiveDetectionDependencies() {
	imgSources_.clear();
	imgSourcePtrs_.clear();
	activeCameraIds_.clear();
	pipelines_.clear();
	pipelinePtrs_.clear();
	sink_.reset();
	imageUdpPublisher_.reset();

	const auto activeCameras = collectActiveCameras();
	if (activeCameras.empty()) {
		throw std::runtime_error("No active cameras configured for LiveDetection supervisor mode.");
	}

	for (const auto* cam : activeCameras) {
		ImageSourceConfig sourceConfig;
		sourceConfig.mode = settings_.sourceMode;
		sourceConfig.cameraSettings = cam;
		sourceConfig.maxCaptureFPS = settings_.frameRate * 3;

		ImageSourceFactory sourceFactory(sourceConfig);
		auto imgSource = sourceFactory.create();
		imgSourcePtrs_.push_back(imgSource.get());
		activeCameraIds_.push_back(cam->id);
		imgSources_.push_back(std::move(imgSource));

		DetectionPipelineConfig pipelineConfig = buildPipelineConfig(settings_, *cam);

		auto pipeline1 = std::make_unique<DetectionPipeline>(pipelineConfig);
		auto pipeline2 = std::make_unique<DetectionPipeline>(std::move(pipelineConfig));

		pipelinePtrs_.push_back(pipeline1.get());
		pipelinePtrs_.push_back(pipeline2.get());
		pipelines_.push_back(std::move(pipeline1));
		pipelines_.push_back(std::move(pipeline2));
	}

	sink_ = std::make_unique<Sink>(buildSinkConfig(settings_));
}

void SupervisorFactory::initializeLiveImagePassthroughDependencies() {
	imgSources_.clear();
	imgSourcePtrs_.clear();
	activeCameraIds_.clear();
	pipelines_.clear();
	pipelinePtrs_.clear();
	sink_.reset();
	imageUdpPublisher_.reset();

	const auto activeCameras = collectActiveCameras();
	if (activeCameras.empty()) {
		throw std::runtime_error("No active cameras configured for LiveImagePassthrough supervisor mode.");
	}

	for (const auto* cam : activeCameras) {
		ImageSourceConfig sourceConfig;
		sourceConfig.mode = settings_.sourceMode;
		sourceConfig.cameraSettings = cam;
		sourceConfig.maxCaptureFPS = settings_.frameRate * 3;

		ImageSourceFactory sourceFactory(sourceConfig);
		auto imgSource = sourceFactory.create();
		imgSourcePtrs_.push_back(imgSource.get());
		activeCameraIds_.push_back(cam->id);
		imgSources_.push_back(std::move(imgSource));
	}

	imageUdpPublisher_ = std::make_unique<ImageUdpPublisher>(
		settings_.udpIp,
		static_cast<uint16_t>(settings_.udpPort)
	);
}

std::vector<const CameraSettings*> SupervisorFactory::collectActiveCameras() const {
	std::vector<const CameraSettings*> activeCameras;
	for (const auto& camera : settings_.cameras) {
		if (camera.active) {
			activeCameras.push_back(&camera);
		}
	}

	LOG_TRACE("SupervisorFactory collected {} active cameras.", activeCameras.size());
	return activeCameras;
}
