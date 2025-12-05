#pragma once

class Timer
{
public:

	Timer();

	void Start();
	int ReadSec() const;
	float ReadMSec() const;

private:
	int startTime;
};