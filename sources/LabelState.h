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
#include "opencv2/core.hpp" 
#include <opencv2/imgcodecs.hpp> 
#include <filesystem>


class LabelState
{
public:
	static LabelState& Instance() {
		static LabelState instance; // Guaranteed to be destroyed. Instantiated on first use.
		instance.buffer_masks.resize(capacity); // initialize the vector with the correct size
		return instance;
	}

	int tryloadmask(std::string path);
	int tryLoadSeperateMasks(std::string masks_path, std::string maskname);
	// made this because default arguments in header declaration did not work 
	cv::Mat load_new_image_no_mask(std::string img_path) {
		return this->load_new_image(img_path, "", false);
	}
	//void load_new_image(std::string img_path, const std::string mask_path, bool load_mask);
	cv::Mat load_new_image(std::string img_path, const std::string mask_path = "/mask", bool load_mask = false);
	int saveLabels(const std::string labelPath, bool seperateImages);
	cv::UMat GetCurrentImg() {
		return currentImg;
	}
	int GetActiveClass() {
		return activeClass;
	}
	cv::UMat GetActiveClassRegion() {
		// make sure that there are enough class masks - by adding empty ones if neccessary
		if(labeledMasks().size() < activeClass + 1) {
			for(int i = labeledMasks().size(); i <= activeClass; i++) //start with 3 as default
				labeledMasks().push_back(cv::UMat::zeros(height, width, CV_8U));
		}
		return labeledMasks().at(activeClass);
	}
	cv::UMat GetClassRegion(int class_number) {
		return labeledMasks().at(class_number);
	}
	bool ChangeActiveClass(int class_number);
	int addRegionToClass(cv::UMat newRegion, bool overwriteExisting, bool multiplePixelLabels);
	int setSegmentationMasks(cv::UMat classMasks, bool overwrite_existing);
	int MasksSize() { return labeledMasks().size(); };
	int h() { return height; }
	int w() { return width; }

	// mask state
	void pushState(const std::vector<cv::UMat>& newLabeledMask) {
		currentIndex = (currentIndex + 1) % capacity;
		buffer_masks[currentIndex] = newLabeledMask;
	}
	bool Undo();
	int GetCurrentIndex() { return currentIndex; }
	// get a reference to the current Masks (= the active state)
	inline std::vector<cv::UMat>& labeledMasks() {
		//return buffer_masks.at(currentIndex);
		//if(currentIndex <= -1) return  ; // can not give back nullptr here
		return buffer_masks.at(currentIndex);
	}

	const std::vector<cv::UMat>& GetCurrentState() {
		if(currentIndex == -1) {
			return {};
		}
		return buffer_masks.at(currentIndex);
	}
	
	void CreateUsageImg();
	bool FillRegion;
	int FillSize;

	void* textureSrData;
	bool drawingFinished = false;

private:
	LabelState() {}                       // (the {} brackets) are needed here.
	// C++ 03
	// ========
	// Don't forget to declare these two. You want to make sure they
	// are inaccessible (especially from outside), otherwise, you may accidentally get copies of
	// your singleton appearing.
	LabelState(LabelState const&);                // Don't Implement.
	void operator = (LabelState const&);          // Don't implement 

	cv::UMat currentImg;
	int activeClass = 0;
	int width;
	int height;

	static const int capacity = 2;
	std::vector<std::vector<cv::UMat>> buffer_masks;
	int currentIndex;

	std::vector<cv::UMat> CopyCurrentState() {
		const std::vector<cv::UMat>& currentState = GetCurrentState(); // Copy the current state into a new vector
		return currentState;
	}
	void ClearState() { // clear the complete state - used after loading
		for(auto& state : buffer_masks) {
			state.clear();
		}
		currentIndex = -1;
	};
};
 
