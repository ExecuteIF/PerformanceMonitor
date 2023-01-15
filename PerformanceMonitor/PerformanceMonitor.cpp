/**
 *	FILENAME		:		PerformanceMonitor.cpp
 *	LASTEDIT		:		2023/01/15 12:14
 *	LICENSE			:		GNU Public License version 3
 *	PROGRAMMER		:		Executif
**/

#include "PM.h"

// global variables
bool stop = false;
bool sleep = false;
double CPUUtilData = 0;
double RAMUtilData = 0;
double finalCPUUtilData = 0;
double finalRAMUtilData = 0;
double CPUFreq = 0;
double finalCPUFreq = 0;
double totalRAM = 0;
HWND hwnd;
vector<pair<string, int> > Processes;
bool finalProcessesInUse = false;
vector<pair<string, int> > finalProcesses;

/// <summary>
/// fatal error handler
/// -1	:	failed to open background.png
/// -2	:	failed to open black.png
/// -3	:	failed to get hwnd
/// 1	:	failed to create pipe
/// 2	:	failed to create process
/// 127	:	attempt to draw a circel actor which hasn't initialized
/// 128	:	attempt to draw a line chart actor which hasn't initialized
/// </summary>
/// <param name="value"></param>
void fatal(int value) {
	MessageBox(hwnd, L"The program went into a problem that it could not handle.\nProgram will exit.", L"PCAM - ERROR", MB_OK | MB_SYSTEMMODAL | MB_ICONHAND);
	exit(value);
}

/// <summary>
/// get current time
/// </summary>
/// <returns></returns>
string getTime()
{
	time_t timep;
	time(&timep);
	char ans[64];
	strftime(ans, sizeof(ans), "%Y-%m-%d %H:%M:%S", localtime(&timep));
	return ans;
}
/// <summary>
/// the base of circle actors and line chart actors
/// </summary>
class Actor {
public:
	bool init_ed = false;
	short minValue;
	short maxValue;
	double valueFactor;
	/// <summary>
	/// get the color
	/// </summary>
	/// <param name="colour"></param>
	/// <returns></returns>
	Scalar getColor(color colour)
	{
		switch (colour) {
		case COLOR_WHITE:
			return Scalar(255, 255, 255);
		case COLOR_AMBER:
			return Scalar(0, 200, 255);
		case COLOR_RED:
			return Scalar(0, 0, 255);
		default:
			throw 0;
		}
	}

};
/// <summary>
/// in charge of drawing circles(engine state)
/// </summary>
class circleActor : public Actor  {
	Point numberPos;
	Point circleCenter;
	short current;
	string tmp;
public:
	void init
	(
		Point _numberPos,
		Point _circleCenter,
		short _minValue,
		short _maxValue,
		double _valueFactor,
		short _initValue
	)
	{
		this->numberPos = _numberPos;
		this->circleCenter = _circleCenter;
		this->minValue = _minValue;
		this->maxValue = _maxValue;
		this->valueFactor = _valueFactor;		// the max angle is 210°
		this->current = _initValue;
		this->init_ed = true;
	}
	/// <summary>
	/// draw(update) the newest frame to the specified image container
	/// </summary>
	/// <param name="value">the new value</param>
	/// <param name="colour">the color</param>
	/// <param name="frame">the specified image container</param>
	void draw(double value, color colour, Mat frame[2], state stat = STATE_NORMAL, short decimals = 1)
	{
		if (!init_ed) {
			fatal(127);
		}
		this->drawNumber(value, colour, frame, stat, decimals);
		this->drawCircle(value, colour, frame, stat);
	}
private:
	/// <summary>
	/// calculate the offset of given number to make the number is always centered.
	/// </summary>
	/// <param name="number"></param>
	/// <returns></returns>
	int calcOffset(string number, short width)
	{
		return (number.length() - 1) * width;
	}
	/// <summary>
	/// draw the value as number
	/// </summary>
	/// <param name="value"></param>
	/// <param name="colour"></param>
	/// <param name="frame">the targeting frames to draw</param>
	/// <param name="stat"></param>
	/// <param name="decimals"></param>
	void drawNumber(double value, color colour, Mat frame[2], state stat, short decimals)
	{
		// different states
		if (stat == STATE_FAIL) {
			this->tmp = "0.0";
			colour = COLOR_AMBER;
			putText(
				frame[0],
				"FAIL",
				{
					this->circleCenter.x - calcOffset(this->tmp, 10),
					this->circleCenter.y + 15
				},
				FONT_HERSHEY_PLAIN,
				1.7,
				getColor(colour),
				1.5
			);
		}
		else if (stat == STATE_ERROR) {
			this->tmp = "0.0";
			colour = COLOR_RED;
			putText(
				frame[0],
				"ERROR",
				{
					this->circleCenter.x - calcOffset(this->tmp, 20),
					this->circleCenter.y + 30
				},
				FONT_HERSHEY_PLAIN,
				1.7,
				getColor(colour),
				1.5
			);
		}
		else {
			this->tmp = to_string(value);
			this->tmp = this->tmp.substr(0, this->tmp.find(".") + decimals + 1);
		}
		putText(
			frame[0],
			this->tmp, 
			{ 
				this->numberPos.x - calcOffset(this->tmp, 7.5),
				this->numberPos.y 
			}, 
			FONT_HERSHEY_PLAIN, 
			1.5,
			getColor(colour),
			1.5
		);
	}
	/// <summary>
	/// draw the circle(graphic display part)
	/// </summary>
	/// <param name="value"></param>
	/// <param name="colour"></param>
	/// <param name="frame"></param>
	/// <param name="stat"></param>
	void drawCircle(double value, color colour, Mat frame[2], state stat)
	{
		// different states, just exit
		if (stat == STATE_FAIL) {
			return;
		}
		if (stat == STATE_ERROR) {
			return;
		}
		// draw the fan-shape
		ellipse (
			frame[1],
			this->circleCenter,
			Size(int(73), int(73)),
			0,
			0,
			value*this->valueFactor,
			getColor(colour),
			-1
		);
		// draw the pointer
		ellipse(
			frame[0],
			this->circleCenter,
			Size(int(80), int(80)),
			0,
			value * this->valueFactor,
			value * this->valueFactor + 1,
			getColor(colour),
			-1
		);
	}
};
/// <summary>
/// in charge of drawing line charts
/// </summary>
class lineChartActor : public Actor {
	short maxn;
	bool doDrawFrame;
	bool doDrawAverage;
	double xFactor;
	vector<short> data;
	Point pos1, pos2;
	int minX, maxX, minY, maxY;
public:
	void init(
		Point _pos1,
		Point _pos2,
		short _minValue,
		short _maxValue,
		double _valueFactor,
		double _xFactor,
		bool _drawFrame,
		bool _drawAverage,
		short _maxn
	)
	{
		this->pos1 = _pos1;
		this->pos2 = _pos2;
		this->minX = min(_pos1.x, _pos2.x);
		this->minY = min(_pos1.y, _pos2.y) + 2;
		this->maxX = max(_pos1.x, _pos2.x);
		this->maxY = max(_pos1.y, _pos2.y) - 2;
		this->minValue = _minValue;
		this->maxValue = _maxValue;
		this->valueFactor = _valueFactor;
		this->xFactor = _xFactor;
		this->doDrawFrame = _drawFrame;
		this->doDrawAverage = _drawAverage;
		this->maxn = _maxn;
		this->init_ed = true;
		for (int i = 0; i < this->maxn + 1; ++i) {
			data.push_back(-1);
		}
	}
	/// <summary>
	/// add a value.
	/// </summary>
	/// <param name="value"></param>
	void addValue(short value)
	{
		for (int i = this->maxn - 1; i >= 0; --i) {
			this->data[i + 1] = this->data[i];
		}
		this->data[0] = value;
	}
	void draw(color colour, Mat frame[2], state stat = STATE_NORMAL)
	{
		if (!init_ed) {
			fatal(128);
		}
		if (this->doDrawFrame) {
			this->drawFrame(colour, frame, stat);
		}
		this->drawData(colour, frame, stat);
		if (this->doDrawAverage) {
			this->drawAverage(frame, stat);
		}
	}
private:
	/// <summary>
	/// calculate the average value
	/// </summary>
	/// <returns></returns>
	short calcAverage()
	{
		int tmp = 0;
		int minus = 0;
		for (int i = 0; i < this->maxn; ++i) {
			if (tmp != -1) {
				tmp += this->data[i];
				++minus;
			}
		}
		return minus == 0 ? -1 : (tmp / minus);
	}
	/// <summary>
	/// draw the frame of all(a rectangle)
	/// </summary>
	/// <param name="colour"></param>
	/// <param name="frame"></param>
	/// <param name="stat"></param>
	void drawFrame(color colour, Mat frame[2], state stat)
	{
		rectangle(frame[0], this->pos1, this->pos2, getColor(colour), 1);
	}
	/// <summary>
	/// draw the data
	/// </summary>
	/// <param name="colour"></param>
	/// <param name="frame"></param>
	/// <param name="stat"></param>
	void drawData(color colour, Mat frame[2], state stat)
	{
		// draw all datas
		for (int i = this->maxn - 1; i > 0; --i) {
			if (this->data[i] != -1 && this->data[i - 1] != -1) { // if the data is valid
				// draw the data
				line(
					frame[0],
					{ 
						int(this->maxX - this->xFactor * (i - 1)), 
						int(round(this->maxY - (this->data[i - 1] * this->valueFactor)))
					},
					{ 
						int(this->maxX - this->xFactor * i), 
						int(round(this->maxY - (this->data[i] * this->valueFactor)))
					},
					getColor(colour),
					2
				);
				// draw the shadow
				line(
					frame[1],
					{
						int(this->maxX - this->xFactor * (i - 1)),
						int(round(this->maxY))
					},
					{
						int(this->maxX - this->xFactor * (i - 1)),
						int(round(this->maxY - (this->data[i] * this->valueFactor)))
					},
					getColor(colour),
					2
				);
			}
		}
	}
	void drawAverage(Mat frame[2], state stat)
	{
		short average = calcAverage();
		// draw the average line covers on the data
		line(
			frame[0],
			{
				int(this->minX - this->xFactor),
				int(round(this->maxY - (average * this->valueFactor)))
			},
			{
				int(this->maxX - this->xFactor),
				int(round(this->maxY - (average * this->valueFactor)))
			},
			Scalar(0, 255, 0),
			1
		);
		// draw the upper part of '>'
		line(
			frame[0],
			{
				int(this->minX - 5 - this->xFactor),
				int(round(this->maxY - (average * this->valueFactor)) + 5)
			},
			{
				int(this->minX + 2 - this->xFactor),
				int(round(this->maxY - (average * this->valueFactor)))
			},
			Scalar(0, 255, 0),
			2
		);
		// draw the downer part of '>'
		line(
			frame[0],
			{
				int(this->minX - 5 - this->xFactor),
				int(round(this->maxY - (average * this->valueFactor)) - 5)
			},
			{
				int(this->minX + 2 - this->xFactor),
				int(round(this->maxY - (average * this->valueFactor)))
			},
			Scalar(0, 255, 0),
			2
		);
	}
};
void drawTopProcesses(Mat frame) {
	while (finalProcessesInUse) {
		Sleep(50);
	}
	finalProcessesInUse = true;
	const int topProcCnt = 15;
	const int maxNameLength = 15;
	const int offsetY = 70;
	const int offsetX = 450;
	const int offsetXText = 160;
	string tmp;
	if (finalProcesses.size() == 0) {
		for (int i = 0; i < topProcCnt; ++i) {
			finalProcesses.push_back({ "FAIL" , 10e5 });
		}
	}
	for (int i = 0; i < min(topProcCnt, finalProcesses.size()); ++i) {
		tmp.clear();
		if (finalProcesses[i].first.length() > maxNameLength - 3) {
			for (int j = 0; j < maxNameLength - 3; ++j) {
				tmp += finalProcesses[i].first[j];
			}
			tmp += "...";
		}
		else {
			tmp = finalProcesses[i].first;
		}
		putText(
			frame,
			tmp,
			{
				offsetX,
				offsetY + i * 20
			},
			FONT_HERSHEY_PLAIN,
			1,
			finalProcesses[i].second < 1e6 ? Scalar(255, 255, 255) : (finalProcesses[i].second < 4e6 ? Scalar(0, 100, 255) : Scalar(0, 0, 255)),
			1
		);
		if (finalProcesses[i].second > 1e6) {
			tmp = to_string(finalProcesses[i].second / 1e6);
			tmp = tmp.substr(0, tmp.find(".") + 2);
			tmp += " G";
		}
		else {
			tmp = to_string(finalProcesses[i].second / 1000.0);
			tmp = tmp.substr(0, tmp.find(".") + 2);
			tmp += " M";
		}
		putText(
			frame,
			tmp,
			{
				offsetX + offsetXText,
				offsetY + i * 20
			},
			FONT_HERSHEY_PLAIN,
			1,
			finalProcesses[i].second < 1e6 ? Scalar(255, 255, 255) : (finalProcesses[i].second < 4e6 ? Scalar(0, 100, 255) : Scalar(0, 0, 255)),
			1
		);
	}
	finalProcessesInUse = false;
}

/// <summary>
/// desides when the program sleep(use very low CPU)
/// </summary>
void sleepUpdater()
{
	while (!stop) {
		sleep = IsIconic(hwnd);
		Sleep(250);
	}
}

/// <summary>
/// get the list of all the processes
/// </summary>
void getProcesses()
{
	SECURITY_ATTRIBUTES sa;
	HANDLE hRead, hWrite;

	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	if (!CreatePipe(&hRead, &hWrite, &sa, 0))
	{
		fatal(1);
	}

	STARTUPINFO   si;
	PROCESS_INFORMATION   pi;
	si.cb = sizeof(STARTUPINFO);
	GetStartupInfo(&si);
	si.hStdError = hWrite;
	si.hStdOutput = hWrite;
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	TCHAR sCmd[] = L"tasklist /fo csv";
	if (
		!CreateProcess(
			NULL,
			sCmd,
			NULL, NULL, TRUE, NULL, NULL, NULL,
			&si,
			&pi
		)
		)
	{
		fatal(2);
	}
	CloseHandle(hWrite);

	char   buffer[1024] = { 0 };
	DWORD   bytesRead;
	queue<char> data;
	string tmp, temp, currentln;
	char temp_;
	int _temp;
	stringstream stream;
	Processes.clear();
	memset(buffer, 0, sizeof(buffer));
	while (true) { // read the result of console command
		memset(buffer, 0, sizeof(buffer));
		if (ReadFile(hRead, buffer, sizeof(buffer), &bytesRead, NULL) == NULL)
			break;
		for (int i = 0; i < strlen(buffer); ++i) {
			data.push(buffer[i]);
		}
	}

	auto nextln = [](queue<char>* dat) { // get next line
		char tmp;
		string ans;
		for (register int i = 0;; ++i) {
			tmp = dat->front();
			dat->pop();
			if (tmp == '\n') {
				break;
			}
			ans += tmp;
		}
		return ans;
	};
	nextln(&data);
	while (!data.empty()) { // get all the process datas and store them into vector<pair<string, int>>Processes
		currentln = nextln(&data);
		tmp.clear();
		temp.clear();
		for (int i = 1;; ++i) {
			if (currentln[i] == '\"') {
				break;
			}
			tmp += currentln[i];
		}
		_temp = 0;
		for (int i = 9; i > 0;) {
			if (currentln[_temp] == '\"') {
				--i;
			}
			++_temp;
		}
		for (int j = _temp; j < currentln.length(); ++j) {
			if (currentln[j] == '\"') {
				break;
			}
			temp += currentln[j];
		}
		_temp = 0;
		stream.clear();
		stream << temp;
		for (int j = 0;; ++j) {
			stream >> temp_;
			if (temp_ == ' ') {
				continue;
			}
			if (temp_ == ',') {
				continue;
			}
			if (temp_ == 'K') {
				break;
			}
			if (temp_ >= '0' && temp_ <= '9') {
				_temp *= 10;
				_temp += temp_ - '0';
			}
		}
		Processes.push_back({ tmp,_temp });
		//std::cout << tmp << "\t" << _temp << endl;
	}
}
__int64 Filetime2Int64(const FILETIME& ftime)
{
	LARGE_INTEGER li;
	li.LowPart = ftime.dwLowDateTime;
	li.HighPart = ftime.dwHighDateTime;
	return li.QuadPart;
}
__int64 CompareFileTime2(const FILETIME& preTime, const FILETIME& nowTime)
{
	return Filetime2Int64(nowTime) - Filetime2Int64(preTime);
}
/// <summary>
/// get CPU utility (x%)
/// </summary>
/// <returns></returns>
double getCPUUtilPercent()
{
	FILETIME preIdleTime;
	FILETIME preKernelTime;
	FILETIME preUserTime;
	GetSystemTimes(&preIdleTime, &preKernelTime, &preUserTime);

	Sleep(1000);

	FILETIME idleTime;
	FILETIME kernelTime;
	FILETIME userTime;
	GetSystemTimes(&idleTime, &kernelTime, &userTime);

	auto idle = CompareFileTime2(preIdleTime, idleTime);
	auto kernel = CompareFileTime2(preKernelTime, kernelTime);
	auto user = CompareFileTime2(preUserTime, userTime);

	if (kernel + user == 0)
		return 0;

	return 1.0 * (kernel + user - idle) / (kernel + user);
}
/// <summary>
/// get RAM utility (x%)
/// </summary>
/// <param name="getPhysicalMemory"></param>
/// <returns></returns>
double getRAMUtilPercent(bool getPhysicalMemory = false)
{
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);

	DWORDLONG physical_memory = statex.ullTotalPhys / (1024 * 1024);
	DWORDLONG usePhys = physical_memory - (statex.ullAvailPhys / (1024 * 1024));
	if (getPhysicalMemory) {
		return physical_memory;
	}
	return (double)usePhys / (double)physical_memory;
}
/// <summary>
/// get CPU frequency (GHZ)
/// </summary>
/// <returns></returns>
double getCPUFreq()
{
	unsigned __int64 t1, t2;
	t1 = __rdtsc();
	Sleep(1000);
	t2 = __rdtsc();
	return (t2 - t1) / 1e9;
}


// update the datas in given frequency(currently 1 second per update)
void updateProcessesData()
{
	auto sortCondition = [](pair<string, int> a, pair<string, int> b) {
		return a.second > b.second;
	};
	while (!stop) {
		if (sleep) {
			Sleep(1000);
		}
		getProcesses();
		sort(Processes.begin(), Processes.end(), sortCondition);
		while (finalProcessesInUse) {
			Sleep(50);
		}
		finalProcessesInUse = true;
		finalProcesses.clear();
		finalProcesses.insert(finalProcesses.end(),Processes.begin(),Processes.end());
		finalProcessesInUse = false;
	}
}
void updateCPUUtilData()
{
	while (!stop) {
		/*
		CPUUtilData += 0.1;
		if (CPUUtilData > 100) {
			CPUUtilData = 0;
		}
		Sleep(10);
		*/
		CPUUtilData = getCPUUtilPercent() * 100;
	}
}
void updateRAMUtilData()
{
	while (!stop) {
		/*
		RAMUtilData += 0.1;
		if (RAMUtilData > 100) {
			RAMUtilData = 0;
		}
		Sleep(10);
		*/
		Sleep(1000);
		RAMUtilData = getRAMUtilPercent() * 100;
	}
}
void updateCPUFreqData()
{
	while (!stop) {
		/*
		CPUFreq += 0.01;
		if (CPUFreq > 10) {
			CPUFreq = 0;
		}
		finalCPUFreq = CPUFreq;
		Sleep(10);
		*/
		CPUFreq = getCPUFreq();
	}
}

// smooth the update makes it not seems to bad
void updateSmoother()
{
	while (!stop) {
		if (sleep) {
			finalCPUUtilData = CPUUtilData;
			finalRAMUtilData = RAMUtilData;
			finalCPUFreq = CPUFreq;
			Sleep(250);
			continue;
		}
		finalCPUUtilData = finalCPUUtilData - (finalCPUUtilData - CPUUtilData) * 0.1;
		finalRAMUtilData = finalRAMUtilData - (finalRAMUtilData - RAMUtilData) * 0.1;
		finalCPUFreq = finalCPUFreq - (finalCPUFreq - CPUFreq) * 0.1;
		/*
		// make the change appears faster(when boosts up from 0 to 100 will not take so much time), 
		// but seems worse when running normally
		if (abs(finalCPUUtilData - CPUUtilData) < 1) {
			finalCPUUtilData = CPUUtilData;
		}
		*/
		Sleep(50);
	}
}

int main(int argc, char* argv[])
{
	// hide console
	HWND console = FindWindow(L"ConsoleWindowClass", NULL);
	if (console == nullptr) {
		fatal(-1);
	}
	ShowWindow(console, SW_HIDE);

	// read initial images
	register Mat img = cv::imread("GFX/background.png");
	Mat black = imread("GFX/blank.png");
	if (img.empty() || black.empty()) {
		fatal(-2);
	}

	// pre-process initial images
	// attach RAM state
	totalRAM = getRAMUtilPercent(true);
	int tmp = totalRAM / 1000;
	double temp;
	putText(
		img,
		to_string(tmp),
		{
			305,
			255
		},
		FONT_HERSHEY_DUPLEX,
		0.5,
		Scalar(255, 255, 255),
		1.5
	);
	// attach upper lines of texts
	WCHAR PCName[255];
	unsigned long size = 255;
	GetComputerName(PCName, &size);
	char _tmp[512] = {"\0"};
	wcstombs(_tmp, PCName, 255);
	putText(
		img,
		"real-time hardware performance of computer named \"" + ((string)_tmp) + "\",",
		{ 5,11 },
		FONT_HERSHEY_PLAIN,
		0.8,
		Scalar(255, 255, 255),
		0.8
	);

	// create openCV window and get the handle of targeting window
	cv::namedWindow("PCAM", WINDOW_AUTOSIZE);
	hwnd = FindWindow(NULL, L"PCAM");
	if (hwnd == nullptr) {
		fatal(-3);
	}

	// create updater processes
	thread tSleepUpdater(sleepUpdater);
	thread tUpdateCPUUtilData(updateCPUUtilData);
	thread tUpdateRAMUtilData(updateRAMUtilData);
	thread tUpdateCPUFreqData(updateCPUFreqData);
	thread tUpdateSmoother(updateSmoother);
	thread tUpdateProcessesData(updateProcessesData);

	// create actors
	circleActor CPUUtil, RAMUtil, CPUFreq, RAMStat;
	lineChartActor CPUChart, RAMChart;
	CPUUtil.init({ 165, 87 }, { 117, 96 }, 0, 100, 2.1, 0);
	RAMUtil.init({ 400,87 }, { 349,96 }, 0, 100, 2.1, 0);
	CPUFreq.init({ 165,256 }, { 117,267 }, 0, 10, 20, 0);
	RAMStat.init({ 400,256 }, { 349,267 }, 0, totalRAM / 1000.0, 210 / (totalRAM / 1000), 0);
	CPUChart.init({ 40, 380 }, { 340, 520 }, 0, 100, 1.3, 2, true, true, 151);
	RAMChart.init({ 365,380 }, { 665, 520 }, 0, 100, 1.3, 2, true, true, 151);

	// init frames
	Mat frame[2];

	// init FPS loggers
	clock_t start, end, mspf = 0;
	int fps = 0;

	// main loop
	while(true) {
		// log fps data 1/2
		start = clock();

		// check if can sleep
		if (sleep) {
			cv::waitKey(1000);
		}
		// clone basic frames and start drawing
		frame[0] = img.clone();
		frame[1] = black.clone();
		//                                                 ↓
		// check if need to exit(close window button "_ 口 X" clicked)
		//                                                 ↑
		try {
			getWindowProperty("PCAM", 0);
		}
		catch(exception){
			break;
		}
		// draw frames
		CPUUtil.draw(
			finalCPUUtilData, // the data
			CPUUtilData > 90 ? COLOR_RED : (CPUUtilData > 80 ? COLOR_AMBER : COLOR_WHITE), // the color, currently controlled by the utility(<80%: white; <90%: amber; <100%: red)
			frame, // the targeting frames
			finalCPUUtilData == 0 ? STATE_FAIL : STATE_NORMAL // the state, currently controlled by utility data(0: fail, any other: normal)
		);
		RAMUtil.draw(
			finalRAMUtilData,
			RAMUtilData > 90 ? COLOR_RED : (RAMUtilData > 80 ? COLOR_AMBER : COLOR_WHITE),
			frame,
			finalRAMUtilData == 0 ? STATE_FAIL : STATE_NORMAL
		);
		CPUFreq.draw(
			finalCPUFreq,
			COLOR_WHITE,
			frame,
			finalCPUFreq == 0 ? STATE_FAIL : STATE_NORMAL,
			2
		);
		temp = finalRAMUtilData / 100.0 * totalRAM / 1000.0;
		RAMStat.draw(
			temp,
			RAMUtilData > 90 ? COLOR_RED : (RAMUtilData > 80 ? COLOR_AMBER : COLOR_WHITE),
			frame,
			temp == 0 ? STATE_FAIL : STATE_NORMAL
		);
		// draw last data update time and fps information on the upper line
		putText(
			frame[0],
			"last data update: " + getTime() + ", " + to_string(fps) + " fps" + (sleep?"[SLEEP MODE]":"[NORMAL]"),
			{ 5,23 },
			FONT_HERSHEY_PLAIN,
			0.8,
			Scalar(255, 255, 255),
			0.8
		);
		// draw top ram use processes
		drawTopProcesses(frame[0]);

		// draw line chart of CPU utility
		CPUChart.addValue(finalCPUUtilData);
		CPUChart.draw(
			COLOR_WHITE, // color
			frame,  // targeting frames
			CPUUtilData==0?STATE_FAIL:STATE_NORMAL // the state, currently controlled by utility(0: fail, any other: normal)
		);

		// draw line chart of RAM utility
		RAMChart.addValue(finalRAMUtilData);
		RAMChart.draw(
			COLOR_WHITE,
			frame,
			RAMUtilData == 0 ? STATE_FAIL : STATE_NORMAL
		);

		// combie the two frames into 1, get 100% of frame0 and 30% frame1
		cv::addWeighted(frame[0], 1, frame[1], 0.3, 0, frame[0]);
		// display the frame to the window
		cv::imshow("PCAM", frame[0]);
		// wait 17 ms
		cv::waitKey(17);
		// log fps data 2/2
		end = clock();
		// process fps data
		mspf = (double)(end - start);
		fps = 1000 / max(mspf, 1);
	}

	// normal end process
	stop = true;
	if (tSleepUpdater.joinable()) {
		tSleepUpdater.join();
	}
	sleep = false;
	if (tUpdateProcessesData.joinable()) {
		tUpdateProcessesData.detach();
	}
	if (tUpdateCPUUtilData.joinable()) {
		tUpdateCPUUtilData.detach();
	}
	if (tUpdateRAMUtilData.joinable()) {
		tUpdateRAMUtilData.detach();
	}
	if (tUpdateCPUFreqData.joinable()) {
		tUpdateCPUFreqData.detach();
	}
	if (tUpdateSmoother.joinable()) {
		tUpdateSmoother.detach();
	}
	// destroy the console window
	::SendMessage(console, WM_CLOSE, 0, 0);
	return 0;
}
