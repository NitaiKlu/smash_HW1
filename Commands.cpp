#include "Commands.h"
#define BUILT_IN 1
//**************text parsing**********************
string _ltrim(const std::string &s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args)
{
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for (std::string s; iss >> s;)
  {
    args[i] = (char *)malloc(s.length() + 1);
    memset(args[i], 0, s.length() + 1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

vector<string> _parseCommandLineVector(const char *cmd_line)
{
  FUNC_ENTRY()
  vector<string> args;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for (std::string s; iss >> s;)
  {
    args.push_back(s);
  }
  return args;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line)
{
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line)
{
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos)
  {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&')
  {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}
//**************classes implementation**********************

//**************Command**********************
Command::Command(const char *cmd_line, int type)
{
  if (type == BUILT_IN && _isBackgroundComamnd(cmd_line))
  {
    char *cmd_line_new = new char[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_line_new, cmd_line);
    _removeBackgroundSign(cmd_line_new);
    args = _parseCommandLineVector(cmd_line_new);
    delete cmd_line_new;
    return;
  }
  args = _parseCommandLineVector(cmd_line);
}

string Command::getCmdStr()
{
  string str = "";
  for (auto arg : args)
  {
    str += arg;
    str += " ";
  }
  return str;
}

char **Command::getArgsArr()
{
  char **argsArr = new char *[4];
  argsArr[0] = (char *)("/bin/bash");
  argsArr[1] = (char *)("-c");
  string str = getCmdStr();
  argsArr[2] = new char[str.length() + 1];
  strcpy(argsArr[2], getCmdStr().c_str());
  // argsArr[2] = const_cast<char *>(getCmdStr().c_str());
  argsArr[3] = NULL;
  return argsArr;
}

//****************************************************
//**************BuiltInCommand************************
//****************************************************

BuiltInCommand::BuiltInCommand(const char *cmd_line)
    : Command(cmd_line, BUILT_IN)
{
}

//**************chprompt**********************
ChangePromptCommand::ChangePromptCommand(const char *cmd_line)
    : BuiltInCommand(cmd_line)
{
}

void ChangePromptCommand::execute()
{
  SmallShell &smash = SmallShell::getInstance();
  string new_dir = (args.size() > 1) ? args[1] : "smash";
  smash.changePrompt(new_dir);
}

//**************showpid**********************
ShowPidCommand::ShowPidCommand(const char *cmd_line)
    : BuiltInCommand(cmd_line)
{
}

void ShowPidCommand::execute()
{
  // SmallShell &smash = SmallShell::getInstance();
  pid_t pid = getpid();
  printf("%d \n", pid);
}

//**************cd command**********************
ChangeDirCommand::ChangeDirCommand(const char *cmd_line)
    : BuiltInCommand(cmd_line)
{
}

void ChangeDirCommand::execute()
{
  SmallShell &smash = SmallShell::getInstance();
  char path[COMMAND_ARGS_MAX_LENGTH];
  if (args.size() != 2)
  {
    cout << "smash error: cd: too many arguments" << endl;
    return;
  }
  if (args[1].compare("-") == 0)
  { // cd to last working cd
    if (smash.isEmpty_dir())
    {
      cout << "smash error: cd: OLDPWD not set" << endl;
      return;
    }
    strcpy(path, smash.top_dir().c_str());
    smash.pop_dir();
    chdir(path);
    return;
  }
  // push current wd to the stack
  getcwd(path, COMMAND_ARGS_MAX_LENGTH);
  string s_path = path;
  smash.push_dir(s_path);
  // cd to whatever path specified
  int res = chdir(args[1].c_str());
  if (res == -1)
  {
    cout << "smash error: chdir failed" << endl;
    smash.pop_dir();
    return;
  }
}

//**************pwd command**********************
GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line)
    : BuiltInCommand(cmd_line)
{
}

void GetCurrDirCommand::execute()
{
  char curr_dir[COMMAND_ARGS_MAX_LENGTH];
  getcwd(curr_dir, COMMAND_ARGS_MAX_LENGTH);
  std::cout << curr_dir << std::endl;
}

//**************JobEntry**********************
JobsList::JobEntry::JobEntry(Command *cmd, int process_id, bool is_stopped, int job_id)
    : job_id(job_id), process_id(process_id), is_stopped(is_stopped)
{
  cmd_name = cmd->getCmdStr();
  time(&create_time);
}

void JobsList::JobEntry::printJobWithTime()
{
  time_t now;
  time(&now);
  double elapsed = difftime(now, create_time);
  std::cout << cmd_name << ": " << process_id << " " << elapsed << " secs";
  if (is_stopped)
    std::cout << " (stopped)";
  std::cout << std::endl;
}

int JobsList::JobEntry::getJobId()
{
  return this->job_id;
}

int JobsList::JobEntry::getProcessID()
{
  return this->process_id;
}

void JobsList::JobEntry::printAndDie()
{
  std::cout << process_id << ": " << cmd_name << std::endl;
  kill(this->process_id, SIGKILL);
}

void JobsList::JobEntry::stopJob()
{
  is_stopped = true;
}

void JobsList::JobEntry::contJob()
{
  is_stopped = false;
}

void JobsList::JobEntry::printJob()
{
  std::cout << cmd_name << ": " << process_id << std::endl;
}

bool JobsList::JobEntry::isStopped()
{
  return is_stopped;
}
void JobsList::JobEntry::printAlarm()
{
  cout << "smash: " << cmd_name << " timed_out!" << endl;
}

//**************JobList**********************
JobsList::JobsList()
    : max_id(0)
{
}

void JobsList::printJobsList()
{
  for (auto &job_it : jobs)
  {
    std::cout << "[" << job_it.first << "] ";
    job_it.second.printJobWithTime();
  }
}

void JobsList::addJob(Command *cmd, int process_id, bool isForeground, bool is_stopped)
{
  removeFinishedJobs();
  JobsList::JobEntry job(cmd, process_id, is_stopped, max_id + 1);
  addJob(job, isForeground);
}

void JobsList::addJob(JobEntry &job, bool isForeground)
{
  removeFinishedJobs();
  if (isForeground)
  {
    foregroundJob.push(job);
  }
  else
  {
    jobs.insert(pair<int, JobEntry>(++max_id, job));
  }
}

// this func is in use iff the job to be added is stopped by ^z
void JobsList::addJobFromZsignal(JobEntry &job)
{
  if (job.getJobId() == 0)
  { // this was never in jobsList
    jobs.insert(pair<int, JobEntry>(++max_id, job));
  }
  else
  { // this was already in jobsList. no need to give new job id
    jobs.insert(pair<int, JobEntry>(job.getJobId(), job));
  }
}

void JobsList::killAllJobs()
{
  std::cout << "sending SIGKILL signal to " << getCount() << " jobs:" << std::endl;
  for (auto job_pair : jobs)
  {
    job_pair.second.printAndDie();
  }
}

void JobsList::removeFinishedJobs()
{
  if (jobs.empty() == 1)
    return;
  pid_t pid;
  for (auto job_pair_it = jobs.begin(), next_it = job_pair_it; job_pair_it != jobs.end(); job_pair_it = next_it)
  {
    ++next_it;
    pid = job_pair_it->second.getProcessID();
    int stat;
    pid_t res = waitpid(pid, &stat, WNOHANG);
    if (res < 0)
    {
      perror("wait failed");
    }
    else
    {
      if (WIFEXITED(stat)) // child terminated normally
      {
        removeJobById(job_pair_it->first);
      }
      else if (WIFSIGNALED(stat)) // child terminated by a signal
      {
        int sig = WTERMSIG(stat);
        if (sig == SIGKILL) // job was killed
        {
          removeJobById(job_pair_it->first);
        }
      }
    }
  }
}

void JobsList::removeJobById(int jobId)
{
  jobs.erase(jobId);
  if (jobId == max_id)
  {
    max_id = (jobs.empty()) ? 0 : jobs.rbegin()->first;
  }
}

JobsList::JobEntry &JobsList::getFgJob()
{
  return this->foregroundJob.top();
}

JobsList::JobEntry *JobsList::getJobById(int jobId)
{
  if (jobs.find(jobId) == jobs.end())
  {
    return nullptr;
  }
  return &(jobs.find(jobId)->second);
}

void JobsList::AlarmCheck()
{
  if (jobs.empty() == 1)
    return;
  pid_t pid;
  for (auto job_pair_it = jobs.begin(), next_it = job_pair_it; job_pair_it != jobs.end(); job_pair_it = next_it)
  {
    ++next_it;
    pid = job_pair_it->second.getProcessID();
    int stat;
    waitpid(pid, &stat, WNOHANG);
    if (WIFSIGNALED(stat)) // child terminated by a signal
    {
      int sig = WTERMSIG(stat);
      if (sig == SIGALRM)
      {
        kill(job_pair_it->first, SIGKILL);
        job_pair_it->second.printAlarm();
      }
    }
  }
}

int JobsList::getPID(int jobId)
{
  if (jobs.find(jobId) == jobs.end()) // doesn't exist
  {
    return -1;
  }
  auto job = jobs.find(jobId)->second;
  return job.getProcessID();
}

int JobsList::getCount()
{
  return jobs.size();
}

void JobsList::killForegroundJob()
{
  foregroundJob.pop();
}

bool JobsList::isForeground()
{
  return !foregroundJob.empty();
}
pid_t JobsList::getForegroundPid()
{
  return foregroundJob.top().getProcessID();
}
void JobsList::stopForeground()
{
  JobsList::JobEntry &job = foregroundJob.top();
  foregroundJob.pop();
  job.stopJob();
  addJobFromZsignal(job);
}
pid_t JobsList::lastToFront()
{
  if (jobs.empty())
  {
    cout << "smash error: fg: jobs list is empty" << endl;
    return -1;
  }
  JobsList::JobEntry job = jobs.rbegin()->second;
  jobs.erase(std::prev(jobs.end()));
  job.printJob();
  job.contJob();
  addJob(job, true);
  return job.getProcessID();
}
pid_t JobsList::jobIdToFront(int JobId)
{
  if (jobs.empty() || jobs.find(JobId) == jobs.end())
  {
    cout << "smash error: fg: job-id " << JobId << " does not exist" << endl;
    return -1;
  }
  JobsList::JobEntry job = jobs.find(JobId)->second;
  jobs.erase(jobs.find(JobId));
  job.printJob();
  job.contJob();
  addJob(job, true);
  return job.getProcessID();
}
pid_t JobsList::lastToBack()
{
  map<int, JobEntry>::reverse_iterator rit;
  for (rit = jobs.rbegin(); rit != jobs.rend(); ++rit)
  {
    if (rit->second.isStopped())
    {
      break;
    }
  }
  if (rit == jobs.rend())
  {
    cout << "smash error: bg: there is no stopped jobs to resume" << endl;
    return -1;
  }
  rit->second.printJob();
  rit->second.contJob();
  return rit->second.getProcessID();
}
pid_t JobsList::jobIdToBack(int JobId)
{
  map<int, JobEntry>::iterator it = jobs.find(JobId);
  if (jobs.empty() || it == jobs.end())
  {
    cout << "smash error: bg: job-id " << JobId << " does not exist" << endl;
    return -1;
  }
  if (!it->second.isStopped())
  {
    cout << "smash error: bg: job-id <job-id> is already running in the background" << endl;
    return -1;
  }
  it->second.printJob();
  it->second.contJob();
  return it->second.getProcessID();
}

//**************JobsCommand**********************
JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs)
    : BuiltInCommand(cmd_line), job_ptr(jobs)
{
}

void JobsCommand::execute()
{
  job_ptr->removeFinishedJobs();
  job_ptr->printJobsList();
}

//**************Kill Command**********************
bool is_number(const std::string &s)
{
  std::string::const_iterator it = s.begin();
  while (it != s.end() && std::isdigit(*it))
    ++it;
  return !s.empty() && it == s.end();
}

int GetSignal(string flag)
{ // turns -<signal> to <signal> and returns it as an integer
  if (flag.find_first_not_of('-') != 1)
  {
    return -1;
  }
  string newFlag = flag.substr(flag.find_first_not_of('-'));
  if (is_number(newFlag))
  {
    return std::stoi(newFlag);
  }
  return -1;
}

KillCommand::KillCommand(const char *cmd_line, JobsList *jobs)
    : BuiltInCommand(cmd_line), job_ptr(jobs) {}

void KillCommand::execute()
{
  int signal = GetSignal(args[1]);
  // invalid command check:
  if (args.size() != 3 || signal == -1 || !is_number(args[2]))
  {
    cout << "smash error: kill: invalid arguments" << endl;
    return;
  }
  // checking for the job in the jobs list:
  int jid = std::stoi(args[2]);
  int pid = job_ptr->getPID(jid); // returns -1 if doesn't exist in the jobs list
  if (pid == -1)
  { // job doesn't exist in the Jobs List
    cout << "smash error: kill: job-id " << args[2] << " does not exist" << endl;
    return;
  }
  if (kill(pid, signal) == -1)
  {
    cout << "smash error: kill failed" << endl;
  }
  else
  {
    if (signal == SIGSTOP)
    {
      job_ptr->getJobById(jid)->stopJob();
    }
    else if (signal == SIGCONT)
    {
      job_ptr->getJobById(jid)->contJob();
    }
    cout << "signal number " << signal << " was sent to pid " << pid << endl;
  }
}

//**************ForegroundCommand**********************
ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs)
    : BuiltInCommand(cmd_line), jobs_ptr(jobs) {}

void ForegroundCommand::execute()
{
  bool jobSpecified = args.size() > 1;
  if (jobSpecified && (args.size() > 2 || !is_number(args[1])))
  {
    cout << "smash error: fg: invalid arguments" << endl;
    return;
  }
  pid_t pid = jobSpecified ? jobs_ptr->jobIdToFront(stoi(args[1])) : jobs_ptr->lastToFront();
  SmallShell &smash = SmallShell::getInstance();
  if (pid == -1)
  {
    return;
  }
  kill(pid, SIGCONT);
  smash.runAtFront(pid);
}

//**************BackgroundCommand**********************
BackgroundCommand::BackgroundCommand(const char *cmd_line, JobsList *jobs)
    : BuiltInCommand(cmd_line), jobs_ptr(jobs) {}

void BackgroundCommand::execute()
{
  bool jobSpecified = args.size() > 1;
  if (args.size() > 2 || (args.size() > 1 && !is_number(args[1])))
  {
    cout << "smash error: fg: invalid arguments" << endl;
    return;
  }
  pid_t pid = jobSpecified ? jobs_ptr->jobIdToBack(stoi(args[1])) : jobs_ptr->lastToBack();
  // SmallShell &smash = SmallShell::getInstance();
  kill(pid, SIGCONT);
  // smash.runAtFront(pid);
}

//**************QuitCommand**********************
QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs)
    : BuiltInCommand(cmd_line), jobs_ptr(jobs)
{
}

void QuitCommand::execute()
{
  if (args.size() > 1 && args[1] == "kill")
  {
    jobs_ptr->killAllJobs();
  }
  exit(0);
}

//****************************************************
//**************ExternalCommand***********************
//****************************************************
ExternalCommand::ExternalCommand(const char *cmd_line)
    : Command(cmd_line, 0)
{
}

void ExternalCommand::execute()
{
}

//****************************************************
//**************Special Commands**********************
//****************************************************

//**************IOCommand**********************
IOCommand::IOCommand(const char *cmd_line)
    : BuiltInCommand(cmd_line)
{
}

void IOCommand::execute() {}

//**************RedirectFileCommand**********************
RedirectFileCommand::RedirectFileCommand(const char *cmd_line)
    : IOCommand(cmd_line)
{
  int redirect_loc = args[0].find_first_of(">");
  if (redirect_loc == std::string::npos)
  { // no '>' found, illegal
    destination = "*";
  }
  /**
   * add destination and args check!
   * */

  destination = args[0].substr(redirect_loc + 1);
}

void RedirectFileCommand::execute()
{
  /**if(destination.compare("*") == 0) { //illegal command
    cout << "smash error: > invalid arguments" << endl;
    return;
  }**/
  pid_t pid = fork();
  if (pid == 0)
  {           // son
    close(1); // close the standard output
    FILE *fp = freopen(destination.c_str(), "w", stdout);
    int redirect_loc = args[0].find_first_of(">");
    SmallShell &smash = SmallShell::getInstance();
    Command *cmd = smash.CreateCommand(args[0].substr(0, redirect_loc).c_str());
    char **argsArr = cmd->getArgsArr();
    setpgrp();
    execv(argsArr[0], argsArr);
    fclose(fp);
  }
  else
  { // parent
    int stat;
    if (waitpid(pid, &stat, WUNTRACED) < 0)
    {
      perror("wait failed");
    }
  }
}

//**************TailCommand**********************
TailCommand::TailCommand(const char *cmd_line)
    : BuiltInCommand(cmd_line), n(10) {}

void TailCommand::execute()
{
  if (args.size() > 3)
  {
    perror("smash error: tail: invalid arguments");
    return;
  }
  else if (args.size() == 3) // if there are 3 parameters- the second is N
  {
    if (args[1].at(0) != '-' || !is_number(args[1].substr(1)))
    {
      perror("smash error: tail: invalid arguments");
      return;
    }
    n = std::stoi(args[1].substr(1));
    if (n < 0)
    {
      perror("smash error: tail: invalid arguments");
      return;
    }
    else if (n == 0)
    {
      return;
    }
  }
  // the last parameter is the path
  const char *path = args.back().c_str();

  char buf;
  int fd = open(path, O_RDONLY);
  lseek(fd, -1, SEEK_END); // seek pointer at the last char
  int i = 0;               // number of "\n" read from EOF
  bool last_char = true;   // we are not counting new_line if it's the last char

  /* Reading one char at a time from the end of the file,
  until n 'new-line's were read (going BACKWARDS).
  By doing so- the seek pointer is set. */
  while (read(fd, &buf, 1) == 1)
  {
    if (buf == '\n' && !last_char)
    {
      i++;
      if (i == n)
      {
        break;
      }
    }
    if (lseek(fd, -2, SEEK_CUR) < 0) // handeling reading the first char of the file
    {
      lseek(fd, 0, SEEK_SET);
      read(fd, &buf, 1);
      lseek(fd, 0, SEEK_SET);
      break;
    }
    last_char = false;
  }

  while (read(fd, &buf, 1) == 1) // writing to stdout
  {
    if (write(1, &buf, 1) != 1)
    {
      break;
    }
  }

  close(fd);
}

//**************TouchCommand**********************
TouchCommand::TouchCommand(const char *cmd_line)
    : BuiltInCommand(cmd_line) {}

void TouchCommand::execute()
{
  if (args.size() != 3)
  {
    perror("smash error: touch: invalid arguments");
    return;
  }
  struct tm tm;
  strptime(args[2].c_str(), "%S:%M:%H:%d:%m:%Y", &tm);
  time_t t = mktime(&tm);
  if (t == -1)
  {
    perror("smash error: mktime failed");
    return;
  }
  struct utimbuf new_time;
  new_time.actime = t;
  new_time.modtime = t; 
  if (utime(args[1].c_str(),&new_time) == -1)
  {
    perror("smash error: utime failed");
    return;
  }
}

//**************SmallShell**********************
SmallShell::SmallShell()
    : prompt("smash")
{
  // TODO: add your implementation
}

SmallShell::~SmallShell()
{
  // TODO: add your implementation
}

/**
 * Creates and returns a pointer to Command class which matches the given command line (cmd_line)
 */

bool isBuiltIn(string cmd, const string built_in)
{
  string bg_built_in = built_in + "&";
  if (cmd.compare(built_in) == 0 || cmd.compare(bg_built_in) == 0)
  {
    return true;
  }
  return false;
}

bool isRedirect(string cmd)
{
  int redirect_loc = cmd.find_first_of(">");
  if (redirect_loc == std::string::npos)
  { // no '>' found
    return false;
  }
  return true;
}

bool isRedirectAppend(string cmd)
{
  int redirect_loc = cmd.find_first_of(">>");
  if (redirect_loc == std::string::npos)
  { // no '>>' found
    return false;
  }
  return true;
}

Command *SmallShell::CreateCommand(const char *cmd_line)
{
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (isBuiltIn(firstWord, "chprompt"))
  {
    return new ChangePromptCommand(cmd_line);
  }
  else if (isBuiltIn(firstWord, "pwd"))
  {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (isBuiltIn(firstWord, "showpid"))
  {
    return new ShowPidCommand(cmd_line);
  }
  else if (isBuiltIn(firstWord, "cd"))
  {
    return new ChangeDirCommand(cmd_line);
  }
  else if (isBuiltIn(firstWord, "jobs"))
  {
    return new JobsCommand(cmd_line, &jobs);
  }
  else if (isBuiltIn(firstWord, "kill"))
  {
    return new KillCommand(cmd_line, &jobs);
  }
  else if (isBuiltIn(firstWord, "quit"))
  {
    return new QuitCommand(cmd_line, &jobs);
  }
  else if (isBuiltIn(firstWord, "fg"))
  {
    return new ForegroundCommand(cmd_line, &jobs);
  }
  else if (isBuiltIn(firstWord, "bg"))
  {
    return new BackgroundCommand(cmd_line, &jobs);
  }
  /**else if (isRedirectAppend(cmd_s))
  {
    return new AppendFileCommand(cmd_line);
  }*/
  else if (isRedirect(cmd_s))
  {
    return new RedirectFileCommand(cmd_line);
  }
  else if (isBuiltIn(firstWord, "tail"))
  {
    return new TailCommand(cmd_line);
  }
  else if (isBuiltIn(firstWord, "touch"))
  {
    return new TouchCommand(cmd_line);
  }
  else
  {
    return new ExternalCommand(cmd_line);
  }
}

void SmallShell::executeCommand(const char *cmd_line)
{
  Command *cmd = CreateCommand(cmd_line);
  jobs.removeFinishedJobs();
  if (dynamic_cast<ExternalCommand *>(cmd) == nullptr) // Built-in Command
  {
    cmd->execute();
  }
  else if (dynamic_cast<IOCommand *>(cmd) != nullptr) // IO Command
  {
    cmd->execute();
  }
  else // External Command
  {
    char **argsArr = cmd->getArgsArr();
    pid_t pid = fork();
    if (pid < 0) // fail
    {
      perror("fork failed");
    }
    else if (pid == 0) // child
    {
      setpgrp();
      execv(argsArr[0], argsArr);
      perror("execv failed");
    }
    else // parent
    {
      if (_isBackgroundComamnd(cmd_line))
      {
        jobs.addJob(cmd, pid);
      }
      else
      {
        jobs.addJob(cmd, pid, true);
        runAtFront(pid, cmd);
      }
    }
  }
}

void SmallShell::printPtompt()
{
  std::cout << prompt << "> ";
}
void SmallShell::changePrompt(string new_prompt)
{
  prompt = new_prompt;
}
void SmallShell::push_dir(string dir)
{
  this->directories.push(dir);
}
void SmallShell::pop_dir()
{
  this->directories.pop();
}
string SmallShell::top_dir()
{
  return this->directories.top();
}
bool SmallShell::isEmpty_dir()
{
  return this->directories.empty();
}
void SmallShell::killForegroundJob()
{
  jobs.killForegroundJob();
}
bool SmallShell::isForeground()
{
  return jobs.isForeground();
}
pid_t SmallShell::getForegroundPid()
{
  return jobs.getForegroundPid();
}
void SmallShell::stopForeground()
{
  jobs.stopForeground();
}
void SmallShell::runAtFront(pid_t pid, Command *cmd)
{
  int stat;
  if (waitpid(pid, &stat, WUNTRACED) < 0)
  {
    perror("wait failed");
  }
  else
  {
    if (WIFEXITED(stat)) // child terminated normally
    {
      jobs.killForegroundJob();
    }
    else if (WIFSIGNALED(stat)) // child terminated by a signal
    {
      int sig = WTERMSIG(stat);
      if (sig == SIGTSTP)
      {
        jobs.addJob(cmd, pid, false, true);
      }
    }
  }
}

void SmallShell::runAtFront(pid_t pid)
{
  int stat;
  if (waitpid(pid, &stat, WUNTRACED) < 0)
  {
    perror("wait failed");
  }
  else
  {
    if (WIFEXITED(stat)) // child terminated normally
    {
      jobs.killForegroundJob();
    }
    else if (WIFSIGNALED(stat)) // child terminated by a signal
    {
      int sig = WTERMSIG(stat);
      if (sig == SIGTSTP)
      {
        jobs.addJobFromZsignal(jobs.getFgJob());
      }
    }
  }
}

void SmallShell::AlarmHandle()
{
  jobs.AlarmCheck();
}