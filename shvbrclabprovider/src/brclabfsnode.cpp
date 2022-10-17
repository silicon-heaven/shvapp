#include "brclabfsnode.h"

#include "brclabparser.h"
#include "shvbrclabproviderapp.h"

#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/chainpackreader.h>
#include <shv/coreqt/log.h>

#include <fstream>

static const char M_LSMETA[] = "lsmeta";
static const char M_READ_BRCLAB_SUMMARY[] = "readBrclabSummary";

namespace cp = shv::chainpack;

static std::vector<cp::MetaMethod> meta_methods {
	{M_LSMETA, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_READ},
	{M_READ_BRCLAB_SUMMARY, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_READ}
};

BrclabFsNode::BrclabFsNode(const QString &root_path, Super *parent):
	Super(root_path, parent)
{
}

cp::RpcValue BrclabFsNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const cp::RpcValue &params, const shv::chainpack::RpcValue &user_id)
{
	if (method == M_LSMETA){
		return ndLsMeta(shv_path, params);
	}
	else if (method == M_READ_BRCLAB_SUMMARY){
		return ndReadBrclabSummary(shv_path, params);
	}

	return Super::callMethod(shv_path, method, params, user_id);
}

size_t BrclabFsNode::methodCount(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	size_t method_count = Super::methodCount(shv_path);
	method_count += 1;

	return method_count;
}

const cp::MetaMethod *BrclabFsNode::metaMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, size_t ix)
{
	size_t method_count = Super::methodCount(shv_path);

	if(ix == method_count){
		if (hasChildren(shv_path).toBool()){
			return &(meta_methods[0]);
		}
		else {
			return &(meta_methods[1]);
		}
	}

	return Super::metaMethod(shv_path, ix);
}

shv::chainpack::RpcValue BrclabFsNode::ndReadBrclabSummary(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const shv::chainpack::RpcValue &methods_params)
{
	Q_UNUSED (methods_params);
	cp::RpcValue::Map ret;
	QString dir_path = m_rootDir.absolutePath() + '/' + QString::fromStdString(shv_path.join('/'));
	return BrclabParser::parse(dir_path);
}

cp::RpcValue BrclabFsNode::ndLsMeta(const StringViewList &shv_path, const cp::RpcValue &methods_params)
{
	cp::RpcValue::List ret;
	QString dir_path = m_rootDir.absolutePath() + '/' + QString::fromStdString(shv_path.join('/')) + '/';

	QString filter = (!methods_params.isValid()) ? "*" : QString::fromStdString(methods_params.toStdString());
	QDir dir(dir_path);

	QStringList file_names = dir.entryList(QStringList(filter), QDir::Files);

	for (int i = 0; i < file_names.size(); i++){
		cp::RpcValue md = readMetaData(dir_path + file_names.at(i));

		if (md.isValid()){
			ret.push_back(std::move(md));
		}
	}
	return ret;
}

shv::chainpack::RpcValue BrclabFsNode::readMetaData(const QString &file)
{
	try{
		std::ifstream file_stream;
		file_stream.exceptions( std::ifstream::failbit | std::ifstream::badbit );
		file_stream.open(file.toUtf8().constData(), std::ios::binary);

		cp::RpcValue::MetaData md;
		shv::chainpack::ChainPackReader rd(file_stream);
		rd.read(md);
		file_stream.close();

		QFileInfo fi(file);
		cp::RpcValue ret(fi.fileName().toStdString());
		ret.setMetaData(std::move(md));
		return ret;
	}
	catch (...) {
		return cp::RpcValue();
	}
}
