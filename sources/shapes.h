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
#include "helper.h"

struct DrawPolygon {
	bool closed = false;
	std::vector<ImVec2> points;
	ImVec2 mid = { -1, -1 };
	// point index for polygon mit point --> should be a rather large int 
	const int POLYMIDPOINT = 10000000; 
	inline void Reset() {
		points.clear(); 
		closed = false;  
		mid = { -1, -1 }; 
	}
	int find_closest_segment(const ImVec2& new_point) {
		double min_distance = std::numeric_limits<double>::max();
		int best_index = -1;

		for(size_t i = 0; i < points.size(); ++i) {
			int next_index = (i + 1) % points.size();
			double dist = point_to_segment_distance(new_point, points[i], points[next_index]);
			if(dist < min_distance) {
				min_distance = dist;
				best_index = next_index;
			}
		}
		return best_index;
	}
	void InsertPoint(ImVec2 currentPoint) {
		int best_index = find_closest_segment(currentPoint);
		points.insert(points.begin() + best_index, currentPoint);
	}
	inline bool IsOutOfBounds(ImVec2 difference, ImVec2 size) {
		float minX, maxX, minY, maxY;
		std::vector<ImVec2> newPoints;
		for (auto p : points) {
			newPoints.push_back(ImVec2(p.x + difference.x, p.y + difference.y));
		}
		calculateMinMax(newPoints, minX, minY, maxX, maxY);
		if (minX < 0 || minY < 0) return true;
		if (maxX > size.x || maxY > size.y) return true;
		return false;
	}
	bool Move(ImVec2 currentPoint, ImVec2 imgSize) {
		// calculate offset between the last mid location and the one now
		auto diffX = currentPoint.x - mid.x;
		auto diffY = currentPoint.y - mid.y;

		// check first if any point would be out of boundaries
		if (!IsOutOfBounds(ImVec2(diffX, diffY), imgSize)) {
			for (int i = 0; i < points.size(); i++) {
				points.at(i).x += diffX;
				points.at(i).y += diffY;
			}
			mid = currentPoint; // check if that is valid!
			return true; 
		}
		return false; 
	}
};
struct DrawRect {
	int	left;
	int	top;
	int right;
	int bottom;
	void Reset() {
		left = top = right = bottom = -1; 
	}
};
struct DrawCircle {
	int mid_x = -1;
	int mid_y = -1;
	int out_x = -1;
	int out_y = -1;
	int dragging_point = -1;
	void Reset() {
		mid_x = -1; mid_y = -1; out_x = -1; out_y = -1;
	}
};
struct DrawEllipse {
	std::vector<ImVec2> points;
	ImVec2 center;
};
struct Marker {
	//std::vector<std::tuple<int, int, int>> point_Label; // points with label 
	std::vector<ImVec2> points;
	std::vector<int> labels; // classnumber of the marker 
	bool tooClose; // for determining when not to add a new point - important for deleting on double click!

	size_t Count() {
		return points.size();
	}
	void Scale(double factor) {
		for(auto& p : points) {
			p.x *= factor;
			p.y *= factor;
		}
	}
	void Reset() {
		points.clear();
		labels.clear();
	}

	bool RemovePoint(ImVec2 ClickPosition, double min_distance_to_remove = 12) {

		size_t closestIndex = 0;
		double minDistance = min_distance_to_remove;
		for(size_t i = 0; i < points.size(); ++i) {
			double d = norm2d(ImVec2(points[i].x, points[i].y), ClickPosition);
			if(d < minDistance) {
				minDistance = d;
				closestIndex = i;
			}
		}

		// Remove the closest point
		if(!points.empty()) {
			points.erase(points.begin() + closestIndex);
			labels.erase(labels.begin() + closestIndex);
			tooClose = true;
		}
		return tooClose;
	}
};

