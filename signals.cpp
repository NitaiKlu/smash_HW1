#include "signals.h"
#include "Commands.h"
using namespace std;

void ctrlZHandler(int sig_num)
{
  cout << "smash: got ctrl-Z" << endl;
  SmallShell &smash = SmallShell::getInstance();
  if (!smash.isForeground()) return;
  pid_t pid = smash.getForegroundPid();
  smash.stopForeground();
  kill(pid, SIGTSTP);
  cout << "smash: process "<< pid <<" was stopped" << endl;
}

void ctrlCHandler(int sig_num)
{
  cout << "smash: got ctrl-C" << endl;
  SmallShell &smash = SmallShell::getInstance();
  if (!smash.isForeground()) return;
  pid_t pid = smash.getForegroundPid();
  smash.killForegroundJob();
  kill(pid, SIGKILL);
  cout << "smash: process "<< pid <<" was killed" << endl;
}

void alarmHandler(int sig_num)
{
  SmallShell &smash = SmallShell::getInstance();
  cout << "smash: got an alarm" << endl;
  smash.AlarmHandle();  
}
