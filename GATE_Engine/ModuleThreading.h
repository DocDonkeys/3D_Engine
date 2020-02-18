#ifndef MODULETHREADING_H
#define MODULETHREADING_H

#include "Module.h"
#include <mutex>
#include <queue>
#include <functional>
#include <vector>
#include <thread>
#include <condition_variable>

class ModuleThreading : public Module {
public:
	ModuleThreading(Application* app, const char* name = "null", bool start_enabled = true);
	~ModuleThreading();

	bool Init();
	bool Start();
	update_status Update(float dt);
	bool CleanUp();

	void AddTask(std::function<void()> newTask);

private:
	void ShutdownPool();
	void ProcessTasks(int threadID);

private:
	std::queue <std::function<void()>> tasksQueue;
	std::vector <std::thread> threadVector;
	//True when thread is executing a task
	std::vector <bool> threadStatus;
	std::mutex tQueueMutex;
	std::mutex threadPoolMutex;
	std::condition_variable condition;

	unsigned int concurrentThreads = 0;

	//Flags
	bool stopPool = false;
	bool poolTerminated = false;
};

#endif