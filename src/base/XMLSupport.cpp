/* XMLSupport.cpp

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
#include <base/XMLSupport.h>

#include <StringConverter.h>
#include <Log.h>

#include <stdio.h>

#include <iostream>
#include <fstream>

namespace base {

XMLSupport::XMLSupport(const std::string &filePath) : _filePath(filePath) {}

XMLSupport::~XMLSupport() {}

bool XMLSupport::findXMLElement(const std::string &xml, const std::string &elementToFind, std::string &element) {
	std::string tag;
	std::string::const_iterator it = xml.begin();
	std::string::const_iterator itEndElement;
	bool found = false;
	element.clear();
	const bool ok = parseXML(xml, elementToFind, found, element, it, tag, itEndElement);
	return ok & found;
}

bool XMLSupport::parseXML(const std::string &xml, const std::string &elementToFind, bool &found, std::string &element,
                          std::string::const_iterator &it, std::string &tagEnd, std::string::const_iterator &itEndElement) {
	enum class StateXML {
		SEARCH_START_TAG,
		COMMENT,
		XML_DECL,
		TAG_START,
		TAG_END,
		ELEMENT,
		ESCAPE
	};
	StateXML state = StateXML::SEARCH_START_TAG;
	std::string::const_iterator itBegin;
	std::string tag;
	for (; it != xml.end(); ++it) {
		switch (state) {
		case StateXML::SEARCH_START_TAG:
			// Check begin of 'tag'?
			if (*it == '<') {
				++it;
				if (it != xml.end()) {
					itBegin = it;
					switch (*it) {
					case '!':
						state = StateXML::COMMENT;
						break;
					case '?':
						state = StateXML::XML_DECL;
						break;
					case '/':
						itEndElement = it - 1;
						state = StateXML::TAG_END;
						break;
					case '&':
						state = StateXML::ESCAPE;
						break;
					default:
						state = StateXML::TAG_START;
						break;
					}
				} else {
					return false;
				}
			} else {
				itBegin = it;
				state = StateXML::ELEMENT;
			}
			break;
		case StateXML::XML_DECL:
			if (*it == '?') {
				++it;
				if (it != xml.end() && *it == '>') {
					state = StateXML::SEARCH_START_TAG;
				} else {
					return false;
				}
			}
			break;
		case StateXML::TAG_START:
			if (*it == '>') {
				tag = xml.substr(itBegin - xml.begin(), it - itBegin);
				const std::string::size_type end = elementToFind.find_first_of('.', 0);
				const std::string findTag = elementToFind.substr(0, end);
				const bool tagMatch = findTag == tag;
				const std::string nextElementToFind = (tagMatch) ? ((end != std::string::npos) ? elementToFind.substr(end + 1) : "") : elementToFind;
				++it;
				if (it != xml.end()) {
					itBegin = it;
					if (!parseXML(xml, nextElementToFind, found, element, it, tagEnd, itEndElement)) {
						return false;
					}
					if (tag != tagEnd) {
						return false;
					}
					if (tagMatch && end == std::string::npos) {
						element = xml.substr(itBegin - xml.begin(), itEndElement - itBegin);
						found = true;
					}
					state = StateXML::SEARCH_START_TAG;
				} else {
					return false;
				}
			}
			break;
		case StateXML::TAG_END:
			if (*it == '>') {
				++itBegin;
				tagEnd = xml.substr(itBegin - xml.begin(), it - itBegin);
				return true;
			}
			break;
		case StateXML::COMMENT:
			if (*it == '>') {
				state = StateXML::SEARCH_START_TAG;
			}
			break;
		case StateXML::ESCAPE:
			if (*it == ';') {
				state = StateXML::SEARCH_START_TAG;
			}
			break;
		case StateXML::ELEMENT:
			if (*it == '<') {
				itEndElement = it - 1;
				state = StateXML::SEARCH_START_TAG;
				--it;
			}
			break;
		}
	}
	return true;
}

std::string XMLSupport::makeXMLString(const std::string &msg) {
	std::string xml;
	std::string::const_iterator it = msg.begin();
	while (it != msg.end()) {
		if (*it == '&') {
			xml += "&amp;";
		} else if (*it == '"') {
			xml += "&quot;";
		} else if (*it == '>') {
			xml += "&gt;";
		} else if (*it == '<') {
			xml += "&lt;";
		} else {
			xml += *it;
		}
		++it;
	}
	return xml;
}

std::string XMLSupport::getFileName() const {
	std::string file("");
	if (!_filePath.empty()) {
		std::string path;
		StringConverter::splitPath(_filePath, path, file);
	}
	return file;
}

void XMLSupport::saveXML(const std::string &xml) const {
	if (!_filePath.empty()) {
		std::ofstream file;
		file.open(_filePath);
		if (file.is_open()) {
			file << xml;
			file.close();
		}
	}
}

bool XMLSupport::restoreXML() {
	if (!_filePath.empty()) {
		std::ifstream file;
		file.open(_filePath);
		if (file.is_open()) {
			std::string xml((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			file.close();
			fromXML(xml);
			return true;
		}
	}
	return false;
}

} // namespace base
