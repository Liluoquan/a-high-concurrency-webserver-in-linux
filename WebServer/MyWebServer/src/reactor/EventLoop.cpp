// @Author: Lawson
// @Date: 2022/03/31

#include "EventLoop.h"
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include "../package/Util.h"
#include "../base/Logging.h"
#include "../memory/MemoryPool.h"

using namespace std;

bool EventLoop::quit_ = false;

EventLoop::EventLoop()
	: poller_(newElement<Epoll>(), deleteElement<Epoll>),
	  looping_(false),
	  timerManager_(newElement<TimerManager>(), deleteElement<TimerManager>)
{
	wakeupfd_ = Eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
}

void EventLoop::removeFromPoller(sp_Channel channel)
{
	poller_->epoll_del(channel);
}

void EventLoop::updatePoller(sp_Channel channel)
{
	poller_->epoll_mod(channel);
}

void EventLoop::addToPoller(sp_Channel channel)
{
	poller_->epoll_add(channel);
}

void EventLoop::addTimer(sp_Channel channel, int timeout)
{
	timerManager_->addTimer(std::move(channel), timeout);
}

void EventLoop::loop()
{
	pwakeupChannel_ = sp_Channel(newElement<Channel>(shared_from_this()), deleteElement<Channel>);
	pwakeupChannel_->setFd(wakeupfd_);
	pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
	pwakeupChannel_->setReadHandler(std::bind(&EventLoop::doPendingFunctors, shared_from_this()));
	addToPoller(pwakeupChannel_);

	std::vector<sp_Channel> ret;
	// LOG << "enter loop";
	while (!quit_)
	{
		poller_->poll(ret);
		for (auto &it : ret)
			it->handleEvents();
		ret.clear();
		timerManager_->handleExpiredEvent();
	}
}

void EventLoop::queueInLoop(Functor &&cb)
{
	{
		MutexLockGuard lock(mutex_);
		pendingFunctors_.emplace_back(std::move(cb));
	}

	uint64_t buffer = 1;
	if (write(wakeupfd_, &buffer, sizeof(buffer)) < 0)
		LOG << "wake up write error";
}

void EventLoop::doPendingFunctors()
{
	uint64_t buffer;
	if (read(wakeupfd_, &buffer, sizeof(buffer)) < 0)
		LOG << "wake up read error";

	// 使用swap缩短了临界区的长度
	std::vector<Functor> next;
	{
		MutexLockGuard lock(mutex_);
		next.swap(pendingFunctors_);
	}

	for (auto &it : next)
		it();
}

void EventLoop::setquit(int a)
{
	quit_ = true;
}
