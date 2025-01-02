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
#include <tchar.h>

#include <iostream>
#include <filesystem>
#include <core/directx.hpp>
#include <core/ocl.hpp>
#include <imgproc.hpp>

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "load_image.h"  

#include "LabelState.h"
#include "ImageProcessing.h" 
#include "Timer.h"
#include "../resource.h" 
#include "helper.h"
#include "user_interaction.h"

namespace fs = std::filesystem;
#define M_PI 3.14159265358979323846

// Data
static ID3D11Device* g_pd3dDevice = NULL;
/*DS: further information:
The ID3D11Device is used for creating resources, for example, the many shaders
in the pipeline. The ID3D11DeviceContext is used for binding those resources to
the pipeline, for example, the many shaders.
https://stackoverflow.com/questions/37509559/understanding-id3d11device-and-id3d11devicecontext
*/
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

cv::ocl::Context m_oclCtx;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

cv::UMat ApplyCVOperation(ImageProcParameters params, float* color, CvOperation op);


// Main code
int main(int, char**) {
	// Create application window
	ImGui_ImplWin32_EnableDpiAwareness();
	WNDCLASSEX wc = { sizeof(WNDCLASSEX),     CS_CLASSDC, WndProc, 0L,   0L,
					 GetModuleHandle(NULL),  NULL,       NULL,    NULL, NULL,
					 _T("PixLabelCV "), NULL };
	// loading the icon
	HICON loadedIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	//wc.hIcon = loadedIcon;
	wc.hIconSm = loadedIcon;
	//wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	::RegisterClassEx(&wc);
	HWND hwnd =
		::CreateWindow(wc.lpszClassName, _T("PixLabelCV"), WS_OVERLAPPEDWINDOW, 100,
					   100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

	// Initialize Direct3D
	if(!CreateDeviceD3D(hwnd)) {
		CleanupDeviceD3D();
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	// Show the window maximized - alternative would be SW_SHOWDEFAULT
	::ShowWindow(hwnd, SW_SHOWMAXIMIZED);
	::UpdateWindow(hwnd);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();


	// create CV ocl context
	// initialize OpenCL context of OpenCV lib from DirectX
	cv::ocl::Context   m_oclCtx;
	if(cv::ocl::haveOpenCL()) {
		m_oclCtx = cv::directx::ocl::initializeContextFromD3D11Device(g_pd3dDevice);
	}
	cv::String m_oclDevName =
		cv::ocl::useOpenCL() ? cv::ocl::Context::getDefault().device(0).name()
		: "No OpenCL device";

	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |=
		ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
	// io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // implement this to support multiple screens
	//  --> semms to require "docking" branch	//  https://github.com/ocornut/imgui/tree/docking
	io.WantCaptureMouse = true;  // capture mouse events

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	// ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	// Load Fonts
	/*
	-If no fonts are loaded, dear imgui will use the default font.You can
	// also load multiple fonts and use ImGui::PushFont()/PopFont() to select
	// them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you
	// need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please
	// handle those errors in your application (e.g. use an assertion, or display
	// an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored
	// into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which
	// ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string
	// literal you need to write a double backslash \\ !
	// io.Fonts->AddFontDefault();
	// io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	// io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	// io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	// io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	// ImFont* font =
	// io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f,
	// NULL, io.Fonts->GetGlyphRangesJapanese()); IM_ASSERT(font != NULL);
	// Initializing vector with values
	*/


	// State
	bool show_demo_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	float picked_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	static ImageProcParameters ImPar = ImageProcParameters();

	std::vector<std::string> files_in_path;
	std::string current_img_path;

	// DS: loading the default image on startup
	int image_width = 0;
	int image_height = 0;
	ID3D11ShaderResourceView* tex_shader_res_view = NULL;  // DS: actually a resource view to the texture in the shader
	std::string img_file = "sled.png";
	//LabelState::Instance().load_new_image(img_file); // DS: not needed anymore
	bool ret = LoadTextureFromFile(img_file.c_str(), &tex_shader_res_view,
								   &image_width, &image_height, g_pd3dDevice);
	IM_ASSERT(ret); //DS: is pointless now - changed to display hint text img 

	static int last_draw_shape = 0;
	static bool drawClassRegion;
	DrawRect draw_rect = { -1, -1, -1, -1 };
	//DrawRect cv_rect = { -1, -1, -1, -1 };
	Marker marker;
	DrawEllipse ell;
	DrawCircle circ;

	static DrawPolygon poly;
	static bool use_floodfill = false;
	static bool use_grabcut = false;
	int dragging_point = -1;  // index of point that is currently dragged - <0 if none

	std::vector<PointRad> brush_point_details;
	bool is_drawing_brush = false;
	static int brush_rad = 10;

	// program state
	bool evaluate = false;
	static float font_scale = 1.0;
	static float alpha = 0.5;
	static bool expert_window;
	static bool display_help_window = false;
	static bool show_img_name = true;
	static bool is_drawing = false;
	static bool reset_gui = false;
	static bool save_key = false;
	const char* drawshape[] = { "Rectangle", "Polygon", "Circle", "Brush", "Watershed", "Graph Cut" }; // ,"Arc" };
	static int current_draw_shape = 0;
	ImVec2 position_correction;
	Zoom zoom;

	static int num_files_in_folder = 0;
	static std::string mask_postfix = "_mask";
	static bool fill_inner_pixels = false;
	static bool overwrite_classes = false;
	static bool multipleClassLabels = false;
	// bool not the overwrite the explicitly set background - maybe implement later (but currently not needed)
	// static bool not_overwrite_background = false;

	// expert window parameters
	const char* ff_fill_mode[] = { "Simple", "Fixed Range", "Gradient", "4-connectivity", "8-connectivity" };
	static int current_fill_mode = 1;
	//int lo_diff = 20, up_diff = 40;
	ImVec2 f_point;
	static int snap_to_border_distance = 8;
	static bool seperateMasks = false;
	static bool disp_region_after_adding = false;
	static bool standard_brush_sizes = true;

	// Main loop
	bool done = false;
	static bool show_message = false;
	static std::string WarningMessage = "None";
	static bool show_timer_window = false;
	static Timer labelTimer = Timer();


#ifdef DEBUG
	std::cout << cv::getBuildInformation() << std::endl;
#endif
	// check wheter 
	if(checkIfFirstRun("imgui.ini")) {
		display_help_window = true;
	}

	while(!done) {
		// Poll and handle messages (inputs, window resize, etc.)
		// See the WndProc() function below for our to dispatch events to the Win32
		// backend.
		MSG msg;

		while(::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if(msg.message == WM_QUIT)
				done = true;
			else if(msg.message == WM_KEYUP) { // WM_RBUTTONUP
				auto res = msg.wParam;
			}
		}
		if(done) break;

		// Start the Dear ImGui frame
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// ImGui Demo window for reference only 
		// if(show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);

		if(show_message) {
			ImGui::OpenPopup("Warning");

			// Always center this window when appearing
			ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

			if(ImGui::BeginPopupModal("Warning", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
				std::string SegmentationMsg = "An error adding the segmented region to the class region. Probably you did not segment anything but tried to add?\n\n";
				if(WarningMessage == "None") {
					WarningMessage = SegmentationMsg;
				}
				ImGui::Text(WarningMessage.c_str());
				ImGui::Separator();

				static bool dont_ask_me_next_time = false;
				if(WarningMessage == SegmentationMsg) {
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
					ImGui::Checkbox("I won't do it again", &dont_ask_me_next_time);
					ImGui::PopStyleVar();
				}

				if(ImGui::Button("OK", ImVec2(120, 0))) {
					ImGui::CloseCurrentPopup();
					show_message = false;
					// reset warning message
					WarningMessage = "None";
				}
				ImGui::SetItemDefaultFocus();
				ImGui::EndPopup();
			}
		}


		//		Create the window to work on 
		static bool use_work_area = true;
		// Main Area = entire viewport,\nWork Area = entire viewport minus sections used by the main menu bars, 
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(use_work_area ? viewport->WorkPos : viewport->Pos);
		ImGui::SetNextWindowSize(use_work_area ? viewport->WorkSize : viewport->Size);

		// Image as Texture object and User input (drawing into image)
		static bool* p_open = new bool{ true };
		{
			static ImGuiWindowFlags flags =
				ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus;
			// ImGuiWindowFlags_NoDecoration is not usable as it disable the Horizontal Scrollbar
			ImGui::Begin("Image to draw Labels on ", p_open, flags);  // define flags before so that they may be changed

			ImTextureID textureId = (void*)tex_shader_res_view;
			ImVec2 textureSize = ImVec2(image_width, image_height);
			// ImGuiIO& io = ImGui::GetIO();
			// ImTextureID textureId = io.Fonts->TexID;
			// ImVec2 textureSize = ImVec2(io.Fonts->TexWidth, io.Fonts->TexHeight);

			if(zoom.current >= zoom.min) {
				// if neccessary translate (by scrolling) after zoom https://github.com/ocornut/imgui/issues/1757
				// because currently position after zoom is oriented on the upper left corner
				textureSize = ImVec2(zoom.current * image_width, zoom.current * image_height);
			}
			// better use Image than ImageButton 
			ImGui::Image(textureId, textureSize);

			bool isHovered = ImGui::IsItemHovered();
			bool isFocused = ImGui::IsItemFocused();
			ImVec2 mousePositionAbsolute = ImGui::GetMousePos();
			ImVec2 screenPositionAbsolute = ImGui::GetItemRectMin();
			//ImVec2 screenBottomCornerAbsolute = ImGui::GetItemRectMax();
			//ImVec2 screenSizeAbsolute = ImGui::GetItemRectSize(); // gives Difference between RectMax() and RectMin()
			position_correction = screenPositionAbsolute;
			ImVec2 mousePositionRelative = ImVec2(mousePositionAbsolute.x - screenPositionAbsolute.x,
												  mousePositionAbsolute.y - screenPositionAbsolute.y);
			ImGui::Text("Mouse Position: %f, %f", mousePositionRelative.x,
						mousePositionRelative.y);
			/* Further information - only used for debugging
			ImGui::Text("Is mouse over screen? %s", isHovered ? "Yes" : "No");
			ImGui::Text("Is screen focused? %s", isFocused  ? "Yes" : "No");
			ImGui::Text("Mouse clicked: %s",
			ImGui::IsMouseDown(ImGuiMouseButton_Left) ? "Yes" : "No"); */

			// check if mouse is hovered over the image -> to draw
			if(isHovered) {
				ImVec2 currentPoint = ImVec2(mousePositionRelative.x, mousePositionRelative.y);

				float mW = io.MouseWheel; // positiv is up; negative is down
				double last_zoom = zoom.current;
				if(mW < 0.0f && (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || (ImGui::IsKeyDown(ImGuiKey_RightCtrl)))) { // IsKeyPressed did not work as intended here --> probably is regarded as true only after a certain time (maybe 0.5-1s)
					zoom.decrease();
					scalePoints(current_draw_shape, double(zoom.current / last_zoom), draw_rect, poly, marker, circ, brush_point_details);
				} else if(mW > 0.0f && (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || (ImGui::IsKeyDown(ImGuiKey_RightCtrl)))) {
					zoom.increase();
					scalePoints(current_draw_shape, double(zoom.current / last_zoom), draw_rect, poly, marker, circ, brush_point_details);
				}

				if(ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
					auto scrollAbsX = ImGui::GetScrollX();
					auto scrollAbsY = ImGui::GetScrollY();
					float drag_speed = 1.5;
					ImGui::SetScrollX(scrollAbsX + drag_speed * io.MouseDelta.x);
					ImGui::SetScrollY(scrollAbsY + drag_speed * io.MouseDelta.y);
				}


#pragma region UserDrawingOnImage

				// brush tool paints while LMB dragged
				if(ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
					if(current_draw_shape == RectangleD) {
						if(!is_drawing) {  // when starting to draw
							draw_rect.left = mousePositionRelative.x;
							draw_rect.top = mousePositionRelative.y;

							is_drawing = true;
							// snap to border - left and up
							draw_rect.left = (draw_rect.left) < snap_to_border_distance ? 0 : draw_rect.left; // this includes negative x
							draw_rect.top = (draw_rect.top) < snap_to_border_distance ? 0 : draw_rect.top;
						} else {  // while drawing
							draw_rect.right = mousePositionRelative.x;
							draw_rect.bottom = mousePositionRelative.y;

							// snap to border - right and down
							//int distance_border_x = image_width * zoom.current + screenPositionAbsolute.x - draw_rect.right;
							int distance_border_x = textureSize.x - draw_rect.right;
							// int distance_border_y = image_height * zoom.current + screenPositionAbsolute.y - draw_rect.bottom;
							int distance_border_y = textureSize.y - draw_rect.bottom;
							// if closer to rigth border then the snap distance set to the image with (and adjust to transform)
							draw_rect.right = distance_border_x <= snap_to_border_distance ? textureSize.x : draw_rect.right;
							//image_width * zoom.current + screenPositionAbsolute.x : draw_rect.right;
							draw_rect.bottom = distance_border_y <= snap_to_border_distance ? textureSize.y : draw_rect.bottom;
							//image_height * zoom.current + screenPositionAbsolute.y : draw_rect.bottom;
						}
						// start to draw when user holds down left mouse
					}

					if(current_draw_shape == BrushD || current_draw_shape == CutsD) {

						// https://github.com/ocornut/imgui/issues/493 
						// Take absolute mouse position to draw the circle into the GUI
						ImVec2 current_brush_position = mousePositionRelative;
						//ImVec2 current_brush_position = { mousePositionAbsolute.x / current_zoom, mousePositionAbsolute.y / current_zoom };

						bool foreground = true; // for graphCut
						if(io.KeyShift || io.KeyAlt) foreground = false;

						PointRad Point_with_rad{ current_brush_position, brush_rad, (float)zoom.current, foreground };
						if(brush_point_details.size() == 0) {
							brush_point_details.push_back(Point_with_rad);
						} else {
							PointRad last_p_r = brush_point_details.back();
							double distance = norm2d(ImVec2(last_p_r.pt.x, last_p_r.pt.y), current_brush_position);
							if(distance >= 2) {
								brush_point_details.push_back(Point_with_rad);
								// ImVec2 screenPositionAbsolute = ImGui::GetItemRectMin();
							}
						}
						is_drawing_brush = true;
					}
				} else {  // on released
					is_drawing_brush = false;
					if(brush_point_details.size() > 0) LabelState::Instance().drawingFinished = true;
				}

				if(ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
					// if alternative version of rectangle by clicking and manipulating points (like polygon) include it here 
					bool isDrawingOnRelease = mouse_released(current_draw_shape, currentPoint, &poly, &marker, &ell,
															 &circ, dragging_point);
					if(isDrawingOnRelease) is_drawing = true;
				}

				if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					double_clicked(current_draw_shape, currentPoint, poly, marker, dragging_point);
				}

				// Dragging Shapes and Points (polygon needs to be closed)
				// if(ImGui::IsMouseDown(ImGuiMouseButton_Left)) {  // both work here
				if(ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
					bool shouldDraw = dragging(current_draw_shape, currentPoint, poly, marker, circ, image_width * zoom.current, image_height * zoom.current, dragging_point);
					if(shouldDraw) is_drawing = true;
				}
				// recalculate the polygon mid when dragging ends
				else if(current_draw_shape == PolygonD && poly.closed) {
					poly.mid = calculateCentroid(poly.points);
				}


#pragma endregion UserDrawingOnImage
			}  // End isHovered

			DrawShapeOnGui(dragging_point, is_drawing, current_draw_shape, draw_rect, mousePositionAbsolute, screenPositionAbsolute, snap_to_border_distance, image_width, zoom,
						   image_height, is_drawing_brush, brush_point_details, alpha, poly, marker, circ, ell);

#pragma region KeyInputs

			// saving the results on CTRL + S Key
			if(io.KeyCtrl && ImGui::IsKeyDown(83)) {  // S=83 und S= 564  | RCtrl=531 LCtrl=527, ModCtrl=
				fs::path cwd = fs::current_path();
				save_key = true;
			}
			// saving the results on CTRL + D Key
			if(io.KeyCtrl && ImGui::IsKeyPressed(68)) {
				int return_code = SaveLabels(files_in_path, seperateMasks, current_img_path, tex_shader_res_view, image_width, image_height, mask_postfix);
				//save_key = false;
			}

			if(ImGui::IsKeyPressed(71)) {  // G Key pressed --> pick color !
				// take the pixel value form the mouse position if it is over the screen
				if(isHovered) {
					ImVec2 hoveredPixel = { (mousePositionRelative.x / (float)zoom.current),
											 (mousePositionRelative.y / (float)zoom.current) };
					pickColor(hoveredPixel, (float*)&clear_color);
					pickColor(hoveredPixel, (float*)&picked_color);
				}
			} else if(ImGui::IsKeyPressed(82)) {// R Key to reset 
				poly.Reset();
				dragging_point = -1;
				marker.Reset();
				circ.Reset();
				brush_point_details.clear();
				LabelState::Instance().drawingFinished = false;

				// reset the image as well 
				reset_gui = true;

			} else if(ImGui::IsKeyPressed(70)) {// F Key to change the fill box
				fill_inner_pixels = !fill_inner_pixels;
				// do trigger evaluation 
				evaluate = true;
			}


			if(ImGui::IsMouseReleased(ImGuiMouseButton_Right) || (ImGui::IsKeyReleased(83) & !io.KeyCtrl)) { // S key without control
				evaluate = true;
			} else if(ImGui::IsKeyPressed(65)) {   // A Key --> Add segmentation result to current class 
				int confirmedSegResult = -1;

				if(current_draw_shape == MarkerPointsD) {
					confirmedSegResult = addMaskToClassregion(overwrite_classes, true);
				} else { // not watershed
					confirmedSegResult = addMaskToClassregion(overwrite_classes, false, multipleClassLabels);
					// draw region only if adding was successfull (and flag is set)
					if(disp_region_after_adding && confirmedSegResult >= 0) drawClassRegion = true;
				}

				if(confirmedSegResult != 0) {
					show_message = true;
					WarningMessage = "An error adding the segmented region to the class region. Maybe there is no image left. Could not add region\n";
				}
			} else if(ImGui::IsKeyPressed(90) && io.KeyCtrl) { // Strg + Z Key to undo 
				LabelState::Instance().Undo();
				// display the changes after the undo step (to show difference to user)
				drawClassRegion = true;
				ImPar.drawAllClasses = true;
			} else if(ImGui::IsKeyPressed(68)) {  // D key
				drawClassRegion = true;
			} else if(ImGui::IsKeyReleased(81)) { // Q key
				drawClassRegion = true;
				ImPar.drawAllClasses = true;
			} else if(ImGui::IsKeyPressed(67) && current_draw_shape == CutsD) {  // C key
				use_grabcut = true;
				evaluate = true;
			} else {
				drawClassRegion = false;
				if(ImGui::IsKeyPressed(77) || ImGui::IsKeyPressed(69)) { // M key = 77 or E key = 69
					use_floodfill = true;
					f_point = mousePositionRelative;
					evaluate = true; // set evalue to trigger CV 
				}
			}

			if(ImGui::IsKeyPressed(72)) {
				display_help_window = !display_help_window;
			}

#pragma endregion

			// ImGui::ShowMetricsWindow(); // "to explore/visualize/understand how the ImDrawList are generated."
			//ImGui::ShowStackToolWindow(); // for debugging potential ID collisions.
			ImVec2 p = ImGui::GetCursorScreenPos();
			ImGui::End();
		}

		// Interaction and Settings Window
		{
			static int h_tol = 255;
			static int s_tol = 255;
			static int v_tol = 255;
			static int counter_gui = 0;

			static bool* p_open = new bool{ true };
			ImGui::Begin("Change State of Labeling", p_open,
						 ImGuiWindowFlags_HorizontalScrollbar |
						 ImGuiWindowFlags_AlwaysAutoResize);
			const char* items[] = { "RGB", "HSV" }; 
			const char* n_classes[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10",
											   "11", "12", "13", "14", "15", "16", "17", "18", "19" };
			//const char* n_classes[] = { "Hintergrund", "Pruefobjekt", "Unterspritzung", "Flash", "Schlieren", "Dieseleffekt" };
 
			static int colorspace = 0;

			static int active_class = LabelState::Instance().GetActiveClass();

			ImGui::Combo("active class: ", &active_class, n_classes, IM_ARRAYSIZE(n_classes));
			if(active_class != LabelState::Instance().GetActiveClass()) { // change classes via GUI
				if(LabelState::Instance().ChangeActiveClass(active_class))
					std::cout << "switched class to " << active_class << " (via gui) \n";
			}
			// Combobox for shape drawing
			ImGui::Combo("Shape / Procedure", &current_draw_shape, drawshape,
						 IM_ARRAYSIZE(drawshape));
			ImGui::NewLine();

			ImGui::Combo("colorspace (upper boundary)", &colorspace, items,
						 IM_ARRAYSIZE(items));
			ImGui::SliderInt("Hue or R upper boundary", &h_tol, 0, 255);
			ImGui::SliderInt("Saturation or G upper boundary", &s_tol, 0, 255);
			ImGui::SliderInt("Value or B tolerance", &v_tol, 0, 255);

			// reset the other shapes if switched
			// ResetShapes(); 
			if(last_draw_shape != current_draw_shape) {
				if(current_draw_shape != PolygonD) {  // reset polygon
					poly.Reset();
					dragging_point = -1;
				}
				if(current_draw_shape != RectangleD) {  // reset rect
					draw_rect.Reset();
				}
				if(current_draw_shape != CircleD) {  // reset circle
					circ.Reset();
				}
				if(current_draw_shape != BrushD) { // reset brush
					brush_point_details.clear();
				} else if(standard_brush_sizes) brush_rad = 10; // reset brush radius (DS: Maybe there is a better way here)
				if(current_draw_shape != MarkerPointsD) {  // reset Points for polygon and makers					
					dragging_point = -1;
					marker.Reset();
				}
				if(current_draw_shape != ArcD) {  // reset arc
					ell.points.clear();
				}
				if(current_draw_shape != CutsD) {
					brush_point_details.clear();
				} else if(standard_brush_sizes) brush_rad = 6; // reset brush radius so it is more suited for grabCut
				LabelState::Instance().drawingFinished = false;
				last_draw_shape = current_draw_shape;
			}

			ImGui::ColorPicker3("Pick color for segmentation here", (float*)&picked_color);
			// get HSV values
			float out_h, out_s, out_v;
			ImGui::ColorConvertRGBtoHSV(picked_color[0], picked_color[1],
										picked_color[2], out_h, out_s, out_v);

			ImGui::SliderFloat("Alpha to display class", &alpha, 0.0f, 1.0f);
			ImGui::NewLine();
			ImGui::Checkbox("Fill region", &fill_inner_pixels);
			if(ImGui::IsItemHovered())
				ImGui::SetTooltip("Fill all pixel that are completely inside the region resulting from the processing operation.");
			ImGui::SameLine();

			ImGui::Checkbox("Overwrite other classes", &overwrite_classes);
			if(ImGui::IsItemHovered())
				ImGui::SetTooltip("If set the current class overwrites all other class pixels at the same location.\n For Watershed marker points usually all masks are overwritten");

			ImGui::SliderInt("Radius of the Brush in Pixels", &brush_rad, 1, 40);


			if(colorspace == 0)
				ImPar.setRGB(picked_color[0], picked_color[1], picked_color[2], h_tol,
							 s_tol, v_tol, alpha);
			else
				ImPar.setHSV(out_h, out_s, out_v, h_tol, s_tol, v_tol, alpha);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
						1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::Text("Current zoom ratio = %.2f", zoom.current);
			ImGui::NewLine();


#pragma region BottonsForImageInteraction

			// Save results and load new image
			if(ImGui::Button("Save Result") || save_key) {
				int return_code = SaveLabels(files_in_path, seperateMasks, current_img_path, tex_shader_res_view, image_width, image_height, mask_postfix);

				// load the next image 
				if(return_code == 0) {
					if(counter_gui >= num_files_in_folder || counter_gui == 0) {
						WarningMessage = "All images processed. Please load another image or choose another directory.";
						show_message = true;
					} else {
						current_img_path = files_in_path.at(counter_gui);
						int ret = LoadImageAndMask(current_img_path, tex_shader_res_view, g_pd3dDevice, image_width, image_height, seperateMasks, mask_postfix);
						if(ret == -1) {  // there was a problem loading the image ! - 1/24 does not occur anymore
							WarningMessage = "An error occured loading the image into the display";
							show_message = true;
						} else if(ret == 1) {
							WarningMessage = "A mask with the specified postfix \'" + mask_postfix + "\' did not exist. But a mask with the same name as the image was found and loaded.";
							show_message = true;
						}
						labelTimer = Timer(); // reset the label timer
					}
					if (counter_gui < num_files_in_folder)
						counter_gui++;
				} else {
					WarningMessage = "An error occured saving the result image";
					show_message = true;
				}
			}
			if(ImGui::IsItemHovered())
				ImGui::SetTooltip("Save the mask and load the next image.");
			ImGui::SameLine(0.0f, 20);

			if(ImGui::ArrowButton("##previous", ImGuiDir_Left) && files_in_path.size() > 0
			   || (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) && ImGui::IsKeyDown(ImGuiKey_ModCtrl))) {
				int newIndex = counter_gui - 1;
				int index = counter_gui > 0 ? counter_gui - 1 : 0;
				std::string filename;
				// continue from the back of the labeled images - load in circle
				if(index < 1) {
					counter_gui = num_files_in_folder;
					filename = files_in_path.at(num_files_in_folder - 1);
				} else {
					counter_gui--;
					filename = files_in_path.at(counter_gui - 1);
				}

				try {
					current_img_path = filename;
					int ret = LoadImageAndMask(current_img_path, tex_shader_res_view, g_pd3dDevice, image_width, image_height, seperateMasks, mask_postfix);
					if(ret == 1) {
						WarningMessage = "A mask with the specified postfix \'" + mask_postfix + "\' did not exist. But a mask with the same name as the image was found and loaded.";
						show_message = true;
					} else if(ret <= -1) {
						WarningMessage = "An error occured loading the image.";
						show_message = true;
					}
					// draw class regions to display loaded mask
					drawClassRegion = true;
					ImPar.drawAllClasses = true;
					labelTimer = Timer(); // reset the label timer
				}
				catch(std::exception& e) {
					// maybe instead show message box with error					 
					std::cout << e.what() << "\n";
				}

			}
			ImGui::SameLine(0.0f, 5);
			if(ImGui::ArrowButton("##next", ImGuiDir_Right) && files_in_path.size() > 0 ||
			   (ImGui::IsKeyPressed(ImGuiKey_RightArrow) && ImGui::IsKeyDown(ImGuiKey_ModCtrl))
			   ) {
				// if number higher than last img - start again
				if(counter_gui + 1 > num_files_in_folder) {
					counter_gui = 1;
				} else {
					counter_gui++;
				}
				// try to load the next file
				try {
					// get filename
					current_img_path = files_in_path.at(counter_gui - 1); // counter already incremented
					int ret = LoadImageAndMask(current_img_path, tex_shader_res_view, g_pd3dDevice, image_width, image_height, seperateMasks, mask_postfix);
					if(ret == 1) {
						WarningMessage = "A mask with the specified postfix \'" + mask_postfix + "\' did not exist. But a mask with the same name as the image was found and loaded.";
						show_message = true;
					} else if(ret == -1) {
						WarningMessage = "An error occured loading the image.";
						show_message = true;
					}
					// draw class regions to display loaded mask
					drawClassRegion = true;
					ImPar.drawAllClasses = true;
					labelTimer = Timer(); // reset the label timer
				}
				catch(std::exception& e) {
					// maybe instead show message box with error					 
					std::cout << e.what() << "\n";
				}
			}
			ImGui::SameLine();
			ImGui::Text("counter = %d/%d", counter_gui, num_files_in_folder);

			//ImGui::SameLine();
			ImGui::Text("Switch to image number: ");
			static int im_num;
			// Text box to load a specific number e.g. to continue labeling after n images
			if(ImGui::InputInt(" ", &im_num, 1, 10, ImGuiInputTextFlags_EnterReturnsTrue)
			   && num_files_in_folder > 0) { // only evaluate if a folder with files was loaded
				if(im_num < 1) im_num = 1;
				if(im_num > num_files_in_folder) im_num = num_files_in_folder;
				// load the n-th file
				counter_gui = im_num;
				try {
					current_img_path = files_in_path.at(im_num - 1); // counter already incremented
					int ret = LoadImageAndMask(current_img_path, tex_shader_res_view, g_pd3dDevice, image_width, image_height, seperateMasks, mask_postfix);
					if(ret == 1) {
						WarningMessage = "A mask with the specified postfix \'" + mask_postfix + "\' did not exist. But a mask with the same name as the image was found and loaded.";
						show_message = true;
					} else if(ret == -1) {
						WarningMessage = "An error occured loading the image.";
						show_message = true;
					}
					// draw class regions to display loaded mask
					drawClassRegion = true;
					ImPar.drawAllClasses = true;
					labelTimer = Timer(); // reset the label timer
				}
				catch(std::exception& e) {
					// maybe instead show message box with error					 
					std::cout << e.what() << "\n";
				}
			}

			if(ImGui::Button("Select Folder")) {  // Buttons return true when clicked 
				std::string current_dir = fs::current_path().string();
				std::string selected_dir = BrowseFolder(current_dir);

				if(selected_dir != "") {
					files_in_path = getAllImagesInPath(selected_dir);
					num_files_in_folder = files_in_path.size();

					if(num_files_in_folder < 1) {
						WarningMessage = "No images found! Please select a folder that contains images";
						show_message = true;
						counter_gui = 0; // set counter to 0 - Maybe do not change
					}
					// all right, load first image from the folder
					else {
						current_img_path = files_in_path.at(0); // choose first image
						int ret = LoadImageAndMask(current_img_path, tex_shader_res_view, g_pd3dDevice, image_width, image_height, seperateMasks, mask_postfix);
						if(ret == 1) {
							WarningMessage = "A mask with the specified postfix \'" + mask_postfix + "\' did not exist. But a mask with the same name as the image was found and loaded.";
							show_message = true;
						} else if(ret == -1) {
							WarningMessage = "An error occured loading the image.";
							show_message = true;
						}
						// start the counter for the (segmented) images - for gui start from 1
						counter_gui = 1;
						labelTimer = Timer(); // reset the label timer
					}
				}
			}
			ImGui::SameLine();
			if(ImGui::Button("Select image")) {
				std::string current_dir = fs::current_path().string();
				std::string selected_path;
				bool isImageFile = BrowseImageFile(current_dir, selected_path);
				if(isImageFile) {
					current_img_path = selected_path;

					counter_gui = 1; // 0 would also work here
					num_files_in_folder = 1;
					bool ret = LoadTextureFromFile(current_img_path.c_str(),
												   &tex_shader_res_view, &image_width,
												   &image_height, g_pd3dDevice);

				}
			}
			if(ImGui::IsItemHovered()) { ImGui::SetTooltip("Select one specific image to label."); }
#pragma endregion BottonsForImageInteraction


			ImGui::Checkbox("Expert options", &expert_window);
			ImGui::SameLine();
			ImGui::Checkbox("Display Help", &display_help_window);
			if(ImGui::IsItemHovered())
				ImGui::SetTooltip("Opens help window. Can also be opened and closed with H key.");

			if(show_img_name) {
				std::string img_name = current_img_path.c_str();
				img_name = img_name.substr(img_name.find_last_of("/\\") + 1);
				ImGui::Text(img_name.c_str()); 
				 
				if(ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
					ImVec2 mousePos = ImGui::GetMousePos();
					ImVec2 itemMin = ImGui::GetItemRectMin();
					ImVec2 itemMax = ImGui::GetItemRectMax();

					// check if the mouse position is within the bounds of the text
					if(mousePos.x >= itemMin.x && mousePos.x <= itemMax.x &&
					   mousePos.y >= itemMin.y && mousePos.y <= itemMax.y) { 
						ImGui::SetClipboardText(img_name.c_str());
					}
				}
			}


			// switch to draw shape on pushing T
			if((io.KeyCtrl && ImGui::IsKeyPressed(84)) || ImGui::IsKeyPressed(ImGuiKey_P)) {
				show_timer_window = !show_timer_window;
			} else if(ImGui::IsKeyPressed(84)) { // T Key to switch shape
				int shape_num = current_draw_shape + 1;
				int num_shapes = sizeof(drawshape) / 8; // sizeof() gives size in bytes
				current_draw_shape = shape_num % num_shapes;
				std::cout << current_draw_shape;
			}
			// if no key for CV or other control is pressed, check for class numbers
			else if(ImGui::IsKeyDown(220) || ImGui::IsKeyDown(594)) { // ^ Key or ` Key (left to 1)
				active_class = 0; // switch to background (easy/quick)
			} else {
				for(ImGuiKey key = 48; key <= 58; key++) { // 48 = 0 to 57 = 9
					if(ImGui::IsKeyPressed(key)) {
						active_class = key - 48; // classes 0-9
						if(ImGui::IsKeyDown(ImGuiKey_LeftAlt)) {
							ImGui::SameLine();
							ImGui::Text("\"%s\" %d", ImGui::GetKeyName(key), key);
							std::string debugstr = ImGui::GetKeyName(key);
							active_class += 10;// classes 10-19
						} // classes 20 -29 may be added with Shift or Ctrl
						if(LabelState::Instance().ChangeActiveClass(active_class))
							std::cout << "switched class to " << active_class << "\n";
					}
				}
				// key 68 = d
				// LeftCrtl 162 LeftCrtl 527 ModCtrl 641 RCtrl=531
			}
			ImGui::End();
		}


		// Timer window - optional
		if(show_timer_window) {
			ImGui::Begin("Time current Label", &show_timer_window, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

			double elapsed_time = labelTimer.GetTimeInSeconds();
			// Convert elapsed time to minutes and seconds
			int minutes = static_cast<int>(elapsed_time) / 60;
			int seconds = static_cast<int>(elapsed_time) % 60;

			// Display the timer in the format "MM:SS"
			std::ostringstream oss;
			oss << minutes << ":" << (seconds < 10 ? "0" : "") << seconds;

			ImGui::SetWindowFontScale(1.7f);
			// Use color stack to change text color after a certain interval 
			if(elapsed_time > 60.0 * 5) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.31f, 1.0f));
				ImGui::Text("%s", oss.str().c_str());
				ImGui::PopStyleColor();  // Pop color to revert to the default color
			} else {
				ImGui::Text("%s", oss.str().c_str());  // default color
			}
			ImGui::End();
		}


		// Help/Instructions Window
		if(display_help_window) {
			// If the program has never been started before display this windows at lower right
			ImGui::SetNextWindowPos(ImVec2(1920 * 0.65, 1080 * 0.55), ImGuiCond_FirstUseEver);
			//ImGui::SetNextWindowSize(ImVec2(demo_window_size_x, demo_window_size_y), ImGuiCond_FirstUseEver);
			ImGui::Begin("Usage guide", &display_help_window, ImGuiWindowFlags_NoCollapse);

			ImGui::Text("Basic Information");
			// ImGui::NewLine(); 
			ImGui::TextWrapped("This programm is designed specifically for semantic segementation. You can apply and combine a variety of computer vision algorithms to get a pixel-precise temporary object (foreground) mask quickly and add it to the currently selected class.\n If you are not satisfied after applying the operation, e.g. thresholding the color values in an ROI, you can simply change a parameter and perform the operation again very quickly, before adding the segmented Pixels to the respective class. ");
			ImGui::NewLine();
			ImGui::TextWrapped("Starting a New Project: \nBegin by selecting your project's image folder. Click on the \"Select Folder\" button, and the software will automatically count the images and load the first one.\n");
			ImGui::NewLine();
			ImGui::Text("General procedure:");
			ImGui::Text("  1. Select the folder with the images to label or a single image.\n  2. Select the algorithm to use or shape to draw (for thresholding and flood fill).\n  3. Draw the shape, lines or set the point markers and execute the operation.\n  4. Add the resulting pixel-mask to the class or redu step 3 \n  5. Redu until all classes are labeled and save the image.\n");
			ImGui::NewLine();

			ImGui::Text("Labeling Shapes and Tools:\n");
			ImGui::TextWrapped("Rectangle / Polygon / Circle: Draw shapes with the left mouse click. Close polygons by connecting to the starting point. Drag points with Crtl + left mouse \nBrush Tool: Paint pixels directly onto the image for temporary class mask additions. \nThresholding: Choose lower color thresholds via the R, G, B, H, S, V inputs or the color picker in the right menu. The upper threshold can be adjusted for\n  each channel with the sliders above the color picker. Perform thresholding with a right - click (or 'S') after drawing shap to create a temporary\n  mask based on selected colors. \nWatershed: Add Points by left clicking, drag them with the left mouse and delete by clicking with a double click. Swich to class to add markers to another one.\n GrabCut (Graph Cut): Add Points or rather lines by holding the left mouse button and dragging. These Elements will be counted as foreground / the current class.\n  Hold Shift or Alt and draw to add to the background. When executing the algorithms wait until the result is displayed. Warning! This may be slow.");

			ImGui::NewLine();

			ImGui::Text("Keyboard Shortcuts for efficiency: \n");
			ImGui::Text("Switch classes with number keys(Alt for 10 - 19).\n");
			ImGui::Text("A : Add region to class mask.\n");
			ImGui::Text("D : Display active class region.\n");
			ImGui::Text("S : Apply theresholding (same as right-click)\n");
			ImGui::Text("F : Toggle fill and reevaluate.\n");
			ImGui::Text("G : Color picker.\n");
			ImGui::Text("E, M: magic wand (flood fill).\n");
			ImGui::Text("Q : Display all class regions.\n");
			//ImGui::Text("W : Watershed.\n");
			ImGui::Text("R : Reset drawn elements\n");
			ImGui::Text("H : Help window toggle.\n");
			ImGui::Text("T : Tool switching.\n");
			ImGui::Text("Ctrl + Z : Undo. (undo again to 'redo')\n");
			ImGui::Text("Ctrl + S : Save and load next image\n");
			ImGui::Text("Ctrl + D : Save the current mask\n");
			ImGui::NewLine();
			ImGui::Text("Mouse Functions for interaction:");
			ImGui::Text("Draw and define shapes with left clicks. Right click to execute the selected algorithm.\n");
			//ImGui::Text("Mousewheel: scroll up and down (if applicable).\n");
			ImGui::Text("Zoom with Ctrl + Mousewheel; Scroll with Mousewheel(Shift for horizontal).\n");
			ImGui::Text("Holding down the middle mouse button (while zoomed in) also allows scrolling.\n");

			ImGui::NewLine();

			ImGui::Text("Hints: ");
			ImGui::TextWrapped(" - Start with the classes to segement, as the background (0) class can be used to \"Reset\".");
			ImGui::TextWrapped(" - Often it is helpful to select the color via the color-picker, then lower the Value (V) slider for the lower boundary. Press S to see the results. If dark pixels are missing lower the Value a bit more. If there are to much light pixels or the wrong color lower the upper color boundary (in RGB space) press S again until the region can be added to the class region.");
			// ImGui::Text("Use the simple rectangle as often as possible and combine it with the magic tool, brush and background class to reset for fastest labeling. (Only use polgon and circle where possible). ");
			ImGui::TextWrapped(" - Points additional points can be added to the polygon once it is finished by holding Ctrl and double clicking.");
			ImGui::TextWrapped(" - If clear contours exist drawing a rectangle roughly around it and applying flood fill algorithm (repeatedly) may be beneficial.");

			ImGui::NewLine();

			ImGui::TextWrapped("Example 1:\n Draw a rectangle by clicking and holding the left mouse button. When then rectangle is done release the button and apply thresholding but right click or pressing 'S' button.\n As a result you get the all the pixels that are inside of the color range and can be changed by e.g. increasing the min Value (HSV) via the 'V' slider to get only the brighter pixels inside the ROI. The resulting mask is overlayed in the color of the selected class and is only a temporary mask. If you are satisfied with this result add the temporary mask to the current classes mask by pressing the 'A' button.\n Press 'D' to show all the pixels that belong to the currently selected class or 'Q' to display all classes labels.");
			ImGui::NewLine();
			ImGui::TextWrapped("Example 2:\n After loading the image select the 'Watershed' entry in the 'Shape / Procedure' dropdown menu. For each class and the background select the respective number (you can use the number keys for this) and click to set marker points into the image for this class. Execute the algorithm by pressing 'S' or right clicking. Inspect the resulting mask and set, delete or push the markers where neccessary and apply the algorithm again. ");
			ImGui::End();//TODO: Add Example 3 maybe with GrabCut						
		}

		static int low = 10, up = 20;
		static bool ff_use_gray = false;
		static bool show_keys_pressed = false;

		if(expert_window) {
			ImGui::Begin("Expert Window", &expert_window); //	ImGuiWindowFlags_HorizontalScrollbar | nImGuiWindowFlags_AlwaysAutoResize); 
			ImGui::Text("Floodfill (Magic Wand) parameters");
			ImGui::Combo("method ", &current_fill_mode, ff_fill_mode,
						 IM_ARRAYSIZE(ff_fill_mode));
			if(ImGui::IsItemHovered())
				ImGui::SetTooltip("Select which method for the filling you want to use.\n For fixed range the lower boundary around 20 and upper boundary around 40 usually lead to good results.");

			// sliders need to be named differently/uniquely - use ## not to display it in the gui 
			if(ImGui::Button("-##Low")) low--; ImGui::SameLine();
			if(ImGui::Button("+##Low")) low++; ImGui::SameLine();
			ImGui::SliderInt("Lower Boundary", &low, 1, 155); 
			up = low > up ? std::min(low + 1, 155) : up;

			if(ImGui::Button("-##Up")) up--; ImGui::SameLine();
			if(ImGui::Button("+##Up")) up++; ImGui::SameLine();
			ImGui::SliderInt("Upper Boundary", &up, 1, 156);
			low = up < low ? std::max(0, up - 1) : low;

			ImGui::Checkbox("use Gray Image", &ff_use_gray);
			if(ImGui::IsItemHovered())
				ImGui::SetTooltip("Decide if you want to use the gray image (instead of RGB one) for applying the filling.");
			ImGui::NewLine();

			static int kernel_size = 1;
			static bool closeRegion = false;

			ImGui::Text("Morphological smooth region");
			if(ImGui::IsItemHovered())
				ImGui::SetTooltip("This parameter applys a morphological closing to the mask region. Is currently for rectangle only and EXPERIMENTAL!");
			ImGui::Checkbox("Apply", &closeRegion);
			ImGui::SliderInt("size of kernel radius ", &kernel_size, 1, 11);
			LabelState::Instance().FillRegion = closeRegion;
			LabelState::Instance().FillSize = (int)kernel_size;

			ImGui::NewLine();
			ImGui::Text("Distance to snap clicked point to the edge");
			ImGui::SliderInt("Distance in Pixels", &snap_to_border_distance, 0, 20);
			if(ImGui::IsItemHovered())
				ImGui::SetTooltip("This parameter defines at which distance from the borders the drawn points are snapped to the border.\n For example, if this value is 3 and the user clicks on the pixel (2, 100), the edge point of the rectangle will be set to (0, 100).");

			/*	Stub for future implementation
			ImGui::Checkbox("Not overwrite background", &not_overwrite_background);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("If set, explicitly set background pixels are not overwritten by other classes (just like any other class.");*/
			ImGui::NewLine();
			ImGui::Checkbox("Use standard brush sizes", &standard_brush_sizes);
			if(ImGui::IsItemHovered())
				ImGui::SetTooltip("As default the size of the brush points is reset to 6 when grab cut is chosen and to 10 for pixel brush.");

			ImGui::NewLine();
			ImGui::Text("Zoom factor (per mouse wheel step)");
			//ImGui::SliderFloat("zoom factor", &zoom.factor, 0.05, 0.5);
			if(ImGui::InputFloat(" ", &zoom.factor, 0.01, 0.1, "%.2f")) {
				if(zoom.factor < 0.01) zoom.factor = 0.01; // maybe change to 0.005
				else if(zoom.factor > 1.0) zoom.factor = 1.0;
			}
			// Create a slider to adjust the font scale factor
			if(ImGui::SliderFloat("Font Scale (global)", &font_scale, 0.5f, 2.0f, "%.1f")) {
				ImGuiIO& io = ImGui::GetIO();
				io.FontGlobalScale = font_scale; // Apply the scale factor to ImGui
			}
			ImGui::NewLine();
			ImGui::Checkbox("Show active region after adding to it", &disp_region_after_adding);
			if(ImGui::IsItemHovered())
				ImGui::SetTooltip("Is like pressing D to display the active region after having pressed A to add the temporary sementation result to the active region.");
			ImGui::NewLine();


			static char suffix_mask[128] = "_mask";
			ImGui::InputText("suffix", suffix_mask, IM_ARRAYSIZE(suffix_mask));
			ImGui::SameLine();
			ImGui::TextDisabled("(?)");
			if(ImGui::IsItemHovered()) {
				ImGui::BeginTooltip();
				ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
				ImGui::TextUnformatted("This text is added at the end of the mask's filename e.g. _mask (Default). Remove if you want the mask to have the same name as the image.");
				ImGui::PopTextWrapPos();
				ImGui::EndTooltip();
			}

			mask_postfix = filter_chars(suffix_mask);


			ImGui::Checkbox("Save Classes seperately ", &seperateMasks);
			if(ImGui::IsItemHovered())
				ImGui::SetTooltip("Save each mask in a seperate (binary 0 or 255) file instead of one PNG.\nEnable this option AND the multiple class labels flag if you want a pixel to be able to belong to multiple class labels (like car and tire).");
			ImGui::Checkbox("Enable multiple (ambiguous) class label for pixels", &multipleClassLabels);
			if(ImGui::IsItemHovered())
				ImGui::SetTooltip("Allow each Pixel to have more than one label (like part and scratch). \nEnable this option to be able to assign more than one label to each pixel. \nThe overwrite other pixels label is then ignored and only the background class can be used to reset class labels. \nMultiple labels can only be saved correctly if the Save Classes seperately option is active.");
			ImGui::NewLine();
			ImGui::Checkbox("Display image name", &show_img_name);
			if(ImGui::IsItemHovered())
				ImGui::SetTooltip("Displays the name of the currently loaded image. Allows you to copy the filename with a right click on it.");
			//ImGui::Checkbox("Display Keys pressed", &show_keys_pressed);
			//if (show_keys_pressed) {
			//	ImGui::Text("Keys pressed:");
			//	const ImGuiKey key_first = 0;
			//	for (ImGuiKey key = key_first; key < ImGuiKey_COUNT; key++) {
			//		if (ImGui::IsKeyPressed(key)) {
			//			ImGui::SameLine();
			//			ImGui::Text("\"%s\" %d", ImGui::GetKeyName(key), key);
			//			std::string debugstr = ImGui::GetKeyName(key);
			//			// std::cout << ImGui::GetKeyName(key) << " " << key << " ";
			//		}
			//	}
			//}

			ImGui::End();
		}


		// Passing CV parameters
		if(evaluate) {
			CreateImageProcParam(current_draw_shape, draw_rect, ImPar, poly, zoom.current, marker, brush_point_details,
								 position_correction, circ, f_point, current_fill_mode, low, up, ff_use_gray);
		}

		// Execute computer vision algorithm(s) and copy the resulting image to the texture
		if((LabelState::Instance().drawingFinished && evaluate)
		   || drawClassRegion
		   || reset_gui) {

#pragma region M1 : MAP_FROM_DEVICE_CONTEXT
			/* Documentation:
			You can use the CreateTexture2D Method of your device to create a
			texture, and then the UpdateSubresource Method of a device-context to
			update the texture data. There is also a Map method for the device -
			context that can be used to give you a pointer to the texture -
			memory. If you want to use that method to update your texture, you
			need to create it with the appropriate CPU access flags and dynamic
			usage.See the D3D11_TEXTURE2D_DESC Structure for information on all
			the flags.UpdateSubresource will probably be more efficient.  */

			// Method 1: mapping from an to the device context
			D3D11_MAPPED_SUBRESOURCE mappedTex;  // Provides access to subresource data.
			auto D3D_feature_info = g_pd3dDevice->GetFeatureLevel();
			// is D3D_FEATURE_LEVEL_11_0 --> the pointerD3D11_MAPPED_SUBRESOURCE{void *pData) is aligned to 16 bytes.)

			// get the texture pointer for the shader view
			ID3D11Texture2D* pTextureInterface = 0;
			ID3D11Resource* res;
			tex_shader_res_view->GetResource(&res);
			res->QueryInterface<ID3D11Texture2D>(&pTextureInterface);
			D3D11_TEXTURE2D_DESC desc;
			pTextureInterface->GetDesc(&desc);  // desc seems to be correct

			// I) First try to get and map the texture pointer, then change the texture and memcopy; 
			// whatever second parameter (UNIT subresource)	may be, set_with_check 0
			auto HRes = g_pd3dDeviceContext->Map(
				pTextureInterface, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedTex);

			// Do the computer vision
			cv::UMat updatedTexCV;
			if(drawClassRegion) {
				if(ImPar.drawAllClasses) {
					updatedTexCV = ApplyCVOperation(ImPar, (float*)&picked_color, DisplayAllClasses);
					ImPar.drawAllClasses = false; // reset 
				} else {
					updatedTexCV = ApplyCVOperation(ImPar, (float*)&picked_color, DisplayClass);
				}
			} else if(current_draw_shape == CutsD) {
				CvOperation OP = GrabCut; // only one of the two
				updatedTexCV = ApplyCVOperation(ImPar, (float*)&picked_color, GrabCut);
				use_grabcut = false;
			} else if(use_floodfill) {
				CvOperation OP = Floodfill;

				if(fill_inner_pixels) {
					OP = (CvOperation)(FillMask | OP);
				}
				updatedTexCV =
					ApplyCVOperation(ImPar, (float*)&picked_color, OP);
				use_floodfill = false;
			} else if(reset_gui) {
				CvOperation OP = Clear;
				updatedTexCV =
					ApplyCVOperation(ImPar, (float*)&picked_color, OP);
				reset_gui = false;
			} else  // default apply CV
			{
				CvOperation OP = Threshold;
				if(fill_inner_pixels) {
					OP = (CvOperation)(FillMask | OP);
				}
				updatedTexCV = ApplyCVOperation(ImPar, (float*)&picked_color, OP);
				// clear brush after adding
				if(ImPar.roi_shape == BrushD) {
					brush_point_details.clear();
				}
			}

			// II) Update the texture with the changes of the image
			uint8_t* dst = (uint8_t*)mappedTex.pData;
			auto pitchINfo = mappedTex.DepthPitch;
			auto pitchINfoRow = mappedTex.RowPitch;

			// pay attention to add the fourth channel !!
			if(updatedTexCV.channels() != 4) {
				cv::cvtColor(updatedTexCV, updatedTexCV, cv::COLOR_BGR2RGBA);
			}
			// else color channel order should be already correct here!

			// uint8_t* src = (uint8_t*)updatedTexCV.data; // only cv::Mat

			// If the UMat is not on the CPU, this will download it to a Mat
			cv::Mat tempMat = updatedTexCV.getMat(cv::ACCESS_READ);
			// Now the Mat can and its .data member can be accessed
			uint8_t* src = (uint8_t*)tempMat.data;

			// src and dst should be rgba data
			int count = 0;
			const uint32_t pixelSize = sizeof(desc.Format);
			const uint32_t srcPitch = pixelSize * desc.Width;
			for(int i = 0; i < desc.Height; i++) {
				// copying should actually be done on the mapped Ressource
				std::memcpy(dst, src, desc.Width * 4);

				dst += mappedTex.RowPitch;
				src += desc.Width * 4;
				count++;
			}

			// uint8_t* pPixel = reinterpret_cast<uint8_t*>(mappedTex.pData); //-->
			// actually does work 	for (int k = 0; k < desc.Height * srcPitch; k++)
			// {
			//		*pPixel = 250;
			//		pPixel++;
			//	}

			//// var2:
			// const uint32_t pixelSize = sizeof(desc.Format);
			// const uint32_t srcPitch = pixelSize * desc.Width;
			// uint8_t* textureData = reinterpret_cast<uint8_t*>(mappedTex.pData);
			// const uint8_t* srcData = reinterpret_cast<const
			// uint8_t*>(pTextureInterface.data()); for (uint32_t i = 0; i <
			// textureHeight; ++i) {
			//	// Copy the texture data for a single row
			//	memcpy(textureData, srcData, srcPitch);

			//	// Advance the pointers
			//	textureData += data.RowPitch;
			//	srcData += srcPitch;
			//}

			// III). call ID3D11DeviceContext::Unmap after finishing writing data to memory (ram)
			g_pd3dDeviceContext->Unmap(pTextureInterface, 0);
			pTextureInterface->Release();
			pTextureInterface = NULL;

#pragma endregion M1 : MAP_FROM_DEVICE_CONTEXT


			evaluate = false;
		}

		// Testing / experimantational

		// Info: winDrawList outside of ::Begin and ::End triggers a debug window !
		// ImDrawList* winDrawList = ImGui::GetWindowDrawList(); // get draw list associated to the current window, to append your own drawing primitives

		// Rendering
		ImGui::Render();
		const float clear_color_with_alpha[4] = {
			clear_color.x * clear_color.w, clear_color.y * clear_color.w,
			clear_color.z * clear_color.w, clear_color.w };
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView,
												   clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		g_pSwapChain->Present(1, 0);  // Present with vsync - DS: limits to monitor's refresh rate (60Hz)
		//g_pSwapChain->Present(0, 0); // Present without vsync

		save_key = false; // reset save with key flag
	}

	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);

	return 0;
}

#pragma region D3DFuncs
// Helper functions 
/* The following functions are based on usage examples from Omar Cornut's Dear ImGui
* with D3D11.For more details, see the original example code at :
* https://github.com/ocornut/imgui
* The functions have been adapted for use in PixLableCV.
*/

bool CreateDeviceD3D(HWND hWnd) {
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	// createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0,
	};
	// DS: create device and swap chain --> DX11 works with two buffer (back- and front-) images are rendered to the backbuffer, then the buffers are swapped
	if(D3D11CreateDeviceAndSwapChain(
		NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags,
		featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
		&g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
		return false;

	CreateRenderTarget();
	return true;
}

void CleanupDeviceD3D() {
	CleanupRenderTarget();
	if(g_pSwapChain) {
		g_pSwapChain->Release();
		g_pSwapChain = NULL;
	}
	if(g_pd3dDeviceContext) {
		g_pd3dDeviceContext->Release();
		g_pd3dDeviceContext = NULL;
	}
	if(g_pd3dDevice) {
		g_pd3dDevice->Release();
		g_pd3dDevice = NULL;
	}
}

void CreateRenderTarget() {
	ID3D11Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL,
										 &g_mainRenderTargetView);
	pBackBuffer->Release();
}

void CleanupRenderTarget() {
	if(g_mainRenderTargetView) {
		g_mainRenderTargetView->Release();
		g_mainRenderTargetView = NULL;
	}
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
															 UINT msg,
															 WPARAM wParam,
															 LPARAM lParam);
// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell
// if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to
// your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data
// to your main application, or clear/overwrite your copy of the keyboard
// data. Generally you may always pass all inputs to dear imgui, and hide them
// from your application based on those two flags. DS: e.g.
// "AddMouseButtonEvent" in imgui_impl_win32.cpp leads here when mouse button
// is clicked
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if(ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;

	switch(msg)  // 258
	{
	case WM_SIZE:
		if(g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
			CleanupRenderTarget();
			g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam),
										(UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN,
										0);
			CreateRenderTarget();
		}
		return 0;
	case WM_SYSCOMMAND:
		if((wParam & 0xfff0) == SC_KEYMENU)  // Disable ALT application menu
			return 0;
		break;
	case WM_MBUTTONUP:  // middle mouse button 
		return 0;
	case WM_RBUTTONUP:
		return 0;

	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}

	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

#pragma endregion