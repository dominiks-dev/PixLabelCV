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
#include "LabelState.h"
#include "helper.h"
#include <iostream>
#include "ImageProcessing.h"
#include "shapes.h"

 
struct Zoom {
	float min = 0.1;
	float max = 4.0;
	float factor = 0.10; // 0.25;
	double current = 1.0;
	void increase() {
		current = current+ factor > max ? max : current + factor;
	}
	void decrease() {
		current = current - factor < min ? min : current- factor;
	}
};


bool dragging(int current_draw_shape, ImVec2 currentPoint, DrawPolygon& poly, Marker& marker, DrawCircle& circ, int image_width, int image_height, int& dragging_point);
bool mouse_released(int current_draw_shape, ImVec2 currentPoint, DrawPolygon* poly, Marker* m, DrawEllipse* ell, DrawCircle* c, int dragging_point);

void double_clicked(int current_draw_shape, ImVec2 currentPoint, DrawPolygon& poly, Marker& marker, int& dragging_point);
void scalePoints(int current_draw_shape, double zoom_ratio, DrawRect d, DrawPolygon& poly, Marker& marker, DrawCircle& c, std::vector<PointRad>& lines);

void CreateImageProcParam(int current_draw_shape, DrawRect& draw_rect, ImageProcParameters& ImPar, DrawPolygon& poly, double current_zoom,
	Marker m, std::vector<PointRad>& brush_points_rad, const ImVec2& position_correction, DrawCircle c, ImVec2& f_point, int current_fill_mode, int low, int up, bool ff_use_gray);

void DrawShapeOnGui(int& dragging_point, bool& is_drawing, int current_draw_shape, DrawRect& draw_rect, ImVec2& mousePositionAbsolute, ImVec2& screenPositionAbsolute, int snap_to_border_distance, int image_width, Zoom& zoom, int image_height, bool is_drawing_brush, std::vector<PointRad>& brush_points_rad, float alpha, DrawPolygon& poly, Marker& marker, DrawCircle& circ, DrawEllipse& ell);

