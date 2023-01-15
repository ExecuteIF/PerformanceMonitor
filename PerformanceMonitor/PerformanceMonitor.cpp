#include "PM.h"

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

string getTime()
{
	time_t timep;
	time(&timep);
	char ans[64];
	strftime(ans, sizeof(ans), "%Y-%m-%d %H:%M:%S", localtime(&timep));
	return ans;
}
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
	void drawNumber(double value, color colour, Mat frame[2], state stat, short decimals)
	{
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
	void drawCircle(double value, color colour, Mat frame[2], state stat)
	{
		if (stat == STATE_FAIL) {
			return;
		}
		if (stat == STATE_ERROR) {
			return;
		}
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
		this->minX = min(_pos1.x, _pos2.x) + 5;
		this->minY = min(_pos1.y, _pos2.y) + 5;
		this->maxX = max(_pos1.x, _pos2.x) - 5;
		this->maxY = max(_pos1.y, _pos2.y) - 5;
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
	void addValue(short value)
	{
		for (int i = this->maxn - 1; i > 0; --i) {
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
	}
private:
	short calcAverage()
	{
		int tmp = 0;
		for (int i = 0; i < this->maxn; ++i) {
			tmp += this->data[i];
		}
		return tmp / this->maxn;
	}
	void drawFrame(color colour, Mat frame[2], state stat)
	{
		rectangle(frame[0], this->pos1, this->pos2, getColor(colour), 1);
	}
	void drawData(color colour, Mat frame[2], state stat)
	{
		for (int i = this->maxn - 1; i > 0; --i) {
			if (this->data[i] != -1 && this->data[i - 1] != -1) {
				line(
					frame[0],
					{ int(this->minX + this->xFactor * (i + 1)), this->data[i - 1] },
					{ int(this->minX + this->xFactor * i), this->data[i] },
					getColor(colour),
					1
				);
			}
		}
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

void sleepUpdater()
{
	while (!stop) {
		sleep = GetForegroundWindow() != hwnd;
		Sleep(250);
	}
}

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
	while (true) {
		memset(buffer, 0, sizeof(buffer));
		if (ReadFile(hRead, buffer, sizeof(buffer), &bytesRead, NULL) == NULL)
			break;
		for (int i = 0; i < strlen(buffer); ++i) {
			data.push(buffer[i]);
		}
	}

	auto nextln = [](queue<char>* dat) {
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
	while (!data.empty()) {
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
double getCPUFreq()
{
	unsigned __int64 t1, t2;
	t1 = __rdtsc();
	Sleep(1000);
	t2 = __rdtsc();
	return (t2 - t1) / 1e9;
}

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
	HWND console = FindWindow(L"ConsoleWindowClass", NULL);
	if (console == nullptr) {
		fatal(-1);
	}
	ShowWindow(console, SW_HIDE); // hide console
	register Mat img = cv::imread("GFX/background.png");
	Mat black = imread("GFX/blank.png");
	if (img.empty() || black.empty()) {
		fatal(-2);
	}
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
	cv::namedWindow("PCAM", WINDOW_AUTOSIZE);
	hwnd = FindWindow(NULL, L"PCAM");
	if (hwnd == nullptr) {
		fatal(-3);
	}
	thread tSleepUpdater(sleepUpdater);
	thread tUpdateCPUUtilData(updateCPUUtilData);
	thread tUpdateRAMUtilData(updateRAMUtilData);
	thread tUpdateCPUFreqData(updateCPUFreqData);
	thread tUpdateSmoother(updateSmoother);
	thread tUpdateProcessesData(updateProcessesData);

	circleActor CPUUtil, RAMUtil, CPUFreq, RAMStat;
	CPUUtil.init({ 165, 87 }, { 117, 96 }, 0, 100, 2.1, 0);
	RAMUtil.init({ 400,87 }, { 349,96 }, 0, 100, 2.1, 0);
	CPUFreq.init({ 165,256 }, { 117,267 }, 0, 10, 20, 0);
	RAMStat.init({ 400,256 }, { 349,267 }, 0, totalRAM / 1000.0, 210 / (totalRAM / 1000), 0);
	lineChartActor CPUChart, RAMChart;
	CPUChart.init({ 40, 380 }, { 340, 520 }, 0, 100, 1, 1, true, true, 100);
	RAMChart.init({ 365,380 }, { 665, 520 }, 0, 100, 1, 1, true, true, 100);

	Mat frame[2];

	clock_t start, end, mspf = 0;
	int fps = 0;

	while(true) {
		start = clock();
		if (sleep) {
			cv::waitKey(1000);
		}
		frame[0] = img.clone();
		frame[1] = black.clone();
		try {
			getWindowProperty("PCAM", 0);
		}
		catch(exception){
			break;
		}
		CPUUtil.draw(
			finalCPUUtilData,
			CPUUtilData > 90 ? COLOR_RED : (CPUUtilData > 80 ? COLOR_AMBER : COLOR_WHITE),
			frame,
			finalCPUUtilData == 0 ? STATE_FAIL : STATE_NORMAL
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
		putText(
			frame[0],
			"last data update: " + getTime() + ", " + to_string(fps) + " fps" + (sleep?"[SLEEP MODE]":"[NORMAL]"),
			{ 5,23 },
			FONT_HERSHEY_PLAIN,
			0.8,
			Scalar(255, 255, 255),
			0.8
		);
		drawTopProcesses(frame[0]);
		CPUChart.addValue(CPUUtilData);
		CPUChart.draw(
			COLOR_WHITE, 
			frame, 
			CPUUtilData==0?STATE_FAIL:STATE_NORMAL
		);
		RAMChart.addValue(RAMUtilData);
		RAMChart.draw(
			COLOR_WHITE,
			frame,
			RAMUtilData == 0 ? STATE_FAIL : STATE_NORMAL
		);


		cv::addWeighted(frame[0], 1, frame[1], 0.3, 0, frame[0]);
		cv::imshow("PCAM", frame[0]);
		cv::waitKey(17);
		end = clock();
		mspf = (double)(end - start);
		fps = 1000 / max(mspf, 1);
	}

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
	::SendMessage(console, WM_CLOSE, 0, 0);
	return 0;
}
