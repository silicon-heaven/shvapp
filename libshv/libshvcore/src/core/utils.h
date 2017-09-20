#pragma once

#include "../shvcoreglobal.h"
#include "shvexception.h"

#define SHV_SAFE_DELETE(x) if(x != nullptr) {delete x; x = nullptr;}
/*
#define SHV_CARG(s) "" + QString(s) + ""
#define SHV_SARG(s) "'" + QString(s) + "'"
#define SHV_IARG(i) "" + QString::number(i) + ""
*/
#define SHV_QUOTE(x) #x
#define SHV_EXPAND_AND_QUOTE(x) SHV_QUOTE(x)
//#define SHV_QUOTE_QSTRINGLITERAL(x) QStringLiteral(#x)

#define SHV_FIELD_IMPL(ptype, lower_letter, upper_letter, name_rest) \
	private: ptype m_##lower_letter##name_rest; \
	public: ptype lower_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: bool set##upper_letter##name_rest(const ptype &val) { \
		if(m_##lower_letter##name_rest != val) { m_##lower_letter##name_rest = val; return true; } \
		return false; \
	}

#define SHV_FIELD_IMPL2(ptype, lower_letter, upper_letter, name_rest, default_value) \
	private: ptype m_##lower_letter##name_rest = default_value; \
	public: ptype lower_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: bool set##upper_letter##name_rest(const ptype &val) { \
		if(m_##lower_letter##name_rest != val) { m_##lower_letter##name_rest = val; return true; } \
		return false; \
	}

#define SHV_FIELD_BOOL_IMPL2(lower_letter, upper_letter, name_rest, default_value) \
	private: bool m_##lower_letter##name_rest = default_value; \
	public: bool is##upper_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: bool set##upper_letter##name_rest(bool val) { \
		if(m_##lower_letter##name_rest != val) { m_##lower_letter##name_rest = val; return true; } \
		return false; \
	}

#define SHV_FIELD_BOOL_IMPL(lower_letter, upper_letter, name_rest) \
	SHV_FIELD_BOOL_IMPL2(lower_letter, upper_letter, name_rest, false)
/*
#define SHV_PROPERTY_IMPL2(ptype, lower_letter, upper_letter, name_rest, default_value) \
	private: ptype m_##lower_letter##name_rest = default_value; \
	public: Q_SIGNAL void lower_letter##name_rest##Changed(const ptype &new_val); \
	public: ptype lower_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: Q_SLOT bool set##upper_letter##name_rest(const ptype &val) { \
		if(m_##lower_letter##name_rest != val) { \
			m_##lower_letter##name_rest = val; \
			emit lower_letter##name_rest##Changed(m_##lower_letter##name_rest); \
			return true; \
		} \
		return false; \
	}

#define SHV_PROPERTY_IMPL(ptype, lower_letter, upper_letter, name_rest) \
	private: ptype m_##lower_letter##name_rest; \
	public: Q_SIGNAL void lower_letter##name_rest##Changed(const ptype &new_val); \
	public: ptype lower_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: Q_SLOT bool set##upper_letter##name_rest(const ptype &val) { \
		if(m_##lower_letter##name_rest != val) { \
			m_##lower_letter##name_rest = val; \
			emit lower_letter##name_rest##Changed(m_##lower_letter##name_rest); \
			return true; \
		} \
		return false; \
	}

#define SHV_PROPERTY_BOOL_IMPL2(lower_letter, upper_letter, name_rest, default_value) \
	private: bool m_##lower_letter##name_rest = default_value; \
	public: Q_SIGNAL void lower_letter##name_rest##Changed(const bool &new_val); \
	public: bool is##upper_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: Q_SLOT bool set##upper_letter##name_rest(bool val) { \
		if(m_##lower_letter##name_rest != val) { \
			m_##lower_letter##name_rest = val; \
			emit lower_letter##name_rest##Changed(m_##lower_letter##name_rest); \
			return true; \
		}\
		return false; \
	}

#define SHV_PROPERTY_BOOL_IMPL(lower_letter, upper_letter, name_rest) \
	SHV_PROPERTY_BOOL_IMPL2(lower_letter, upper_letter, name_rest, false)

#define SHV_VARIANTMAP_FIELD(ptype, getter_prefix, setter_prefix, name_rest) \
	public: bool getter_prefix##name_rest##_isset() const {return contains(SHV_QUOTE_QSTRINGLITERAL(getter_prefix##name_rest));} \
	public: ptype getter_prefix##name_rest() const {return qvariant_cast<ptype>(value(SHV_QUOTE_QSTRINGLITERAL(getter_prefix##name_rest)));} \
	public: void setter_prefix##name_rest(const ptype &val) {(*this)[SHV_QUOTE_QSTRINGLITERAL(getter_prefix##name_rest)] = QVariant::fromValue(val);}
/// for default values other than QVariant()
#define SHV_VARIANTMAP_FIELD2(ptype, getter_prefix, setter_prefix, name_rest, default_value) \
	public: bool getter_prefix##name_rest##_isset() const {return contains(SHV_QUOTE_QSTRINGLITERAL(getter_prefix##name_rest));} \
	public: ptype getter_prefix##name_rest() const {return qvariant_cast<ptype>(value(SHV_QUOTE_QSTRINGLITERAL(getter_prefix##name_rest), default_value));} \
	public: void setter_prefix##name_rest(const ptype &val) {(*this)[SHV_QUOTE_QSTRINGLITERAL(getter_prefix##name_rest)] = QVariant::fromValue(val);}
	//since c++14 public: auto& setter_prefix##name_rest(const ptype &val) {(*this)[SHV_QUOTEME(getter_prefix##name_rest)] = val; return *this;}

/// for implicitly shared classes properties
#define SHV_SHARED_CLASS_FIELD_RW(ptype, getter_prefix, setter_prefix, name_rest) \
	public: const ptype& getter_prefix##name_rest() const {return d->getter_prefix##name_rest;} \
	public: void setter_prefix##name_rest(const ptype &val) {d->getter_prefix##name_rest = val;}

/// for implicitly shared classes properties
#define SHV_SHARED_CLASS_BIT_FIELD_RW(ptype, getter_prefix, setter_prefix, name_rest) \
	public: ptype getter_prefix##name_rest() const {return d->getter_prefix##name_rest;} \
	public: void setter_prefix##name_rest(const ptype &val) {d->getter_prefix##name_rest = val;}

class QString;
*/
namespace shv {
namespace core {

class SHVCORE_DECL_EXPORT Utils
{
public:
	static std::string removeJsonComments(const std::string &json_str);

	static int versionStringToInt(const std::string &version_string);
	static std::string intToVersionString(int ver);

#if 0
	static const QString &nullValueString();
	static void parseFieldName(const QString& full_field_name, QString *pfield_name = NULL, QString *ptable_name = NULL, QString *pdb_name = NULL);
	static QString composeFieldName(const QString &field_name, const QString &table_name = QString(), const QString &db_name = QString());
	/// @returns: True if @a field_name1 ends with @a field_name2. Comparision is case insensitive
	static bool fieldNameEndsWith(const QString &field_name1, const QString &field_name2);
	static bool fieldNameCmp(const QString &fld_name1, const QString &fld_name2);
	static QVariant retypeVariant(const QVariant &_val, int meta_type_id);
	static QVariant retypeStringValue(const QString &str_val, const QString &type_name);

	static int findCaption(const QString &caption_format, int from_ix, QString *caption);
	/**
	 * @brief findCaptions
	 * Finds in string all captions in form {{captionName}}
	 * @param str
	 * @return Set of found captions.
	 */
	static QSet<QString> findCaptions(const QString caption_format);
	static QString replaceCaptions(const QString format_str, const QString &caption_name, const QVariant &caption_value);
	static QString replaceCaptions(const QString format_str, const QVariantMap &replacements);

	/// invoke method of prototype bool method()
	static bool invokeMethod_B_V(QObject *obj, const char *method_name);

	template <class T>
	static T findParent(const QObject *_o, bool throw_exc = shv::core::Exception::Throw)
	{
		T t = NULL;
		QObject *o = const_cast<QObject*>(_o);
		while(o) {
			o = o->parent();
			if(!o)
				break;
			t = qobject_cast<T>(o);
			if(t)
				break;
		}
		if(!t && throw_exc) {
			SHV_EXCEPTION(QString("object 0x%1 has not any parent of requested type.").arg((ulong)_o, 0, 16));
		}
		return t;
	}

	template <typename V, typename... T>
	constexpr static inline auto make_array(T&&... t) -> std::array < V, sizeof...(T) >
	{
		return {{ std::forward<T>(t)... }};
	}
#endif
};

}}

