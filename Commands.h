#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iomanip>
#include <time.h>
#include <utime.h>
#include <stack>
#include <string>
#include <map>

using std::cout;
using std::endl;
using std::map;
using std::pair;
using std::stack;
using std::string;
using std::vector;

#if 0
#define FUNC_ENTRY() \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT() \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

const string WHITESPACE = " \n\r\t\f\v";

class Command
{
  // TODO: Add your data members
protected:
  vector<string> args;

public:
  Command(const char *cmd_line, int type);
  virtual ~Command() = default;
  virtual void execute() = 0;
  // virtual void prepare();
  // virtual void cleanup();
  //  TODO: Add your extra methods if needed
  string getCmdStr();
  char **getArgsArr();
};

class BuiltInCommand : public Command
{
public:
  BuiltInCommand(const char *cmd_line);
  virtual ~BuiltInCommand() = default;
};

class ExternalCommand : public Command
{
public:
  ExternalCommand(const char *cmd_line);
  virtual ~ExternalCommand() = default;
  void execute() override;
};

class PipeCommand : public Command
{
  // TODO: Add your data members
public:
  PipeCommand(const char *cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command
{
  // TODO: Add your data members
public:
  explicit RedirectionCommand(const char *cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  // void prepare() override;
  // void cleanup() override;
};

class ChangePromptCommand : public BuiltInCommand
{
  // TODO: Add your data members
public:
  ChangePromptCommand(const char *cmd_line);
  virtual ~ChangePromptCommand() = default;
  void execute() override;
};

class ChangeDirCommand : public BuiltInCommand
{
private:
  stack<string> directories;

public:
  ChangeDirCommand(const char *cmd_line);
  virtual ~ChangeDirCommand() = default;
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand
{
public:
  GetCurrDirCommand(const char *cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand
{
public:
  ShowPidCommand(const char *cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class JobsList
{
private:
  class JobEntry
  {
  private:
    int job_id;
    string cmd_name;
    int process_id;
    time_t create_time;
    bool is_stopped;

  public:
    JobEntry(Command *cmd, int process_id, bool is_stopped,int job_id = 0);
    ~JobEntry() = default;
    void printJobWithTime();
    int getProcessID();
    int getJobId();
    void printAndDie();
    void printAlarm();
    void stopJob();
    void contJob();
    void printJob();
    bool isStopped();
  };
  
  int max_id;
  map<int, JobEntry> jobs;
  stack<JobEntry> foregroundJob;

public:
  JobsList();
  ~JobsList() = default;
  void addJob(Command *cmd, int process_id, bool isForeground = false ,bool is_stopped = false);
  void addJob(JobEntry &job, bool isForeground = false);
  void addJobFromZsignal(JobEntry &job);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry *getJobById(int jobId);
  JobEntry &getFgJob();
  void removeJobById(int jobId);
  JobEntry *getLastJob(int *lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  int getPID(int jobID);
  int getCount();
  void killForegroundJob();
  bool isForeground();
  pid_t getForegroundPid();
  void stopForeground();
  pid_t lastToFront();
  pid_t jobIdToFront(int JobId);
  pid_t lastToBack();
  pid_t jobIdToBack(int JobId);
  void AlarmCheck();
};

class JobsCommand : public BuiltInCommand
{
  // TODO: Add your data members
  JobsList *job_ptr;

public:
  JobsCommand(const char *cmd_line, JobsList *jobs);
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand
{
private:
  int job_id;
  JobsList *job_ptr;

public:
  KillCommand(const char *cmd_line, JobsList *jobs);
  virtual ~KillCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand
{
  JobsList *jobs_ptr;
public:
  ForegroundCommand(const char *cmd_line, JobsList *jobs);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand
{
  JobsList *jobs_ptr;
public:
  BackgroundCommand(const char *cmd_line, JobsList *jobs);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class QuitCommand : public BuiltInCommand
{
  JobsList *jobs_ptr;

public:
  QuitCommand(const char *cmd_line, JobsList *jobs);
  virtual ~QuitCommand() {}
  void execute() override;
};

class IOCommand : public BuiltInCommand
{
protected:
  string destination = "";
  string source = "";
public:
  IOCommand(const char *cmd_line);
  virtual ~IOCommand() {}
  void execute() override;
};

//command > file
class RedirectFileCommand : public IOCommand
{
public:
  RedirectFileCommand(const char *cmd_line);
  virtual ~RedirectFileCommand() {}
  void execute() override;
};

//command >> file
class AppendFileCommand : public IOCommand
{
public:
  AppendFileCommand(const char *cmd_line);
  virtual ~AppendFileCommand() {}
  void execute() override;
};


class TailCommand : public BuiltInCommand
{
public:
  TailCommand(const char *cmd_line);
  virtual ~TailCommand() {}
  void execute() override;
};

class TimeOutCommand : public BuiltInCommand
{
public:
  TimeOutCommand(const char *cmd_line);
  virtual ~TimeOutCommand() {}
  void execute() override;
};

class TouchCommand : public BuiltInCommand
{
public:
  TouchCommand(const char *cmd_line);
  virtual ~TouchCommand() {}
  void execute() override;
};

class SmallShell
{
private:
  string prompt;
  JobsList jobs;
  stack<string> directories;
  SmallShell();

public:
  Command *CreateCommand(const char *cmd_line);
  SmallShell(SmallShell const &) = delete;     // disable copy ctor
  void operator=(SmallShell const &) = delete; // disable = operator
  static SmallShell &getInstance()             // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char *cmd_line);
  void printPtompt();
  void changePrompt(string new_prompt);
  void push_dir(string dir);
  void pop_dir();
  string top_dir();
  bool isEmpty_dir();
  void killForegroundJob();
  bool isForeground();
  pid_t getForegroundPid();
  void stopForeground();
  void runAtFront(pid_t pid);
  void runAtFront(pid_t pid, Command* cmd);
  void AlarmHandle();
};

#endif // SMASH_COMMAND_H_