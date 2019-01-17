#include "fileproviderlocalfsnode.h"

#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/chainpackreader.h>
#include <shv/coreqt/log.h>

#include <fstream>

static const char M_LSMETA[] = "lsmeta";

namespace cp = shv::chainpack;

static std::vector<cp::MetaMethod> meta_methods_brclab {
	{M_LSMETA, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_READ},
};

FileProviderLocalFsNode::FileProviderLocalFsNode(const QString &root_path, Super *parent):
	Super(root_path, parent)
{

}

cp::RpcValue FileProviderLocalFsNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const cp::RpcValue &params)
{
	if (method == M_LSMETA){
		return ndLsMeta(shv_path, params);
	}

	return Super::callMethod(shv_path, method, params);
}

size_t FileProviderLocalFsNode::methodCount(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	size_t method_count = Super::methodCount(shv_path);
	if (hasChildren(shv_path).toBool()){
		method_count += 1;
	}

	return method_count;
}

const cp::MetaMethod *FileProviderLocalFsNode::metaMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, size_t ix)
{
	if (hasChildren(shv_path).toBool()){
		size_t method_count = Super::methodCount(shv_path);

		if(ix == method_count){
			return &(meta_methods_brclab[0]);
		}
	}
	return Super::metaMethod(shv_path, ix);
}

cp::RpcValue FileProviderLocalFsNode::ndLsMeta(const StringViewList &shv_path, const cp::RpcValue &methods_params)
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

shv::chainpack::RpcValue FileProviderLocalFsNode::readMetaData(const QString &file)
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
