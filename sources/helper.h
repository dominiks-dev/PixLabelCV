/*
 * This file is part of PixLabelCV.
 *
 * Copyright (C) 2024 Dominik Schraml
 *
 * PixLabelCV is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Alternatively, commercial licenses are available. Please contact SQB Ilmenau
 * at olaf.glaessner@sqb-ilmenau.de or dominik.schraml@sqb-ilmenau.de for more details.
 *
 * PixLabelCV is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PixLabelCV. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <vector>
#include "imgui.h"
#include "fstream"

inline double norm2d(ImVec2 P1, ImVec2 P2) {
	return  sqrt(pow(P1.x - P2.x, 2) +
				 pow(P1.y - P2.y, 2));
}
inline bool inRange(ImVec2 P1, ImVec2 P2, double range) {
	double d = norm2d(P1, P2);
	if(d <= range) return true;
	else return false;
}

inline ImVec2 calculateCentroid(const std::vector<ImVec2> poly_points) {
	float sumX = 0;
	float sumY = 0;
	int n = poly_points.size();
	for(const auto& point : poly_points) {
		sumX += point.x;
		sumY += point.y;
	}
	return ImVec2(sumX / n, sumY / n);
}
inline void calculateMinMax(const std::vector<ImVec2>& points, float& minX, float& minY, float& maxX, float& maxY) {
	minX = std::numeric_limits<float>::max();
	minY = std::numeric_limits<float>::max();
	maxX = std::numeric_limits<float>::lowest();
	maxY = std::numeric_limits<float>::lowest();

	for(const auto& point : points) {
		if(point.x < minX) minX = point.x;
		if(point.y < minY) minY = point.y;
		if(point.x > maxX) maxX = point.x;
		if(point.y > maxY) maxY = point.y;
	}
}

inline std::string filter_chars(char* chars_raw) {
	std::string filtered = ""; 
	unsigned int j = 0;
	for(unsigned int i = 0; i < sizeof(chars_raw); ++i) {
		char c = chars_raw[i];
		if(c == '|' || c == '?' || c == '/' || c == '\\' || c == '?' || c == '\"'
		   || c == '<' || c == '>' || c == '*' || c == ':')
			continue;
		if(c == '\0')
			break;
		//chars[j] = c;
		//mask_postfix.append(std::to_string(c)); // = c;
		filtered.append(std::string(1, c));
		j++;
	}
	return filtered; 
}
 
inline std::vector<std::string> getAllImagesInPath(std::string path) {
	std::vector<std::string> imagesList;
	//= new vector<string>();
	// for (const auto& file : std::filesystem::directory_iterator(path)) {
	//	// cout << file.path() << endl;
	//	 // imagesList->push_back(reinterpret_cast<string>(file.path()));
	//	auto last_char = file.path().string().end();

	//	if(file.)
	//	imagesList.push_back(file.path().string());
	//	std::string fileName = file.path().string();
	//}

	for(const auto& file : std::filesystem::directory_iterator(path)) {
		// cout << file.path() << endl;
		// imagesList->push_back(reinterpret_cast<string>(file.path()));
		// auto path = file.path();
		std::string ending;
		std::string fileName = file.path().string();
		// strcpy(ending, file.path().string(), file.path().string().rbegin());
		ending = (fileName.length() > 3) ? fileName.substr(fileName.length() - 4, 4)
			: fileName;
		std::transform(ending.begin(), ending.end(), ending.begin(), ::tolower); // transform chars to lower case
		if(ending == ".bmp" || ending == ".jpg" || ending == ".png" ||
		   ending == ".tiff" || ending == ".tif") {
			imagesList.push_back(file.path().string());
		}
	}
	return imagesList;
}

inline static bool checkIfFirstRun(const std::string& iniFileName) {
	std::ifstream iniFile(iniFileName);
	// If file exists, then not first run
	if (iniFile.good()) {
		iniFile.close();
		return false;
	}
	else {
		// If file doesn't exist, it's the first run
		return true;
	}
}

static void HelpMarker(const char* desc) {
	ImGui::TextDisabled("(?)");
	if(ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}