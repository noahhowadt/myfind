#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <cstring>
#include <vector>
#include <string>
#include <sys/wait.h>
#include <sys/file.h>
#include <fcntl.h>

using namespace std;

vector<string> get_folders(const string &path)
{
  vector<string> folders;
  DIR *dir = opendir(path.c_str());

  if (!dir)
  {
    cerr << "Error opening directory: " << path << endl;
    return folders;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr)
  {
    if (entry->d_type == DT_DIR &&
        strcmp(entry->d_name, ".") != 0 &&
        strcmp(entry->d_name, "..") != 0)
    {
      folders.push_back(string(entry->d_name));
    }
  }

  closedir(dir);
  return folders;
}

void write_synchronized(const std::string &message)
{
  int fd = fileno(stdout);
  flock(fd, LOCK_EX);
  std::cout << message << std::endl;
  std::cout.flush();
  flock(fd, LOCK_UN);
}

void searchFolder(const string &path, const string &filename, const bool &recursive, const bool caseInsensitive)
{
  DIR *dir = opendir(path.c_str());

  if (!dir)
  {
    cerr << "Error opening directory: " << path << endl;
    return;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr)
  {
    if (recursive && entry->d_type == DT_DIR &&
        strcmp(entry->d_name, ".") != 0 &&
        strcmp(entry->d_name, "..") != 0)
    {
      string newpath = path + "/" + entry->d_name;
      searchFolder(newpath, filename, true, caseInsensitive);
    }
    else if (entry->d_type == DT_REG && (!caseInsensitive && strcmp(entry->d_name, filename.c_str()) == 0) || (caseInsensitive && strcasecmp(entry->d_name, filename.c_str()) == 0))
    {
      std::string full_path = std::string(path) + "/" + entry->d_name;
      char resolved_path[PATH_MAX];
      write_synchronized(
          to_string(getpid()) + ": " + entry->d_name + ": " + realpath(full_path.c_str(), resolved_path));
    }
  }

  closedir(dir);
}

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    cerr << "Usage: " << argv[0] << " searchpath filename1 [filename2] ... [filenameN] [-i] [-R]\n";
    return 1;
  }

  string searchpath;
  vector<string *> filenames;
  bool caseInsensitive = false;
  bool recursive = false;

  // Parse arguments
  for (int i = 1; i < argc; i++)
  {
    string arg = argv[i];
    if (arg == "-i")
    {
      caseInsensitive = true;
    }
    else if (arg == "-R")
    {
      recursive = true;
    }
    else if (searchpath.empty())
    {
      searchpath = arg; // First non-flag argument is searchpath
    }
    else
    {
      filenames.push_back(new string(arg)); // Remaining non-flag arguments are filenames
    }
  }

  if (searchpath.empty())
  {
    cerr << "Error: Search path not provided.\n";
    return 1;
  }

  if (filenames.empty())
  {
    cerr << "Error: At least one filename must be provided.\n";
    return 1;
  }

  cout << "searchpath: " << searchpath << endl;
  cout << "filenames: ";
  for (auto filename : filenames)
  {
    cout << *filename << " ";
  }
  cout << endl;

  vector<pid_t> pids;
  for (int i = 0; i < (int)filenames.size(); i++)
  {
    pid_t pid = fork();
    if (pid < 0)
    {
      // Fork failed
      cerr << "Fork failed" << endl;
      exit(1);
    }
    else if (pid == 0)
    {
      // Child process
      searchFolder(searchpath, *filenames[i], recursive, caseInsensitive);
      exit(0);
    }
    else
    {
      // Parent process
      pids.push_back(pid);
    }
  }

  for (pid_t pid : pids)
  {
    int status;
    waitpid(pid, &status, 0);
  }

  return 0;
}