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

#include <iostream>
#include <memory>
#include <chrono>

class Timer {
public:
	Timer()	{
		m_StartTimestamp = std::chrono::high_resolution_clock::now();
	}
	~Timer(){
		Stop();
	}
	double GetTimeInSeconds() {
		auto start = std::chrono::time_point_cast<std::chrono::seconds>(m_StartTimestamp).time_since_epoch().count();
		auto endTimestamp = std::chrono::high_resolution_clock::now();
		auto end = std::chrono::time_point_cast<std::chrono::seconds>(endTimestamp).time_since_epoch().count();
		return end - start; 
	}

	void Stop() {
		auto endTimestamp = std::chrono::high_resolution_clock::now();
		// this will give the microsecond (precise) start and end time
		auto start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimestamp).time_since_epoch().count();
		auto end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimestamp).time_since_epoch().count();

		auto duration = end - start;
		double ms = duration * 0.001;
		 
		std::cout << ms << " ms\n";
	}

private:
	std::chrono::time_point <std::chrono::high_resolution_clock> m_StartTimestamp;

};

static const std::string current_time_and_date() 
{
	time_t now = time(0);
	struct tm tstruct;
	char buf[80];
	tstruct = *localtime(&now);
	//strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
	strftime(buf, sizeof(buf), "%Y-%m-%d %H_%M_%S", &tstruct);
	return buf;
}