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
#include "user_interaction.h"

bool dragging(int current_draw_shape, ImVec2 currentPoint, DrawPolygon& poly, Marker& marker, DrawCircle& circ, int max_width, int max_height, int& dragging_point) {
	//DS: remember this function only gets called when user is already dragging  
	bool is_drawing = false;

	if((poly.closed && current_draw_shape == PolygonD) // for polygon
	   || current_draw_shape == MarkerPointsD) {  // for maker points
		// check if a point was clicked to be dragged
		if(dragging_point < 0) { // means no point is dragged yet 
			//DS: for loops suffice here, as there should be no points for not active shape

			// drag point next to mouse down position
			for(size_t i = 0; i < poly.points.size(); i++) {
				ImVec2 pt = poly.points.at(i);
				if(inRange(pt, currentPoint, 15)) {
					dragging_point = i;
					LabelState::Instance().drawingFinished = false;
					// isDrawing = true;
				}
			}
			// drag polyon mid point
			if(inRange(currentPoint, poly.mid, 15))
				dragging_point = poly.POLYMIDPOINT;
			// select a marker point to drag
			for(size_t i = 0; i < marker.points.size(); i++) { // no if condition here because either of the two is empty
				if(inRange(marker.points.at(i), currentPoint, 15)) {
					dragging_point = i;
					// isDrawing = true;
					LabelState::Instance().drawingFinished = false;
				}
			}
		}
		// adjust the point while dragging
		else if(dragging_point >= 0) {
			if(current_draw_shape == PolygonD)
				// check first if mid is dragged and adjust all points as necessary
				if(dragging_point == poly.POLYMIDPOINT) {
					// stop the dragging if a point of the polygon would go out of bounds - probably there may be a better solution 
					bool couldMove = poly.Move(currentPoint, ImVec2(max_width, max_height));
					if(!couldMove) dragging_point = -1;
				} else // select dragged point by index
					poly.points.at(dragging_point) = currentPoint;
			else if(current_draw_shape == MarkerPointsD) {
				marker.points.at(dragging_point) = ImVec2(currentPoint.x, currentPoint.y);
			}
			is_drawing = true;
		}
	}
	// dragging circle
	else if(current_draw_shape == CircleD && circ.out_x != -1) {  // check that both points are set
		// select point to drag
		if(dragging_point < 0) {
			//if (norm2d(ImVec2(circ.mid_x, circ.mid_y), currentPoint) < 15)
			if(inRange(ImVec2(circ.mid_x, circ.mid_y), currentPoint, 15))
				dragging_point = 0;
			else if(inRange(ImVec2(circ.out_x, circ.out_y), currentPoint, 15))
				dragging_point = 1;
		}
		// adjust the point while dragging
		else if(dragging_point == 0) {
			is_drawing = true;
			// adjust outer point on circle first!
			circ.out_x += currentPoint.x - circ.mid_x;
			circ.out_y += currentPoint.y - circ.mid_y;
			circ.mid_x = currentPoint.x;
			circ.mid_y = currentPoint.y;
		} else if(dragging_point == 1) {
			is_drawing = true;
			circ.out_x = currentPoint.x;
			circ.out_y = currentPoint.y;
		}
	}
	return is_drawing;
}


void double_clicked(int current_draw_shape, ImVec2 currentPoint, DrawPolygon& poly, Marker& marker, int& dragging_point) {

	if(current_draw_shape == PolygonD && poly.closed) {
		// Add point to polygon
		if(ImGui::IsKeyDown(ImGuiKey_ModCtrl)) {
			double distance, min_distance = DBL_MAX, second_min_distance = DBL_MAX;
	 
			poly.InsertPoint(currentPoint); 
	/*		int best_index = find_closest_segment(poly, currentPoint);
			poly.points.insert(poly.points.begin() + best_index, currentPoint);*/
		}
		// delete point from polygon
		else if(poly.closed) {
			// use iterator to go over all elements
			for(auto it = poly.points.begin(); it != poly.points.end(); /* empty */) {
				ImVec2 pt = *it;
				double d = norm2d(pt, currentPoint);
				if(d <= 12) {
					it = poly.points.erase(it); // erase returns the iterator to the next element
					// resetting the closed state seems logic
					if(poly.points.size() < 3) poly.closed = false; 
					break; // leaves the loop after deleting one element -> rest will be unchanged
				} else {
					++it; // only increment the iterator if you didn't erase
				}
			}
		}
		// recalculate mid point
		if(poly.closed) poly.mid = calculateCentroid(poly.points);
	}
	// delete points form markers
	else if(current_draw_shape == MarkerPointsD) {
		bool removed = marker.RemovePoint(currentPoint);
		if(removed) dragging_point = -1; 
	}
}

bool mouse_released(int current_draw_shape, ImVec2 currentPoint, DrawPolygon* poly, Marker* m, DrawEllipse* ell,
					DrawCircle* c, int dragging_point) {
	bool IsDrawing = false;

	if(current_draw_shape == PolygonD || current_draw_shape == ArcD) {
		
		// check that the same point is not added twice
		if(std::any_of(poly->points.begin(), poly->points.end(),
		   [&] (const ImVec2& pt) {
			   if(pt.x == currentPoint.x && pt.y == currentPoint.y)
				   return true;
			   else
				   return false;
		   })) {
			std::cout << currentPoint.x << ", " << currentPoint.y
				<< " is present in the vector\n";
		} 
		else { 
			double distance;
			bool add_new_point = false;

			// check if click was next to already set point --> interpret it
			// as closing the polygon (at point 0) or wanting to drag the
			// point
			for(int i = 0; i < poly->points.size(); i++) {
				ImVec2 pt = poly->points.at(i);
				distance = norm2d(pt, currentPoint);
				if(distance <= 15) {
					std::cout << "Distance to closest point " << i << " = "
						<< distance << "\n";

					// close polygon if user clicks on first point
					if(i == 0 && poly->closed == false
					   && poly->points.size() > 2 ) { // must at least have 3 points
						poly->closed = true;
						LabelState::Instance().drawingFinished = true;
						// calculate Polygon mid point
						poly->mid = calculateCentroid(poly->points);
						break;
					}
				} else if(poly->closed == false) {  // add the new point
					add_new_point = true;
					IsDrawing = true;
				}
			}
			if(add_new_point || poly->points.size() == 0) {
				poly->points.push_back(currentPoint);
				// recalculate midpoint if further point is added 
				if(poly->closed) poly->mid = calculateCentroid(poly->points);

				// LabelState::Instance().drawingFinished==true; --> when polygon is complete
				IsDrawing = true;
			}

			// DS: first implementation of ellipse - copy points of polygon
			if(current_draw_shape == 2 && ell->points.size() < 3) {
				ell->points = std::vector<ImVec2>(poly->points);
				IsDrawing = true;
			}
		}
	}  // end poly or circle
	else if(current_draw_shape == MarkerPointsD) {
		// check that the same point or close point is not added 
		for(int i = 0; i < m->points.size(); i++) {
			ImVec2 pt = m->points.at(i);
			auto distance = norm2d(pt, currentPoint);
			if(distance <= 10) {
				m->tooClose = true; // this is used to determine when point is deleted
			}
		} 
		// if not dragging and 
		if(dragging_point < 0 && m->tooClose != true) {	// add new point
			m->points.push_back(ImVec2(currentPoint.x, currentPoint.y));
			m->labels.push_back(LabelState::Instance().GetActiveClass());
		}
		if(m->Count() > 0) {
			LabelState::Instance().drawingFinished = true;
		}
		m->tooClose = false; // reset 
	}
	else if(current_draw_shape == CircleD) {
		// define points for circle
		// check that the same point is not added twice
		if(c->mid_x == -1 &&
		   c->mid_y == -1) {  // add First point for new circle
			c->mid_x = currentPoint.x;
			c->mid_y = currentPoint.y;
			IsDrawing = true; // so that middle point is displayed (later)
			LabelState::Instance().drawingFinished = false; // reset drawing finished flag (when clicking next point)
		}
		else if((c->out_x == -1 && c->out_y == -1) &&
				(c->mid_x != currentPoint.x)) {  // point in vector is not the same
			c->out_x = currentPoint.x;
			c->out_y = currentPoint.y;
			LabelState::Instance().drawingFinished = true;
		}
	}
	return IsDrawing;

}

// Scale the points when zooming in and out [--> they are directly used in the gui and converted to CV later]
void scalePoints(int current_draw_shape, double zoom_ratio, DrawRect& d,  DrawPolygon& p, Marker& marker, DrawCircle& c, std::vector<PointRad>& lines) {
	if(current_draw_shape == RectangleD) {
		d.top *= zoom_ratio;
		d.left *= zoom_ratio;
		d.bottom *= zoom_ratio;
		d.right *= zoom_ratio;
	}
	else if(current_draw_shape == PolygonD) {
		for(int i = 0; i < p.points.size(); i++) {
			p.points.at(i).x *= zoom_ratio;
			p.points.at(i).y *= zoom_ratio;
		}
		p.mid.x *= double(zoom_ratio);
		p.mid.y *= double(zoom_ratio);
	} else if(current_draw_shape == MarkerPointsD) {
		marker.Scale(zoom_ratio);
	} else if(current_draw_shape == CircleD) {
		c.mid_x *= zoom_ratio;
		c.mid_y *= zoom_ratio;
		c.out_x *= zoom_ratio;
		c.out_y *= zoom_ratio;
	} else if(current_draw_shape == CutsD) { // scale just GraphCut not Brush by now 
		for(int i = 0; i < lines.size(); i++) {
			lines.at(i).pt.x *= zoom_ratio;
			lines.at(i).pt.y *= zoom_ratio;
			lines.at(i).rad* zoom_ratio >= 1.0 ? lines.at(i).rad *= zoom_ratio : 1.00; // make sure not to scale the lines smaller than 1.0 
		}
	}
}

// User Input to CV_parameters
void CreateImageProcParam(int current_draw_shape, DrawRect& draw_rect, ImageProcParameters& ImPar, DrawPolygon& poly, double current_zoom,
						  Marker m, std::vector<PointRad>& brush_points_rad, const ImVec2& position_correction, DrawCircle c, ImVec2& f_point, int current_fill_mode, int low, int up, bool ff_use_gray) {

	std::vector<std::pair<int, int>> _dummy;
	if(current_draw_shape == RectangleD) {		
		int points[4] = { (int) (draw_rect.left / current_zoom), 
			(int)(draw_rect.top / current_zoom),
			(int)(draw_rect.right / current_zoom),
			(int)(draw_rect.bottom / current_zoom) };
		ImPar.addROI(points, std::vector<std::pair<int, int>>(), current_draw_shape);
	} else if(current_draw_shape == PolygonD) {
		// conversion from ImVec2 to std::pair
		std::vector<std::pair<int, int>> pPs;
		//for each (auto pP in poly_points) { //MSVC 
		for(auto pP : poly.points) { // clang
			pPs.push_back(std::pair(pP.x / current_zoom, pP.y / current_zoom));
		}
		ImPar.addPolygonPoints(pPs, current_draw_shape);
	} else if(current_draw_shape == MarkerPointsD) {
		// conversion from ImVec2 to std::pair
		std::vector<std::tuple<int, int, int>> mps;
		//for each (auto m in markers) { // MSVC
		//for (auto m : markers) { // clang
		for(size_t i = 0; i < m.Count(); i++) {
			mps.push_back(std::tuple<int, int, int>(m.points[i].x / current_zoom, m.points[i].y / current_zoom, m.labels[i]));
		}
		ImPar.addMarkers(mps, current_draw_shape);
	} else if(current_draw_shape == BrushD || current_draw_shape == CutsD) {
		ImPar.addBrushPoints(brush_points_rad, current_draw_shape, current_zoom);
	} else if(current_draw_shape == CircleD) {  // circle
		int points[] = { (int)(c.mid_x / current_zoom), (int)(c.mid_y / current_zoom),
			(int)(c.out_x / current_zoom), (int)(c.out_y / current_zoom) };
		ImPar.addROI(points, _dummy, current_draw_shape);
	}
	// when Magic Wand key (M) was pressed --> evaluate is set to true and  
	// Do this also when expert window is not displayed
	ImVec2 cv_point = ImVec2(f_point.x / current_zoom, f_point.y / current_zoom);
	ImPar.addFF(cv_point, current_fill_mode, low, up, ff_use_gray);
}

// Display the drawn shapes in the GUIs
void DrawShapeOnGui(int& dragging_point, bool& is_drawing, int current_draw_shape, DrawRect& draw_rect, ImVec2& mousePositionAbsolute, ImVec2& screenPositionAbsolute,
					int snap_to_border_distance, int image_width, Zoom& zoom, int image_height, bool is_drawing_brush, std::vector<PointRad>& brush_points_rad, float alpha,
					DrawPolygon& poly, Marker& marker, DrawCircle& circ, DrawEllipse& ell) {
	// release the point when dragging is finished --> moved outside of (is Hovered)
	if(ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
	   (dragging_point >= 0)) {
		dragging_point = -1;
		is_drawing = false;
		LabelState::Instance().drawingFinished = true;
	} 

	// drawing retangle	
	else if(current_draw_shape == RectangleD) {
		//TODO: refactor this
		// finsh drawing Rect 
		if(current_draw_shape == RectangleD && !ImGui::IsMouseDown(ImGuiMouseButton_Left)
				&& is_drawing == true) {
		 
			// make sure that the rectangle is aligned correctly - if not switch x and/or y
			if(draw_rect.right < draw_rect.left) {
				auto temp = draw_rect.left;
				draw_rect.left = draw_rect.right;
				draw_rect.right = temp;
			}
			if(draw_rect.bottom < draw_rect.top) {
				auto temp = draw_rect.top;
				draw_rect.top = draw_rect.bottom;
				draw_rect.bottom = temp;
			}					

			LabelState::Instance().drawingFinished = true;
			is_drawing = false;
		}
	} // END finish drawing rect

	// Draw brush
	if(is_drawing_brush || LabelState::Instance().drawingFinished) {
		ImDrawList* foreground_drawlist = ImGui::GetWindowDrawList();
		// ImDrawList* foreground_drawlist = ImGui::GetForegroundDrawList(); 
		// DS: drawing on window seems to be faster
		for(auto PnR : brush_points_rad) {
			ImVec2 p = PnR.pt;
			float rad = PnR.rad;
			static ImU32* brush_col = new ImU32(); // hex value like 0x6AB020F2 also possible

			if(current_draw_shape == CutsD) {
				if(PnR.foreground)
					(*brush_col) = ((ImU32)(1.0 * 255.0f)) | ((ImU32)(1.0 * 255.0f) << 8) |
					((ImU32)(1.0 * 255.0f) << 16) | ((ImU32)(alpha * 255.0f) << 24);
				else
					(*brush_col) = ((ImU32)(0.0 * 255.0f)) | ((ImU32)(0.0 * 255.0f) << 8) |
					((ImU32)(0.0 * 255.0f) << 16) | ((ImU32)(alpha * 255.0f) << 24);
			} else {
				// draw the points in the color of the current class
				(*brush_col) = ((ImU32)(1.0 * 255.0f)) | ((ImU32)(0 * 255.0f) << 8) |
					((ImU32)(0 * 255.0f) << 16) | ((ImU32)(alpha * 255.0f) << 24);
			}

			// foreground_drawlist->AddCircleFilled(p, rad, *brush_col, 0);
			foreground_drawlist->AddCircleFilled(
				ImVec2(p.x + screenPositionAbsolute.x, p.y + screenPositionAbsolute.y), rad, *brush_col, 0);
		}
	}

	// Draw selected shape into the GUI while drawing and when finished
	if(is_drawing || LabelState::Instance().drawingFinished) {
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		// DRAW Rect
		if(current_draw_shape == RectangleD) {
			//draw_list->AddRect(ImVec2(draw_rect.left, draw_rect.top),
							   //ImVec2(draw_rect.right, draw_rect.bottom),
			// 6/24 Added position correction here --> this accounts for scrolling 
			draw_list->AddRect(ImVec2(draw_rect.left + screenPositionAbsolute.x, draw_rect.top + screenPositionAbsolute.y),
							   ImVec2(draw_rect.right + screenPositionAbsolute.x, draw_rect.bottom +screenPositionAbsolute.y),
							   IM_COL32(255, 255, 0, 255), ImDrawFlags_Closed,
							   3.0F);
		}
		// DRAW polygon
		else if(current_draw_shape == PolygonD) {
			// for each (auto point in polyPoints) {
			for(int i = 0; i < poly.points.size(); i++) {
				if(poly.points.size() > 2 && i < poly.points.size() - 1) {
					//  pay respect to changing screen position --> only apply the
					//  change to display but not to the image to label!
					draw_list->AddLine(
						ImVec2(poly.points.at(i).x + screenPositionAbsolute.x,
						poly.points.at(i).y + screenPositionAbsolute.y),
						ImVec2(poly.points.at(i + 1).x + screenPositionAbsolute.x,
						poly.points.at(i + 1).y + screenPositionAbsolute.y),
						IM_COL32(255, 255, 255, 255));
				}
				// if polygon is closed also display the last line - and mid point
				else if(poly.closed && i == poly.points.size() - 1) {
					//  pay respect to changing screen position
					draw_list->AddLine(
						ImVec2(poly.points.at(i).x + screenPositionAbsolute.x,
						poly.points.at(i).y + screenPositionAbsolute.y),
						ImVec2(poly.points.at(0).x + screenPositionAbsolute.x,
						poly.points.at(0).y + screenPositionAbsolute.y),
						IM_COL32(255, 255, 255, 255));
					draw_list->AddCircleFilled(ImVec2(poly.mid.x + screenPositionAbsolute.x, poly.mid.y + screenPositionAbsolute.y), 5, IM_COL32(255, 50, 0, 200));
				}
				// draw the Polygon points
				draw_list->AddCircle(
					ImVec2(poly.points.at(i).x + screenPositionAbsolute.x,
					poly.points.at(i).y + screenPositionAbsolute.y),
					4, IM_COL32(0, 255, 0, 255), 24);
			}
		} else if(current_draw_shape == MarkerPointsD) { 
			//for each (auto m in markers) { // MSVC 
			//for (auto m : markers) { // clang
			for(size_t i = 0; i < marker.Count(); ++i) {
				auto classColor = getColor(marker.labels.at(i));
				draw_list->AddCircleFilled(
					ImVec2(marker.points[i].x + screenPositionAbsolute.x,
					marker.points[i].y + screenPositionAbsolute.y),
					5, IM_COL32(classColor.at(0), classColor.at(1), classColor.at(2), 200), 24);
				draw_list->AddCircle( // highlight the borders, especially usefull for background points in dark areas
					ImVec2(marker.points[i].x + screenPositionAbsolute.x,
					marker.points[i].y + screenPositionAbsolute.y),
					6, IM_COL32(255, 0, 0, 255), 25), 0.05; // 
			}
		}
		// DRAW circle
		else if(current_draw_shape == CircleD) {
			// first point - middle
			draw_list->AddCircle(
				ImVec2(circ.mid_x + screenPositionAbsolute.x,
				circ.mid_y + screenPositionAbsolute.y),
				5, IM_COL32(255, 0, 0, 255), 20);
			if(circ.out_x != -1 &&
			   circ.out_y != -1) {  // maybe check all points
				int radius = sqrt(pow(circ.out_x - circ.mid_x, 2) +
								  pow(circ.out_y - circ.mid_y, 2));
				// the outer circle
				draw_list->AddCircle(
					ImVec2(circ.mid_x + screenPositionAbsolute.x,
					circ.mid_y + screenPositionAbsolute.y),
					radius, IM_COL32(255, 255, 0, 255),
					50);
				// the dragging point on the outer circle
				draw_list->AddCircle(
					ImVec2(circ.out_x + screenPositionAbsolute.x,
					circ.out_y + screenPositionAbsolute.y),
					6, IM_COL32(255, 0, 0, 255), 20);
			}
		}
		// DRAW ellipse (currently not used)
		else {
			for(int i = 0; i < ell.points.size(); i++) {
				draw_list->AddCircle(
					ImVec2(poly.points.at(i).x + screenPositionAbsolute.x,
					poly.points.at(i).y + screenPositionAbsolute.y),
					4, IM_COL32(192, 255, 0, 255), 15);
			}
			if(ell.points.size() == 3) {
				// rendering ellipse via arcs
				double r1 =
					sqrt(pow(ell.points.at(0).x - ell.points.at(1).x, 2) +
						 pow(ell.points.at(0).y - ell.points.at(1).y, 2));
				double r2 =
					sqrt(pow(ell.points.at(0).x - ell.points.at(2).x, 2) +
						 pow(ell.points.at(0).y - ell.points.at(2).y, 2));
				draw_list->PathArcTo(ell.points.at(0), r1, 0.0f, 3.14159265359, 20);  // this is a half circle with 20 segments
				draw_list->PathArcTo(ell.points.at(0), r2, 3.14159265359,
									 3.14159265359 * 2, 20);  // this is a half circle with 20 segments
				draw_list->PathStroke(ImColor(155, 50, 255), ImDrawFlags_None,
									  3);
				// ImDrawFlags_Closed
				// draw_list->PathArcTo(center, (RADIUS_MIN + RADIUS_MAX) * 0.5f,
				// 0.0f, IM_PI * 2.0f * 0.99f, 32);
			}

		}
	}
}

