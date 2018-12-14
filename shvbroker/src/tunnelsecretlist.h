#ifndef TUNNELSECRETLIST_H
#define TUNNELSECRETLIST_H

#include <string>
#include <vector>

class TunnelSecretList
{
public:
	bool checkSecret(const std::string &s);
	std::string createSecret();
private:
	void removeOldSecrets(int64_t now);

	struct Secret
	{
		int64_t createdMsec;
		std::string secret;
		uint8_t hitCnt = 0;
	};
	std::vector<Secret> m_secretList;
};

#endif // TUNNELSECRETLIST_H
