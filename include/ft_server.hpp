#ifndef FT_SERVER_HPP
#define FT_SERVER_HPP

#include "ft_includes.hpp"

class ft_server
{

public:

	class client_connection
	{

	public:

		asio::ip::tcp::socket socket;
		std::vector<char> buffer;
		std::list<client_connection>::iterator iterator;

		client_connection() = default;
		client_connection(const client_connection&) = default;
		client_connection& operator=(const client_connection&) = default;
		client_connection(client_connection&&) = default;
		client_connection& operator=(client_connection&&) = default;
		~client_connection() = default;

		client_connection(asio::ip::tcp::socket& new_socket) : socket(std::move(new_socket)) {}
	};

	std::vector<std::thread> m_threads;
	asio::io_context m_asio_context;
	asio::ip::tcp::acceptor* m_asio_acceptor = nullptr;
	std::list<client_connection> m_clients;
	std::size_t m_buffer_size = 1024;

	std::function<std::int32_t(std::int32_t)> m_validation_function = [](std::int32_t x) { return x; };
	std::random_device rd;
	std::mt19937 mt{ rd() };
	std::uniform_int_distribution<> rng{ -(1 << 30), (1 << 30) };
	bool m_client_validation_enabled = true;

	std::mutex m_mutex;
	std::mutex m_write_mutex;
	asio::error_code m_error_code;

	std::mutex m_connect_disconnect_mutex;
	bool m_running = false;

	ft_server() = default;
	ft_server(const ft_server&) = delete;
	ft_server& operator=(const ft_server&) = delete;
	ft_server(ft_server&&) = delete;
	ft_server& operator=(ft_server&&) = delete;
	~ft_server();

	std::size_t number_of_clients();

	bool start(std::uint16_t port, std::size_t number_of_threads = 1);

	void stop();

	void info();

	void broadcast(const void* const ptr, std::size_t n);

	void disconnect_all_clients();

	void set_validation_function(std::function<std::int32_t(std::int32_t)> fn);

	void enable_client_validation(bool enable) noexcept;

	void set_buffer_size(std::size_t new_size) noexcept;

private:

	void listen();

	void handle_client_validation(client_connection& client_socket);

	void handle_client_request(client_connection& client_socket);


	void ping_subroutine(client_connection& client_socket);

	void send_subroutine(client_connection& client_socket, std::size_t incoming_buffer_length);

	void app_subroutine(client_connection& client_socket, std::size_t incoming_buffer_length);

	void get_subroutine(client_connection& client_socket, std::size_t incoming_buffer_length);

	void list_subroutine(client_connection& client_socket, std::size_t incoming_buffer_length);

	void lsfp_subroutine(client_connection& client_socket, std::size_t incoming_buffer_length);

	void rem_subroutine(client_connection& client_socket, std::size_t incoming_buffer_length);

	void chck_subroutine(client_connection& client_socket, std::size_t incoming_buffer_length);
};

#endif // FT_SERVER_HPP
