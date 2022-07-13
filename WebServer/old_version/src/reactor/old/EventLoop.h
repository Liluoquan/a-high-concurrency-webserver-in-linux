// @Author: Lawson
// @Date: 2022/03/30

#pragma once
#include <functional>
#include <memory>
#include <vector>
#include "Channel.h"
#include "Epoll.h"
#include "../package/Util.h"
#include "Timer.h"
#include "../base/Logging.h"
#include "../base/Thread.h"
#include <iostream>

class Channel;
class Epoll;
class TimerManager;
typedef std::shared_ptr<Channel> sp_Channel;
typedef std::weak_ptr<Channel> wp_Channel;
typedef std::shared_ptr<Epoll> sp_Epoll;
typedef std::shared_ptr<TimerManager> sp_TimerManager;

class EventLoop:public std::enable_shared_from_this<EventLoop>{
private:
	typedef std::function<void()> Functor;
	std::vector<Functor> pendingFunctors_;
	int wakeupfd_;
	sp_Channel pwakeupChannel_;
	sp_Epoll poller_;
	bool looping_;
	static bool quit_;
	sp_TimerManager timerManager_;
	MutexLock mutex_;

public:
	EventLoop();
	void addToPoller(sp_Channel channel);
	void updatePoller(sp_Channel channel);
	void removeFromPoller(sp_Channel channel);
	void loop();
	void addTimer(sp_Channel channel,int timeout);
	void queueInLoop(Functor &&cb);
	void doPendingFunctors();
	static void setquit(int);
};

typedef std::shared_ptr<EventLoop> sp_EventLoop;
typedef std::weak_ptr<EventLoop> wp_EventLoop;






