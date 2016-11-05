/* Stream.cpp

   Copyright (C) 2015, 2016 Marc Postema (mpostema09 -at- gmail.com)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
   Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 */
#include <Stream.h>

#include <Log.h>
#include <StringConverter.h>
#include <Utils.h>
#include <input/dvb/Frontend.h>
#include <input/dvb/FrontendData.h>
#include <input/dvb/delivery/DVBS.h>
#include <output/StreamThreadHttp.h>
#include <output/StreamThreadRtp.h>
#include <output/StreamThreadRtpTcp.h>
#include <output/StreamThreadTSWriter.h>
#include <socket/SocketClient.h>

#include <stdio.h>
#include <stdlib.h>

static unsigned int seedp = 0xFEED;
const unsigned int Stream::MAX_CLIENTS = 8;

Stream::Stream(int streamID, input::SpDevice device, decrypt::dvbapi::SpClient decrypt) :
	_streamID(streamID),
	_streamingType(StreamingType::NONE),
	_enabled(true),
	_streamInUse(false),
	_client(new StreamClient[MAX_CLIENTS]),
	_streaming(nullptr),
	_decrypt(decrypt),
	_device(device),
	_ssrc((uint32_t)(rand_r(&seedp) % 0xffff)),
	_spc(0),
	_soc(0),
	_timestamp(0),
	_rtp_payload(0.0),
	_rtcpSignalUpdate(1) {
	ASSERT(device);
	for (std::size_t i = 0; i < MAX_CLIENTS; ++i) {
		_client[i].setStreamIDandClientID(streamID, i);
	}
}

Stream::~Stream() {
	DELETE_ARRAY(_client);
}

// ===========================================================================
// -- StreamInterface --------------------------------------------------------
// ===========================================================================

int Stream::getStreamID() const {
	return _streamID;
}

StreamClient &Stream::getStreamClient(std::size_t clientNr) const {
	return _client[clientNr];
}

input::SpDevice Stream::getInputDevice() const {
	return _device;
}

uint32_t Stream::getSSRC() const {
	return _ssrc;
}

long Stream::getTimestamp() const {
	return _timestamp;
}

uint32_t Stream::getSPC() const {
	return _spc;
}

unsigned int Stream::getRtcpSignalUpdateFrequency() const {
	return _rtcpSignalUpdate;
}

uint32_t Stream::getSOC() const {
	return _soc;
}

void Stream::addRtpData(uint32_t byte, long timestamp) {
	// inc RTP packet counter
	++_spc;
	_soc += byte;
	_rtp_payload = _rtp_payload + byte;
	_timestamp = timestamp;
}

double Stream::getRtpPayload() const {
	return _rtp_payload;
}

std::string Stream::attributeDescribeString(bool &active) const {
	active = _streamActive;
	return _device->attributeDescribeString();
}

// =======================================================================
//  -- base::XMLSupport --------------------------------------------------
// =======================================================================

void Stream::addToXML(std::string &xml) const {
	{
		base::MutexLock lock(_mutex);

		ADD_CONFIG_NUMBER(xml, "streamindex", _streamID);

		ADD_CONFIG_CHECKBOX(xml, "enable", (_enabled ? "true" : "false"));
		ADD_CONFIG_TEXT(xml, "attached", _streamInUse ? "yes" : "no");
		ADD_CONFIG_TEXT(xml, "owner", _client[0].getIPAddress().c_str());
		ADD_CONFIG_TEXT(xml, "ownerSessionID", _client[0].getSessionID().c_str());

		ADD_CONFIG_NUMBER_INPUT(xml, "rtcpSignalUpdate", _rtcpSignalUpdate, 0, 5);

		StringConverter::addFormattedString(xml, "<spc>%d</spc>", _spc.load());
		StringConverter::addFormattedString(xml, "<payload>%.3f</payload>", _rtp_payload.load() / (1024.0 * 1024.0));
	}
	_device->addToXML(xml);
}

void Stream::fromXML(const std::string &xml) {
	{
		base::MutexLock lock(_mutex);

		std::string element;
		if (findXMLElement(xml, "enable.value", element)) {
			_enabled = (element == "true") ? true : false;
		}
		if (findXMLElement(xml, "rtcpSignalUpdate.value", element)) {
			_rtcpSignalUpdate = atoi(element.c_str());
		}
	}
	_device->fromXML(xml);
}

// ===========================================================================
// -- Other member functions -------------------------------------------------
// ===========================================================================
#ifdef LIBDVBCSA
input::dvb::SpFrontendDecryptInterface Stream::getFrontendDecryptInterface() {
	return std::dynamic_pointer_cast<input::dvb::FrontendDecryptInterface>(_device);
}
#endif

bool Stream::findClientIDFor(SocketClient &socketClient,
                             bool newSession,
                             std::string sessionID,
                             const std::string &method,
                             int &clientID) {
	base::MutexLock lock(_mutex);

	const input::InputSystem msys = StringConverter::getMSYSParameter(socketClient.getMessage(), method);

	// Check if the input device is set, else this stream is not usable
	if (!_device) {
		SI_LOG_ERROR("Stream: %d, Input Device not set?...", _streamID);
		return false;
	}

	// Do we have a new session then check some things
	if (newSession) {
		if (!_enabled) {
			SI_LOG_INFO("Stream: %d, New session but this stream is not enabled, skipping...", _streamID);
			return false;
		} else if (_streamInUse) {
			SI_LOG_INFO("Stream: %d, New session but this stream is in use, skipping...", _streamID);
			return false;
		} else if (!_device->capableOf(msys)) {
			SI_LOG_INFO("Stream: %d, Not capable of handling msys=%s",
						_streamID, StringConverter::delsys_to_string(msys));
			return false;
		}
	}

	// if we have a session ID try to find it among our StreamClients
	for (std::size_t i = 0; i < MAX_CLIENTS; ++i) {
		// If we have a new session we like to find an empty slot so '-1'
		if (_client[i].getSessionID().compare(newSession ? "-1" : sessionID) == 0) {
			if (msys != input::InputSystem::UNDEFINED) {
				SI_LOG_INFO("Stream: %d, StreamClient[%d] with SessionID %s for %s",
				            _streamID, i, sessionID.c_str(), StringConverter::delsys_to_string(msys));
			} else {
				SI_LOG_INFO("Stream: %d, StreamClient[%d] with SessionID %s",
				            _streamID, i, sessionID.c_str());
			}
			_client[i].setSocketClient(socketClient);
			_client[i].setSessionID(sessionID);
			_streamInUse = true;
			clientID = i;
			return true;
		}
	}
	if (msys != input::InputSystem::UNDEFINED) {
		SI_LOG_INFO("Stream: %d, No StreamClient with SessionID %s for %s",
		            _streamID, sessionID.c_str(), StringConverter::delsys_to_string(msys));
	} else {
		SI_LOG_INFO("Stream: %d, No StreamClient with SessionID %s",
		            _streamID, sessionID.c_str());
	}
	return false;
}

void Stream::setSocketClient(SocketClient &socketClient) {
	base::MutexLock lock(_mutex);

	_client[0].setSocketClient(socketClient);
}

void Stream::checkForSessionTimeout() {
	base::MutexLock lock(_mutex);

	for (std::size_t i = 0; i < MAX_CLIENTS; ++i) {
		if (_client[i].sessionTimeout()) {
			SI_LOG_INFO("Stream: %d, Watchdog kicked in for StreamClient[%d] with SessionID %s",
			            _streamID, i, _client[i].getSessionID().c_str());
			teardown(i, false);
		}
	}
}

bool Stream::update(int clientID, bool start) {
	base::MutexLock lock(_mutex);

	const bool changed = _device->hasDeviceDataChanged();

	// first time streaming?
	if (!_streaming && start) {
		switch (_streamingType) {
			case StreamingType::NONE:
				_streaming.reset(nullptr);
				SI_LOG_ERROR("Stream: %d, No streaming type found!!", _streamID);
				break;
			case StreamingType::HTTP:
				SI_LOG_DEBUG("Stream: %d, Found Streaming type: HTTP", _streamID);
				_streaming.reset(new output::StreamThreadHttp(*this, _decrypt));
				break;
			case StreamingType::RTSP:
				SI_LOG_DEBUG("Stream: %d, Found Streaming type: RTSP", _streamID);
				_streaming.reset(new output::StreamThreadRtp(*this, _decrypt));
				break;
			case StreamingType::RTP_TCP:
				SI_LOG_DEBUG("Stream: %d, Found Streaming type: RTP/TCP", _streamID);
				_streaming.reset(new output::StreamThreadRtpTcp(*this, _decrypt));
				break;
			case StreamingType::FILE:
				SI_LOG_DEBUG("Stream: %d, Found Streaming type: FILE", _streamID);
				_streaming.reset(new output::StreamThreadTSWriter(*this, _decrypt, "test.ts"));
				break;
			default:
				_streaming.reset(nullptr);
				SI_LOG_ERROR("Stream: %d, Unknown streaming type!", _streamID);
		};
		if (!_streaming) {
			return false;
		}
	}

	// Channel changed?.. stop/pause Stream
	if (_streaming && changed) {
		_streaming->pauseStreaming(clientID);
	}

	if (!_device->update()) {
		return false;
	}

	// start or restart streaming again
	if (_streaming) {
		if (!_streamActive) {
			_streamActive = _streaming->startStreaming();
		} else if (changed) {
			_streaming->restartStreaming(clientID);
		}
	}
	return true;
}

void Stream::close(int clientID) {
	base::MutexLock lock(_mutex);

	if (_client[clientID].sessionCanClose()) {
		SI_LOG_INFO("Stream: %d, Close StreamClient[%d] with SessionID %s",
		            _streamID, clientID, _client[clientID].getSessionID().c_str());

		processStopStream_L(clientID, false);
	}
}

bool Stream::teardown(int clientID, bool gracefull) {
	base::MutexLock lock(_mutex);

	SI_LOG_INFO("Stream: %d, Teardown StreamClient[%d] with SessionID %s",
	            _streamID, clientID, _client[clientID].getSessionID().c_str());

	processStopStream_L(clientID, gracefull);
	return true;
}

bool Stream::processStreamingRequest(const std::string &msg, int clientID, const std::string &method) {
	base::MutexLock lock(_mutex);

	if ((method == "OPTIONS" || method == "SETUP" ||
	     method == "PLAY"    || method == "GET") &&
	    StringConverter::hasTransportParameters(msg)) {

		_device->parseStreamString(msg, method);
	}

	// Get transport type from request, and maybe ports
	if (_streamingType == StreamingType::NONE) {
		if (method == "GET") {
			_streamingType = StreamingType::HTTP;
		} else {
			std::string transport;
			StringConverter::getHeaderFieldParameter(msg, "Transport:", transport);
			// First check 'RTP/AVP/TCP' then 'RTP/AVP'
			if (transport.find("RTP/AVP/TCP") != std::string::npos) {
				_streamingType = StreamingType::RTP_TCP;
				const int interleaved = StringConverter::getIntParameter(msg, "Transport:", "interleaved=");
				if (interleaved != -1) {
				}
			} else if (transport.find("RTP/AVP") != std::string::npos) {
				_streamingType = StreamingType::RTSP;
				const int port = StringConverter::getIntParameter(msg, "Transport:", "client_port=");
				if (port != -1) {
					_client[clientID].getRtpSocketAttr().setupSocketStructure(port, _client[clientID].getIPAddress().c_str());
					_client[clientID].getRtcpSocketAttr().setupSocketStructure(port + 1, _client[clientID].getIPAddress().c_str());
				}
			}
		}
	}

	std::string cseq("0");
	if (StringConverter::getHeaderFieldParameter(msg, "CSeq:", cseq)) {
		_client[clientID].setCSeq(atoi(cseq.c_str()));
	}

	bool sessionCanClose = false;
	if (method != "SETUP") {
		std::string sessionID;
		const bool foundSessionID = StringConverter::getHeaderFieldParameter(msg, "Session:", sessionID);
		const bool teardown = (method == "TEARDOWN");
		sessionCanClose = teardown || !foundSessionID;
	}
	_client[clientID].setSessionCanClose(sessionCanClose);

	_client[clientID].restartWatchDog();
	return true;
}

/// private NO Locking
void Stream::processStopStream_L(int clientID, bool gracefull) {

	// Stop streaming by deleting object
	_streaming.reset(nullptr);

	_device->teardown();

	// as last, else sessionID and IP is reset
	_client[clientID].teardown(gracefull);

	// @TODO Are all other StreamClients stopped??
	if (clientID == 0) {
		for (std::size_t i = 1; i < MAX_CLIENTS; ++i) {
			// Not gracefully teardown for other clients
			_client[i].teardown(false);
		}
		_streamActive = false;
		_streamInUse = false;
		_streamingType = StreamingType::NONE;
	}
}
