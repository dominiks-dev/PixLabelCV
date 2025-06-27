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
#include "imgui.h"
#include "core.hpp"
#include "imgcodecs.hpp"
#include "core/directx.hpp"
#include <vector>
#include "helper.h"
#include <iostream> //todo: remove

// ImVec4: 4D vector used to store clipping rectangles, colors etc.
// [Compile-time configurable type]
// struct CvData {
//  float x, y, z, w;
//  int whatToDo;  // CVOpertionFlag
//};

// Flags for the CV Algorithm to process
enum CvOperation {
	Nothing = 0,
	Threshold = 1 << 0,  // apply the threshold which the user selected
	DisplayClass = 1 << 1,  // fill the current segmented Region (or just toggle bool??)
	FillMask = 1 << 2,  // apply the threshold result to the class
	Floodfill = 1 << 3,
	DisplayAllClasses = 1 << 4,            // display all labeled classes
	Watershed = 1 <<5, 
	GrabCut = 1 << 6,   
	Clear = 1 << 7,                   // Clear displayed img
	ReplaceClass = 1 << 8                  

};

enum Shape { RectangleD = 0, PolygonD = 1, CircleD = 2, BrushD = 3, MarkerPointsD=4, CutsD = 5, ArcD = 15, };

static std::vector<cv::Vec3b> colors2 = {
	cv::Vec3b(53, 56, 57),     // Onyx(background)
	cv::Vec3b(124, 25, 134),   // Purple
	cv::Vec3b(234, 31, 89),    // Red
	cv::Vec3b(134, 234, 111),  // Lime
	cv::Vec3b(255, 163, 6),    // Orange
	cv::Vec3b(29, 251, 221),   // Turquoise
	cv::Vec3b(99, 99, 198),    // Blue
	cv::Vec3b(255, 255, 102),  // yellow
	cv::Vec3b(255, 29, 206),    // Pink / hot margenta
	cv::Vec3b(255, 235, 205),  // Blanched Almond
	cv::Vec3b(132, 151, 115),  // Oliv
	cv::Vec3b(205, 92, 92),    // Indian Red
	cv::Vec3b(30, 144, 255),   // Dodger Blue
	cv::Vec3b(184, 134, 11),   // Dark Goldenrod
	cv::Vec3b(72, 209, 204),   // Medium Turquoise
	cv::Vec3b(186, 85, 211),   // Medium Orchid
	cv::Vec3b(60, 179, 113),   // Medium Sea Green
	cv::Vec3b(165, 42, 42),    // Brown
	cv::Vec3b(32, 178, 170),   // Light Sea Green
	cv::Vec3b(240, 128, 128),  // Light Coral
	cv::Vec3b(128, 0, 0),      // Maroon
	cv::Vec3b(0, 128, 0),      // Green
	cv::Vec3b(0, 0, 128),      // Navy
	cv::Vec3b(128, 128, 0),    // Olive
	cv::Vec3b(0, 128, 128),    // Teal
	cv::Vec3b(128, 0, 128),    // Purple
	cv::Vec3b(255, 255, 0),    // Yellow
	cv::Vec3b(255, 0, 255),    // Magenta
	cv::Vec3b(0, 255, 255),     // Cyan

	cv::Vec3b(210, 105, 30),   // Chocolate
	cv::Vec3b(0, 250, 154),    // Medium Spring Green
	cv::Vec3b(220, 20, 60),    // Crimson
	cv::Vec3b(123, 104, 238),  // Medium Slate Blue
	cv::Vec3b(255, 105, 180),  // Hot Pink
	cv::Vec3b(255, 250, 205),  // Lemon Chiffon
	cv::Vec3b(138, 43, 226),   // Blue Violet
	cv::Vec3b(250, 128, 114),  // Salmon
	cv::Vec3b(0, 255, 127)     // Spring Green

};

static std::vector<int> getColor(int index)
{
	std::vector<int> Color = { 0,0,0 };
	if (index == 0) return Color;  

	if (colors2.size() > index) {
		auto col = colors2.at(index);
		Color = { col[0], col[1], col[2] }; 
	}
	return Color; 
}

struct RGBrange {
	int R_low = 0, R_high = 255;
	int G_low = 0, G_high = 255;
	int B_low = 0, B_high = 255;

	void set_with_check(int rl, int rh, int gl, int gh, int bl, int bh) {
		R_low = rl > 0 && rl < 255 ? rl : 0;  // check values reasonable
		R_high = rh > rl && rh < 255 ? rh : 255;
		G_low = gl > 0 && gl < 255 ? gl : 0;
		G_high = gh > gl && gh < 255 ? gh : 255;
		B_low = bl > 0 && bl < 255 ? bl : 0;
		B_high = bh > bl && bh < 255 ? bh : 255;
	}
	// RGBrange testr;
	// testr.set_with_check(-3, -23, 54, 345, 43, 3); // check --> would be a good function test
};

/// <summary>
/// converts HSV int values range (360, 255, 255) into rgb values (255,255,255)
/// </summary>
/// <param name="h">Hue: range 0-360 </param>
/// <param name="s">Saturation: 0-255 </param>
/// <param name="v">Value: 0-255 </param>
/// <param name="r">out red value (0-255) </param>
/// <param name="g">out green value (0-255) </param>
/// <param name="b">out blue value (0-255) </param>
static void hsv2rgbCVrange(int h, int s, int v, int& r, int& g, int& b) {
	// Different software use different scales. So we are using normalized input
	// here and convert it later if neccesary as for Opencv HSV, hue range is
	// [0,179], saturation range is [0,255], and value range is [0,255].
	double hh, ss, vv;

	// normalize this
	hh = h / 360.0;  // maybe just the 60 here was wrong
	ss = s / 255.0;
	vv = v / 255.0; 

	// DS: works but some of the test values are a bit of
	hh = std::fmod(hh, 1.0f) / (60.0f / 360.0f);
	int i = (int)hh;
	double f = hh - (float)i;
	double p = vv * (1.0f - ss);
	double q = vv * (1.0f - ss * f);
	double t = vv * (1.0f - ss * (1.0f - f));

	switch (i) {
	case 0:
		r = static_cast<int>(vv * 255);
		g = static_cast<int>(t * 255);
		b = static_cast<int>(p * 255);
		break;
	case 1:
		r = static_cast<int>(q * 255);
		g = static_cast<int>(vv * 255);
		b = static_cast<int>(p * 255);
		break;
	case 2:
		r = static_cast<int>(p * 255);
		g = static_cast<int>(vv * 255);
		b = static_cast<int>(t * 255);
		break;
	case 3:
		r = static_cast<int>(p * 255);
		g = static_cast<int>(q * 255);
		b = static_cast<int>(vv * 255);
		break;
	case 4:
		r = static_cast<int>(t * 255);
		g = static_cast<int>(p * 255);
		b = static_cast<int>(vv * 255);
		break;
	case 5:
	default:
		r = static_cast<int>(vv * 255);
		g = static_cast<int>(p * 255);
		b = static_cast<int>(q * 255);
		break;
	} 
}

static RGBrange ConvertHSVnormToRGBrange(float h, float s, float v, bool h_both,
	bool s_both, bool v_both, int h_tol,
	int s_tol, int v_tol) {
	// hue
	int low = h * 360;
	int h_low = floor(h * 360 + 0.5);
	int h_high = h_low + h_tol < 360 ? h_low + h_tol
		: 360;  

	if (h_both) {
		// low = low - h_tol > 0 ? low - h_tol : 0;
		h_low = h_low - h_tol > 0 ? h_low - h_tol : 0;
		h_high = h_low + 2 * h_tol < 360 ? h_low + 2 * h_tol : 360;
	}

	// sat
	int s_low = floor(s * 255 + 0.5);
	int s_high = s_low + s_tol < 255 ? s_low + s_tol : 255;
	if (s_both) {
		s_low = s_low - s_tol > 0 ? s_low - s_tol : 0;
		s_high = s_low + 2 * s_tol < 255 ? s_low + 2 * s_tol : 255;
	}

	// val
	int v_low = floor(v * 255 + 0.5);

	int v_high = v_low + v_tol < 255 ? v_low + v_tol : 255;
	if (v_both) {
		v_low = v_low - v_tol > 0 ? v_low - v_tol : 0;
		v_high = v_low + 2 * v_tol < 255 ? v_low + 2 * v_tol : 255;
	}

	int R_lower = 0, G_lower = 0, B_lower = 0;
	int R_higher = 0, G_higher = 0, B_higher = 0;

	// get rgb values for low hsv
	hsv2rgbCVrange(h_low, s_low, v_low, R_lower, G_lower, B_lower); 

	// get rgb values for high hsv
	hsv2rgbCVrange(h_high, s_high, v_high, R_higher, G_higher, B_higher);

	// OpenCV HSV range
	// Normal H, S, V: (0 - 360, 0 - 100 %, 0 - 100 %)
	// OpenCV H, S, V: (0 - 180, 0 - 255, 0 - 255)

	RGBrange range = { R_lower,  R_higher, G_lower,
					  G_higher, B_lower,  B_higher };  // works
	return range;
};

struct PointRad {
	ImVec2 pt;
	int rad;
	float zoom; 
	bool foreground; // added for graphcut  1= foreground 0=background
};
struct PointClass{
	int x;
	int y;
	int activeClass; 
};
struct FF { // floodfill parameters
	int low;
	int up;
	cv::Point f_point;
	int current_fillMode;
	bool use_gray_img;
};

struct ImageProcParameters {
    int pixelClassToReplace;

	int up_HorR;
	int up_SorG;
	int up_VorB;
	float H, S, V;
	float R, G, B;
	bool isHSV = false;

	float alpha_display;
	int roi_shape = -1;
	int roi_Points[4] = { -1, -1, -1, -1 };
	std::vector<std::pair<int, int>> poly_Points;
	std::vector<PointRad> PnR;
	std::vector<PointClass> markers;

	FF ff; 
	RGBrange colorThresholds;
	bool drawAllClasses; 
	cv::Point m_point; 

	void setHSV(float h, float s, float v, int h_tol, int s_tol, int v_tol,
		float alpha = 0.5) {
		isHSV = true;
		H = h;
		S = s;
		V = v;
		up_HorR = h_tol;
		up_SorG = s_tol;
		up_VorB = v_tol;
		alpha_display = alpha;
	}
	void setRGB(float r, float g, float b, int h_tol, int s_tol, int v_tol,
		float alpha = 0.5) {
		isHSV = false;
		R = r;
		G = g;
		B = b;
		up_HorR = h_tol;
		up_SorG = s_tol;
		up_VorB = v_tol;
		alpha_display = alpha;
	}

	inline void removeOverlappingPoints(std::vector<PointRad>& points) {
		auto areOverlapping = [] (const PointRad& a, const PointRad& b) {
			double distance = norm2d(a.pt, b.pt);
			return distance < (a.rad + b.rad);
			};

		for(size_t i = points.size(); i-- > 0;) {
			for(size_t j = i; j-- > 0;) {
				if(areOverlapping(points[i], points[j]) && points[i].foreground != points[j].foreground) {
					points.erase(points.begin() + j);
					// Adjust the outer loop index if necessary
					if(j < i) {
						i--;
					}
				}
			}
		}
	}

	void addROI(int points[4], std::vector<std::pair<int, int>> pP, int roi) {
		roi_Points[0] = points[0];
		roi_Points[1] = points[1];
		roi_Points[2] = points[2];
		roi_Points[3] = points[3];
		// std::array<int, 4> roi_Points = points;
		poly_Points = std::vector<std::pair<int, int>>(pP);
		roi_shape = roi;
	}
	void addPolygonPoints(std::vector<std::pair<int, int>> pP, int roi) {
		poly_Points = pP; 
		roi_shape = roi;
	}

	void addMarkers(std::vector<std::tuple<int, int, int>> m, int roi){
		markers.clear(); 
		for (int i = 0; i < m.size(); i++) { 
			PointClass pc = { std::get<0>(m.at(i)), std::get<1>(m.at(i)), std::get<2>(m.at(i)) };
			//PointClass(point.first, point.second, active_class);
			markers.push_back(pc); 
		}
		roi_shape = roi;
	}

	// used for pixelBrush and GrabCut
	void addBrushPoints(std::vector<PointRad>& PointsRVec, int roi, float current_zoom) {
		
		// for grabCut remove conflicting points
		removeOverlappingPoints(PointsRVec); 

		PnR.clear();
		for (PointRad pR : PointsRVec) { 
			// Do not track zoom per point (as it is applied already when zooming in the gui) 
			pR.pt.x /= current_zoom; 
			pR.pt.y /= current_zoom; 

			// zoom per point is considered with rad
			pR.rad = pR.rad/pR.zoom>=1.0 ? pR.rad/pR.zoom: 1; // scale down radius, but min to 1

			pR.foreground = pR.foreground; // for GraphCut
			PnR.push_back(pR);
		}		

		// brush_PnR = PnR ;  // does copy the vector 
		roi_shape = roi;
	}
	void addFF(ImVec2 f_point, int current_fillMode, int low, int up, int ff_use_gray) {
		ff.f_point = cv::Point(f_point.x, f_point.y);
		ff.current_fillMode = current_fillMode; 
		ff.low = low; 
		ff.up = up; 
		ff.use_gray_img = ff_use_gray;
	}
	void addMousePt(ImVec2 mousePosition) {
		m_point = cv::Point(mousePosition.x, mousePosition.y);
	}
    void replaceClass(int classNr) { 
			pixelClassToReplace = classNr < 30 ? classNr : 0;
	}

};


bool pickColor(ImVec2 pixel, float* color);
int addMaskToClassregion(bool overwrite_other_classes = false, bool setCompleteMask = false, bool multiplePixelLabels=false);


 