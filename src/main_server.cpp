#include "ft_server.hpp"


int main()
{
	ft_server SV;
	SV.set_buffer_size(16 * 1024 * 1024);
	SV.enable_client_validation(false);
	SV.start(33333, 3);

	while (true)
	{
#if defined(WIN32) || defined(__MINGW32__) || defined(__MINGW64__)
		std::system("cls");
#endif // WIN
#ifdef __linux__
		std::system("clear")
#endif // __linux__
		std::cout << "FT server running. "; SV.info();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}
