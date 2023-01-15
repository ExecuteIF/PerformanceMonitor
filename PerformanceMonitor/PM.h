/**
 *	FILENAME		:		PM.h
 *	LASTEDIT		:		2023/01/15 12:14
 *	LICENSE			:		GNU Public License version 3
 *	PROGRAMMER		:		Executif
**/

#pragma once

#define _CRT_SECURE_NO_WARNINGS true

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <string>
#include <thread>
#include <sstream>

#include <opencv2/opencv.hpp>

#include <windows.h>

using namespace std;
using namespace cv;

typedef char state;
#define STATE_UNDETECTED	-1
#define STATE_NORMAL		0
#define STATE_FAIL			1
#define STATE_ERROR			2

typedef char color;
#define COLOR_WHITE			0
#define COLOR_AMBER			1
#define COLOR_RED			2
