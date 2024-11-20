#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <cstring>
#include <vector>
#include <string>
#include <sys/wait.h>

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

void searchFolder(const string &path, const string &filename, const bool &recursive, const bool caseSensitive)
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
      searchFolder(newpath, filename, true, caseSensitive);
    }
    else if (entry->d_type == DT_REG && (caseSensitive && strcmp(entry->d_name, filename.c_str()) == 0) || (!caseSensitive && strcasecmp(entry->d_name, filename.c_str()) == 0))
    {
      cout << "Found " << filename << " at " << path << "/" << filename << endl;
    }
  }

  closedir(dir);
}

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    cerr << "Usage: " << argv[0] << " searchpath filename1 [filename2] ... [filenameN]\n";
    return 1;
  }

  string searchpath = argv[1];
  vector<string *> filenames;

  for (int i = 2; i < argc; i++)
  {
    filenames.push_back(new string(argv[i]));
  }

  cout << "searchpath: " << searchpath << endl;
  cout << "filenames: ";
  for (auto filename : filenames)
  {
    cout << *filename << " ";
  }
  cout << endl;

  /*
  cout << "Folders in searchpath: " << endl;
   vector<string> folders = get_folders(searchpath);
   for (auto folder : folders)
   {
     cout << folder << endl;
   }
  */

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
      searchFolder(searchpath, *filenames[i], true, true);
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