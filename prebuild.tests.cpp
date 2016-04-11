#ifndef TEST_SPINLOCK
#define TEST_SPINLOCK
#endif
#ifdef TEST_CXX11_TREAD
#include <thread>
void g(){}
int main(){
	{
		std::thread t(g);
		t.join();
	}
	return 0;
}
#elif defined(TEST_PTHREAD)
#include <pthread.h>

void *g(void *){ return 0; }
int main(){
	pthread_t inc_x_thread;
	if(pthread_create(&inc_x_thread, 0, g, 0))
		return 1;
	if(pthread_join(inc_x_thread, 0)) {
		return 1;
	}
	return 0;
}
#elif defined(TEST_CXX11_REGEX) || defined(TEST_TR1_REGEX) || defined(TEST_BOOST_REGEX)
#	ifdef TEST_TR1_REGEX
#		include <tr1/regex>
		using namespace std::tr1;
#	elif defined TEST_BOOST_REGEX
#		include <boost/regex.hpp>
		using namespace boost;
#	else
#		include <regex>
		using namespace std;
#	endif
int main(){
	try {
		regex re("^te.t$");
		if(regex_match("test", re)) {
			return 0;
		}
	}
	catch(...) {
	}
	return 1;
}
#elif defined(TEST_GETTIMEOFDAY)
#include <sys/time.h>
int main(){
	timeval tv;
	gettimeofday(&tv, 0);
	return 0;
}
#elif defined(TEST_SPINLOCK)
#include <atomic>
int main(){
	std::atomic_flag locker = ATOMIC_FLAG_INIT;
	locker.test_and_set(std::memory_order_acquire);
	locker.clear(std::memory_order_release);
	return 0;
}
#endif

