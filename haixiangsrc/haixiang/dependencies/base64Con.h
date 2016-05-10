#include <string>  


class Base64Con
{
public:
	static std::string base64_encode(unsigned char const* , unsigned int len);
	static std::string base64_decode(std::string const& s);
private:
	static inline bool Base64Con::is_base64(unsigned char c);
};

