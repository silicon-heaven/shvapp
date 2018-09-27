#pragma once

#include <QDialog>

#include <QTimer>
#include <functional>

#include <shv/chainpack/rpcvalue.h>

namespace Ui {
class InputParametersDialog;
}

class LastUsedParamsWidget;

class InputParametersDialog : public QDialog
{
	Q_OBJECT

public:
	explicit InputParametersDialog(const QString &path, const QString &method, const shv::chainpack::RpcValue &params, QWidget *parent = 0);
	~InputParametersDialog();

	shv::chainpack::RpcValue value() const;

private:
	using ValueSetter = std::function<void(const shv::chainpack::RpcValue &param)>;
	using Switcher = ValueSetter (InputParametersDialog::*)(int);

	void newParameter();
	void newParameter(const shv::chainpack::RpcValue &param);
	void onCurrentCellChanged(int row, int col);
	void removeParameter();
	bool tryParseParams(const shv::chainpack::RpcValue &params);

	ValueSetter switchToBool(int row);
	ValueSetter switchToInt(int row);
	ValueSetter switchToString(int row);
	ValueSetter switchToDouble(int row);
	ValueSetter switchToDateTime(int row);

	ValueSetter switchByType(const shv::chainpack::RpcValue::Type &type, int row);

	void clearParams();
	void clear();
	void onCurrentTabChanged(int index);
	void checkSyntax();
	void loadLastUsed();
	void loadParams(const QString &s);

	shv::chainpack::RpcValue paramValue() const;

	Ui::InputParametersDialog *ui;
	QList<std::function<shv::chainpack::RpcValue()>> m_cellValues;
	static QMap<shv::chainpack::RpcValue::Type, Switcher> m_typeMap;
	QTimer m_syntaxCheckTimer;
	QString m_path;
	QString m_method;
	LastUsedParamsWidget *m_usedParamsWidget;
};
