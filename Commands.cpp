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
  if(type == BUILT_IN && _isBackgroundComamnd(cmd_line)) {
    char* cmd_line_new = new char[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_line_new, cmd_line);
    _removeBackgroundSign(cmd_line_new);
    args = _parseCommandLineVector(cmd_line_new);
    delete cmd_line_new;
    return;
  }
  args = _parseCommandLineVector(cmd_line);
}

string Command::getCmdName()
{
  return args[0];
}

char **Command::getArgsArr()
{
  int len = args.size();
  char **argsArr = new char *[len + 3];
  argsArr[0] = (char *)("/bin/bash");
  argsArr[1] = (char *)("-c");
  for (int i = 2; i < len + 2; i++)
  {
    argsArr[i] = const_cast<char *>(args[i - 2].c_str());
  }
  argsArr[len + 2] = NULL;
  return argsArr;
}

//**************BuiltInCommand**********************
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
  if (args.size() > 2)
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
JobsList::JobEntry::JobEntry(Command *cmd, int process_id, bool isStopped)
    : process_id(process_id), isStopped(isStopped)
{
  cmd_name = cmd->getCmdName();
  time(&create_time);
}

void JobsList::JobEntry::printJob()
{
  time_t now;
  time(&now);
  double elapsed = difftime(now, create_time);
  std::cout << cmd_name << " : " << process_id << " " << elapsed << " secs";
  if (isStopped)
    std::cout << " (stopped)";
  std::cout << std::endl;
}

int JobsList::JobEntry::getProcessID() {
  return this->process_id;
}

//**************JobList**********************
JobsList::JobsList()
    : max_id(1)
{
}

void JobsList::printJobsList()
{
  for (auto &job_it : jobs)
  {
    std::cout << "[" << job_it.first << "] ";
    job_it.second.printJob();
  }
}

void JobsList::addJob(Command *cmd, bool isStopped)
{
  jobs.insert(pair<int, JobEntry>(max_id++, JobsList::JobEntry(cmd, 2, isStopped)));
}

int JobsList::getPID(int jobId) {
  if(jobs.find(jobId) == jobs.end()) {
    return -1;
  }
  auto job = this->jobs.find(jobId)->second;
  return job.getProcessID();
}

//**************JobsCommand**********************
JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs)
    : BuiltInCommand(cmd_line), job_ptr(jobs)
{
}

void JobsCommand::execute()
{
  job_ptr->addJob(this);
  job_ptr->addJob(this, true);
  job_ptr->printJobsList();
}
//**************Kill Command**********************
bool is_number(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

int GetSignal(string flag) { //turns -<signal> to <signal> and returns it as an integer
  string newFlag = flag.substr(flag.find_first_not_of('-'));
  if(is_number(newFlag)) {
    return std::stoi(newFlag);
  }
  return -1;
}

KillCommand::KillCommand(const char* cmd_line, JobsList* jobs)
: BuiltInCommand(cmd_line), job_ptr(jobs) {}

void KillCommand::execute()
{
  int signal = GetSignal(args[1]);
  //invalid command check:
  if(args.size() != 3 || signal == -1 || !is_number(args[2]) ) {
    cout << "smash error: kill: invalid arguments" << endl;
    return;
  }
  //checking for the job in the jobs list:
  int pid = std::stoi(args[2]);
  pid = job_ptr->getPID(pid);
  if(pid == -1) { //job doesn't exist in the Jobs List
    cout << "smash error: kill: job-id " << args[2] << " does not exist" << endl;
    return;
  }
  if(kill(pid, signal) == -1) {
    cout << "smash error: kill failed" << endl;
  }
}

//**************ExternalCommand**********************
ExternalCommand::ExternalCommand(const char *cmd_line)
    : Command(cmd_line, 0)
{
}

void ExternalCommand::execute()
{
}

//**************SmallShell**********************
SmallShell::SmallShell() : prompt("smash")
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
Command *SmallShell::CreateCommand(const char *cmd_line)
{
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord.compare("chprompt") == 0)
  {
    return new ChangePromptCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0)
  {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0)
  {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("cd") == 0)
  {
    return new ChangeDirCommand(cmd_line);
  }
  else if (firstWord.compare("jobs") == 0)
  {
    return new JobsCommand(cmd_line, &jobs);
  }
  else if (firstWord.compare("kill") == 0)
  {
    return new KillCommand(cmd_line, &jobs);
  }
  else
  {
    return new ExternalCommand(cmd_line);
  }
}

void SmallShell::executeCommand(const char *cmd_line)
{
  Command *cmd = CreateCommand(cmd_line);
  if (dynamic_cast<ExternalCommand *>(cmd) == nullptr) // Built-in Command
  {
    cmd->execute();
  }
  else // External Command
  {
    char **argsArr = cmd->getArgsArr();
    pid_t pid = fork();
    if (pid == 0) // child
    {
      execv(argsArr[0], argsArr);
    }
    else // parent
    {
      wait(NULL);
      /*
      if (_isBackgroundComamnd(cmd_line))
      {
      }
      else
      {
      }
      */
    }
  }
  // Please note that you must fork smash process for some commands (e.g., external commands....)
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