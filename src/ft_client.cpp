#include "ft_client.hpp"


ft_client::~ft_client()
{
	disconnect();
}


std::size_t ft_client::last_incoming_buffer_size() const noexcept
{
	return static_cast<std::size_t>(m_end_ptr - buff.data());
}

void ft_client::set_buffer_size(std::size_t new_buffer_size)
{
	buff.resize(new_buffer_size);
	buff.shrink_to_fit();
	m_end_ptr = buff.data() + new_buffer_size;
}

float ft_client::connect(const char* ip, std::uint16_t port)
{
	float ret;
	m_thread = std::thread([&]() { m_asio_context.run(); });

	try
	{
		m_endpoint = asio::ip::tcp::endpoint(asio::ip::make_address(ip, m_error_code), port);
		m_socket.connect(m_endpoint, m_error_code);

		if (m_client_validation_enabled)
		{
			std::int32_t random_number;
			std::int32_t answer_number;
			m_socket.read_some(asio::buffer(&random_number, sizeof(std::int32_t)), m_error_code);
			answer_number = m_validation_function(random_number);
			m_socket.write_some(asio::buffer(&answer_number, sizeof(std::int32_t)), m_error_code);

			ret = ping();
		}
	}
	catch (...)
	{
		m_socket.close();
		ret = 1.0f / 0.0f;
	}

	return ret;
}

void ft_client::disconnect()
{
	m_socket.close();
	m_asio_context.stop();
	if (m_thread.joinable())
	{
		m_thread.join();
	}
}

void ft_client::set_validation_function(std::function<std::int32_t(std::int32_t)> fn)
{
	m_validation_function = std::move(fn);
}

bool ft_client::no_error() const
{
	if (!m_error_code)
	{
		return true;
	}
	else
	{
		std::cout << m_error_code.message() << std::endl;
		return false;
	}
}

bool ft_client::connection_running() const
{
	return m_socket.is_open();
}


float ft_client::ping()
{
	// send file to sever
	if (m_socket.is_open())
	{
		const char* ptr = "ping    ";

		std::chrono::time_point<std::chrono::steady_clock> ping_start = std::chrono::steady_clock::now();

		m_socket.write_some(asio::buffer(ptr, 8), m_error_code);
		std::this_thread::sleep_for(std::chrono::microseconds(1));

		char answer[4];
		m_socket.read_some(asio::buffer(&answer, 1));
		std::chrono::time_point<std::chrono::steady_clock> ping_stop = std::chrono::steady_clock::now();
		
		constexpr double factor_s_per_tick = static_cast<double>(std::chrono::steady_clock::duration::period::num)
			/ static_cast<double>(std::chrono::steady_clock::duration::period::den);

		return static_cast<float>(factor_s_per_tick * static_cast<double>((ping_stop - ping_start).count()));
	}
	else
	{
		return 1.0f / 0.0f;
	}
}

bool ft_client::send_file(const std::string& file_name, const std::string& destination_file_name)
{
	std::ifstream file(file_name, std::ios::binary | std::ios::ate);
	std::streamsize file_size = file.tellg();
	file.seekg(0, std::ios::beg);
	std::vector<char> buffer(4 * sizeof(char) + sizeof(std::uint32_t) + destination_file_name.size() + file_size);

	// copy file
	if (file.read(buffer.data() + 4 * sizeof(char) + sizeof(std::uint32_t) + destination_file_name.size(), file_size))
	{
		// first 4 chars are "send"
		std::memcpy(buffer.data(), "send", 4 * sizeof(char));
		// 4 bytes as the length of name of the destination file name
		std::uint32_t destination_file_name_size = destination_file_name.size();
		std::memcpy(buffer.data() + 4 * sizeof(char), &destination_file_name_size, sizeof(std::uint32_t));
		// copy destination file name
		std::memcpy(buffer.data() + 4 * sizeof(char) + sizeof(std::uint32_t), destination_file_name.data(), destination_file_name.size() * sizeof(char));

		// send file to sever
		if (m_socket.is_open())
		{
			m_socket.write_some(asio::buffer(buffer.data(), buffer.size()), m_error_code);
			std::this_thread::sleep_for(std::chrono::microseconds(1));
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

bool ft_client::get_file(const std::string& file_name, const std::string& destination_file_name)
{
	std::vector<char> buffer(4 * sizeof(char) + sizeof(std::uint32_t) + file_name.size());
	// first 4 chars are "get "
	std::memcpy(buffer.data(), "get ", 4 * sizeof(char));
	// 4 bytes as the length of name of the destination file name
	std::uint32_t file_name_size = file_name.size();
	std::memcpy(buffer.data() + 4 * sizeof(char), &file_name_size, sizeof(std::uint32_t));
	// copy destination file name
	std::memcpy(buffer.data() + 4 * sizeof(char) + sizeof(std::uint32_t), file_name.data(), file_name.size() * sizeof(char));

	// send file to sever
	if (m_socket.is_open())
	{
		m_socket.write_some(asio::buffer(buffer.data(), buffer.size()), m_error_code);
		std::this_thread::sleep_for(std::chrono::microseconds(1));

		std::size_t incoming_buffer_length = m_socket.read_some(asio::buffer(buff.data(), buff.size()));
		m_end_ptr = buff.data() + incoming_buffer_length;

		std::fstream file(destination_file_name, std::ios::out | std::ios::binary);
		if (file.is_open())
		{
			file.write(buff.data(), incoming_buffer_length);
			file.close();
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

bool ft_client::load_file(const std::string& file_name)
{
	std::vector<char> buffer(4 * sizeof(char) + sizeof(std::uint32_t) + file_name.size());
	// first 4 chars are "get "
	std::memcpy(buffer.data(), "get ", 4 * sizeof(char));
	// 4 bytes as the length of name of the destination file name
	std::uint32_t file_name_size = file_name.size();
	std::memcpy(buffer.data() + 4 * sizeof(char), &file_name_size, sizeof(std::uint32_t));
	// copy destination file name
	std::memcpy(buffer.data() + 4 * sizeof(char) + sizeof(std::uint32_t), file_name.data(), file_name.size() * sizeof(char));

	// send file to sever
	if (m_socket.is_open())
	{
		m_socket.write_some(asio::buffer(buffer.data(), buffer.size()), m_error_code);
		std::this_thread::sleep_for(std::chrono::microseconds(1));

		std::size_t incoming_buffer_length = m_socket.read_some(asio::buffer(buff.data(), buff.size()));
		m_end_ptr = buff.data() + incoming_buffer_length;
		return incoming_buffer_length;
	}
	else
	{
		return false;
	}
}

void ft_client::remove_file(const std::string& file_name)
{
	std::vector<char> buffer(4 * sizeof(char) + sizeof(std::uint32_t) + file_name.size());
	// first 6 chars are "rem "
	std::memcpy(buffer.data(), "rem ", 4 * sizeof(char));
	// 4 bytes as the length of name of the destination file name
	std::uint32_t file_name_size = file_name.size();
	std::memcpy(buffer.data() + 4 * sizeof(char), &file_name_size, sizeof(std::uint32_t));
	// copy destination file name
	std::memcpy(buffer.data() + 4 * sizeof(char) + sizeof(std::uint32_t), file_name.data(), file_name.size() * sizeof(char));

	// send request to remove to server
	if (m_socket.is_open())
	{
		m_socket.write_some(asio::buffer(buffer.data(), buffer.size()), m_error_code);
		std::this_thread::sleep_for(std::chrono::microseconds(1));
	}
}

char ft_client::check_file(const std::string& file_name)
{
	std::vector<char> buffer(4 * sizeof(char) + sizeof(std::uint32_t) + file_name.size());
	// first 6 chars are "rem "
	std::memcpy(buffer.data(), "chck", 4 * sizeof(char));
	// 4 bytes as the length of name of the destination file name
	std::uint32_t file_name_size = file_name.size();
	std::memcpy(buffer.data() + 4 * sizeof(char), &file_name_size, sizeof(std::uint32_t));
	// copy destination file name
	std::memcpy(buffer.data() + 4 * sizeof(char) + sizeof(std::uint32_t), file_name.data(), file_name.size() * sizeof(char));

	// send file to sever
	if (m_socket.is_open())
	{
		m_socket.write_some(asio::buffer(buffer.data(), buffer.size()), m_error_code);
		std::this_thread::sleep_for(std::chrono::microseconds(1));

		m_socket.read_some(asio::buffer(buff.data(), buff.size()));
		m_end_ptr = buff.data() + 1;
		return buff[0];
	}
	else
	{
		return 'u';
	}
}

std::string ft_client::get_list()
{
	std::string buffer = "list    ";

	// send file to sever
	if (m_socket.is_open())
	{
		m_socket.write_some(asio::buffer(buffer.data(), buffer.size()), m_error_code);
		std::this_thread::sleep_for(std::chrono::microseconds(1));

		std::size_t incoming_buffer_length = m_socket.read_some(asio::buffer(buff.data(), buff.size()));
		m_end_ptr = buff.data() + incoming_buffer_length;
		buffer.resize(incoming_buffer_length);
		std::memcpy(&buffer[0], buff.data(), incoming_buffer_length);

		return buffer;
	}
	else
	{
		return buffer;
	}
}

bool ft_client::load_list()
{
	std::string buffer = "list    ";

	// send file to sever
	if (m_socket.is_open())
	{
		m_socket.write_some(asio::buffer(buffer.data(), buffer.size()), m_error_code);
		std::this_thread::sleep_for(std::chrono::microseconds(1));

		std::size_t incoming_buffer_length = m_socket.read_some(asio::buffer(buff.data(), buff.size()));
		m_end_ptr = buff.data() + incoming_buffer_length;
		return true;
	}
	else
	{
		return false;
	}
}

std::string ft_client::get_list_from_path(const std::string& path)
{
	std::string buffer = "lsfp    ";
	buffer += path;
	std::uint32_t path_size = static_cast<std::uint32_t>(path.size());
	std::memcpy(&buffer[4], &path_size, sizeof(std::uint32_t));
	std::cout << buffer;

	// send file to sever
	if (m_socket.is_open())
	{
		m_socket.write_some(asio::buffer(buffer.data(), buffer.size()), m_error_code);
		std::this_thread::sleep_for(std::chrono::microseconds(1));

		std::size_t incoming_buffer_length = m_socket.read_some(asio::buffer(buff.data(), buff.size()));
		m_end_ptr = buff.data() + incoming_buffer_length;
		buffer.resize(incoming_buffer_length);
		std::memcpy(&buffer[0], buff.data(), incoming_buffer_length);

		return buffer;
	}
	else
	{
		return buffer;
	}
}

bool ft_client::load_list_from_path(const std::string& path)
{
	std::string buffer = "lsfp    ";
	buffer += path;
	std::uint32_t path_size = static_cast<std::uint32_t>(path.size());
	std::memcpy(&buffer[4], &path_size, sizeof(std::uint32_t));
	std::cout << buffer;

	// send file to sever
	if (m_socket.is_open())
	{
		m_socket.write_some(asio::buffer(buffer.data(), buffer.size()), m_error_code);
		std::this_thread::sleep_for(std::chrono::microseconds(1));

		std::size_t incoming_buffer_length = m_socket.read_some(asio::buffer(buff.data(), buff.size()));
		m_end_ptr = buff.data() + incoming_buffer_length;
		buffer.resize(incoming_buffer_length);
		std::memcpy(&buffer[0], buff.data(), incoming_buffer_length);

		return true;
	}
	else
	{
		return false;
	}
}

bool ft_client::append_text(const std::string& str, const std::string& destination_file_name)
{
	std::vector<char> buffer(4 * sizeof(char) + sizeof(std::uint32_t) + destination_file_name.size() + str.size());

	// copy text
	std::memcpy(buffer.data() + 4 * sizeof(char) + sizeof(std::uint32_t) + destination_file_name.size(), str.data(), str.size());
	// first 4 chars are "app "
	std::memcpy(buffer.data(), "app ", 4 * sizeof(char));
	// 4 bytes as the length of name of the destination file name
	std::uint32_t destination_file_name_size = destination_file_name.size();
	std::memcpy(buffer.data() + 4 * sizeof(char), &destination_file_name_size, sizeof(std::uint32_t));
	// copy destination file name
	std::memcpy(buffer.data() + 4 * sizeof(char) + sizeof(std::uint32_t), destination_file_name.data(), destination_file_name.size() * sizeof(char));
	// send file to sever
	if (m_socket.is_open())
	{
		m_socket.write_some(asio::buffer(buffer.data(), buffer.size()), m_error_code);
		std::this_thread::sleep_for(std::chrono::microseconds(1));
		return true;
	}
	else
	{
		return false;
	}
}
