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
#include <d3d11.h>
#include <codecvt> // for wstring_convert 
#include <filesystem>
#include"opencv2/core.hpp"
#include"opencv2/imgcodecs.hpp"
#include"opencv2/core/directx.hpp"
#include"opencv2/imgproc.hpp"

namespace fs = std::filesystem;

bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height, ID3D11Device* g_pd3dDevice);

//std::wstring utf8ToUtf16(const std::string& utf8Str) {
//	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
//	return conv.from_bytes(utf8Str);
//}

inline std::string utf16ToUtf8(const std::wstring& utf16Str) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
	return conv.to_bytes(utf16Str);
}
inline bool createDir(std::string path, bool createRecursive = true) {
	bool created = false;
	if (!fs::exists(path)) {  // check path is folder and does not already exist
		created = fs::create_directory(
			path);  // does catch the error if path string is invalid
	}
	return created;
}
static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam,
	LPARAM lpData);
bool BrowseImageFile(std::string current_dir, std::string& picked_img_path);
std::string BrowseFolder(std::string saved_path);

int SaveLabels(const std::vector<std::string>& files_in_path, bool save_classes_separately, const std::string& current_img_path,
			   ID3D11ShaderResourceView*& tex_shader_res_view, int image_width, int image_height, const std::string& mask_postfix);
int LoadImageAndMask(const std::string& current_img_path, ID3D11ShaderResourceView*& tex_shader_res_view, ID3D11Device* g_pd3dDevice,
					  int& image_width, int& image_height, bool seperate_masks, const std::string& mask_postfix);
cv::Mat CreateDefaultTextImg( std::string text =
        "No image loaded. Please select a folder with images to label or a "
        "single image using the buttons in the menu.");