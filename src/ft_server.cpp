#include "ft_server.hpp"


ft_server::~ft_server()
{
	if (m_running)
	{
		stop();
	}
}

std::size_t ft_server::number_of_clients()
{
	std::lock_guard<std::mutex> lock(m_connect_disconnect_mutex);
	return m_clients.size();
}

bool ft_server::start(std::uint16_t port, std::size_t number_of_threads)
{
	if (m_running)
	{
		stop();
	}

	try
	{
		m_asio_acceptor = new asio::ip::tcp::acceptor(m_asio_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port));

		if (m_asio_acceptor != nullptr)
		{
			m_threads.resize(number_of_threads);
			for (std::size_t n = 0; n < number_of_threads; n++)
			{
				m_threads[n] = std::thread([&]() { m_asio_context.run(); });
			}
			m_running = true;
			listen();
			return true;
		}
		else
		{
			return false;
		}
	}
	catch (...)
	{
		if (m_asio_acceptor != nullptr)
		{
			delete m_asio_acceptor;
		}
		return false;
	}
}

void ft_server::stop()
{
	m_asio_context.stop();
	for (std::size_t n = 0; n < m_threads.size(); n++)
	{
		if (m_threads[n].joinable())
		{
			m_threads[n].join();
		}
	}
	m_threads.clear();
	if (m_asio_acceptor != nullptr)
	{
		delete m_asio_acceptor;
	}
	m_running = false;
}

void ft_server::info()
{
	{
		std::lock_guard<std::mutex> lock(m_connect_disconnect_mutex);
		std::cout << "number of clients : " << m_clients.size() << '\n';
		int n = 0;
		for (auto& s : m_clients)
		{
			std::cout << "client " << ++n << " : " << s.socket.remote_endpoint() << '\n';
		}
	}
	std::cout << std::endl;
}

void ft_server::broadcast(const void* const ptr, std::size_t n)
{
	std::lock_guard<std::mutex> lock(m_connect_disconnect_mutex);
	for (std::list<client_connection>::iterator iter = m_clients.begin(); iter != m_clients.end(); ++iter)
	{
		std::lock_guard<std::mutex> lock(m_connect_disconnect_mutex);
		m_asio_context.post(
			[&]()
			{
				if (iter->socket.is_open())
				{
					iter->socket.write_some(asio::const_buffer(ptr, n), m_error_code);
				}
			}
		);
	}
}

void ft_server::disconnect_all_clients()
{
	std::lock_guard<std::mutex> lock(m_connect_disconnect_mutex);
	for (std::list<client_connection>::iterator iter = m_clients.begin(); iter != m_clients.end(); ++iter)
	{
		iter->socket.close();
	}
	m_clients.clear();
}

void ft_server::set_validation_function(std::function<std::int32_t(std::int32_t)> fn)
{
	m_validation_function = std::move(fn);
}

void ft_server::enable_client_validation(bool enable) noexcept
{
	m_client_validation_enabled = enable;
}

void ft_server::set_buffer_size(std::size_t new_size) noexcept
{
	m_buffer_size = new_size;
}


void ft_server::listen()
{
	m_asio_acceptor->async_accept(
		[&](std::error_code ec, asio::ip::tcp::socket new_client_connection)
		{
			if (!ec)
			{
				new_client_connection.set_option(asio::socket_base::keep_alive(true));

				client_connection* client_connection_ptr;

				{
					std::lock_guard<std::mutex> lock(m_connect_disconnect_mutex);
					m_clients.push_back(client_connection(new_client_connection));
					std::list<client_connection>::iterator temp = --m_clients.end();
					client_connection_ptr = &(*temp);
					temp->iterator = std::move(temp);
				}

				if (m_client_validation_enabled)
				{
					m_asio_context.post([&]() { handle_client_validation(*client_connection_ptr); });
				}
				else
				{
					client_connection_ptr->buffer.resize(m_buffer_size);
					handle_client_request(*client_connection_ptr);
				}
			}

			listen();
		}
	);
}

void ft_server::handle_client_validation(client_connection& client_socket)
{
	std::int32_t random_number = rng(mt);
	std::int32_t answer_number;

	client_socket.socket.write_some(asio::buffer(&random_number, sizeof(std::int32_t)), m_error_code);

	client_socket.socket.async_read_some(asio::buffer(&answer_number, sizeof(std::int32_t)),
		[&](std::error_code ec, std::size_t incoming_buffer_length)
		{
			if ((!ec) && (answer_number == m_validation_function(random_number)))
			{
				client_socket.buffer.resize(m_buffer_size);
				handle_client_request(client_socket);
			}
			else
			{
				client_socket.socket.close();

				{
					std::lock_guard<std::mutex> lock(m_connect_disconnect_mutex);
					m_clients.erase(client_socket.iterator);
				}
			}
		}
	);
}

void ft_server::handle_client_request(client_connection& client_socket)
{
	client_socket.socket.async_read_some(asio::buffer(client_socket.buffer.data(), client_socket.buffer.size()),
		[&](std::error_code ec, std::size_t incoming_buffer_length)
		{
			if (!ec)
			{
				if (incoming_buffer_length >= 8)
				{
					std::string_view str_view(client_socket.buffer.data(), 8);

					if (str_view.compare(0, 4, "ping") == 0) { ping_subroutine(client_socket); }
					else if (str_view.compare(0, 4, "send") == 0) { send_subroutine(client_socket, incoming_buffer_length); }
					else if (str_view.compare(0, 4, "app ") == 0) { app_subroutine(client_socket, incoming_buffer_length); }
					else if (str_view.compare(0, 4, "get ") == 0) { get_subroutine(client_socket, incoming_buffer_length); }
					else if (str_view.compare(0, 4, "list") == 0) { list_subroutine(client_socket, incoming_buffer_length); }
					else if (str_view.compare(0, 4, "lsfp") == 0) { lsfp_subroutine(client_socket, incoming_buffer_length); }
					else if (str_view.compare(0, 4, "rem ") == 0) { rem_subroutine(client_socket, incoming_buffer_length); }
					else if (str_view.compare(0, 4, "chck") == 0) { chck_subroutine(client_socket, incoming_buffer_length); }
				}

				handle_client_request(client_socket);
			}
			else
			{
				client_socket.socket.close();

				{
					std::lock_guard<std::mutex> lock(m_connect_disconnect_mutex);
					m_clients.erase(client_socket.iterator);
				}
			}
		}
	);
}

void ft_server::ping_subroutine(client_connection& client_socket)
{
	char c = 'p';

	if (client_socket.socket.is_open())
	{
		client_socket.socket.write_some(asio::buffer(&c, 1), m_error_code);
		std::this_thread::sleep_for(std::chrono::microseconds(1));
	}
	else
	{
		std::lock_guard<std::mutex> lock(m_connect_disconnect_mutex);
		m_clients.erase(client_socket.iterator);
	}
}

void ft_server::send_subroutine(client_connection& client_socket, std::size_t incoming_buffer_length)
{
	std::size_t file_name_size = static_cast<std::size_t>(*reinterpret_cast<std::uint32_t*>(&client_socket.buffer[4]));
	std::string file_name(file_name_size, 0);
	std::memcpy(&file_name[0], &client_socket.buffer[4 * sizeof(char) + sizeof(std::uint32_t)], file_name_size * sizeof(char));

	std::fstream file(file_name, std::ios::out | std::ios::binary);
	file.write(client_socket.buffer.data() + 4 * sizeof(char) + sizeof(std::uint32_t) + file_name_size, incoming_buffer_length
		- (4 * sizeof(char) + sizeof(std::uint32_t) + file_name_size));
	file.close();
}

void ft_server::app_subroutine(client_connection& client_socket, std::size_t incoming_buffer_length)
{
	std::size_t file_name_size = static_cast<std::size_t>(*reinterpret_cast<std::uint32_t*>(&client_socket.buffer[4]));
	std::string file_name(file_name_size, 0);
	std::memcpy(&file_name[0], &client_socket.buffer[4 * sizeof(char) + sizeof(std::uint32_t)], file_name_size * sizeof(char));

	std::fstream file(file_name, std::ios::app);
	file.write(client_socket.buffer.data() + 4 * sizeof(char) + sizeof(std::uint32_t) + file_name_size, incoming_buffer_length
		- (4 * sizeof(char) + sizeof(std::uint32_t) + file_name_size));
	file.close();
}

void ft_server::get_subroutine(client_connection& client_socket, std::size_t incoming_buffer_length)
{
	std::size_t file_name_size = static_cast<std::size_t>(*reinterpret_cast<std::uint32_t*>(&client_socket.buffer[4]));
	std::string file_name(file_name_size, 0);
	std::memcpy(&file_name[0], &client_socket.buffer[4 * sizeof(char) + sizeof(std::uint32_t)], file_name_size * sizeof(char));

	std::ifstream file(file_name, std::ios::binary | std::ios::ate);
	std::streamsize file_size = file.tellg();
	file.seekg(0, std::ios::beg);
	std::vector<char> buffer(file_size);

	// copy file
	if (file.read(buffer.data(), file_size))
	{
		if (client_socket.socket.is_open())
		{
			client_socket.socket.write_some(asio::buffer(buffer.data(), buffer.size()), m_error_code);
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}
		else
		{
			std::lock_guard<std::mutex> lock(m_connect_disconnect_mutex);
			m_clients.erase(client_socket.iterator);
		}
	}
}

void ft_server::list_subroutine(client_connection& client_socket, std::size_t incoming_buffer_length)
{
	std::string files("");
	{
		std::filesystem::directory_iterator file_list(std::filesystem::current_path());
		for (const std::filesystem::directory_entry& item : file_list)
		{
			std::string temp = item.path().generic_string();
			files += temp;
			if (std::filesystem::is_directory(temp))
			{
				files += '/';
			}
			files += ';';
		}
	}
	if (files.size() != 0)
	{
		files.pop_back();
	}

	if (client_socket.socket.is_open())
	{
		client_socket.socket.write_some(asio::buffer(files.data(), files.size()), m_error_code);
		std::this_thread::sleep_for(std::chrono::microseconds(1));
	}
	else
	{
		std::lock_guard<std::mutex> lock(m_connect_disconnect_mutex);
		m_clients.erase(client_socket.iterator);
	}
}

void ft_server::lsfp_subroutine(client_connection& client_socket, std::size_t incoming_buffer_length)
{
	std::size_t path_name_size = static_cast<std::size_t>(*reinterpret_cast<std::uint32_t*>(&client_socket.buffer[4]));
	std::string path_name(path_name_size, 0);
	std::memcpy(&path_name[0], &client_socket.buffer[4 * sizeof(char) + sizeof(std::uint32_t)], path_name_size * sizeof(char));
	
	std::string files("");
	{
		std::filesystem::directory_iterator file_list(path_name);
		for (const std::filesystem::directory_entry& item : file_list)
		{
			files += item.path().generic_string();
			files += ';';
		}
	}
	if (files.size() != 0)
	{
		files.pop_back();
	}

	if (client_socket.socket.is_open())
	{
		client_socket.socket.write_some(asio::buffer(files.data(), files.size()), m_error_code);
		std::this_thread::sleep_for(std::chrono::microseconds(1));
	}
	else
	{
		std::lock_guard<std::mutex> lock(m_connect_disconnect_mutex);
		m_clients.erase(client_socket.iterator);
	}
}

void ft_server::rem_subroutine(client_connection& client_socket, std::size_t incoming_buffer_length)
{
	std::size_t file_name_size = static_cast<std::size_t>(*reinterpret_cast<std::uint32_t*>(&client_socket.buffer[4]));
	std::string file_name(file_name_size, 0);
	std::memcpy(&file_name[0], &client_socket.buffer[4 * sizeof(char) + sizeof(std::uint32_t)], file_name_size * sizeof(char));

	std::filesystem::remove(file_name);
}

void ft_server::chck_subroutine(client_connection& client_socket, std::size_t incoming_buffer_length)
{
	std::size_t file_name_size = static_cast<std::size_t>(*reinterpret_cast<std::uint32_t*>(&client_socket.buffer[4]));
	std::string file_name(file_name_size, 0);
	std::memcpy(&file_name[0], &client_socket.buffer[4 * sizeof(char) + sizeof(std::uint32_t)], file_name_size * sizeof(char));

	char c;
	if (std::filesystem::exists(file_name)) { c = 'y'; }
	else { c = 'n'; }

	if (client_socket.socket.is_open())
	{
		client_socket.socket.write_some(asio::buffer(&c, 1), m_error_code);
		std::this_thread::sleep_for(std::chrono::microseconds(1));
	}
	else
	{
		std::lock_guard<std::mutex> lock(m_connect_disconnect_mutex);
		m_clients.erase(client_socket.iterator);
	}
}
