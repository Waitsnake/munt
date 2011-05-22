/* Copyright (C) 2011 Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PortAudioDriver.h"

#include "../QSynth.h"

using namespace MT32Emu;

static const qint64 NANOS_PER_SECOND = 1000000000;

static const int FRAME_SIZE = 4; // Stereo, 16-bit
static const int FRAMES_PER_CALLBACK = 4096;

static bool paInitialised = false;

PortAudioDriver::PortAudioDriver(QSynth *useSynth, unsigned int useSampleRate) : synth(useSynth), sampleRate(useSampleRate), stream(NULL) {
	if (!paInitialised) {
		PaError err = Pa_Initialize();
		if (err != paNoError) {
			qDebug() << "Error initialising PortAudio";
			// FIXME: Do something drastic instead of continuing on happily
		}
		paInitialised = true;
	}
}

PortAudioDriver::~PortAudioDriver() {
	if (stream != NULL) {
		Pa_StopStream(stream);
		Pa_CloseStream(stream);
	}
}

SynthTimestamp PortAudioDriver::getPlayedAudioNanosPlusLatency() {
	if (stream == NULL) {
		qDebug() << "Stream NULL at getPlayedAudioNanosPlusLatency()";
		return 0;
	}
	return Pa_GetStreamTime(stream) * NANOS_PER_SECOND + latency;
}

int PortAudioDriver::paCallback(const void *inputBuffer, void *outputBuffer, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData) {
	Q_UNUSED(inputBuffer);
	Q_UNUSED(statusFlags);
	PortAudioDriver *driver = (PortAudioDriver *)userData;
	//qDebug() << statusFlags << timeInfo->outputBufferDacTime;
	SynthTimestamp timestamp = (timeInfo->outputBufferDacTime) * NANOS_PER_SECOND;
	unsigned int rendered = driver->synth->render((Bit16s *)outputBuffer, frameCount, timestamp, Pa_GetStreamInfo(driver->stream)->sampleRate);
	if (rendered < frameCount) {
		char *out = (char *)outputBuffer;
		// PortAudio requires that the buffer is filled no matter what
		memset(out + rendered * FRAME_SIZE, 0, (frameCount - rendered) * FRAME_SIZE);
	}
	return paContinue;
}

static void dumpPortAudioDevices() {
	PaHostApiIndex hostApiCount = Pa_GetHostApiCount();
	if (hostApiCount < 0) {
		qDebug() << "Pa_GetHostApiCount() returned error" << hostApiCount;
		return;
	}
	PaDeviceIndex deviceCount = Pa_GetDeviceCount();
	if (deviceCount < 0) {
		qDebug() << "Pa_GetDeviceCount() returned error" << deviceCount;
		deviceCount = 0;
	}
	for(PaHostApiIndex hostApiIndex = 0; hostApiIndex < hostApiCount; hostApiIndex++) {
		const PaHostApiInfo *hostApiInfo = Pa_GetHostApiInfo(hostApiIndex);
		if (hostApiInfo == NULL) {
			qDebug() << "Pa_GetHostApiInfo() returned NULL for" << hostApiIndex;
			continue;
		}
		qDebug() << "HostAPI: " << hostApiInfo->name;
		qDebug() << " type =" << hostApiInfo->type;
		qDebug() << " deviceCount =" << hostApiInfo->deviceCount;
		qDebug() << " defaultInputDevice =" << hostApiInfo->defaultInputDevice;
		qDebug() << " defaultOutputDevice =" << hostApiInfo->defaultOutputDevice;
		for(PaDeviceIndex deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {
			const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(deviceIndex);
			if (deviceInfo == NULL) {
				qDebug() << "Pa_GetDeviceInfo() returned NULL for" << deviceIndex;
				continue;
			}
			if (deviceInfo->hostApi != hostApiIndex)
				continue;
			qDebug() << " Device:" << deviceIndex << deviceInfo->name;
		}
	}
}

QList<QString> PortAudioDriver::getDeviceNames() {
	dumpPortAudioDevices();
	QList<QString> deviceNames;
	PaDeviceIndex deviceCount = Pa_GetDeviceCount();
	if (deviceCount < 0) {
		qDebug() << "Pa_GetDeviceCount() returned error" << deviceCount;
		deviceCount = 0;
	}
	for(PaDeviceIndex deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {
		const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(deviceIndex);
		if (deviceInfo == NULL) {
			qDebug() << "Pa_GetDeviceInfo() returned NULL for" << deviceIndex;
			continue;
		}
		qDebug() << " Device:" << deviceIndex << deviceInfo->name;
		deviceNames << deviceInfo->name;
	}
	return deviceNames;
}

bool PortAudioDriver::start(int deviceIndex) {
	if (stream != NULL) {
		return currentDeviceIndex == deviceIndex;
	}
	if (deviceIndex < 0) {
		deviceIndex = Pa_GetDefaultOutputDevice();
		if (deviceIndex == paNoDevice) {
			qDebug() << "No default output device found";
			return false;
		}
	}
	const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(deviceIndex);
	if (deviceInfo == NULL) {
		qDebug() << "Pa_GetDeviceInfo() returned NULL for" << deviceIndex;
		return false;
	}
	qDebug() << "Using PortAudio device:" << deviceIndex << deviceInfo->name;
	/*
	if (deviceInfo->maxOutputChannels < 2) {
		qDebug() << "Device does not support stereo; maxOutputChannels =" << deviceInfo->maxOutputChannels;
		return false;
	}
	*/
	PaStreamParameters outStreamParameters = {deviceIndex, 2, paInt16, deviceInfo->defaultHighOutputLatency, NULL};
	PaError err =  Pa_OpenStream(&stream, NULL, &outStreamParameters, sampleRate, FRAMES_PER_CALLBACK, paNoFlag, paCallback, this);
	if(err != paNoError) {
		qDebug() << "Pa_OpenStream() returned PaError" << err;
		return false;
	}
	err = Pa_StartStream(stream);
	if(err != paNoError) {
		qDebug() << "Pa_StartStream() returned PaError" << err;
		Pa_CloseStream(stream);
		stream = NULL;
		return false;
	}
	const PaStreamInfo *streamInfo = Pa_GetStreamInfo(stream);
	qDebug() << "Device Output latency (s):" << streamInfo->outputLatency;
	latency = NANOS_PER_SECOND * streamInfo->outputLatency + (NANOS_PER_SECOND * FRAMES_PER_CALLBACK / sampleRate);
	qDebug() << "Using latency (ns):" << latency;
	return true;
}

void PortAudioDriver::close() {
	if(stream == NULL)
		return;
	Pa_StopStream(stream);
	Pa_CloseStream(stream);
	stream = NULL;
}