#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;

struct StatData
{
  void parse(const string& content)
  {
	size_t rp = content.rfind(')');
    std::istringstream iss(content.data() + rp + 1);

    //            0    1    2    3     4    5       6   7 8 9  11  13   15
    // 3770 (cat) R 3718 3770 3718 34818 3770 4202496 214 0 0 0 0 0 0 0 20
    // 16  18     19      20 21                   22      23      24              25
    //  0 1 0 298215 5750784 81 18446744073709551615 4194304 4242836 140736345340592
    //              26
    // 140736066274232 140575670169216 0 0 0 0 0 0 0 17 0 0 0 0 0 0

    iss >> state;
    iss >> ppid >> pgrp >> session >> tty_nr >> tpgid >> flags;
    iss >> minflt >> cminflt >> majflt >> cmajflt;
    iss >> utime >> stime >> cutime >> cstime;
    iss >> priority >> nice >> num_threads >> itrealvalue >> starttime;
  }
  string name; 
  char state;
  int ppid;
  int pgrp;
  int session;
  int tty_nr;
  int tpgid;
  int flags;

  long minflt;
  long cminflt;
  long majflt;
  long cmajflt;

  long utime;
  long stime;
  long cutime;
  long cstime;

  long priority;
  long nice;
  long num_threads;
  long itrealvalue;
  long starttime;
};

int clockTicks = static_cast<int>(::sysconf(_SC_CLK_TCK));
const int period = 2;
int pid;
int ticks;
StatData lastStatData;

bool processExists(pid_t pid)
{
  char filename[256];
  snprintf(filename, sizeof filename, "/proc/%d/stat", pid);
  return ::access(filename, R_OK) == 0;
}

//read /proc/pid/stat
string readProcFile(int pid) {
	char filename[256];
	snprintf(filename, sizeof filename, "/proc/%d/stat", pid);
	ifstream in;
	in.open(filename);
	stringstream ss;
	ss << in.rdbuf();
	
	string ret = ss.str();
	return ret;
}

double cpuUsage(int userTicks, int sysTicks, double kPeriod, double kClockTicksPerSecond)
{
	return (userTicks + sysTicks) / (kClockTicksPerSecond * kPeriod); //CPU使用率计算
}

void tick(int num) {
	string content = readProcFile(pid);

	StatData statData;
	memset(&statData, 0, sizeof statData);
	statData.parse(content);
	if (ticks > 0) {
		int userTicks = std::max(0, static_cast<int>(statData.utime - lastStatData.utime));	
		int sysTicks = std::max(0, static_cast<int>(statData.stime - lastStatData.stime));
		printf("pid %d cpu usage:%.1f%%\n", pid, cpuUsage(userTicks, sysTicks, period, clockTicks) * 100);
	}
	ticks++;
	lastStatData = statData;
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Usage: %s pid\n", argv[0]);
		return 0;
	}
	pid = atoi(argv[1]);
	if (!processExists(pid)) {
		printf("Process %d doesn't exist.\n", pid);
    	return 1;
	}

	if (signal(SIGALRM, tick) == SIG_ERR) {
		exit(0);
	}
	
	struct itimerval tick;
	memset(&tick, 0, sizeof tick);
	tick.it_value.tv_sec = period;
	tick.it_value.tv_usec = 0;
	tick.it_interval.tv_sec = period;
	tick.it_interval.tv_usec = 0;

	setitimer(ITIMER_REAL, &tick, NULL);

	while (1) {
		pause();
	}
	
	return 0;
}
