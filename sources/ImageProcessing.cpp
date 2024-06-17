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
#include "load_image.h"
#include "opencv2/core.hpp"
#include "opencv2/core/directx.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include <filesystem>

#include "ImageProcessing.h"
#include "LabelState.h"
#include "Timer.h"
#include "imgui.h"
#include "shapes.h"

using namespace cv;
using namespace std;

static UMat currentClassRegion;
static UMat tempMask;

#pragma region helpers


void returnHSVvalues(vector<float>& rgb, float hsv[3]) {
	float hue, sat;
	float maxval, minval;

	maxval = *max_element(rgb.begin(), rgb.end());
	minval = *min_element(rgb.begin(), rgb.end());

	float difference = maxval - minval;

	float red, green, blue;
	red = rgb.at(0);
	green = rgb.at(1);
	blue = rgb.at(2);

	if(difference == 0)
		hue = 0;
	else if(red == maxval)
		hue = fmod(((60 * ((green - blue) / difference)) + 360), 360.0);
	else if(green = maxval)
		hue = fmod(((60 * ((blue - red) / difference)) + 120), 360.0);
	else if(blue = maxval)
		hue = fmod(((60 * ((red - green) / difference)) + 240), 360.0);

	hsv[0] = (hue);

	if(maxval == 0)
		sat = 0;
	else
		sat = 100 * (difference / maxval);

	hsv[1] = (sat);
	hsv[2] = (maxval * 100);
}

inline void keepRectInBounds(cv::Rect& Rect, int w, int h) {
	if(Rect.x < 0) Rect.x = 0;
	if(Rect.y < 0) Rect.y = 0;
	if(Rect.x + Rect.width > w) {
		Rect.width = w - Rect.x;
	}
	if(Rect.y + Rect.height > h) {
		Rect.height = h - Rect.y;
	}
	// 11.3.24 if all values of one axis are 0 set the width or heigt to 1
	if (Rect.width < 1) Rect.width = 1; 
	if (Rect.height < 1) Rect.height= 1; 
}

// alternative bounding function
inline void adjustRectangleToBounds(int& x, int& y, int& width, int& height, int imageWidth, int imageHeight) {
	// Ensure the rectangle starts within the image
	x = std::max(0, x);
	y = std::max(0, y);

	// Adjust width and height if they extend beyond the image boundaries
	if (x + width > imageWidth) {
		width = imageWidth - x; // Adjust width to fit within the image
	}

	if (y + height > imageHeight) {
		height = imageHeight - y; // Adjust height to fit within the image
	}

	// Handle cases where the initial width or height might be larger than the image
	width = std::min(width, imageWidth);
	height = std::min(height, imageHeight);

	// Additional safety check in case the rectangle is completely out of bounds
	if (x >= imageWidth || y >= imageHeight) {
		// Optionally, set the rectangle to a default position or size
		// For example, making the ROI invalid or clamping to the nearest valid position
		x = y = 0;
		width = std::min(width, imageWidth);
		height = std::min(height, imageHeight);
	}
}

#pragma endregion helpers

bool compare_only_second(PointRad pt, PointRad pt2) {
	return (pt.rad < pt2.rad);
}


bool fill_mask(UMat classPixelMask) { 

	UMat Points;
	findNonZero(classPixelMask, Points);
	// check that input region is not empty / blank!
	if(!Points.empty()) {
		vector<vector<Point> > contours;
		std::vector<cv::Vec4i> hierarchy;
		findContours(classPixelMask, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
		//TODO: check difference Ret_External to RETR_FLOODFILL
		// findContours(tempMask, contours, hierarchy, RETR_FLOODFILL); 
		fillPoly(classPixelMask, contours, Scalar(255));
		return true;
	} else return false;
}


/// Use the ImageProcParameters (roi, thresholds etc.) to calulate the region of the current class and add them to a temporary mask
/// [for the user to decide afterwards whether he wants to add the region to the class]
cv::UMat ApplyCVOperation(ImageProcParameters params, float* color, CvOperation op) {
	Timer timer;
	cv::UMat img = LabelState::Instance().GetCurrentImg();
	int height = img.rows;
	int width = img.cols;
	UMat image_rgba;
	// reset temp classPixelMask !
	tempMask = cv::UMat::zeros(LabelState::Instance().h(), LabelState::Instance().w(), CV_8U);
	UMat classPixelMask; 

#pragma region Threshold
	if((op & Threshold) == Threshold) {
		Rect RectRoi;
		UMat imgRoi;
		UMat circleMask;  // only for circle

		// add 0.5 and use floor as there seems not to be a proper round function 
		int thresh_r = floor(params.R * 255 + 0.5);
		int thresh_g = floor(params.G * 255 + 0.5);
		int thresh_b = floor(params.B * 255 + 0.5);

		if(params.roi_shape == RectangleD) {
			RectRoi = Rect(cv::Point2i(params.roi_Points[0], params.roi_Points[1]), // left, top 
						   cv::Point2i(params.roi_Points[2], params.roi_Points[3])); // right, down
			keepRectInBounds(RectRoi, width, height);
			if (RectRoi.width == 0) RectRoi.width = 1;
			if (RectRoi.height == 0) RectRoi.height = 1;
 
			// Work on the current Region Only
			imgRoi = img(RectRoi).clone();

			if(params.isHSV) {
				int low_h = floor(params.H * 180 + 0.5);
				int low_s = floor(params.S * 255 + 0.5);
				int low_v = floor(params.V * 255 + 0.5);
				UMat imgRoiHSV;
				// Convert from BGR to HSV colorspace
				cvtColor(imgRoi, imgRoiHSV, COLOR_BGR2HSV);

				// Detect the object based on HSV Range Values
				inRange(imgRoiHSV, Scalar(low_h, low_s, low_v),
						Scalar(floor(params.up_HorR * 180 / 255 + 0.5), params.up_SorG,
						params.up_VorB), classPixelMask);
			} else {
				// apply segmentation to the image region
				inRange(imgRoi, Scalar(thresh_b, thresh_g, thresh_r),
						Scalar(params.up_VorB, params.up_SorG, params.up_HorR),
						classPixelMask);
			}
			if((op & FillMask) == FillMask)
				fill_mask(classPixelMask);

			// save the classPixelMask correctly to the temporary mask
			// this mask is the size of the complete image and can be added by the
			// user to the classes segmentation result (pressing 'A' button)
			classPixelMask.copyTo(tempMask(RectRoi));

			// Color to display the results
			Vec3b col = colors2.at(LabelState::Instance().GetActiveClass());
			Scalar classColor = Scalar(col[2], col[1], col[0]);

			// Alt: UMat classRegion = UMat(imgRoi); UMat classRegion = imgRoi; 
			// invalid --> both still reference and change imgRoi !!
			UMat classRegion(
				imgRoi.rows, imgRoi.cols, CV_8UC3,
				Scalar(0, 0, 0));  // displayed result img is a bit darker in the ROI!

			imgRoi.copyTo(classRegion);  // prevents the darkening in the blended ROI

			// apply closing with circle size defined in gui
			if(LabelState::Instance().FillRegion ==
			   true) {  // just toggle filling with normal thresholding

				// Get region
				UMat im_in = classPixelMask;
				
				// use morphilogic operators
				//cv::Mat tempMat = getStructuringElement(
				//	MORPH_ELLIPSE, Size(LabelState::Instance().FillSize, LabelState::Instance().FillSize));
				cv::UMat erodeCirc1 = getStructuringElement(
					MORPH_ELLIPSE, Size(LabelState::Instance().FillSize, LabelState::Instance().FillSize)).getUMat(ACCESS_RW);

				UMat im_morphed;
				morphologyEx(im_in, im_morphed, MORPH_CLOSE, erodeCirc1);
				morphologyEx(im_morphed, im_morphed, MORPH_OPEN, erodeCirc1); 
				classPixelMask = im_morphed;
			}  // END closing

			classRegion.setTo(classColor, classPixelMask);
			addWeighted(classRegion, params.alpha_display, imgRoi,
						(1.0 - params.alpha_display), 0.0, imgRoi);

		} else if(params.roi_shape == BrushD) {

			// imRoi has to be set to the whole image (maybe later check if faster to reduce according to the painted points)
			RectRoi = Rect(cv::Point2i(0, 0),
						   cv::Point2i(img.cols, img.rows));
			imgRoi = img.clone();

			// sort and see if time is better
			std::vector<PointRad> vec_sorted = params.PnR;
			std::stable_sort(params.PnR.begin(), params.PnR.end(),
							 compare_only_second);  // works, but might be not neccessary

			classPixelMask = UMat::zeros(img.size(), CV_8U);
			for(auto p : params.PnR) {
				circle(classPixelMask, Point(p.pt.x, p.pt.y), p.rad, (255), -1);
			}

			// save the current segmentation results temporarly
			classPixelMask.copyTo(tempMask);

			// Color to display the results
			Vec3b col = colors2.at(LabelState::Instance().GetActiveClass());
			Scalar classColor = Scalar(col[2], col[1], col[0]);

			// create a 3-channel black image
			UMat classRegion( imgRoi.rows, imgRoi.cols, CV_8UC3,
				Scalar(0, 0, 0));  // displayed result img is a bit darker in the ROI!
			// copy the roi part of the image to the matrix to prevent darkening of the result after blending
			imgRoi.copyTo(classRegion);

			classRegion.setTo(classColor, classPixelMask);
			addWeighted(classRegion, params.alpha_display, imgRoi,
						(1.0 - params.alpha_display), 0.0, imgRoi);

		}

		else if(params.roi_shape == CircleD || params.roi_shape == PolygonD) {
			if(params.roi_shape == CircleD) {
				// get the Rect arround the circle
				int radius = sqrt(pow(params.roi_Points[2] - params.roi_Points[0], 2) +
								  pow(params.roi_Points[1] - params.roi_Points[3], 2));
				RectRoi = Rect(params.roi_Points[0] - radius,
							   params.roi_Points[1] - radius, radius * 2, radius * 2);

				// make sure the boundaries are not overshot
				int deviation_x = 0; int deviation_y = 0;
				if(RectRoi.x < 0) {
					deviation_x = deviation_x - RectRoi.x; // also adjust with or else the whole circle is pushed right
					RectRoi.x = 0;
				}
				if(RectRoi.y < 0) {
					deviation_y = deviation_y - RectRoi.y; // also adjust with or else the whole circle is pushed down
					RectRoi.y = 0;
				}
				if(RectRoi.x + RectRoi.width > img.cols)
					RectRoi.width = img.cols - RectRoi.x;
				if(RectRoi.y + RectRoi.height > img.rows)
					RectRoi.height = img.rows - RectRoi.y;
				// obtain the image ROI:
				imgRoi = img(RectRoi).clone();  // alt: UMat roi(img, RectRoi);

				// make a black mask, same size:
				UMat circleMask(imgRoi.size(), imgRoi.type(), Scalar::all(0));
				// with a white, filled circle in it (adjusting for deviation of the mid point)
				circle(circleMask, Point(radius - deviation_x, radius - deviation_y), radius, Scalar::all(255), -1);

				// DS note: use the whole roi and reduce the region to the circle later! 
				// apply segmentation to the image region
				inRange(imgRoi, Scalar(thresh_b, thresh_g, thresh_r),
						Scalar(params.up_VorB, params.up_SorG, params.up_HorR),
						classPixelMask);
				// reduce the mask to one channel - to work for classPixelMask
				extractChannel(circleMask, circleMask, 0);
				// reduce the class pixel mask to the cirlce roi only - else black values would also be taken
				//classPixelMask = (classPixelMask & circleMask); // is only overloaded for cv:Mat (not UMat)
				cv::bitwise_and(classPixelMask, circleMask, classPixelMask);

			} else { // Polygon
				vector<Point> cvPts;
				for(auto p : params.poly_Points) {
					Point cvP = Point(p.first, p.second);
					cvPts.push_back(cvP);
				}
				// create a binary mask of the polygons area
				UMat polyMask = UMat::zeros(img.size(), CV_8U);
				//UMat polyMask = UMat::zeros(img.size(), CV_8UC3);
				fillPoly(polyMask, cvPts, Scalar::all(255));

				// get the smallest bounding rectangle 
				Rect boundRect = boundingRect(cvPts);
				RectRoi = boundRect;
				// 1/24 DS: when evaluating while dragging the outmost border may be slightly outside the img size
				keepRectInBounds(RectRoi, width, height);

				// obtain the image ROI:
				imgRoi = img(RectRoi).clone();

				polyMask = polyMask(RectRoi);
				// apply segmentation to the image region
				inRange(imgRoi, Scalar(thresh_b, thresh_g, thresh_r),
						Scalar(params.up_VorB, params.up_SorG, params.up_HorR),
						classPixelMask);

				// reduce the class pixel mask to the cirlce roi only - else black values would also be taken
				// classPixelMask = (classPixelMask & polyMask);
				cv::bitwise_and(classPixelMask, polyMask, classPixelMask);
			}

			if((op & FillMask) == FillMask)
				fill_mask(classPixelMask);

			// save the current segmentation results temporarly
			classPixelMask.copyTo(tempMask(RectRoi));

			// Color to display the results
			Vec3b col = colors2.at(LabelState::Instance().GetActiveClass());
			Scalar classColor = Scalar(col[2], col[1], col[0]);

			UMat classRegion(
				imgRoi.rows, imgRoi.cols, CV_8UC3,
				Scalar(0, 0, 0));  // displayed result img is a bit darker in the ROI!

			imgRoi.copyTo(classRegion);  // prevents the darkening in the blended ROI

			classRegion.setTo(classColor, classPixelMask);
			addWeighted(classRegion, params.alpha_display, imgRoi,
						(1.0 - params.alpha_display), 0.0, imgRoi);

			// imgRoi = imgRoi | (Mask & roi);
		}

		// implementation of watershed on whole image
		else if(params.roi_shape == MarkerPointsD) {

			// DS: either use polygon points for watershed or just the center of a rectangle! --> + Mouse key
			RectRoi = Rect(0, 0, img.cols, img.rows);
			imgRoi = img.clone();
			UMat markers = UMat::zeros(imgRoi.size(), CV_32S);

			for(auto m_tuple : params.markers) {
				// start counting the classes at 1, because watershed does count 0 as nothing
				circle(markers, Point(m_tuple.x, m_tuple.y), 10, Scalar(m_tuple.activeClass + 1));
			}
			watershed(imgRoi, markers);  

#pragma region UsingUMatWS

			// Create masks based on conditions
			cv::UMat mask1, mask2, mask3;
			cv::compare(markers, -1, mask1, cv::CMP_EQ);
			cv::UMat condition2a, condition2b;
			cv::compare(markers, 0, condition2a, cv::CMP_LE);
			cv::compare(markers, 500, condition2b, cv::CMP_GT);
			cv::bitwise_or(condition2a, condition2b, mask2);

			cv::bitwise_or(mask1, mask2, mask3);
			cv::bitwise_not(mask3, mask3); // Invert to get the third condition

			// Apply conditions to imgRoi using masks
			imgRoi.setTo(cv::Scalar(255, 255, 255), mask1); 
			imgRoi.setTo(cv::Scalar(0, 0, 0), mask2);

			// For the third condition, we need to loop over the unique indices 
			// in markers since we can't vectorize the color lookup from colors2. 
			cv::Mat uniqueIndices = markers.getMat(cv::ACCESS_READ).reshape(1, 1); // Flatten and convert to Mat for unique operation
			std::vector<int> uniqueVec;
			uniqueVec.reserve(uniqueIndices.total());
			for(int i = 0; i < uniqueIndices.total(); i++) {
				int val = uniqueIndices.at<int>(i);
				if(std::find(uniqueVec.begin(), uniqueVec.end(), val) == uniqueVec.end())
					uniqueVec.push_back(val);
			}

			for(int val : uniqueVec) {
				if(val > 0 && val <= 500) {
					cv::UMat currentMask, combinedMask;
					cv::compare(markers, val, currentMask, cv::CMP_EQ);

					cv::Mat DBG_currentMask = currentMask.getMat(ACCESS_READ); 
					cv::bitwise_and(currentMask, mask3, combinedMask);
					// get the right class color
					Vec3b col = colors2.at(val -1);
					// Display with inverted color
					Vec3b invertedCol = Vec3b(255, 255, 255) - col; // Invert the color
					imgRoi.setTo(cv::Scalar(invertedCol[2], invertedCol[1], invertedCol[0]), combinedMask); // notice BGR 
					tempMask.setTo(val - 1, combinedMask);
				}
			}

#pragma endregion UsingUMatWS

			// Blend image with resulting class colors
			addWeighted(imgRoi, params.alpha_display, img,
						(1.0 - params.alpha_display), 0.0, imgRoi);
		}

		// Display Image to result image in GUI - and else the original image would be changed
		UMat imgToDisplay = img.clone();
		// copy result in ROI to Display image and add alpha
		imgRoi.copyTo(imgToDisplay(RectRoi));
		cv::cvtColor(imgToDisplay, image_rgba, cv::COLOR_BGR2RGBA);

	}
#pragma endregion Threshold


	if((op & Floodfill) == Floodfill) {

		Rect RectRoi;
		Mat polyMask;
		if(params.roi_shape == RectangleD) { 
			RectRoi = Rect(cv::Point2i(params.roi_Points[0], params.roi_Points[1]), // left, top 
						   cv::Point2i(params.roi_Points[2], params.roi_Points[3])); // right, down
		} else if(params.roi_shape == PolygonD) {
			vector<Point> cvPts;
			for(auto p : params.poly_Points) {
				Point cvP = Point(p.first, p.second);
				cvPts.push_back(cvP);
			}
			// create a binary mask of the polygons area
			polyMask = Mat::zeros(img.size(), CV_8U);
			fillPoly(polyMask, cvPts, Scalar::all(255));
			// get the smallest bounding rectangle 
			RectRoi = boundingRect(cvPts);

			// obtain the image ROI: --> is done below  
			polyMask = polyMask(RectRoi);
		}
		// check that rect is valid
		keepRectInBounds(RectRoi, width, height);

		UMat imgRoi = img(RectRoi).clone();

		// ff parameters
		Point seed;
		// check that mouse point is in rectangle - else use middle 
		if(params.ff.f_point.x > RectRoi.x && params.ff.f_point.x < RectRoi.x + RectRoi.width // x mouse coord is inside of the rectangle ROI
		   && params.ff.f_point.y> RectRoi.y && params.ff.f_point.y < RectRoi.y + RectRoi.height) // y mouse coord inside rect
			// Seed is the point inside the cropped ROI --> adjust the point by subtracting the offset
			seed = Point(params.ff.f_point.x - RectRoi.x, params.ff.f_point.y - RectRoi.y);
		else
			seed = Point(RectRoi.width / 2, RectRoi.height / 2);

		/*int ffillMode = 0; // Simple floodfill
						= 1; // Fixed Range floodfill mode
						= 2; // Gradient (floating range) floodfill mode
						= 4; // 4-connectivity mode
						= 8; // 8-connectivity mode */
		int ffillMode = params.ff.current_fillMode;
		int loDiff = params.ff.low;
		int upDiff = params.ff.up;
		int lo = ffillMode == 0 ? 0 : loDiff;
		int up = ffillMode == 0 ? 0 : upDiff;
		Rect ccomp;	// Optional output parameter set by the function to the minimum bounding rectangle of the repainted domain.
		int connectivity = 4;
		int isColor = !params.ff.use_gray_img;
		bool useMask = false;
		int newMaskVal = 255;

		UMat gray;
		if(params.ff.use_gray_img)
			cvtColor(imgRoi, gray, COLOR_BGR2GRAY);
		UMat dst = isColor ? imgRoi.clone() : gray.clone(); // - DS: needs clone ! 

		int a = newMaskVal << 8;
		int asdf = connectivity;

		int flags = connectivity + (newMaskVal << 8) + (ffillMode == 1 ? FLOODFILL_FIXED_RANGE : 0);
		/*	Operation flags.The first 8 bits contain a connectivity value.The default value of
				4 means that only the four nearest neighbor pixels(those that share an edge) are considered.A
				connectivity value of 8 means that the eight nearest neighbor pixels(those that share a corner)
				will be considered.
				The next 8 bits(8 - 16) contain a value between 1 and 255 with which to fill
				the mask(the default value is 1).
				The other two FloodFillFlags are  FLOODFILL_FIXED_RANGE = 65536 (2^16) ; FLOODFILL_MASK_ONLY = 2^17  */

		/*	int b = (unsigned)theRNG() & 255;
				int g = (unsigned)theRNG() & 255;
				int r = (unsigned)theRNG() & 255;*/

		Vec3b col = colors2.at(LabelState::Instance().GetActiveClass());
		//Scalar newVal = !params.ff.use_gray_img ? Scalar(col[2], col[1], col[0]) : Scalar(col[2] * 0.299 + col[1] * 0.587 + col[0] * 0.114);
		Scalar colBGR = Scalar(col[2], col[1], col[0]);

		int area;
		UMat mask = UMat::zeros(Size(dst.cols + 2, dst.rows + 2), CV_8U);
		if(params.ff.use_gray_img) {
			area = floodFill(dst, mask, seed, Scalar(255), &ccomp, Scalar(lo),// use white for gray image?
							 Scalar(up), flags);
			classPixelMask = mask(Rect(1, 1, imgRoi.cols, imgRoi.rows));

		} else {
			//area = floodFill(dst, seed, newVal, &ccomp, Scalar(lo, lo, lo),
			//	Scalar(up, up, up), flags); // without mask

			//flags |= FLOODFILL_MASK_ONLY; // does not draw into dst image 
			//Docu: Operation mask that should be a single - channel 8 - bit image, 2 pixels wider and 2 pixels taller than image.Since this is both an input and output parameter, you must take responsibility of initializing it
			area = floodFill(dst, mask, seed, colBGR, &ccomp, Scalar(lo, lo, lo),
							 Scalar(up, up, up), flags);
			// taking the center seems to be exactly right - because the mat is 2 bigger in each axis 
			classPixelMask = mask(Rect(1, 1, imgRoi.cols, imgRoi.rows));

		}

		// apply filling to the resulting mask
		if((op & FillMask) == FillMask)
			fill_mask(classPixelMask);

		// limit to polygon shape for polygon input
		if(params.roi_shape == PolygonD) {
			// optional operation mask, 8 - bit single channel array, that specifies elements of the output array to be changed.
			cv::bitwise_and(classPixelMask, polyMask, classPixelMask);
		}

		imgRoi.setTo(colBGR, classPixelMask);

		// save the classPixelMask correctly to the temporary mask
		// this mask is the size of the complete image and can be added by the
		// user to the classes segmentation result (pressing 'A' button)
		classPixelMask.copyTo(tempMask(RectRoi));

		UMat imgToDisplay = img.clone();
		// copy result in ROI to Display image and add alpha
		imgRoi.copyTo(imgToDisplay(RectRoi));
		cv::cvtColor(imgToDisplay, image_rgba, cv::COLOR_BGR2RGBA);
	} 
	else if( op == GrabCut) {
		// 25.1.24 DS: GrabCut is not implemented with UMat as of opencv 4.6.0 (asserts type=Mat for the input parameters)

		int distance_from_border = 10; 
		// init rect for default INIT_WITH_RECT flag (must be smaller then whole image)
		Rect RectRoi = Rect(cv::Point(0, 0), cv::Point(width, height)); 
	
		// Initialize mask
		// creating complete mask as potential BG works only with the GC_INIT_WITH_RECT is used with the input mask (NOT GC_INIT_WITH_MASK)
		// cv::Mat mask(img.size(), CV_8UC1, cv::Scalar(cv::GC_PR_BGD));	
		cv::Mat mask(img.size(), CV_8UC1, cv::Scalar(cv::GC_BGD));	
		
		// get extension of the drawn lines
		cv::Point minPoint(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
		cv::Point maxPoint(std::numeric_limits<int>::min(), std::numeric_limits<int>::min());
		int minXRad = 8, minYRad = 8, maxXRad = 8, maxYRad = 8;
		for(const auto& pR : params.PnR) {
			auto P = cv::Point(pR.pt.x, pR.pt.y);
			if(P.x < minPoint.x) { minPoint.x = P.x; minXRad = pR.rad; }
			if(P.y < minPoint.y) { minPoint.y = P.y; minYRad = pR.rad; }
			if(P.x > maxPoint.x) { maxPoint.x = P.x; maxXRad = pR.rad; }
			if(P.y > maxPoint.y) { maxPoint.y = P.y; maxYRad = pR.rad; }
		}
		
		// using bounding rect around drawn points 
		RectRoi = { minPoint.x - minXRad , minPoint.y - minYRad, 
			(maxPoint.x + maxXRad)-(minPoint.x - minXRad), (maxPoint.y + maxYRad) - (minPoint.y-minYRad)};
 
		keepRectInBounds(RectRoi, width, height);
		cv::Mat mask_inner(RectRoi.size(), CV_8UC1, cv::Scalar(cv::GC_PR_BGD));
		mask_inner.copyTo(mask(RectRoi)); 

		// add all the stroked pixels to the corresponding value in the mask
		int countFG = 0; 
		for(const auto& pR : params.PnR) {
			uchar value; 
			if(pR.foreground) { 
				value = cv::GC_FGD; 
				countFG++;
			}
			else value = cv::GC_BGD;

			auto P = cv::Point(pR.pt.x, pR.pt.y); 
			cv::circle(mask, P, pR.rad, cv::Scalar(value), cv::FILLED); 
		}
		std::cout << " FG:" << countFG <<  " CV points added \n";
		// make sure User does not do shit - at least one FG point must exist (not only BG)
		if(countFG == 0) {
			cv::circle(mask, cv::Point(RectRoi.x+RectRoi.width/2, RectRoi.y+RectRoi.height/2), 10, cv::Scalar(cv::GC_FGD), cv::FILLED);
		}

		// Create foreground and background models
		cv::Mat bgdModel, fgdModel; 
		//UMat imgRoi = img(RectRoi).clone();
		cv::Mat result = img.getMat(ACCESS_RW).clone();
		//Note: iterCount (1) Number of iterations the algorithm should make before returning the result. Result can be refined with further calls with mode==GC_INIT_WITH_MASK or mode==GC_EVAL 
		//cv::grabCut(img, mask, RectRoi, bgdModel, fgdModel, 1, cv::GC_INIT_WITH_RECT); // input rect works with RectRoi
		cv::grabCut(result, mask, RectRoi, bgdModel, fgdModel, 1, cv::GC_INIT_WITH_MASK ); // combining flags does not work as Documentation says!!


		Vec3b col = colors2.at(LabelState::Instance().GetActiveClass());
		Scalar colorBGR = Scalar(col[2], col[1], col[0]);

		// Blend images by Iterate over each pixel - CPU Time: ~303ms 
		//for(int y = 0; y < img.rows; y++) {
		//	for(int x = 0; x < img.cols; x++) {
		//		// Check if the current mask pixel matches the condition
		//		if(mask.at<uchar>(y, x) == cv::GC_PR_FGD || mask.at<uchar>(y, x) == cv::GC_FGD) {
		//			// Blend the highlight color with the original pixel
		//			cv::Vec3b& pixel = result.at<cv::Vec3b>(y, x);
		//			for(int c = 0; c < 3; c++) {
		//				pixel[c] = (pixel[c] + col[c]) / 2;
		//			}
		//		}
		//	}
		//}	
		
		// DS: this could be used 
		// cv::Mat result(img.size(), CV_8UC3, cv::Scalar(0, 0, 0));
		// img.copyTo(result, (mask == cv::GC_PR_FGD) | (mask == cv::GC_FGD)); // needs probable FG as results (FG seems to be only the input)	
	
		//UMat classRegion = result.clone().getUMat(ACCESS_FAST); 		 ;

		Mat classRegion = result.clone(); 	
		Mat mask_FG = (mask == cv::GC_PR_FGD) | (mask == cv::GC_FGD); 
		classRegion.setTo(colorBGR, mask_FG);
		addWeighted( classRegion, params.alpha_display, result,    //CPU Time ~105ms
					(1.0 - params.alpha_display), 0.0, result);

		// save the classPixelMask (where mask is Forground)  to the temporary mask 
		//mask_FG.copyTo(tempMask(RectRoi), mask_FG);
		mask_FG.copyTo(tempMask);

		UMat imgToDisplay = result.getUMat(ACCESS_FAST);
		cv::cvtColor(imgToDisplay, image_rgba, cv::COLOR_BGR2RGBA);

	} else if((op & DisplayClass) == DisplayClass) {
		bool completeImage = false;
		if(completeImage) {
			// A) Display of the resulting mask incl. alpha in the class color over the whole image
			UMat imgToDisplay = img.clone();  // imgToDisplay because else the original image is changed

			// get color and region for class 
			Vec3b col = colors2.at(LabelState::Instance().GetActiveClass());
			Scalar color = Scalar(col[2], col[1], col[0]);
			UMat classPixelMask = LabelState::Instance().GetActiveClassRegion();

			UMat classRegion = img.clone();
			classRegion.setTo(color, classPixelMask);			
			addWeighted(imgToDisplay, params.alpha_display, classRegion,
						(1.0 - params.alpha_display), 0.0, imgToDisplay);

			/* // this only changes the color of the displayed class region (from black to full colored) --> round about 6-9ms
			color *= params.alpha_display;
			imgToDisplay.setTo(color, LabelState::GetActiveClassRegion());*/

			cv::cvtColor(imgToDisplay, image_rgba, cv::COLOR_BGR2RGBA);

		} else {
			// B) use a bounding box arround the mask to get the max extension - only faster if area is way smaller
			// than the whole image - maybe because of the found Zero-Points (e.g. 364718 ) 
			UMat imgToDisplay = img.clone();
			UMat Points;
			UMat classPixelMask = LabelState::Instance().GetActiveClassRegion();
			findNonZero(classPixelMask, Points);
			Rect Min_Rect = boundingRect(Points);
			if(Points.empty())  Min_Rect = { 0, 0, img.cols, img.rows }; 

			// obtain the image ROI:
			UMat imgRoi = img(Min_Rect).clone();
			// get color and region for class 
			Vec3b col = colors2.at(LabelState::Instance().GetActiveClass());
			Scalar colorBGR = Scalar(col[2], col[1], col[0]);

			UMat classRegion = imgRoi.clone();
			// set all pixels of the classRegion image ROI to the color of the class
			classRegion.setTo(colorBGR, classPixelMask(Min_Rect));
			// in the ROI blend the original image and the one with the pixel mask 
			addWeighted(classRegion, params.alpha_display, imgRoi,
						(1.0 - params.alpha_display), 0.0, imgRoi);

			imgRoi.copyTo(imgToDisplay(Min_Rect));
			cv::cvtColor(imgToDisplay, image_rgba, cv::COLOR_BGR2RGBA);
		}
	} else if((op & DisplayAllClasses) == DisplayAllClasses) {

		UMat imgToDisplay = img.clone();
		int num_classes = LabelState::Instance().MasksSize() - 1;
		UMat classImg = UMat::zeros(LabelState::Instance().h(), LabelState::Instance().w(), CV_8UC3);
		Rect minRect;

		for(int i = num_classes; i >= 0; i--) {

			UMat region = LabelState::Instance().GetClassRegion(i);
			if(region.empty() || countNonZero(region) == 0) continue; // skip empty or complete black class

			Vec3b classColor = colors2.at(i);
			Scalar newVal = Scalar(classColor[2], classColor[1], classColor[0]);
			UMat col_region = UMat::zeros(LabelState::Instance().h(), LabelState::Instance().w(), CV_8UC3);
			col_region.setTo(newVal, region);
			// TODO: try optimizing - save topmost and bottom right point for using minRect
			//UMat Points;
			//findNonZero(region, Points);
			
			//classImg |= col_region; // not for UMat
			cv::bitwise_or(classImg, col_region, classImg);
		}
		bool speed_optim = false;
		if(speed_optim) { 
			// TODO: find fast solution - maybe calculate the bounding rect from the for loop above
			UMat region;
			threshold(classImg, region, 0, 255, THRESH_BINARY);
			UMat Points;
			findNonZero(region, Points); // check if faster  
			Rect minRect = boundingRect(classImg);
			if(minRect.empty() || minRect.area() == 0) {
				minRect = { 0, 0, img.cols, img.rows };
			}
			UMat blend = imgToDisplay(minRect).clone();

			// set all pixels of the classRegion image ROI to the color of the class
			classImg(minRect).copyTo(blend);

			//blend.setTo(color, classPixelMask(Min_Rect));
			// in the ROI blend the original image and the one with the pixel mask 
			addWeighted(blend, params.alpha_display, classImg,
						(1.0 - params.alpha_display), 0.0, blend);
 
			blend.copyTo(imgToDisplay(minRect));

			//TESTING
			//imgToDisplay = classImg | (imgToDisplay & ~classImg);
			//classImg.copyTo(imgToDisplay); 
		} else {
 
			// this makes the original image darker, the lower the alpha value is
			addWeighted(classImg, params.alpha_display, imgToDisplay,
						(1.0 - params.alpha_display), 0.0, imgToDisplay); // around 120ms
		}

		cv::cvtColor(imgToDisplay, image_rgba, cv::COLOR_BGR2RGBA);
		std::cout << "display all classes ";
	}
	else if ( (op & Clear) == Clear) { // e.g. op == Clear for reseting displayed image

		UMat imgToDisplay = img.clone();
		cv::cvtColor(imgToDisplay, image_rgba, cv::COLOR_BGR2RGBA);
	}
	// Note: no need to Stop() the timer here, as the object gets destroyed at the end of the function
	return image_rgba;
}


int addMaskToClassregion(bool overwriteOtherClasses, bool setCompleteMask, bool multiplePixelLabels) {
	// for watershed etc. set the complete resulting class masks
	if(setCompleteMask) {
		return LabelState::Instance().setSegmentationMasks(tempMask, true);
	}
	// add the active class mask to the labels
	else
		return LabelState::Instance().addRegionToClass(tempMask, overwriteOtherClasses, multiplePixelLabels);
}


// select the classColor form input point/pixel 
bool pickColor(ImVec2 pixel, float* color) {
	UMat img = LabelState::Instance().GetCurrentImg();
	if(pixel.x > img.cols || pixel.x <= 0 || pixel.y > img.rows || pixel.y <= 0) return false;

	//Vec3b col_pixel = img.at<Vec3b>(Point(pixel.x, pixel.y));  // 24,39, 64 (bgr) // not for UMat
	cv::Mat cpuImg = img.getMat(cv::ACCESS_READ);
	Vec3b col_pixel = cpuImg.at<Vec3b>(Point(pixel.x, pixel.y));  // 24,39, 64 (bgr)

	// alt: BGR& bgr = image.ptr<BGR>(y)[x];
	*color = col_pixel[2] / 255.f;
	*(color + 1) = col_pixel[1] / 255.f;
	*(color + 2) = col_pixel[0] / 255.f;

	return true;
}
