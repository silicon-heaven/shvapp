#include "tunnelsecretlist.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QRandomGenerator>

#include <array>

static constexpr int64_t max_age_msec = 10*1000;

bool TunnelSecretList::checkSecret(const std::string &s)
{
	qint64 now = QDateTime::currentMSecsSinceEpoch();
	for (size_t i = 0; i < m_secretList.size(); ++i) {
		Secret &sc = m_secretList[i];
		int64_t age = now - sc.msec;
		if(age < 0)
			continue; // this should never happen
		if(age > max_age_msec)
			continue;
		if(sc.secret == s) {
			sc.hitCnt++;
			if(sc.hitCnt <= 2)
				return true;
		}
	}
	return false;
}

std::string TunnelSecretList::createSecret()
{
	qint64 now = QDateTime::currentMSecsSinceEpoch();
	removeOldSecrets(now);

	static constexpr size_t DATA_LEN = 64;
	uint32_t data[DATA_LEN];
	QRandomGenerator::global()->generate(data, data + DATA_LEN);
	QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
	hash.addData((const char*)data, DATA_LEN * sizeof(data[0]));
	Secret sc;
	sc.msec = now;
	sc.secret = hash.result().toHex().constData();
	m_secretList.push_back(sc);
	return sc.secret;
}

void TunnelSecretList::removeOldSecrets(int64_t now)
{
	m_secretList.erase(
		std::remove_if(m_secretList.begin(),
						m_secretList.end(),
						[now](const Secret &sc){
							int64_t age = now - sc.msec;
							return age < 0 || age > max_age_msec;
						}),
		m_secretList.end()
	);
}
