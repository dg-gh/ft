#ifndef FT_CLIENT_HPP
#define FT_CLIENT_HPP

#include "ft_includes.hpp"

class ft_client
{

private:

	asio::io_context m_asio_context;
	std::thread m_thread;
	asio::ip::tcp::socket m_socket;
	asio::error_code m_error_code;
	asio::ip::tcp::endpoint m_endpoint;
	std::function<std::int32_t(std::int32_t)> m_validation_function = [](std::int32_t x) { return x; };
	bool m_client_validation_enabled = true;

	std::vector<char> buff;
	char* m_end_ptr = nullptr;

public:

	ft_client() : m_socket(asio::ip::tcp::socket(m_asio_context)) {}
	ft_client(const ft_client&) = delete;
	ft_client& operator=(const ft_client&) = delete;
	ft_client(ft_client&&) = delete;
	ft_client& operator=(ft_client&&) = delete;
	~ft_client();

	inline char* data() noexcept { return buff.data(); }
	inline const char* data() const noexcept { return buff.data(); }

	inline char* begin() noexcept { return buff.data(); }
	inline const char* begin() const noexcept { return buff.data(); }
	inline const char* cbegin() const noexcept { return buff.data(); }

	inline char* end() noexcept { return m_end_ptr; }
	inline const char* end() const noexcept { return m_end_ptr; }
	inline const char* cend() const noexcept { return m_end_ptr; }

	inline char& operator[](std::size_t offset) noexcept { return buff[offset]; }
	inline const char& operator[](std::size_t offset) const noexcept { return buff[offset]; }

	std::size_t last_incoming_buffer_size() const noexcept;

	void set_buffer_size(std::size_t new_buffer_size);

	float connect(const char* ip, std::uint16_t port);

	void disconnect();

	void set_validation_function(std::function<std::int32_t(std::int32_t)> fn);

	bool no_error() const;

	bool connection_running() const;

	float ping();

	bool send_file(const std::string& file_name, const std::string& destination_file_name);

	bool get_file(const std::string& file_name, const std::string& destination_file_name);

	bool load_file(const std::string& file_name);

	void remove_file(const std::string& file_name);

	char check_file(const std::string& file_name);

	std::string get_list();

	bool load_list();

	std::string get_list_from_path(const std::string& path);

	bool load_list_from_path(const std::string& path);

	bool append_text(const std::string& str, const std::string& destination_file_name);
};

#endif // FT_CLIENT_HPP
