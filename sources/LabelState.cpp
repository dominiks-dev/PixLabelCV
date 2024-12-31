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
#include "opencv2/imgproc.hpp"
#include "opencv2/core.hpp"
#include "Timer.h"
#include "fstream"
#include "opencv2/core.hpp" 
#include "opencv2/imgcodecs.hpp"  
#include <filesystem>
using namespace cv;
namespace fs = std::filesystem;

int LabelState::tryloadmask(std::string mask_path) {

	if(!fs::exists(mask_path)) {
		return -1;
	}

	UMat mask = imread(mask_path, IMREAD_GRAYSCALE).getUMat(cv::ACCESS_READ);

	if(mask.empty())
		return -2;

	// check that img has only one channel - else take the first channel
	if(mask.channels() > 1)
		extractChannel(mask, mask, 0);

	double max, min;
	minMaxIdx(mask, &min, &max);
	int max_class = static_cast<int>(max);
	//minMaxLoc(mask, &min, &max_class, &dummy, &maxLoc); 
	//UMat m_empty = UMat::zeros(height, width, CV_8U);
	//auto ffa = new std::vector<UMat>(static_cast<int>(max_class), m_empty);

	// clear the masks
	ClearState();

	std::vector<UMat> labelM;
	for(int i = max_class; i >= 0; i--) { // from high to low just for fun
		cv::UMat classI;
		//threshold(mask, classI, i, 255, CV_8U); // = mask;
		inRange(mask, i, i, classI);

		labelM.insert(labelM.begin(), classI);
	}
	pushState(labelM);
	//delete ffa;  
// }
	return 0;
}


// Tries to load seperate files (checks if the folders and img-file exist first)
int LabelState::tryLoadSeperateMasks(std::string mask_folder, std::string mask_name_postfix) {

	const int max_classes = 30;
	std::vector<UMat> temp_Mask;

	for(int i = 0; i < max_classes; ++i) {
		std::string subfolderName = mask_folder + "/" + std::to_string(i);
		// search in subfolder for each class
		if(fs::exists(subfolderName) && fs::is_directory(subfolderName)) {
			std::string maskFileName = mask_name_postfix;

			for(const auto& entry : fs::directory_iterator(subfolderName)) {
				if(entry.is_regular_file()) {
					auto filename = entry.path().filename().string();
					// Check for the mask with the right name
					if(fs::exists(filename) &&
					   (filename == maskFileName + ".png" ||
					   filename == maskFileName + std::to_string(i) + ".png")) {

						Mat classMask = cv::imread(entry.path().string(), IMREAD_GRAYSCALE);
						// load it into the temporary label first
						temp_Mask.push_back(classMask.clone().getUMat(ACCESS_FAST));
					}
				}
			}

		}
	}

	if(temp_Mask.size() == 0)
		// if no mask could be loaded create a dummy one
		for(int i = 0; i < 3; i++)
			temp_Mask.push_back(cv::UMat::zeros(height, width, CV_8U));

	pushState(temp_Mask);

	//Mat DGB_loadedMask = temp_Mask[0].getMat(ACCESS_READ);
	return 0;
}


cv::Mat LabelState::load_new_image(std::string img_path, const std::string mask_path, bool load_mask) {

	Mat Copy = cv::imread(img_path);
	//currentImg = cv::imread(img_path).getUMat(cv::ACCESS_FAST);
	currentImg = Copy.clone().getUMat(cv::ACCESS_FAST);
	//Mat DBG_Img = currentImg.getMat(cv::ACCESS_READ);

	if(!currentImg.empty()) {
		width = currentImg.size[1];
		height = currentImg.size[0];
		activeClass = 1;
		//initialize vector
		int could_load_mask = -47;
		if(load_mask)
			could_load_mask = tryloadmask(mask_path); // does clear state when masks exist

		// init mask if it does not exist or could not be loaded
		if(could_load_mask != 0) {
			// DS: 26.1.24 seems to be needed when loading new image(s) with loadTextureFromFile() fct.  
			// DS: 7.2.24 switched to clear Masks
			// labeledMasks().clear(); 
			ClearState();
			std::vector<UMat> temp_Mask;
			for(int i = 0; i < 3; i++) //start with 3 as default
				//labeledMasks().push_back(cv::UMat::zeros(height, width, CV_8U));
				temp_Mask.push_back(cv::UMat::zeros(height, width, CV_8U));
			pushState(temp_Mask);
		}
	}
	return Copy;
}


// saves the semantic segmentation result in a image
// it has to be checked before that the directory to save in does exist!
int LabelState::saveLabels(const std::string singleMaskPath, bool seperateImages) {

	if(labeledMasks().size() == 0) return -2;

	// 11.2.24 DS: save seperate mask files 
	if(seperateImages == true) {
		try {
			UMat backgroundMask = UMat::ones(height, width, CV_8U); // TODO: check if Mat would be faster than UMat

			for(int i = 1; i <= labeledMasks().size() - 1; i++) {
				cv::UMat classI = labeledMasks()[i];

				// Check if the mask contains any value greater than 0
				if(cv::countNonZero(labeledMasks()[i]) > 0) {
					fs::path maskClassFolder = fs::path(singleMaskPath).parent_path() / std::to_string(i);
					fs::create_directories(maskClassFolder);

					// Prepare the mask for saving: Ensure it's 8-bit binary image
					cv::Mat binaryMask;
					labeledMasks()[i].convertTo(binaryMask, CV_8U);
					// cv::threshold(binaryMask, binaryMask, 0, 255, cv::THRESH_BINARY);// works - but should also work without

					// Construct the filename
					fs::path filePath = fs::path(maskClassFolder) / (fs::path(singleMaskPath).stem().string() +
																	 std::to_string(i) + ".png");

					// Save the binary mask
					cv::imwrite(filePath.string(), binaryMask);

					//calculate Background as class 0 
					bitwise_xor(backgroundMask, classI, backgroundMask);
				}
			}
			// Invert backgroundMask to get the final background mask (0 where any mask is non-zero)
			cv::bitwise_not(backgroundMask, backgroundMask);

			// Save the background mask
			fs::path backgroundFolder = fs::path(singleMaskPath).parent_path() / "0";
			fs::create_directories(backgroundFolder);
			cv::imwrite((backgroundFolder / fs::path(singleMaskPath).stem()).string() + ".png", backgroundMask);
		}
		catch(std::exception& e) {
			std::cout << e.what() << "\n";
			return -1;
		}
	}
	// create a mask with the class values from all the binary masks
	// DS 2.8.23 switch so higher classes have highter priority! - Due to BUG: Adding to class 1 also inner region which was already labeled as class 2
	else {
		cv::UMat labelImg = cv::UMat::zeros(height, width, CV_8U);
		for(int i = 1; i <= labeledMasks().size() - 1; i++) {
			cv::UMat classI = labeledMasks()[i];

			// Alt: get min and max vals of UMat
			//double minVal, maxVal; 
			//Point minLoc, maxLoc; 
			//minMaxLoc(classI, &minVal, &maxVal, &minLoc, &maxLoc);		
			//if (!classI.empty() && maxVal!=0.0) {
			if(countNonZero(classI) != 0) {
				UMat src = UMat::ones(height, width, CV_8U);
				cv::multiply(src, Scalar(i), src); // *= i does not work for UMat
				//bitwise_and( src, classI, labelImg);
				//cv::copy(labelImg, src, classI);
				labelImg.setTo(i, classI);
			}
		}

		try {
			std::string imgname = singleMaskPath;

			if(singleMaskPath.substr(singleMaskPath.find_last_of('.'), singleMaskPath.size()) != ".png") {
				imgname = singleMaskPath + ".png";
			}
			//imgname = "out.png";
			bool result = cv::imwrite(imgname, labelImg);
			auto a = result;

		}
		catch(std::exception& e) {
			//show message box with error
			return -1;
			//std::cout << e.what() << "\n";
		}

	}
	return 0;
}


bool LabelState::ChangeActiveClass(int class_number) {
	if(class_number < 0 || class_number > 19) return false;
	activeClass = class_number;
	// add mask regions to the result, if there are not enough yet
	if(labeledMasks().size() < class_number + 1) {
		// class 0 is background so we need one more
		while(labeledMasks().size() < class_number + 1) {
			labeledMasks().push_back(cv::UMat::zeros(height, width, CV_8U));
		}
	}
	return true;
}


int LabelState::addRegionToClass(cv::UMat newRegion, bool overwrite_existing, bool multiplePixelLabelsAllowed) {

	// check that mask exists
	if(GetCurrentState().empty()) {
		return -2;
	}
	if(newRegion.empty()) return -3;
	// start timer
	Timer timer1 = Timer();

	// get a copy of the current labelMask
	auto newState = CopyCurrentState();

	// check which masks exist and are not empty
	for(int i = 0; i < newState.size(); i++) {
		if(i == activeClass) continue; // skip current class for performance 

		UMat classMask = newState.at(i);
		UMat intersection;
		cv::bitwise_and(classMask, newRegion, intersection);
		std::vector<cv::Point2i> locations;
		// get locations of non-zero pixels in the intersection of the current class and the new 
		//cv::findNonZero(intersection, locations); // TODO: remove if possible
		int non_zeros = cv::countNonZero(intersection);

		// check if there are any points (pixels) in the i-th mask that are not zero
		if(non_zeros > 0) {
			// Background: remove this region from all other class masks - This has to be done even when multiple lables are allowed
			if(activeClass == 0) {
				UMat region_to_keep;
				//  cv::bitwise_and, cv::bitwise_or and cv::bitwise_xor on binary images:
				// region to keep is classMask minus intersection 
				cv::bitwise_xor(classMask, intersection, region_to_keep);
				newState.at(i) = region_to_keep; // |= would be wrong here
				//UMat b = classMask.mul(intersection); // just testing 
			}
			// 
			else if(multiplePixelLabelsAllowed == false &&
					overwrite_existing == false &&
					i != 0) { // current class is not background
				// Remove the pixels the already belong to other classes from the new (active) region, so they are not added when only one label per pixel is allowed
				// result is new region - pixels already belonging to the i-th class
				//Timer timer_for_loop = Timer();
				cv::bitwise_xor(newRegion, intersection, newRegion);
				std::cout << "timer for loop adding class pixels: ";
				timer1.Stop();
				// RemoveFrom(locations, newRegion);				
			}
			// Background or overwrite class: remove the pixels that do not belong to the  
			else if((multiplePixelLabelsAllowed == false &&
					overwrite_existing == true) || i == 0) {
				UMat region_to_keep;
				// region to keep is classMask minus intersection
				cv::bitwise_xor(classMask, intersection, region_to_keep);
				//labeledMasks().at(0) = region_to_keep; // DS 18.1. remove when working
				newState.at(i) = region_to_keep;
			}

		}
	}

	// add resulting region to current class
	//labeledMasks.at(activeClass) |= newRegion; // only Mat
	UMat result;
	cv::bitwise_or(newState.at(activeClass), newRegion, result);
	newState.at(activeClass) = result;

	pushState(newState);

	std::cout << " add region to class " << activeClass << " took: ";

	return 0;
}


// Is currently only used for watershed transform 
int LabelState::setSegmentationMasks(cv::UMat classMasks, bool overwrite_existing) { // overwrite_existing is dummy for now - might be used later

	if(classMasks.empty()) return -3;
	// start timer
	Timer timer1;

	// get the number of classes from the segmentation (watershed etc.) result 
	// --> Calculate the max class number 
	double minValue, maxValue;
	//cv::minMaxLoc(classMasks, nullptr, &maxValue); // does also work
	cv::minMaxLoc(classMasks, &minValue, &maxValue, nullptr, nullptr);
	uint8_t maxPixelValue = static_cast<uint8_t>(maxValue);

	// get a copy of the current labelMask
	// auto newState = CopyCurrentState(); // acutally we do not need to copy the state when the mask is applied to the complete image
	auto newState = std::vector<UMat>(maxPixelValue);

	// extend the class masks vector if to short - starting from class 0 (background)
	while(newState.size() < maxPixelValue + 1) {
		newState.push_back(cv::UMat::zeros(height, width, CV_8U));
	}

	// Iterate over each unique class index (= pixel Value)
	for(int i = 0; i <= maxPixelValue; i++) {
		// Threshold the image to obtain pixels with the target value
		cv::UMat thresholdedImage;

		// use opencv's compare function to get the mask for the target class
		cv::compare(classMasks, Scalar(i), thresholdedImage, cv::CMP_EQ);

		// Assign the mask to the vector
		newState.at(i) = thresholdedImage;
	}
	pushState(newState);
	timer1.Stop();

	return 0;
}

bool LabelState::Undo() {

	if(currentIndex == -1) {
		std::cerr << "No history available for undo." << std::endl;
		return false;
	}

	currentIndex = (currentIndex - 1 + capacity) % capacity;
	std::cout << "index = " << currentIndex << "\n";
	return true;
}
