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
#include"load_image.h"
#include"opencv2/core.hpp"
#include"opencv2/imgcodecs.hpp"
#include"opencv2/core/directx.hpp"
#include"opencv2/imgproc.hpp"

#include "LabelState.h"

static cv::Mat CreateDefaultTextImg(std::string text = "No image loaded. Please select a folder with images to label or a single image using the buttons in the menu."
) {
	if(text == "" || text == " ")
		text = "No message specified";

	// Define image size (720p)
	const int width = 1280;
	const int height = 720;
	cv::Mat textImg = cv::Mat(height, width, CV_8UC3, cv::Scalar(255, 255, 255)); // White background

	// Define font scale and thickness
	int fontFace = cv::FONT_HERSHEY_SIMPLEX;
	double fontScale = 1.0;
	int thickness = 2;

	// Colors
	cv::Scalar primaryColor(0, 0, 0); // Black
	cv::Scalar highlightColor(0, 0, 255); // Red

	// Split the text into words
	std::istringstream iss(text);
	std::vector<std::string> words{ std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{} };

	// Starting position for the text
	int x = 30;
	int y = 100;

	// Draw each word
	for(const auto& word : words) {
		// Choose color based on the word
		cv::Scalar color = (word == "image" || word == "select" || word == "folder" || word == "images" || word == "buttons" || word == "menu.") ? highlightColor : primaryColor;

		// Calculate text size
		int baseline = 0;
		cv::Size textSize = cv::getTextSize(word, fontFace, fontScale, thickness, &baseline);

		// Check if the word goes outside the image bounds
		if(x + textSize.width >= width) {
			x = 30; // Reset x to start of next line
			y += textSize.height + 10; // Move to next line
		}

		// Position for this word
		cv::Point textOrg(x, y);

		// Draw the word
		cv::putText(textImg, word, textOrg, fontFace, fontScale, color, thickness);

		// Move x-coordinate for next word
		x += textSize.width + 10; // Add space between words
	}
	return textImg; 
}

bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height, ID3D11Device* g_pd3dDevice)
{
    // Load from disk into a raw RGBA buffer
    int image_width = 0;
    int image_height = 0;
    //unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
	// DS: verry important to release the old ressource if image is loaded from file (again) like above
	// If opencv is used on image is loaded just once, releasing out_srv like blow results in casualty1
    /*if (*out_srv != nullptr) {
        (*out_srv)->Release();
        *out_srv = nullptr;
    }*/
	
	cv::Mat image = LabelState::Instance().load_new_image(filename); // clone is done at loading
	//cv::Mat DBG_image = image.getMat(cv::ACCESS_READ);  

	auto DBG_tet = image.empty(); 
	if( DBG_tet){ 
		image = CreateDefaultTextImg(); 
		auto DBG_seppl = true;
	}
 
    image_height = image.rows; 
    image_width = image.cols; 

    // Create texture 
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
   // desc_rgba.SampleDesc.Quality = 0;
    // USAGE_DYNAMIC-Texturen sind für das Streaming von Daten von der CPU zur GPU und nicht nur für GPU-Lesevorgänge optimiert.
    // alternatives are described here
    // https://learn.microsoft.com/de-de/windows/win32/direct3d11/how-to--use-dynamic-resources?redirectedfrom=MSDN
    // https://learn.microsoft.com/de-de/windows/win32/direct3d11/overviews-direct3d-11-resources-textures-how-to-set_with_check-manually
    desc.Usage = D3D11_USAGE_DYNAMIC; //D3D11_USAGE_DEFAULT; //D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    //desc.CPUAccessFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    ID3D11Texture2D* pTexture = NULL;
    g_pd3dDevice->CreateTexture2D(&desc, 0, &pTexture);

    //DS: maybe save the texture descriptor in program state - it may be used to retrieve the texture from shaderResourceView back again

    // Try to save the Subresource 
    //// convert that matrix (or the image data) to D3d11 subresource    

    // ***  DS: out 24.8.23
    //D3D11_SUBRESOURCE_DATA tSData = {}; // difference between MAPPEDSUBRESOURCE AND SUBRESOURCE_DATA?
    //tSData.pSysMem = &(pTexture[0]);
    //tSData.SysMemPitch = desc.Width * 4;
    //tSData.SysMemSlicePitch = 0;
    // *** 24.8.23


    //// Alternatively: 
    //subResource.pSysMem = image.data;   
    //subResource.SysMemPitch = desc.Width * 4;
    //subResource.SysMemSlicePitch = 0;
    //g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

    // save to State (--> maybe remove)
    //LabelState::Instance().textureSrData = & tSData; // DS: MEMORY LEAK !!!!!

    // to prevent texture Type != source type 
  /*  cv::UMat alpha(image_height, image_width, CV_8UC1); not needed here*/
 /*   cv::UMat image4channel;
    int conversion[] = { 0, 0, 1, 1, 2, 2, -1, 3 };
    //https://docs.opencv.org/4.5.3/d2/de8/group__core__array.html#ga51d768c270a1cdd3497255017c4504be
    cv::mixChannels(&image, 2, &image4channel, 1, conversion, 4);*/
    cv::Mat image_rgba; // maybe just one image needed
    cv::cvtColor(image, image_rgba, cv::COLOR_BGR2RGBA); // If conversion adds the alpha channel, its value will set_with_check to the maximum of corresponding channel range: 255 for CV_8U,

    cv::directx::convertToD3D11Texture2D(image_rgba, pTexture);
    if (pTexture == nullptr) {
        throw std::runtime_error("Texture Pointer is null");
    }

    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);

    pTexture->Release();

    *out_width = image_width;
    *out_height = image_height;
    //stbi_image_free(image_data);
    //image.release(); 
    image_rgba.release(); 

    return true;
}
 
int LoadImageAndMask(const std::string& current_img_path, ID3D11ShaderResourceView*& tex_shader_res_view, ID3D11Device* g_pd3dDevice, 
					  int& image_width, int& image_height, bool seperate_masks, const std::string& mask_postfix) {

	/*	DS: depending on where the image and texture are loaded this may be neccessary (when CV image and texture img are loaded separately
	if (tex_shader_res_view != nullptr) {
			tex_shader_res_view->Release();
			tex_shader_res_view = nullptr;
		}*/

	// load the image
	bool loaded_img = LoadTextureFromFile(current_img_path.c_str(), &tex_shader_res_view, &image_width,
								   &image_height, g_pd3dDevice);
	if (!loaded_img) return -1; 

	// check if there is a mask in the folder 
	std::string filename = fs::path(current_img_path).filename().string(); // name of the image, but with ending
	fs::path imgPath(current_img_path);
	fs::path parentPath = imgPath.parent_path();
	std::string baseFilename = imgPath.stem().string(); // Gets the filename without extension
	fs::path maskDir = parentPath / "mask"; // Directly use path concatenation

	if(seperate_masks) {
		// function needs the directory image folders and name of the mask to save
		LabelState::Instance().tryLoadSeperateMasks(maskDir.string(), baseFilename + mask_postfix);
	} else {
		fs::path maskPath = maskDir / (baseFilename + mask_postfix + ".png");
		int ret = LabelState::Instance().tryloadmask(maskPath.string());
		if (ret == 0) return 0; // successfully loaded mask
		// if mask with postfix does not exis try without 
		if (ret == -1) {
			maskPath = maskDir / (baseFilename + ".png");
			ret = LabelState::Instance().tryloadmask(maskPath.string());
			if (ret == 0) return 1; // could load image now 	
		}
	}
}

int SaveLabels(const std::vector<std::string>& files_in_path, bool save_classes_separately, const std::string& current_img_path,
			   ID3D11ShaderResourceView*& tex_shader_res_view, int image_width, int image_height, const std::string& mask_postfix) {

	fs::path imgPath(current_img_path);
	std::string filename = imgPath.stem().string(); // Extract filename without extension
	fs::path outputPath;

	// If no specific output directory is provided, save in a default location
	if(files_in_path.empty()) {
		if(save_classes_separately) {
			outputPath = imgPath.parent_path() / (imgPath.filename().string());
		} else {
			// Default name for saving mask
			outputPath = imgPath.parent_path() / (imgPath.filename().string() + "_mask.png");
		}
	} else {
		// Ensure the "mask" directory exists
		fs::path maskDir = imgPath.parent_path() / "mask";
		fs::create_directories(maskDir); // No problem if directory already exists
		outputPath = maskDir / (filename + mask_postfix + ".png");
	}

	// Save the current results
	int res = LabelState::Instance().saveLabels(outputPath.string(), save_classes_separately);
	return res;
}


#pragma region mainHelper
#include <shlobj.h>
#include <iostream>
#include <sstream> 
#include <codecvt> // for wstring_convert 

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam,
	LPARAM lpData) {
	if (uMsg == BFFM_INITIALIZED) {
		std::string tmp = (const char*)lpData;
		std::cout << "path: " << tmp << std::endl;
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
	}

	return 0;
}

bool BrowseImageFile(std::string current_dir, std::string& picked_img_path) {
	/// alternatively try: https://www.daniweb.com/programming/software-development/code/217307/a-simple-getopenfilename-example
		// initialize the COM library.
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	std::string image_path;
	if (SUCCEEDED(hr)) {
		IFileOpenDialog* pFileOpen;

		// Create the FileOpenDialog object - and get a pointer to the object's IFileOpenDialog interface.
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
			IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

		if (SUCCEEDED(hr)) {
			// Show the Open dialog box.- blocking
			hr = pFileOpen->Show(NULL);

			// Get the file name from the dialog box.
			if (SUCCEEDED(hr)) {
				IShellItem* pItem;
				hr = pFileOpen->GetResult(&pItem);
				if (SUCCEEDED(hr)) {
					PWSTR pszFilePath;
					hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

					// Display the file name to the user.
					if (SUCCEEDED(hr)) {
						//MessageBoxW(NULL, pszFilePath, L"File Path", MB_OK);
						std::wstringstream ss;
						// first convert so wide string stream 
						ss << pszFilePath;
						// then use function to "narrow" wide string to normal one 
						image_path = utf16ToUtf8(ss.str());
						picked_img_path = image_path;
						CoTaskMemFree(pszFilePath);
					}
					pItem->Release();
				}
			}
			pFileOpen->Release();

		}
		CoUninitialize();
	}
	fs::path filePath = image_path;
	std::string extension = image_path.substr(image_path.rfind('.') + 1);
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower); // transform chars to lower case
	if (extension == "jpg" || extension == "png" || extension == "bmp" || extension == "tiff" || extension == "tif") {
		return true;
	}
	else {
		MessageBoxW(NULL, L"Select a valid image in formats like jpg, png, bmp or tiff", L"No valid image was selected.", MB_OK);
		return false;
	}
}

std::string BrowseFolder(std::string saved_path) {
	TCHAR path[MAX_PATH];
	const char* path_param = saved_path.c_str();

	BROWSEINFO bi = { 0 };
	bi.lpszTitle = L"Browse for folder...";
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	bi.lpfn = BrowseCallbackProc;
	bi.lParam = (LPARAM)path_param;

	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

	if (pidl != 0) {
		// get the name of the folder and put it in path
		SHGetPathFromIDList(pidl, path);

		// free memory used
		IMalloc* imalloc = 0;
		if (SUCCEEDED(SHGetMalloc(&imalloc))) {
			imalloc->Free(pidl);
			imalloc->Release();
		}

		// test.begin(), test.end()
		std::wstring test(&path[0]);
		std::string dummy(test.begin(), test.end());
		return dummy;
	}

	return "";
}

#pragma endregion mainHelper