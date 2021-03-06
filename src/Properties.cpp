/* Properties.cpp

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
#include <Properties.h>
#include <Log.h>
#include <Utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

extern const char *satpi_version;

Properties::Properties(const std::string &xmlFilePath, const std::string &uuid,
                       const std::string &appdataPath, const std::string &webPath,
					   unsigned int httpPort, unsigned int rtspPort) :
	XMLSupport(xmlFilePath),
	_uuid(uuid),
	_versionString(satpi_version),
	_appdataPath(appdataPath),
	_webPath(webPath),
	_xSatipM3U("channellist.m3u"),
	_xmlDeviceDescriptionFile("desc.xml"),
	_bootID(1),
	_deviceID(1),
	_ssdpAnnounceTimeSec(60),
	_appStartTime(std::time(nullptr)),
	_exitApplication(false) {
	restoreXML();
	_httpPort = httpPort;
	_rtspPort = rtspPort;
}

Properties::~Properties() {
}

void Properties::fromXML(const std::string &xml) {
	base::MutexLock lock(_mutex);
	std::string element;
	if (findXMLElement(xml, "data.configdata.input1.value", element)) {
		_ssdpAnnounceTimeSec = atoi(element.c_str());
		SI_LOG_INFO("Setting SSDP annouce interval to: %d Sec", _ssdpAnnounceTimeSec);
	}
	if (findXMLElement(xml, "data.configdata.xsatipm3u.value", element)) {
		_xSatipM3U = element;
	}
	if (findXMLElement(xml, "data.configdata.xmldesc.value", element)) {
		_xmlDeviceDescriptionFile = element;
	}
	if (findXMLElement(xml, "data.configdata.httpport.value", element)) {
		_httpPort = atoi(element.c_str());
//		SI_LOG_INFO("Setting HTTP Port to: %d", _httpPort);
	}
	if (findXMLElement(xml, "data.configdata.rtspport.value", element)) {
		_rtspPort = atoi(element.c_str());
//		SI_LOG_INFO("Setting RTSP Port to: %d", _rtspPort);
	}
	saveXML(xml);
}

void Properties::addToXML(std::string &xml) const {
	base::MutexLock lock(_mutex);

	// make config xml
	xml  = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n";
	xml += "<data>\r\n";
	xml += "<configdata>\r\n";

	ADD_CONFIG_NUMBER_INPUT(xml, "httpport", _httpPort, 0, 65535);
	ADD_CONFIG_NUMBER_INPUT(xml, "rtspport", _rtspPort, 0, 65535);
	ADD_CONFIG_NUMBER_INPUT(xml, "input1", _ssdpAnnounceTimeSec, 0, 1800);
	ADD_CONFIG_TEXT_INPUT(xml, "xsatipm3u", _xSatipM3U.c_str());
	ADD_CONFIG_TEXT_INPUT(xml, "xmldesc", _xmlDeviceDescriptionFile.c_str());

	xml += "</configdata>\r\n";
	xml += "</data>\r\n";
}

std::string Properties::getSoftwareVersion() const {
	base::MutexLock lock(_mutex);
	return _versionString;
}

std::string Properties::getUUID() const {
	base::MutexLock lock(_mutex);
	return _uuid;
}

std::string Properties::getAppDataPath() const {
	base::MutexLock lock(_mutex);
	return _appdataPath;
}

std::string Properties::getWebPath() const {
	base::MutexLock lock(_mutex);
	return _webPath;
}

std::string Properties::getXSatipM3U() const {
	base::MutexLock lock(_mutex);
	return _xSatipM3U;
}

std::string Properties::getXMLDeviceDescriptionFile() const {
	base::MutexLock lock(_mutex);
	return _xmlDeviceDescriptionFile;
}

void Properties::setBootID(unsigned int bootID) {
	base::MutexLock lock(_mutex);
	_bootID = bootID;
}

unsigned int Properties::getBootID() const {
	base::MutexLock lock(_mutex);
	return _bootID;
}

void Properties::setDeviceID(unsigned int deviceID) {
	base::MutexLock lock(_mutex);
	_deviceID = deviceID;
}

unsigned int Properties::getDeviceID() const {
	base::MutexLock lock(_mutex);
	return _deviceID;
}

void Properties::setHttpPort(unsigned int httpPort) {
	base::MutexLock lock(_mutex);
	_httpPort = httpPort;
}

unsigned int Properties::getHttpPort() const {
	base::MutexLock lock(_mutex);
	return _httpPort;
}

void Properties::setRtspPort(unsigned int rtspPort) {
	base::MutexLock lock(_mutex);
	_rtspPort = rtspPort;
}

unsigned int Properties::getRtspPort() const {
	base::MutexLock lock(_mutex);
	return _rtspPort;
}

void Properties::setSsdpAnnounceTimeSec(unsigned int sec) {
	base::MutexLock lock(_mutex);
	_ssdpAnnounceTimeSec = sec;
}

unsigned int Properties::getSsdpAnnounceTimeSec() const {
	base::MutexLock lock(_mutex);
	return _ssdpAnnounceTimeSec;
}

std::time_t Properties::getApplicationStartTime() const {
	base::MutexLock lock(_mutex);
	return _appStartTime;
}

bool Properties::exitApplication() const {
	base::MutexLock lock(_mutex);
	return _exitApplication;
}

void Properties::setExitApplication() {
	base::MutexLock lock(_mutex);
	_exitApplication = true;
}
