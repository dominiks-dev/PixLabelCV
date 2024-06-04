# PixLabelCV 
PixLabelCV is a C++ application for offline, pixel-precise image labeling for semantic segmentation. It designed utilizing an iterative approach using common computer vision algorithms (OpenCV functions) and Dear ImGui for the graphical user interface.

## Features

- Offline image labeling
- Intuitive GUI built with Dear ImGui
- High performance by running image processing and graphis discplay algorithms on the graphics card using C++, Direct3D and OpenCV
- Advanced computer vision algorithms for efficient labeling
- Unique pixel assignment to eliminate overlap-induced ambiguities

## Design and Usage

PixLabelCV offers a unique approach to image labeling by integrating conventional techniques with advanced computer vision algorithms, including Watershed, flood-fill and GrabCut. Unlike other software, it performs segmentation in the chosen region of interest (ROI) within milliseconds, generating a preliminary class mask. If the mask meets the annotator's needs, it can be added to the overall image mask. This iterative process continues until the entire image is labeled.

PixLabelCV combines traditional labeling methods, like rectangles and polygons, with computer vision algorithms ranging from basic thresholding to complex procedures like flood fill and watershed. The tool instantaneously computes and displays segmentations, which annotators can quickly add to a class label or refine by adjusting parameters. Features like morphological closing and unique pixel assignment enhance the user experience by eliminating overlap-induced ambiguities and making the labeling process more intuitive.

## Getting Started

### Prerequisites

- C++ compiler
- Windows OS (or Direct3D11)
- OpenCV library (version 4.6 or newer)

### Building

1. Clone the repository:
   ```sh
   git clone https://github.com/dominiks-dev/PixLabelCV.git

## License

PixLabelCV is dual-licensed under the following licenses:


### GPLv3 License

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

A copy GNU General Public License can be found in the file LICENSE. Else see http://www.gnu.org/licenses/. 

### Commercial License
For commercial inquiries, please contact:

SQB GmbH  
Email: olaf.glaessner@sqb-ilmenau.de

For contributions, issues, or general inquiries, please use the GitHub repository.

## Contributing

We welcome contributions from the community! Here’s how you can help:

1. Fork the repository
2. Create your feature branch (git checkout -b feature/AmazingFeature)
3. Commit your changes (git commit -m 'Add some AmazingFeature')
4. Push to the branch (git push origin feature/AmazingFeature)
5. Open a Pull Request

## Contributor License Agreement

By contributing to PixLabelCV, you agree that your contributions will be licensed under the GPLv3.

## Reporting Issues

If you encounter any issues, please report them on the GitHub repository. Provide a clear and detailed description of the problem, including steps to reproduce the issue.

## Acknowledgements

OpenCV - Library for computer vision\
Dear ImGui - Immediate mode GUI libray