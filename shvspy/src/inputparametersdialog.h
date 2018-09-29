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
	using ValueGetter = std::function<shv::chainpack::RpcValue()>;
	using ValueSetter = std::function<void(const shv::chainpack::RpcValue &param)>;

	void newParameter();
	void newParameter(const shv::chainpack::RpcValue &param);
	void onCurrentCellChanged(int row, int col);
	void removeParameter();
	bool tryParseParams(const shv::chainpack::RpcValue &params);

	void switchToBool(int row);
	void switchToInt(int row);
	void switchToString(int row);
	void switchToDouble(int row);
	void switchToDateTime(int row);

	void switchByType(const shv::chainpack::RpcValue::Type &type, int row);

	void clearParams();
	void clear();
	void onCurrentTabChanged(int index);
	void checkSyntax();
	void loadLastUsed();
	void loadParams(const QString &s);

	shv::chainpack::RpcValue paramValue() const;

	Ui::InputParametersDialog *ui;
	QVector<ValueGetter> m_cellValueGetters;
	QVector<ValueSetter> m_cellValueSetters;
	static QVector<shv::chainpack::RpcValue::Type> m_supportedTypes;
	QTimer m_syntaxCheckTimer;
	QString m_path;
	QString m_method;
	LastUsedParamsWidget *m_usedParamsWidget;
};
