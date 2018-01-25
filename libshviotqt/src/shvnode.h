#pragma once

#include "shviotqtglobal.h"

#include <QObject>

namespace shv { namespace chainpack { class RpcValue; }}

namespace shv {
namespace iotqt {

class SHVIOTQT_DECL_EXPORT ShvNode : public QObject
{
	Q_OBJECT
public:
	using StringList = std::vector<std::string>;
	using String = std::string;
public:
	explicit ShvNode(QObject *parent = nullptr);

	virtual ShvNode* childNode(const String &name) const;
	virtual void setParentNode(ShvNode *parent);
	virtual String nodeName() {return m_nodeName;}
	void setNodeName(String &&n) {setObjectName(QString::fromStdString(n)); m_nodeName = std::move(n);}
	void setNodeName(const String &n) {setObjectName(QString::fromStdString(n)); m_nodeName = n;}

	virtual shv::chainpack::RpcValue shvValue() const;

	virtual StringList propertyNames() const;
	virtual shv::chainpack::RpcValue propertyValue(const String &property_name) const;
	virtual bool setPropertyValue(const String &property_name, const shv::chainpack::RpcValue &val);
	Q_SIGNAL void propertyValueChanged(const String &property_name, const shv::chainpack::RpcValue &new_val);
protected:
	virtual bool setPropertyValue_helper(const String &property_name, const shv::chainpack::RpcValue &val);
private:
	String m_nodeName;
};

}}
