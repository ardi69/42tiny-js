
#ifdef TEST_CXX11_THREAD
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

void *g(void *) { return 0; }
int main() {
	pthread_t inc_x_thread;
	if (pthread_create(&inc_x_thread, 0, g, 0))
		return 1;
	if (pthread_join(inc_x_thread, 0)) {
		return 1;
	}
	return 0;
}
#elif defined(TEST_CXX11_FILESYSTEM)
#include <filesystem>

int main() {
	if(std::filesystem::exists("test.txt"))
		do {} while(0);
	return 0;
}
#elif defined(TEST_CPP17)
#if __cplusplus < 201703L
#error C++17 not supported
#endif
int main() {
	return 0;
}
#elif defined(TEST_CPP20)
#if __cplusplus < 202002L
#error C++20 not supported
#endif
int main() {
	return 0;
}
#elif defined(TEST_CPP23)
#if __cplusplus < 202302L
#error C++23 not supported
#endif
int main() {
	return 0;
}
#else
#error "no TEST specified"
#endif

